#ifndef _PID_H_
#define _PID_H_

#include <math.h>
#include <stdint.h>
#include <stdio.h> // 包含printf所需

// 这里保留你项目必需的头文件
#include "global_vars.h"
#include "initialize.h"
#include "encoder.h"
#include "motor.h"
#include "key_mode.h"
#include "zf_common_headfile.h"

// ==================== 1. 数据结构定义 ====================
// 增量式/位置式通用 PID控制器结构体 (用于左右驱动轮等)
typedef struct {
    volatile float Kp, Ki, Kd;
    float output_min, output_max;
    float inc_max, inc_min;

    float error[3];       // [当前, 前次, 前前次]
    float output;
    float output_inc;

    unsigned long update_count;
} PID_Controller;

// 官方外环转向参数结构体
typedef struct {
    volatile float kp;       // 基础比例系数
    volatile float kp2;      // 非线性比例系数 (应对大弯道)
    volatile float kd;       // 误差微分系数
    volatile float gkd;      // 陀螺仪微分阻尼系数
} OuterLoop_Params;

// ==================== 2. 宏定义 ====================
// 前轮机械角度参数
#define HANDLEBAR_MIDDLE_ANGLE  0.0f    // 把手中值角度
#define MAX_STEER_ANGLE         35.0f   // 前轮向左最大打死角度
#define MIN_STEER_ANGLE        -35.0f   // 前轮向右最大打死角度

// 编码器与回中任务参数
#define ENCODER_MAX             4096    // 编码器最大值(12位)
#define MAX_MOTOR_SPEED         10000.0f// 电机最大输出限幅
#define MIN_MOTOR_SPEED        -10000.0f
// 【新增】基于角度的控制阈值
#define PID_OUTPUT_LIMIT        8000.0f // PWM最大限制
#define PID_INTEGRAL_LIMIT      1000.0f // 角度积分限幅值(可根据实际情况调大)
#define I_SEPARATION_ANGLE_DEG  25.0f   // 积分分离阈值(度)：误差小于25度时才开始积分
#define ANGLE_TOLERANCE_DEG     1.5f    // 误差死区(度)：误差小于1.5度认为已回中完毕
// ==================== 3. 外部全局变量 ====================
extern PID_Controller pid_left;
extern PID_Controller pid_right;
extern OuterLoop_Params imu_steer_params;// 惯导
extern OuterLoop_Params vis_steer_params;// 视觉

// 回中任务专属参数
extern float si_kp;
extern float si_ki;
extern float si_kd;
// 转向内环PID参数 (开放给上位机调参)
extern volatile float inner_kp;
extern volatile float inner_kd;
// ==================== 4. 函数声明 ====================
// 回中任务专用
void reset_pid_status(void);
void return_to_middle(void);

// 转向级联闭环
float calc_outer_steering_target(float error, float gyro_z, OuterLoop_Params* p);
int16 calc_inner_servo_pwm(float target_angle, float current_sensor_angle);

// 基础PID库函数
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float out_min, float out_max);
void PID_Reset(PID_Controller *pid);
float PID_Update_Incremental(PID_Controller *pid, float setpoint, float measurement);

#endif // _PID_H_
