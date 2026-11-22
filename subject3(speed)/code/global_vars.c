#include "global_vars.h"
#include "encoder.h"

volatile float FILTER_ALPHA = 0.6;
// 编码器数据，有符号，支持正反转
volatile float encoder_speed_raw_avg = 0.0f;
volatile int32 encoder_count_right = 0;
volatile int32 encoder_count_left = 0;
volatile float encoder_speed_right = 0.0f;
volatile float encoder_speed_left = 0.0f;
volatile uint32 g_system_time_ms = 0;
volatile int8 key1_action = 0;
volatile int8 key2_action = 0;
volatile int8 key3_action = 0;
volatile int8 key4_action = 0;
volatile int8 return_to_mid_flag = 0;
volatile float target_speed_forward = 1.0f;
volatile float target_speed_back = -1.0f;
volatile float ramp_cmd_speed = 0.0f;
volatile float tune_target_speed = 0.0;
volatile float tune_target_angle = 0.0;
volatile CarMode_e current_car_mode = 0;

volatile float ins_lateral_error = 0.0f; // 循迹控制输出（当前代码中为 PP 转向角，单位：度）
volatile float ins_gyro_z        = 0.0f; // Z 轴陀螺仪角速度（用于外环微分反馈，单位：度/秒）
volatile float ins_target_speed  = 0.0f; // 当前导航计算的目标速度（单位：m/s）
volatile float ins_cruise_speed  = 3.0f; // Subject 3 return speed limit (m/s)
volatile float ins_target_yaw    = 0.0f; // 目标航向角（单位：度）
volatile float K_diff            = 1.0f; // 差速增幅，用于狂野模式

volatile uint8 g_is_returning = 0;             // 返回状态标志位
volatile float g_start_yaw = 0.0f;             // 起始航向角
volatile uint32 g_start_time = 0;              // 起始时间
volatile uint8 g_return_stage = 0;             // 返回阶段
volatile float g_rc_max_speed = 2.0f;
float map_sbus_to_speed(uint16 sbus_value) {
    const uint16 SBUS_MIN = 192U;
    const uint16 SBUS_MID = 992U;
    const uint16 SBUS_MAX = 1792U;
    float speed_limit = g_rc_max_speed;

    if (speed_limit < 0.2f) speed_limit = 0.2f;
    if (speed_limit > 7.0f) speed_limit = 7.0f;
    if (sbus_value < SBUS_MIN) sbus_value = SBUS_MIN;
    if (sbus_value > SBUS_MAX) sbus_value = SBUS_MAX;

    if (sbus_value <= SBUS_MID) {
        return ((float)sbus_value - (float)SBUS_MID) * speed_limit /
               ((float)SBUS_MID - (float)SBUS_MIN);
    }
    return ((float)sbus_value - (float)SBUS_MID) * speed_limit /
           ((float)SBUS_MAX - (float)SBUS_MID);
}
// 将 SBUS 通道值映射为转向角（-45°～45°）
float map_sbus_to_angle(uint16 sbus_value) {
    const uint16 SBUS_MIN = 239;    // SBUS 最小值
    const uint16 SBUS_MAX = 1745;   // SBUS 最大值
    const float ANGLE_MIN = -STEER_SENSOR_MAX_ANGLE_DEG; // 目标角度最小值（右转）
    const float ANGLE_MAX =  STEER_SENSOR_MAX_ANGLE_DEG;  // 目标角度最大值（左转）

    // 输入限幅，防止越界
    if(sbus_value < SBUS_MIN) sbus_value = SBUS_MIN;
    if(sbus_value > SBUS_MAX) sbus_value = SBUS_MAX;

    return (float)(sbus_value - SBUS_MIN) * (ANGLE_MAX - ANGLE_MIN) / (SBUS_MAX - SBUS_MIN) + ANGLE_MIN;
}
