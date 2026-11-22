#ifndef __INS_H
#define __INS_H

#include "zf_common_typedef.h"
#include "zf_common_headfile.h"
#include "pathrecorder.h"
#include "imu_ahrs.h"
#include "encoder.h"
#include "global_vars.h"
#include "initialize.h"
#include "zf_device_key.h"

// ===================== 配置参数 =====================
// 基础前瞻距离
#define INS_LOOKAHEAD_DISTANCE             0.30f
#define INS_MAX_SPEED                      1.00f
#define INS_STOP_DISTANCE_THRESHOLD        0.12f

// 动态前瞻：车速每增加 1m/s，前瞻距离增加 0.20m
#define INS_LD_SPEED_COEFF                 0.20f

// --------------------- 路面与横向速度决策 ---------------------
// 队内实测湿滑大理石路面有效附着系数
#define INS_ROAD_FRICTION_MU               0.57f

// 横向安全系数
#define INS_LATERAL_SAFETY_FACTOR          0.90f

// 重力加速度
#define INS_GRAVITY_ACCEL                  9.81f

// --------------------- 曲率计算 ---------------------
// 计算第i点曲率时，向前和向后各取2个路径点
#define INS_CURVATURE_SPAN                 2U

// 曲率滑动平均半宽：2表示最多使用5个曲率点
#define INS_CURVATURE_FILTER_HALF_WIDTH    2U

// 航向变化小于该值时按IMU噪声处理，单位：度
#define INS_CURVATURE_YAW_DEADZONE_DEG     0.30f

// 计算曲率时局部窗口的最小有效路程，单位：m
#define INS_MIN_CURVATURE_DISTANCE_M       0.05f

// 小于该曲率时按直道处理，单位：1/m
#define INS_CURVATURE_EPSILON              0.001f

// --------------------- 速度规划 ---------------------
// 路径速度下限，防止极小半径或噪声造成目标速度接近0
#define INS_MIN_PATH_SPEED                 0.50f

// 两个记录点距离异常过小时，速度传递使用的最小距离，单位：m
#define INS_MIN_SPEED_PLAN_DISTANCE_M      0.01f

// 速度表串口调试：1=打印，0=关闭
#define INS_SPEED_TABLE_DEBUG              0
#define INS_SPEED_TABLE_PRINT_STEP         5U

// ===================== 对外接口 =====================
uint8 ins_init(void);
void ins_update(void);
void ins_start_navigation(void);
void ins_stop_navigation(void);
uint8 ins_is_navigating(void);
void ins_find_switch_point(void);
void ins_calculate_curvature_speed_limits(void);
// 新增：获取目标速度（供 cpu0_main.c 使用）
void ins_pure_pursuit(float *left_speed, float *right_speed);

// 全局速度变量（供外部直接读取）
extern float g_ins_left_speed;
extern float g_ins_right_speed;

// 速度决策物理参数（可在调试阶段实时修改）
extern volatile float K_SAFE_CENTRIPETAL;
extern volatile float TARGET_DECEL;
extern volatile float START_ACCEL_LIMIT;

// 定义路径状态
typedef enum {
    PATH_STATE_STRAIGHT = 0, // 直行
    PATH_STATE_CURVE,        // 弯道
    PATH_STATE_REVERSE       // 倒车
} PathState_e;

// 状态表：记录每个路径点对应的状态
extern PathState_e g_path_states[FLASH_MAX_LOG_RECORDS];

// 点的预处理
void ins_preprocess_path(void);


#endif // __INS_H
