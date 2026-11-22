/*
 * back_ins.h
 *
 *  Created on: 2026年6月13日
 *      Author: 狂蛮魔尊
 */

/*
 * back_ins.h
 *
 * 科目三反向路径复现接口，速度规划核心与科目一保持一致。
 */
#ifndef CODE_BACK_INS_H_
#define CODE_BACK_INS_H_

#include "zf_common_typedef.h"

/*
 * 科目三二维倒车路径复现。
 *
 * 使用前提：
 * 1. Flash 中的每个记录点必须包含 acc_x_g、acc_y_g、yaw_angle；
 * 2. 返航开始前车辆停在录制终点附近，并保持录制结束时的车身方向；
 * 3. back_ins_update() 每 10 ms 调用一次；
 * 4. ins_lateral_error 的单位仍为“目标前轮转角（deg）”。
 *
 * 允许返航前重新上电。back_ins_start() 会把录制终点自动对齐到
 * 当前 x/y/yaw，不要求当前惯导坐标与录制时使用同一个原点。
 */

/* Independent return limits: speed at R=1.30 m and straight-line maximum. */
extern volatile float g_back_curve_limit_speed_mps;
extern volatile float g_back_straight_max_speed_mps;
/* 运行时可调的纵向规划参数，单位均为 m/s^2。 */
extern volatile float g_back_target_decel;
extern volatile float g_back_accel_limit;
/* Extra encoder mileage driven straight after the recorded endpoint, unit: m. */
extern volatile float g_back_end_extension_m;

typedef enum
{
    BACK_INS_RESULT_IDLE = 0,
    BACK_INS_RESULT_RUNNING,
    BACK_INS_RESULT_SUCCESS,
    BACK_INS_RESULT_ABORTED,
    BACK_INS_RESULT_POSE_INVALID,
    BACK_INS_RESULT_LOST_PATH,
    BACK_INS_RESULT_CORRIDOR_VIOLATION,
    BACK_INS_RESULT_ALIGNMENT_FAILED
} BackInsResult_e;

uint8 back_ins_init(void);
uint8 back_ins_start(void);
uint8 back_ins_start_from_ram(void);
void back_ins_update(void);
void back_ins_stop(void);
uint8 back_ins_is_running(void);
BackInsResult_e back_ins_get_result(void);

/* 保留 9 版已有调试接口，菜单和示波器无需修改。 */
float back_ins_get_target_yaw(void);
float back_ins_get_yaw_error(void);
float back_ins_get_ref_yaw(void);
float back_ins_get_run_dist(void);
float back_ins_get_remaining_dist(void);
uint32 back_ins_get_current_idx(void);
uint32 back_ins_get_target_idx(void);
uint32 back_ins_get_total_points(void);
float back_ins_get_current_curvature(void);
float back_ins_get_current_speed_limit(void);
float back_ins_get_current_accel_limit(void);
float back_ins_get_stop_speed_limit(void);

/* 新增：二维跟踪与终点控制调试接口。 */
float back_ins_get_path_progress(void);
float back_ins_get_dynamic_lookahead(void);
float back_ins_get_cross_track_error(void);
float back_ins_get_target_bearing(void);
float back_ins_get_alpha_error(void);
float back_ins_get_nearest_distance(void);
float back_ins_get_end_xy_distance(void);
float back_ins_get_end_longitudinal_error(void);
float back_ins_get_end_lateral_error(void);
float back_ins_get_end_yaw_error(void);
float back_ins_get_end_steer_correction(void);
float back_ins_get_mileage_remaining(void);
float back_ins_get_target_x(void);
float back_ins_get_target_y(void);

/* Software blind-box task.  The garage pose is captured independently and is
 * never inserted into the normal return path. */
extern volatile float g_back_blind_path_distance_m;
extern volatile float g_back_blind_speed_mps;

typedef enum
{
    BACK_INS_BLIND_RESULT_IDLE = 0,
    BACK_INS_BLIND_RESULT_RUNNING,
    BACK_INS_BLIND_RESULT_SUCCESS,
    BACK_INS_BLIND_RESULT_ABORTED,
    BACK_INS_BLIND_RESULT_PATH_TOO_SHORT,
    BACK_INS_BLIND_RESULT_HOME_INVALID,
    BACK_INS_BLIND_RESULT_LOST_PATH,
    BACK_INS_BLIND_RESULT_OVERRUN,
    BACK_INS_BLIND_RESULT_TIMEOUT
} BackInsBlindResult_e;

typedef enum
{
    BACK_INS_BLIND_STAGE_IDLE = 0,
    BACK_INS_BLIND_STAGE_WAIT_FORWARD,
    BACK_INS_BLIND_STAGE_FORWARD_EXIT,
    BACK_INS_BLIND_STAGE_WAIT_REVERSE,
    BACK_INS_BLIND_STAGE_REVERSE_ENTER,
    BACK_INS_BLIND_STAGE_FINAL_STOP,
    BACK_INS_BLIND_STAGE_COMPLETE,
    BACK_INS_BLIND_STAGE_FAILED
} BackInsBlindStage_e;

void back_ins_blind_capture_home_pose(void);
uint8 back_ins_blind_start(void);
void back_ins_blind_update(void);
void back_ins_blind_stop(void);
uint8 back_ins_blind_is_running(void);
BackInsBlindResult_e back_ins_blind_get_result(void);
BackInsBlindStage_e back_ins_blind_get_stage(void);
float back_ins_blind_get_remaining_dist(void);
float back_ins_blind_get_path_error(void);
float back_ins_blind_get_progress(void);
float back_ins_blind_get_longitudinal_error(void);
float back_ins_blind_get_lateral_error(void);
float back_ins_blind_get_yaw_error(void);

#endif /* CODE_BACK_INS_H_ */
