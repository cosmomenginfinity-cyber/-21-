#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H
#include "zf_common_headfile.h"
#include <stdint.h>

extern volatile float FILTER_ALPHA;
// 编码器数据（有符号，支持正反转）
extern volatile float encoder_speed_raw_avg; // 物理原始平均速度
extern volatile int32 encoder_count_right;
extern volatile int32 encoder_count_left;
extern volatile float encoder_speed_right;
extern volatile float encoder_speed_left;
extern volatile uint32 g_system_time_ms;
extern volatile int8 key1_action;
extern volatile int8 key2_action;
extern volatile int8 key3_action;
extern volatile int8 key4_action;
extern volatile int8 return_to_mid_flag;
extern volatile float target_speed_forward;
extern volatile float target_speed_back;
extern volatile float ramp_cmd_speed;
extern volatile float g_rc_max_speed;
// =========================================================
// 开放给惯导（算法）同学对接的专用接口变量
// =========================================================
extern volatile float ins_lateral_error; // 循迹横向误差 (例如：偏离轨迹多少 cm)
extern volatile float ins_gyro_z; // Z轴陀螺仪角速度 (用于外环微分阻尼，防过弯画龙)
extern volatile float ins_target_speed; // 当前期望的低速运行速度 (m/s)
extern volatile float ins_cruise_speed;
// 【新增】：开放给示波器监控的目标航向角 (单位：度)
extern volatile float ins_target_yaw;
// ============================ 调参专用全局变量 ============================
// 用按键切换此变量，0.0f 为停止，2.0f 为调速目标
extern volatile float tune_target_speed;
extern volatile float tune_target_angle;

typedef enum {
    SPEED_MODE_STOP = 0,    // 停止
    SPEED_MODE_STRAIGHT,    // 直道
    SPEED_MODE_TURN,        // 转弯
    SPEED_MODE_REVERSE      // 倒车
} Speed_Mode_t;

// 速度管理结构体
typedef struct {
    float target_final;     // 最终期望的目标速度 (Final Target)
    float target_ramp;      // 经过斜坡处理后的指令速度 (Ramp Output -> PID Input)
    float accel_step;       // 加速度步长 (每5ms增加多少)
    float decel_step;       // 减速度步长 (每5ms减少多少)
    Speed_Mode_t mode;      // 当前模式
} Speed_Manager_t;

extern Speed_Manager_t g_speed_mgr;

// 定义小车当前的工作模式
typedef enum {
    MODE_STOP = 0,    // 停车/待机模式（安全模式）
    MODE_RC,          // 遥控模式（人工接管+记录采点）
    MODE_INS_TRACK,   // 科目一或科目二惯导循迹
    MODE_CENTERING    // 回中模式
} CarMode_e;

extern volatile CarMode_e current_car_mode;// 初始化为停车模式
extern volatile float map_sbus_to_speed(uint16 sbus_value);
extern volatile float map_sbus_to_angle(uint16 sbus_value);
#endif
