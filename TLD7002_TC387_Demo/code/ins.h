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
#define INS_LOOKAHEAD_DISTANCE                  0.30f

// 动态前瞻：车速每增加1m/s，前瞻距离增加0.40m
#define INS_LD_SPEED_COEFF                      0.40f

// 实车测得的最大速度硬上限，单位：m/s
#define INS_MAX_SPEED                           7.00f

// 终点位置容差。进入该范围后目标速度置零，并等待车辆真实静止。
#define INS_STOP_DISTANCE_THRESHOLD             0.03f

// --------------------- 路面与横向速度决策 ---------------------
#define INS_ROAD_FRICTION_MU                    0.57f
#define INS_LATERAL_SAFETY_FACTOR               0.90f
#define INS_GRAVITY_ACCEL                       9.81f

// --------------------- 曲率计算 ---------------------
#define INS_CURVATURE_SPAN                      2U
#define INS_CURVATURE_FILTER_HALF_WIDTH         2U
#define INS_CURVATURE_YAW_DEADZONE_DEG          0.30f
#define INS_MIN_CURVATURE_DISTANCE_M            0.05f
#define INS_CURVATURE_EPSILON                   0.001f

// --------------------- 速度与加速度规划 ---------------------
// 半径不大于2m时按曲率规划速度；大于2m时开放直道巡航速度。
#define INS_CURVE_RADIUS_THRESHOLD_M            2.00f
#define INS_CURVE_LATERAL_ACCEL_LIMIT           4.80f
#define INS_CURVE_MIN_SPEED                     2.00f
#define INS_TIGHT_CURVE_RADIUS_M                1.30f
#define INS_TIGHT_CURVE_ACCEL_MPS2              1.20f

// --------------------- 换向点二维进站控制 ---------------------
// 正值表示把目标点沿录制换向点航向向前平移。
// 初次测试保持0；只有停车仍稳定偏短时，才按0.01m逐步增加。
#define INS_SWITCH_POSITION_COMP_M              0.00f

// 最小专属进站区长度。速度较高时，程序会自动扩大到动态制动距离。
#define INS_SWITCH_2D_APPROACH_MIN_M            3.00f

// 动态制动的额外安全余量。该值越大，开始降速越早。
#define INS_SWITCH_2D_BRAKE_MARGIN_M            0.10f

// 横向二维补偿叠加到原PP打角：补偿角=增益×横向误差，并做限幅。
#define INS_SWITCH_2D_LATERAL_KP_DEG_PER_M      10.0f
#define INS_SWITCH_2D_STEER_COMP_MAX_DEG        6.00f

// 激活二维控制时允许的坐标/里程差异，防止坐标未清零时误补偿。
#define INS_SWITCH_2D_ACTIVATE_GUARD_M           1.00f

// 在当前里程索引前后搜索最近二维路径点，避免里程误差直接变成横向误差。
#define INS_SWITCH_2D_NEAREST_SEARCH_POINTS      12U

// 换向点纵向停车容差、低速逼近区域与逼近速度。
#define INS_SWITCH_POSITION_TOL_M               0.05f
#define INS_SWITCH_CREEP_ZONE_M                 0.25f
#define INS_SWITCH_CREEP_SPEED_MPS              0.15f

// --------------------- 最终终点控制 ---------------------
#define INS_END_POSITION_COMP_M                 0.00f
#define INS_END_CREEP_ZONE_M                    0.10f
#define INS_END_CREEP_SPEED_MPS                 0.10f

// 路径速度下限，防止曲率噪声使普通路径目标速度异常接近0。
// 换向点和终点的专属制动仍可把目标速度降到0。
#define INS_MIN_PATH_SPEED                      0.50f
#define INS_MIN_SPEED_PLAN_DISTANCE_M           0.01f

// 速度表串口调试：1=打印，0=关闭
#define INS_SPEED_TABLE_DEBUG                   0
#define INS_SPEED_TABLE_PRINT_STEP              5U

// ===================== 对外接口 =====================
uint8 ins_init(void);
void ins_update(void);
void ins_start_navigation(void);
void ins_stop_navigation(void);
uint8 ins_is_navigating(void);
void ins_find_switch_point(void);
void ins_calculate_curvature_speed_limits(void);
void ins_pure_pursuit(float *left_speed, float *right_speed);

extern float g_ins_left_speed;
extern float g_ins_right_speed;

// 速度决策物理参数，可在调试阶段实时修改。
extern volatile float K_SAFE_CENTRIPETAL;
extern volatile float TARGET_DECEL;
extern volatile float START_ACCEL_LIMIT;

// 换向点二维进站调试量。
// longitudinal > 0：尚未越过换向点停车线；
// lateral > 0：当前最近路径点在车辆的记录航向左侧。
extern volatile float g_ins_switch_longitudinal_error;
extern volatile float g_ins_switch_lateral_error;
extern volatile float g_ins_switch_xy_distance;
extern volatile uint8 g_ins_switch_2d_active;

typedef enum
{
    PATH_STATE_STRAIGHT = 0,
    PATH_STATE_CURVE,
    PATH_STATE_REVERSE
} PathState_e;

extern PathState_e g_path_states[FLASH_MAX_LOG_RECORDS];

void ins_preprocess_path(void);

#endif // __INS_H






