#include "encoder_motor_ctl.h"
#include "back_ins.h"
// ==================== 自适应闭环换向斜坡函数 ====================
void Speed_Ramp_Update(Speed_Manager_t *mgr)
{
    float cur = mgr->target_ramp;
    float tgt = mgr->target_final;

    // 换向防抱死安全缓冲计数器
    static uint32_t safety_hold_ticks = 0;

    // 1. 如果处于换向最后的物理静止安全缓冲中
    if (safety_hold_ticks > 0) {
        safety_hold_ticks--;
        mgr->target_ramp = 0.0f; // 强制输出 0
        return;
    }

    // 2. 检测本周期是否即将发生跨零换向
    bool is_crossing_zero = (cur > 0.0f && tgt < 0.0f) || (cur < 0.0f && tgt > 0.0f);

    // 选择加速/减速步长
    float step = (fabsf(cur) < fabsf(tgt)) ? mgr->accel_step : mgr->decel_step;

    // 3. 自适应换向闭环保护
    if (is_crossing_zero) {
        // 获取两轮当前实际测量的平均物理速度
        float actual_v = (encoder_speed_left + encoder_speed_right) / 2.0f;

        // 【闭环判定】：只要实际物理车速还没有降到 0.05 m/s 以下，强制输出 0 进行强力制动，拦截换向
        if (fabsf(actual_v) > 0.05f) {
            mgr->target_ramp = 0.0f;
            return; // 暂不换向，继续刹车
        }

        // 【安全垫底】：当实际车速已经低于 0.05 m/s 时，为了防范“轮胎抱死、车身仍在滑行”的极端情况，
        // 额外强制给 50ms (5 个 ticks) 的物理静止收敛时间，确保万无一失
        mgr->target_ramp = 0.0f;
        safety_hold_ticks = 5;
        return;
    }

    // 方向控制
    if (cur < tgt) {
        cur += step;
        if (cur > tgt) cur = tgt;
    } else {
        cur -= step;
        if (cur < tgt) cur = tgt;
    }

    mgr->target_ramp = cur;
}

Speed_Manager_t g_speed_mgr = {
    .target_final = 0.0f,
    .target_ramp  = 0.0f,
    .accel_step = 0.02f,   // 需要按你速度单位/控制周期调
    .decel_step = 0.02f,   // 建议减速比加速快一点
    .mode = SPEED_MODE_STOP
};


void System_Background_Tasks(void)
{
    // 静态变量：永远记住上次发送的时间，不随函数退出而消失
    static uint32 last_send_time = 0;

    // 非阻塞判断：距离上次发送是否超过 50ms (20Hz)
    if (g_system_time_ms - last_send_time >= 10)
    {
        last_send_time = g_system_time_ms; // 更新时间戳

        seekfree_assistant_oscilloscope_struct oscilloscope_data = {0};
        oscilloscope_data.channel_num = 6;
        oscilloscope_data.data[0] = encoder_speed_left;
        oscilloscope_data.data[1] = encoder_speed_right;
        oscilloscope_data.data[2] = g_speed_mgr.target_ramp;
        oscilloscope_data.data[3] = tune_target_angle;
        oscilloscope_data.data[4] = get_current_steer_sensor_angle();
        oscilloscope_data.data[5] = g_back_curve_limit_speed_mps;

        // 瞬间发给上位机
        seekfree_assistant_oscilloscope_send(&oscilloscope_data);
    }
}

void test_update_rear_wheels_for_ins(float target_base_speed,float current_wheel_angle)
{
    // =========================================================
    // 1) 斜坡逻辑注入：将瞬时指令转为平滑指令
    // =========================================================
    g_speed_mgr.target_final = target_base_speed;  // 设定最终目标
    if (current_car_mode == MODE_CAMERA)
    {
        /* Camera follow already integrates its acceleration command at 100 Hz.
         * Applying the generic speed ramp again would add a second rate limiter
         * and make the wheel-speed target lag behind the visual controller. */
        g_speed_mgr.target_ramp = target_base_speed;
    }
    else
    {
        Speed_Ramp_Update(&g_speed_mgr);
    }

    // 从这里开始，后续所有的计算都使用平滑后的速度：g_speed_mgr.target_ramp
    float smooth_speed = g_speed_mgr.target_ramp;

    // =========================================================
    // 2) 根据等效真实轮角计算后轮差速 (使用 smooth_speed)
    // =========================================================
    //float smooth_speed = target_base_speed;
    float set_left  = smooth_speed;
    float set_right = smooth_speed;

    // 当平滑速度不为0，且前轮有一定偏角时，才进行差速计算
    if (fabsf(smooth_speed) > 0.01f && fabsf(current_wheel_angle) > 1.0f)
    {
        float tan_steer = tanf(DEG_TO_RAD_ANGLE(current_wheel_angle));
        // 理论阿克曼差速因子
        float base_diff_factor = (TRACK_WIDTH * tan_steer) / (2.0f * WHEEL_BASE);

        // 差速比因子计算
        float diff_factor = base_diff_factor * K_diff;

        // 计算左右轮的目标速度
        set_left  = smooth_speed * (1.0f - diff_factor);
        set_right = smooth_speed * (1.0f + diff_factor);
    }

    // =========================================================
    // 3) 安全限幅 (保持原逻辑)
    // =========================================================
    if (set_left > MAX_SPEED_M_S)       set_left = MAX_SPEED_M_S;
    else if (set_left < -MAX_SPEED_M_S) set_left = -MAX_SPEED_M_S;

    if (set_right > MAX_SPEED_M_S)       set_right = MAX_SPEED_M_S;
    else if (set_right < -MAX_SPEED_M_S) set_right = -MAX_SPEED_M_S;

    // =========================================================
    // 4) 进行双轮增量PID计算
    // =========================================================
    float duty_left  = PID_Update_Incremental(&pid_left,  set_left,  encoder_speed_left);
    float duty_right = PID_Update_Incremental(&pid_right, set_right, encoder_speed_right);

    // =========================================================
    // 5) 输出PWM驱动电机
    // =========================================================
    Motor_Control(duty_left, duty_right);
}
