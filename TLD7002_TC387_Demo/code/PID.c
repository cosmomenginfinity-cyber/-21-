#include "PID.h"

// ==================== 全局变量实例 ====================
// 左右轮驱动PID实例
PID_Controller pid_left = {0};
PID_Controller pid_right = {0};

OuterLoop_Params imu_steer_params = {0.5f, 0.0f, 0.1f, 0.0f};
OuterLoop_Params vis_steer_params = {0.5f, 0.0f, 0.1f, 0.0f};

// 回中任务专属参数
float si_kp = 200.0f;
float si_ki = 50.0f;
float si_kd = 10.0f;

volatile float inner_kp = 1700.0f;
volatile float inner_kd = 650.0f;
// 局部宏定义 (仅在本文件使用)
#define PID_OUTPUT_LIMIT     8000.0f  // PWM最大限制
//#define PID_INTEGRAL_LIMIT   500.0f   // 积分限幅
#define I_SEPARATION_ERROR   300.0f   // 积分分离阈值
#define ANGLE_TOLERANCE      2.0f     // 误差死区

// 回中任务内部状态变量
static float pid_integral = 0.0f;
static float pid_last_error = 0.0f;


// ==================== 1. 独立任务：方向轮回中 ====================
// 清除回中PID状态
void reset_pid_status(void) {
    pid_integral = 0.0f;
    pid_last_error = 0.0f;
}

// 执行回中 (以角度度数为单位)
void return_to_middle(void)
{
    // 1. 获取当前编码器原始值
    float current_pos = absolute_encoder_get_location();

    // 2. 计算原始脉冲误差 (Target - Current)
    float tick_error = DIR_MID - current_pos;

    // 3. 考虑360度循环最短路径 (在脉冲层级处理最安全)
    if (tick_error > (ENCODER_MAX / 2.0f))       tick_error -= ENCODER_MAX;
    else if (tick_error < -(ENCODER_MAX / 2.0f)) tick_error += ENCODER_MAX;

    // 4. 【核心修改】将脉冲误差转化为角度误差 (单位：度)
    // 4096 对应 360度，正负表示方向
    float error_deg = tick_error * (360.0f / (float)ENCODER_MAX);

    // 5. 到达目标死区(以角度判定)，停机退出
    if (fabs(error_deg) <= ANGLE_TOLERANCE_DEG)
    {
        set_motor_speed(0);
        return_to_mid_flag = 0;
        reset_pid_status();
        // 打印停机时的实际偏离角度
        printf("FINISH_ERROR: %.2f Deg\r\n", error_deg);
        return;
    }

    // 6. PID 计算 (现在误差单位是角度！)
    float p_term = si_kp * error_deg;

    // 积分分离与计算
    if (fabs(error_deg) < I_SEPARATION_ANGLE_DEG) {
        pid_integral += error_deg;
        if (pid_integral > PID_INTEGRAL_LIMIT) pid_integral = PID_INTEGRAL_LIMIT;
        else if (pid_integral < -PID_INTEGRAL_LIMIT) pid_integral = -PID_INTEGRAL_LIMIT;
    } else {
        pid_integral = 0;
    }
    float i_term = si_ki * pid_integral;

    // 微分计算 (记录的也是上一帧的角度误差)
    float d_term = si_kd * (error_deg - pid_last_error);
    pid_last_error = error_deg;

    // 输出限幅与执行
    float pid_output = p_term + i_term + d_term;
    if (pid_output > PID_OUTPUT_LIMIT) pid_output = PID_OUTPUT_LIMIT;
    else if (pid_output < -PID_OUTPUT_LIMIT) pid_output = -PID_OUTPUT_LIMIT;

    set_motor_speed((int16)pid_output);
}


// ==================== 2. 双闭环转向控制算法 ====================

// 外环：计算前轮目标角度 (视觉/惯导误差 -> 期望角度)
float calc_outer_steering_target(float error, float gyro_z, OuterLoop_Params* p)
{
    static float last_error = 0.0f;

    float p_term1 = error * p->kp;
    float p_term2 = error * fabs(error) * p->kp2;
    float d_error = error - last_error;
    float d_term  = d_error * p->kd;
    float gyro_term = gyro_z * p->gkd;

    last_error = error;
    // 如果 current_car_mode 是 STOP，强制清零静态变量
    if (current_car_mode == MODE_STOP) {
        last_error = 0;
        return HANDLEBAR_MIDDLE_ANGLE;
    }

    float steer_output = p_term1 + p_term2 + d_term - gyro_term;
    float target_wheel_angle = HANDLEBAR_MIDDLE_ANGLE + steer_output;

    // 外环机械限幅
    if (target_wheel_angle > MAX_STEER_ANGLE) target_wheel_angle = MAX_STEER_ANGLE;
    if (target_wheel_angle < MIN_STEER_ANGLE) target_wheel_angle = MIN_STEER_ANGLE;

    return target_wheel_angle;
}

// 内环：前轮角度伺服控制 (期望角度 -> PWM)
// 内环：前轮角度伺服控制 (期望角度 -> PWM)
/*int16 calc_inner_servo_pwm(float target_angle, float current_sensor_angle)
{
    static float last_inner_error = 0.0f;

    float error = target_angle - current_sensor_angle;

    // 正常计算 PD 控制输出
    float pwm_out = (error * inner_kp) + ((error - last_inner_error) * inner_kd);

    // 每次都更新历史误差，确保从死区出来时微分（D）项不会发生突变
    last_inner_error = error;

    // 核心修改：当角度偏差绝对值小于 1.0 度时，强制输出 0（消除死区抖动和滋滋声）
    if (fabsf(error) < 1.0f)
    {
        pwm_out = 0.0f;
    }

    // 限幅处理
    if (pwm_out > MAX_MOTOR_SPEED) pwm_out = MAX_MOTOR_SPEED;
    if (pwm_out < MIN_MOTOR_SPEED) pwm_out = MIN_MOTOR_SPEED;

    return (int16)pwm_out;
}*/
// “方案一：分段 PID”
/*int16 calc_inner_servo_pwm(float target_angle, float current_sensor_angle)
{
    static float last_inner_error = 0.0f;
    float error = target_angle - current_sensor_angle;

    // 1. 根据误差绝对值，动态调整 PD 参数
    float kp_active, kd_active;
    if (fabsf(error) > 8.0f) // 偏差大于 8 度时，采用大增益强行推过去
    {
        kp_active = 1200.0f; // 大打角时出力足，响应快
        kd_active = 350.0f;
    }
    else // 偏差小于 8 度时，采用小增益，保证稳态绝不震荡
    {
        kp_active = 300.0f;
        kd_active = 100.0f;
    }

    // 2. 正常进行 PD 计算
    float pwm_out = (error * kp_active) + ((error - last_inner_error) * kd_active);
    last_inner_error = error;

    // 3. 动态死区过滤
    if (fabsf(error) < 1.0f) {
        pwm_out = 0.0f;
    }

    if (pwm_out > MAX_MOTOR_SPEED) pwm_out = MAX_MOTOR_SPEED;
    if (pwm_out < MIN_MOTOR_SPEED) pwm_out = MIN_MOTOR_SPEED;

    return (int16)pwm_out;
}*/
// “方案二：角度自适应补偿”**
int16 calc_inner_servo_pwm(float target_angle, float current_sensor_angle)
{
    static float last_inner_error = 0.0f;
    float error = target_angle - current_sensor_angle;

    // 设定 0 度时的温和基础参数
    float kp_base = 200.0f;
    float kd_base = 100.0f;

    // 根据当前前轮实际打角的绝对值，动态缩放增益
    // 角度每增加 1 度，增益增加 6.7%。当打到 45 度时，增益刚好放大 4 倍 (1 + 0.067 * 45 = 4)
    float scale_factor = 1.0f + 0.067f * fabsf(current_sensor_angle);

    float kp_active = kp_base * scale_factor;
    float kd_active = kd_base * scale_factor;

    float pwm_out = (error * kp_active) + ((error - last_inner_error) * kd_active);
    last_inner_error = error;

    if (fabsf(error) < 1.0f) {
        pwm_out = 0.0f;
    }

    if (pwm_out > MAX_MOTOR_SPEED) pwm_out = MAX_MOTOR_SPEED;
    if (pwm_out < MIN_MOTOR_SPEED) pwm_out = MIN_MOTOR_SPEED;

    return (int16)pwm_out;
}
// ==================== 3. 基础 PID 库实现 ====================

void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float out_min, float out_max) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->output_min = out_min;
    pid->output_max = out_max;

    float range = out_max - out_min;
    pid->inc_max = range * 0.2f;   // 最大增量为量程的20%
    pid->inc_min = -pid->inc_max;
}

void PID_Reset(PID_Controller *pid) {
    pid->error[0] = pid->error[1] = pid->error[2] = 0;
    pid->output = pid->output_inc = 0;
    pid->update_count = 0;
}

float PID_Update_Incremental(PID_Controller *pid, float setpoint, float measurement) {
    float current_error = setpoint - measurement;

    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = current_error;

    if (pid->update_count == 0) {
        pid->error[1] = pid->error[2] = current_error;
        pid->output_inc = 0;
    } else {
        float delta_error = pid->error[0] - pid->error[1];
        float second_diff = pid->error[0] - 2*pid->error[1] + pid->error[2];

        pid->output_inc = pid->Kp * delta_error
                        + pid->Ki * pid->error[0]
                        + pid->Kd * second_diff;

        if (pid->output_inc > pid->inc_max) pid->output_inc = pid->inc_max;
        else if (pid->output_inc < pid->inc_min) pid->output_inc = pid->inc_min;
    }

    pid->output += pid->output_inc;

    if (pid->output > pid->output_max) pid->output = pid->output_max;
    else if (pid->output < pid->output_min) pid->output = pid->output_min;

    pid->update_count++;

    return pid->output;
}
