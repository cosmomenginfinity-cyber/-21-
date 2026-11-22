/*
+ * back_ins.c
 *
 *  Created on: 2026骞?鏈?3鏃? *      Author: 澶х埍浠欏皧
 */
/*
 * back_ins.c
 *
 * 绉戠洰涓変笓鐢細鍩轰簬 XY 璺緞鍜岃褰曡埅鍚戣鐨勫€掕溅璺緞澶嶇幇銆? *
 * 鐩告瘮 back_ins(9).c 鐨勬牴鏈彉鍖栵細
 * 1. 褰撳墠璺緞杩涘害鐢?XY 鏈€杩戠偣鍐冲畾锛屼笉鍐嶇敱鍊掕溅绱閲岀▼鍐冲畾锛? * 2. Pure Pursuit 鐨?alpha 浣跨敤鈥滃綋鍓嶇偣鍒扮洰鏍?XY 鐐圭殑鏂逛綅瑙掆€濓紝
 *    涓嶅啀鎶婄洰鏍囧垏绾胯埅鍚戣宸敊璇湴浠ｅ叆 PP 鍏紡锛? * 3. 鏈熬涓嶅啀鍒囨崲鍒?XY 缁堢偣鎺у埗锛屽彧鎸夎褰曢噷绋嬪噺閫熷苟鍦ㄦ湯灏剧偣鍋滆溅锛? * 4. 褰曞埗璺緞浼氬湪鍚姩鏃惰嚜鍔ㄦ棆杞€佸钩绉诲埌褰撳墠鎯鍧愭爣绯伙紱
 * 5. 鏇茬巼闄愰€熴€佸姞鍑忛€熶紶鎾€佽蒋鍚姩鍜岄潤姝㈢‘璁ゆ部鐢ㄧ鐩竴鎴愮啛閫昏緫銆? */

#include "back_ins.h"

#include "zf_common_typedef.h"
#include "zf_common_headfile.h"
#include "zf_driver_uart.h"

#include "flash.h"
#include "imu_ahrs.h"
#include "imu_datalogger.h"
#include "encoder.h"
#include "global_vars.h"
#include "initialize.h"
#include "PID.h"
#include "motor.h"

#include <math.h>
#include <stdio.h>

#ifndef BACK_INS_PI
#define BACK_INS_PI                              3.14159265358979323846f
#endif

#define BACK_INS_DEG_TO_RAD(x)                   ((x) * BACK_INS_PI / 180.0f)
#define BACK_INS_RAD_TO_DEG(x)                   ((x) * 180.0f / BACK_INS_PI)

/* ===================== 浜岀淮 Pure Pursuit 鍙傛暟 ===================== */

#define BACK_INS_LOOKAHEAD_DISTANCE              0.30f
#define BACK_INS_LD_SPEED_COEFF                  0.40f
#define BACK_INS_LD_MIN                          0.25f
#define BACK_INS_LD_MAX                          1.60f
#define BACK_INS_TARGET_DISTANCE_MIN_M           0.05f
#define BACK_INS_STEER_MAX_ANGLE_DEG             STEER_WHEEL_MAX_ANGLE_DEG

/* 鏈€杩戠偣鍙湪涓婃绱㈠紩闄勮繎鎼滅储锛岄槻姝氦鍙夎矾寰勮烦鍒伴敊璇垎鏀€?*/
#define BACK_INS_NEAREST_BACK_POINTS             4U
#define BACK_INS_NEAREST_FORWARD_POINTS          40U

/* 鍋忕璺緞瓒呰繃璇ヨ窛绂绘椂绔嬪嵆瀹夊叏鍋滆溅锛岄槻姝㈠畾浣嶅紓甯稿悗缁х画鍊掕溅銆?*/
#define BACK_INS_LOST_PATH_DISTANCE_M            2.00f
#define BACK_INS_MILEAGE_OVERRUN_MARGIN_M        2.00f

/* ===================== 鏇茬巼涓庨€熷害瑙勫垝鍙傛暟 ===================== */

#define BACK_INS_MAX_SPEED                       7.00f
#define BACK_INS_CURVATURE_HALF_WINDOW_M         0.40f
#define BACK_INS_CURVATURE_FILTER_RADIUS_M       0.20f
#define BACK_INS_CURVATURE_YAW_DEADZONE_DEG      0.30f
#define BACK_INS_MIN_CURVATURE_DISTANCE_M        0.05f
#define BACK_INS_CURVATURE_EPSILON               0.001f
#define BACK_INS_MIN_SPEED_PLAN_DISTANCE_M       0.01f
#define BACK_INS_MIN_PATH_SPEED                  0.50f

/*
 * Curve speed at R=1.30 m fixes the lateral-acceleration limit.  The
 * independently configured straight cap fixes where speed stops rising.
 */
#define BACK_INS_CURVE_REFERENCE_RADIUS_M        1.30f
#define BACK_INS_ACCEL_RECOVERY_RADIUS_M         2.00f

#define BACK_INS_TIGHT_CURVE_ACCEL_MPS2          1.50f

/* ===================== 鏈熬鐐瑰噺閫熷仠杞﹀弬鏁?===================== */

/* 缁堢偣鍙寜杩旇埅绱閲岀▼鍒ゆ柇锛屼笉浣跨敤褰撳墠 XY 浣嶇疆銆?*/
#define BACK_INS_END_STOP_DISTANCE_M              0.05f
#define BACK_INS_END_BRAKE_MARGIN_M              0.05f
#define BACK_INS_END_CONTROL_DELAY_S             0.20f
#define BACK_INS_END_EXTENSION_DEFAULT_M          0.00f
#define BACK_INS_END_EXTENSION_MAX_M              2.00f

/* 涓庣鐩竴鍊掕溅闃舵涓€鑷达細浣庨€熼噴鏀惧墠浣跨敤杈冨皬鍔犻€熷害銆?*/
#define BACK_INS_REVERSE_ACCEL_LIMIT             3.00f
#define BACK_INS_REVERSE_SOFT_RELEASE_SPEED      0.50f

/* ===================== 璺緞鏈夋晥鎬т笌璋冭瘯 ===================== */

#define BACK_INS_MIN_VALID_PATH_LENGTH_M         0.50f
#define BACK_INS_COORD_ABS_MAX_M                 10000.0f
#define BACK_INS_SPEED_TABLE_DEBUG               0
#define BACK_INS_SPEED_TABLE_PRINT_STEP          5U

/* ===================== Software blind-box task ===================== */

#define BACK_INS_BLIND_PATH_DISTANCE_DEFAULT_M   3.40f
#define BACK_INS_BLIND_SPEED_DEFAULT_MPS         0.70f
#define BACK_INS_BLIND_PATH_MAX_POINTS           32U
#define BACK_INS_BLIND_LOOKAHEAD_BASE_M           0.30f
#define BACK_INS_BLIND_LOOKAHEAD_SPEED_GAIN       0.12f
#define BACK_INS_BLIND_LOOKAHEAD_MIN_M            0.28f
#define BACK_INS_BLIND_LOOKAHEAD_MAX_M            0.48f
#define BACK_INS_BLIND_TARGET_DISTANCE_MIN_M      0.05f
#define BACK_INS_BLIND_ACCEL_MPS2                 0.70f
#define BACK_INS_BLIND_DECEL_MPS2                 1.00f
#define BACK_INS_BLIND_APPROACH_DISTANCE_M        0.50f
#define BACK_INS_BLIND_APPROACH_SPEED_MPS         0.30f
#define BACK_INS_BLIND_TURN_SPEED_MPS             0.35f
#define BACK_INS_BLIND_CONTROL_DELAY_S            0.15f
#define BACK_INS_BLIND_BRAKE_MARGIN_M             0.03f
#define BACK_INS_BLIND_STOP_SPEED_MPS             0.08f
#define BACK_INS_BLIND_STEER_READY_DEG            3.00f
#define BACK_INS_BLIND_STOP_STABLE_TICKS          10U
#define BACK_INS_BLIND_ENTER_LONG_M               0.12f
#define BACK_INS_BLIND_ENTER_LAT_M                0.15f
#define BACK_INS_BLIND_ENTER_YAW_DEG              10.0f
#define BACK_INS_BLIND_EXIT_LONG_M                0.18f
#define BACK_INS_BLIND_EXIT_LAT_M                 0.22f
#define BACK_INS_BLIND_EXIT_YAW_DEG               15.0f
#define BACK_INS_BLIND_PROGRESS_CAPTURE_M         0.15f
#define BACK_INS_BLIND_CROSS_LAT_M                0.20f
#define BACK_INS_BLIND_LOST_PATH_M                0.50f
#define BACK_INS_BLIND_LOST_PATH_TICKS            30U
#define BACK_INS_BLIND_MILEAGE_OVERRUN_M          0.50f
#define BACK_INS_BLIND_MAX_PROGRESS_STEP_M        0.25f
#define BACK_INS_BLIND_MAX_YAW_REANCHOR_DEG       4.0f
#define BACK_INS_BLIND_WAIT_TIMEOUT_MS            3000U
#define BACK_INS_BLIND_MOTION_TIMEOUT_MS          15000U

/* ===================== 杩愯鏃跺彲璋冨弬鏁?===================== */

/*
 * At 3.00 m/s and R=1.30 m, Ay is 6.923 m/s^2.  With the default
 * independent 4.00 m/s straight cap, full speed starts near R=2.31 m.
 */
volatile float g_back_curve_limit_speed_mps = 3.00f;
volatile float g_back_straight_max_speed_mps = 4.00f;

/* 涓庢渶鏂扮鐩竴 TARGET_DECEL=2.5銆丼TART_ACCEL_LIMIT=3.0 瀵归綈銆?*/
volatile float g_back_target_decel = 2.50f;
volatile float g_back_accel_limit  = 3.00f;
volatile float g_back_end_extension_m =
    BACK_INS_END_EXTENSION_DEFAULT_M;

volatile float g_back_blind_path_distance_m =
    BACK_INS_BLIND_PATH_DISTANCE_DEFAULT_M;
volatile float g_back_blind_speed_mps =
    BACK_INS_BLIND_SPEED_DEFAULT_MPS;

/* ===================== 璺緞缂撳瓨鍜岃鍒掕〃 ===================== */

/*
 * Flash 鍊掑簭璇诲彇鍚庯細
 *   g_back_path[0]    = 褰曞埗缁堢偣锛堣繑鑸捣鐐癸級锛? *   g_back_path[last] = 褰曞埗璧风偣锛堣繑鑸粓鐐癸級銆? *
 * back_prepare_xy_path() 浼氾細
 *   - 鎶?acc_x_g / acc_y_g 鏀瑰啓涓哄綋鍓嶅潗鏍囩郴涓殑鐩爣 XY锛? *   - 鎶?yaw_angle 鏀瑰啓涓哄綋鍓嶅潗鏍囩郴涓殑鐩爣杞﹁韩鑸悜锛? *   - 鎶?linear_velocity 涓存椂瀛楁鏀瑰啓涓鸿繑鑸疮璁¤矾寰勯暱搴︺€? */
static imu_encoder_log_t g_back_path[FLASH_MAX_LOG_RECORDS]
    __attribute__((section(".bss.lmubss")));

static float g_back_raw_curvature[FLASH_MAX_LOG_RECORDS]
    __attribute__((section(".bss.lmubss")));

static float g_back_curvature[FLASH_MAX_LOG_RECORDS]
    __attribute__((section(".bss.lmubss")));

static float g_back_speed_limit[FLASH_MAX_LOG_RECORDS]
    __attribute__((section(".bss.lmubss")));

static float g_back_accel_limit_table[FLASH_MAX_LOG_RECORDS]
    __attribute__((section(".bss.lmubss")));

typedef struct
{
    float x;
    float y;
    float yaw;
    float progress;
} back_blind_path_point_t;

static back_blind_path_point_t
    g_back_blind_path[BACK_INS_BLIND_PATH_MAX_POINTS];
static uint32 g_back_blind_path_count = 0U;
static float g_back_blind_path_length = 0.0f;

/* Captured in the original outbound coordinate frame.  It is deliberately
 * separate from g_back_path so normal return recording still starts at the
 * first regular 0.20 m sample. */
static uint8 g_back_blind_home_valid = 0U;
static float g_back_blind_home_record_x = 0.0f;
static float g_back_blind_home_record_y = 0.0f;
static float g_back_blind_home_record_yaw = 0.0f;

static volatile uint8 g_back_blind_running = 0U;
static volatile BackInsBlindResult_e g_back_blind_result =
    BACK_INS_BLIND_RESULT_IDLE;
static volatile BackInsBlindStage_e g_back_blind_stage =
    BACK_INS_BLIND_STAGE_IDLE;
static float g_back_blind_progress = 0.0f;
static float g_back_blind_remaining = 0.0f;
static float g_back_blind_path_error = 0.0f;
static float g_back_blind_longitudinal_error = 0.0f;
static float g_back_blind_lateral_error = 0.0f;
static float g_back_blind_yaw_error = 0.0f;
static float g_back_blind_phase_start_mileage = 0.0f;
static uint32 g_back_blind_stage_start_ms = 0U;
static uint16 g_back_blind_stable_ticks = 0U;
static uint16 g_back_blind_lost_path_ticks = 0U;
static uint8 g_back_blind_arrival_candidate = 0U;
static uint8 g_back_blind_endpoint_crossed_latched = 0U;
static uint8 g_back_blind_last_long_valid = 0U;
static float g_back_blind_last_longitudinal = 0.0f;
static uint8 g_back_blind_end_steer_release_active = 0U;
static float g_back_blind_end_steer_release_start_progress_m = 0.0f;
static float g_back_blind_end_steer_release_start_deg = 0.0f;

/* ===================== 杩愯鐘舵€?===================== */

static uint8  g_back_initialized       = 0U;
static volatile uint8 g_back_running    = 0U;
static uint8  g_back_soft_start        = 0U;
static volatile BackInsResult_e g_back_result = BACK_INS_RESULT_IDLE;
static uint32 g_back_total_points      = 0U;
static uint32 g_back_current_idx       = 0U;
static uint32 g_back_target_idx        = 0U;

static float g_back_start_mileage      = 0.0f;
static float g_back_total_dist_m       = 0.0f;
static float g_back_control_total_dist_m = 0.0f;
static float g_back_end_extension_target_m = 0.0f;
static uint8 g_back_end_extension_active = 0U;

/* 褰曞埗缁堢偣鍙傝€冨拰鏈杩旇埅鍚姩浣嶅Э銆?*/
static float g_back_ref_x              = 0.0f;
static float g_back_ref_y              = 0.0f;
static float g_back_ref_yaw            = 0.0f;
static float g_back_start_x            = 0.0f;
static float g_back_start_y            = 0.0f;
static float g_back_start_yaw          = 0.0f;
static float g_back_alignment_yaw      = 0.0f;

/* 鎺у埗涓庤皟璇曢噺銆?*/
static float g_back_target_yaw          = 0.0f;
static float g_back_yaw_error           = 0.0f;
static float g_back_run_dist            = 0.0f;
static float g_back_path_progress       = 0.0f;
static float g_back_remaining_dist      = 0.0f;
static float g_back_dynamic_lookahead   = 0.0f;
static float g_back_cross_track_error   = 0.0f;
static float g_back_target_bearing      = 0.0f;
static float g_back_alpha_error         = 0.0f;
static float g_back_nearest_distance    = 0.0f;
static float g_back_target_x            = 0.0f;
static float g_back_target_y            = 0.0f;
static float g_back_end_xy_distance     = 0.0f;
static float g_back_end_longitudinal    = 0.0f;
static float g_back_end_lateral         = 0.0f;
static float g_back_end_yaw_error       = 0.0f;
static float g_back_end_steer_correction = 0.0f;
static float g_back_mileage_remaining   = 0.0f;
static uint8 g_back_end_steer_release_active = 0U;
static float g_back_end_steer_release_start_m = 0.0f;
static float g_back_end_steer_release_start_deg = 0.0f;
static float g_back_current_curvature   = 0.0f;
static float g_back_current_speed_limit = 0.0f;
static float g_back_current_accel_limit = 0.0f;
static float g_back_stop_speed_limit    = 0.0f;

/* ===================== 鍩虹杈呭姪鍑芥暟 ===================== */

static float back_normalize_angle(float angle)
{
    while (angle > 180.0f)
    {
        angle -= 360.0f;
    }

    while (angle < -180.0f)
    {
        angle += 360.0f;
    }

    return angle;
}

static float back_angle_diff(float target, float current)
{
    return back_normalize_angle(target - current);
}

static float back_absf(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float back_minf(float a, float b)
{
    return (a < b) ? a : b;
}

static float back_maxf(float a, float b)
{
    return (a > b) ? a : b;
}

static float back_clampf(float value, float minimum, float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }

    if (value > maximum)
    {
        return maximum;
    }

    return value;
}

static uint8 back_coordinate_is_valid(float value)
{
    /* value != value 鏄棤闇€渚濊禆 isfinite() 鐨?NaN 妫€鏌ャ€?*/
    if (value != value)
    {
        return 0U;
    }

    if (back_absf(value) > BACK_INS_COORD_ABS_MAX_M)
    {
        return 0U;
    }

    return 1U;
}

/* linear_velocity 鍦ㄨ矾寰勮浇鍏ュ悗琚笓闂ㄧ敤浣滆繑鑸疮璁¤矾寰勯暱搴︺€?*/
static float back_get_point_progress(uint32 index)
{
    if (g_back_total_points == 0U)
    {
        return 0.0f;
    }

    if (index >= g_back_total_points)
    {
        index = g_back_total_points - 1U;
    }

    return g_back_path[index].linear_velocity;
}

/* ===================== 璺緞鍧愭爣绯诲榻?===================== */

/*
 * 灏嗗綍鍒惰矾寰勪粠鈥滃綍鍒跺潗鏍囩郴鈥濆彉鎹㈠埌鈥滄湰娆¤繑鑸潗鏍囩郴鈥濄€? *
 * P_target = P_start_now + R(yaw_now - yaw_record_end)
 *                          * (P_record - P_record_end)
 *
 * 鍥犳锛? * - 涓嶉噸鏂颁笂鐢垫椂锛屽彉鎹㈡帴杩戝崟浣嶅彉鎹紱
 * - 閲嶆柊涓婄數鍚庯紝鍗充娇褰撳墠 x/y/yaw 浠?0 寮€濮嬶紝涔熻兘浣跨敤鍚屼竴鏉¤矾寰勶紱
 * - 褰曞埗缁堢偣涓€瀹氫笌杩旇埅鍚姩浣嶇疆閲嶅悎锛屾棤闇€浜哄伐澶栫Щ璧风偣銆? */
static uint8 back_prepare_xy_path(void)
{
    uint32 i;
    float align_rad;
    float align_cos;
    float align_sin;
    float previous_progress;
    int64_t record_end_mileage_mm;

    if (g_back_total_points < 3U)
    {
        return 0U;
    }

    g_back_ref_x   = g_back_path[0U].acc_x_g;
    g_back_ref_y   = g_back_path[0U].acc_y_g;
    g_back_ref_yaw = g_back_path[0U].yaw_angle;

    g_back_start_x   = imu_ahrs_get_pos_x();
    g_back_start_y   = imu_ahrs_get_pos_y();
    g_back_start_yaw = imu_ahrs_get_yaw();

    if (!back_coordinate_is_valid(g_back_ref_x)
        || !back_coordinate_is_valid(g_back_ref_y)
        || !back_coordinate_is_valid(g_back_ref_yaw)
        || !back_coordinate_is_valid(g_back_start_x)
        || !back_coordinate_is_valid(g_back_start_y)
        || !back_coordinate_is_valid(g_back_start_yaw))
    {
        return 0U;
    }

    g_back_alignment_yaw =
        back_angle_diff(g_back_start_yaw, g_back_ref_yaw);

    align_rad = BACK_INS_DEG_TO_RAD(g_back_alignment_yaw);
    align_cos = cosf(align_rad);
    align_sin = sinf(align_rad);

    for (i = 0U; i < g_back_total_points; i++)
    {
        float raw_x = g_back_path[i].acc_x_g;
        float raw_y = g_back_path[i].acc_y_g;
        float raw_yaw = g_back_path[i].yaw_angle;
        float relative_x;
        float relative_y;

        if (!back_coordinate_is_valid(raw_x)
            || !back_coordinate_is_valid(raw_y)
            || !back_coordinate_is_valid(raw_yaw))
        {
            return 0U;
        }

        relative_x = raw_x - g_back_ref_x;
        relative_y = raw_y - g_back_ref_y;

        g_back_path[i].acc_x_g =
            g_back_start_x
            + align_cos * relative_x
            - align_sin * relative_y;

        g_back_path[i].acc_y_g =
            g_back_start_y
            + align_sin * relative_x
            + align_cos * relative_y;

        g_back_path[i].yaw_angle =
            back_normalize_angle(
                g_back_start_yaw
                + back_angle_diff(raw_yaw, g_back_ref_yaw));
    }

    /*
     * 杩旇埅杩涘害鍜屾湯灏惧仠杞﹁窛绂荤粺涓€浣跨敤褰曞埗缂栫爜鍣ㄩ噷绋嬨€?     * Flash 宸插€掑簭锛屽洜姝ょ 0 鐐规槸褰曞埗鏈熬锛屽悗缁偣鐩稿瀹冪殑閲岀▼宸?     * 灏辨槸杩旇埅绱杩涘害锛沊Y 鍙敤浜庢甯歌矾寰勮窡韪紝涓嶅弬涓庢湯灏惧垽鏂€?     */
    record_end_mileage_mm = g_back_path[0U].main_encoder_cnt;
    previous_progress = 0.0f;
    g_back_path[0U].linear_velocity = 0.0f;

    for (i = 1U; i < g_back_total_points; i++)
    {
        float point_progress =
            (float)(record_end_mileage_mm
                    - g_back_path[i].main_encoder_cnt)
            / 1000.0f;

        if (point_progress < 0.0f)
        {
            point_progress = -point_progress;
        }

        if (!back_coordinate_is_valid(point_progress))
        {
            return 0U;
        }

        /* 缂栫爜鍣ㄩ噺鍖栨垨鍋跺彂鎶栧姩涓嶈兘璁╁€掑簭璺緞杩涘害鍥為€€銆?*/
        if (point_progress < previous_progress)
        {
            point_progress = previous_progress;
        }

        g_back_path[i].linear_velocity = point_progress;
        previous_progress = point_progress;
    }

    g_back_total_dist_m = previous_progress;

    if (g_back_total_dist_m < BACK_INS_MIN_VALID_PATH_LENGTH_M)
    {
        return 0U;
    }

    return 1U;
}

/* ===================== 浜岀淮鏈€杩戠偣涓庡墠鐬荤偣 ===================== */

static uint32 back_project_xy_to_path(
    float current_x,
    float current_y,
    uint32 center_index,
    float minimum_progress,
    float *projected_progress,
    float *projected_x,
    float *projected_y,
    float *projected_yaw,
    float *nearest_distance)
{
    uint32 i;
    uint32 last_index;
    uint32 start_segment;
    uint32 end_segment;
    uint32 best_segment;
    float best_distance_sq = 1.0e30f;
    float best_progress = 0.0f;
    float lower_s;
    float upper_s;
    float ratio;
    float path_x;
    float path_y;
    float path_yaw;

    if (g_back_total_points < 2U)
    {
        path_x = (g_back_total_points == 1U)
               ? g_back_path[0U].acc_x_g
               : current_x;
        path_y = (g_back_total_points == 1U)
               ? g_back_path[0U].acc_y_g
               : current_y;
        path_yaw = (g_back_total_points == 1U)
                 ? g_back_path[0U].yaw_angle
                 : 0.0f;

        if (projected_progress != NULL) *projected_progress = 0.0f;
        if (projected_x != NULL) *projected_x = path_x;
        if (projected_y != NULL) *projected_y = path_y;
        if (projected_yaw != NULL) *projected_yaw = path_yaw;
        if (nearest_distance != NULL)
        {
            float dx = path_x - current_x;
            float dy = path_y - current_y;
            *nearest_distance = sqrtf(dx * dx + dy * dy);
        }
        return 0U;
    }

    last_index = g_back_total_points - 1U;
    if (center_index >= last_index)
    {
        center_index = last_index - 1U;
    }

    start_segment =
        (center_index > BACK_INS_NEAREST_BACK_POINTS)
        ? (center_index - BACK_INS_NEAREST_BACK_POINTS)
        : 0U;
    end_segment = center_index + BACK_INS_NEAREST_FORWARD_POINTS;
    if (end_segment >= last_index)
    {
        end_segment = last_index - 1U;
    }

    best_segment = center_index;
    for (i = start_segment; i <= end_segment; i++)
    {
        float segment_dx =
            g_back_path[i + 1U].acc_x_g - g_back_path[i].acc_x_g;
        float segment_dy =
            g_back_path[i + 1U].acc_y_g - g_back_path[i].acc_y_g;
        float segment_length_sq =
            segment_dx * segment_dx + segment_dy * segment_dy;
        float t = 0.0f;
        float candidate_x;
        float candidate_y;
        float dx;
        float dy;
        float distance_sq;

        if (segment_length_sq > 0.000001f)
        {
            t = ((current_x - g_back_path[i].acc_x_g) * segment_dx
               + (current_y - g_back_path[i].acc_y_g) * segment_dy)
              / segment_length_sq;
            t = back_clampf(t, 0.0f, 1.0f);
        }

        candidate_x = g_back_path[i].acc_x_g + t * segment_dx;
        candidate_y = g_back_path[i].acc_y_g + t * segment_dy;
        dx = candidate_x - current_x;
        dy = candidate_y - current_y;
        distance_sq = dx * dx + dy * dy;

        if (distance_sq < best_distance_sq)
        {
            best_distance_sq = distance_sq;
            best_segment = i;
            best_progress = back_get_point_progress(i)
                          + t * (back_get_point_progress(i + 1U)
                               - back_get_point_progress(i));
        }
    }

    best_progress = back_clampf(
        best_progress,
        minimum_progress,
        g_back_total_dist_m);

    while ((best_segment > 0U)
           && (best_progress < back_get_point_progress(best_segment)))
    {
        best_segment--;
    }
    while ((best_segment < (last_index - 1U))
           && (best_progress
               >= back_get_point_progress(best_segment + 1U)))
    {
        best_segment++;
    }

    lower_s = back_get_point_progress(best_segment);
    upper_s = back_get_point_progress(best_segment + 1U);
    if ((upper_s - lower_s) > 0.0001f)
    {
        ratio = (best_progress - lower_s) / (upper_s - lower_s);
        ratio = back_clampf(ratio, 0.0f, 1.0f);
    }
    else
    {
        ratio = 0.0f;
    }

    path_x = g_back_path[best_segment].acc_x_g
           + ratio * (g_back_path[best_segment + 1U].acc_x_g
                    - g_back_path[best_segment].acc_x_g);
    path_y = g_back_path[best_segment].acc_y_g
           + ratio * (g_back_path[best_segment + 1U].acc_y_g
                    - g_back_path[best_segment].acc_y_g);
    path_yaw = back_normalize_angle(
        g_back_path[best_segment].yaw_angle
        + ratio * back_angle_diff(
            g_back_path[best_segment + 1U].yaw_angle,
            g_back_path[best_segment].yaw_angle));

    best_distance_sq =
        (path_x - current_x) * (path_x - current_x)
      + (path_y - current_y) * (path_y - current_y);

    if (projected_progress != NULL) *projected_progress = best_progress;
    if (projected_x != NULL) *projected_x = path_x;
    if (projected_y != NULL) *projected_y = path_y;
    if (projected_yaw != NULL) *projected_yaw = path_yaw;
    if (nearest_distance != NULL)
    {
        *nearest_distance = sqrtf(best_distance_sq);
    }

    return best_segment;
}

/*
 * 鎸夎矾寰勭疮璁¤窛绂诲鎵惧墠鐬荤偣锛屽苟鍦ㄧ浉閭昏褰曠偣闂寸嚎鎬ф彃鍊笺€? * 杩欐牱璁板綍鐐归棿璺濊緝澶ф椂锛岀洰鏍囩偣涔熶笉浼氫竴鏍间竴鏍艰烦鍙樸€? */
static void back_interpolate_target(
    float target_progress,
    uint32 start_index,
    float *target_x,
    float *target_y,
    float *target_yaw,
    uint32 *target_index)
{
    uint32 i;
    uint32 upper_index;
    uint32 lower_index;
    float lower_s;
    float upper_s;
    float ratio;

    if (start_index >= g_back_total_points)
    {
        start_index = g_back_total_points - 1U;
    }

    upper_index = g_back_total_points - 1U;

    for (i = start_index; i < g_back_total_points; i++)
    {
        if (back_get_point_progress(i) >= target_progress)
        {
            upper_index = i;
            break;
        }
    }

    lower_index =
        (upper_index > start_index)
        ? (upper_index - 1U)
        : upper_index;

    lower_s = back_get_point_progress(lower_index);
    upper_s = back_get_point_progress(upper_index);

    if ((upper_s - lower_s) > 0.0001f)
    {
        ratio = (target_progress - lower_s) / (upper_s - lower_s);
        ratio = back_clampf(ratio, 0.0f, 1.0f);
    }
    else
    {
        ratio = 1.0f;
    }

    *target_x =
        g_back_path[lower_index].acc_x_g
        + ratio * (g_back_path[upper_index].acc_x_g
                 - g_back_path[lower_index].acc_x_g);

    *target_y =
        g_back_path[lower_index].acc_y_g
        + ratio * (g_back_path[upper_index].acc_y_g
                 - g_back_path[lower_index].acc_y_g);

    *target_yaw = back_normalize_angle(
        g_back_path[lower_index].yaw_angle
        + ratio * back_angle_diff(
            g_back_path[upper_index].yaw_angle,
            g_back_path[lower_index].yaw_angle));

    *target_index = upper_index;
}

/* Interpolate the path profile by continuous progress so speed, acceleration
 * and curvature do not step whenever the nearest recorded index changes. */
static void back_interpolate_profile(
    float progress,
    uint32 lower_index,
    float *speed_limit,
    float *accel_limit,
    float *curvature)
{
    uint32 upper_index;
    float lower_s;
    float upper_s;
    float ratio;

    if (g_back_total_points == 0U)
    {
        if (speed_limit != NULL) *speed_limit = 0.0f;
        if (accel_limit != NULL) *accel_limit = 0.0f;
        if (curvature != NULL) *curvature = 0.0f;
        return;
    }

    if (lower_index >= (g_back_total_points - 1U))
    {
        lower_index = g_back_total_points - 1U;
        upper_index = lower_index;
        ratio = 0.0f;
    }
    else
    {
        upper_index = lower_index + 1U;
        lower_s = back_get_point_progress(lower_index);
        upper_s = back_get_point_progress(upper_index);
        if ((upper_s - lower_s) > 0.0001f)
        {
            ratio = (progress - lower_s) / (upper_s - lower_s);
            ratio = back_clampf(ratio, 0.0f, 1.0f);
        }
        else
        {
            ratio = 0.0f;
        }
    }

    if (speed_limit != NULL)
    {
        *speed_limit = g_back_speed_limit[lower_index]
                     + ratio * (g_back_speed_limit[upper_index]
                              - g_back_speed_limit[lower_index]);
    }
    if (accel_limit != NULL)
    {
        *accel_limit = g_back_accel_limit_table[lower_index]
                     + ratio * (g_back_accel_limit_table[upper_index]
                              - g_back_accel_limit_table[lower_index]);
    }
    if (curvature != NULL)
    {
        *curvature = g_back_curvature[lower_index]
                   + ratio * (g_back_curvature[upper_index]
                            - g_back_curvature[lower_index]);
    }
}

/* ===================== 鏇茬巼閫熷害瑙勫垝 ===================== */

static uint8 back_calculate_speed_profile(uint8 print_summary)
{
    uint32 i;
    uint32 last_index;
    float v_max;
    float curve_limit_speed;
    float ay_limit;
    float full_speed_radius;
    float target_decel;
    float max_accel;
    float tight_curve_accel;

    if (g_back_total_points < 3U)
    {
        return 0U;
    }

    last_index = g_back_total_points - 1U;

    curve_limit_speed = back_clampf(
        back_absf(g_back_curve_limit_speed_mps),
        BACK_INS_MIN_PATH_SPEED,
        BACK_INS_MAX_SPEED);
    v_max = back_clampf(
        back_absf(g_back_straight_max_speed_mps),
        curve_limit_speed,
        BACK_INS_MAX_SPEED);

    ay_limit = back_maxf(
        (curve_limit_speed * curve_limit_speed)
        / BACK_INS_CURVE_REFERENCE_RADIUS_M,
        0.10f);
    full_speed_radius = (v_max * v_max) / ay_limit;
    full_speed_radius = back_maxf(
        full_speed_radius,
        BACK_INS_CURVE_REFERENCE_RADIUS_M);
    target_decel = back_maxf(g_back_target_decel, 0.10f);
    max_accel = back_maxf(g_back_accel_limit, 0.10f);

    tight_curve_accel = BACK_INS_TIGHT_CURVE_ACCEL_MPS2;
    if (tight_curve_accel > max_accel)
    {
        tight_curve_accel = max_accel;
    }

    for (i = 0U; i < g_back_total_points; i++)
    {
        g_back_raw_curvature[i] = 0.0f;
        g_back_curvature[i] = 0.0f;
        g_back_speed_limit[i] = v_max;
        g_back_accel_limit_table[i] = max_accel;
    }

    /* 绗?姝ワ細鎸夊鐐圭獥鍙ｇ疮璁＄粷瀵硅埅鍚戝彉鍖栵紝璁＄畻鍘熷鏇茬巼銆?*/
    /* Estimate signed curvature in a fixed physical-distance window. */
    for (i = 0U; i <= last_index; i++)
    {
        uint32 left = i;
        uint32 right = i;
        uint32 j;
        float sum_xx = 0.0f;
        float sum_xy = 0.0f;

        while ((left > 0U)
               && ((back_get_point_progress(i)
                    - back_get_point_progress(left))
                   < BACK_INS_CURVATURE_HALF_WINDOW_M))
        {
            left--;
        }

        while ((right < last_index)
               && ((back_get_point_progress(right)
                    - back_get_point_progress(i))
                   < BACK_INS_CURVATURE_HALF_WINDOW_M))
        {
            right++;
        }

        for (j = left; j <= right; j++)
        {
            float x = back_get_point_progress(j)
                    - back_get_point_progress(i);
            float yaw_delta_rad = BACK_INS_DEG_TO_RAD(
                back_angle_diff(
                    g_back_path[j].yaw_angle,
                    g_back_path[i].yaw_angle));

            sum_xx += x * x;
            sum_xy += x * yaw_delta_rad;
        }

        if (sum_xx
            >= (BACK_INS_MIN_CURVATURE_DISTANCE_M
              * BACK_INS_MIN_CURVATURE_DISTANCE_M))
        {
            g_back_raw_curvature[i] = sum_xy / sum_xx;

            if (back_absf(g_back_raw_curvature[i])
                < (BACK_INS_DEG_TO_RAD(
                    BACK_INS_CURVATURE_YAW_DEADZONE_DEG)
                   / BACK_INS_CURVATURE_HALF_WINDOW_M))
            {
                g_back_raw_curvature[i] = 0.0f;
            }
        }
    }

    /* 绗?姝ワ細骞虫粦鏇茬巼骞剁敓鎴愭瘡鐐归€熷害銆佺旱鍚戝姞閫熷害涓婇檺銆?*/
    /* Smooth signed curvature in a short physical-distance window. */
    for (i = 0U; i <= last_index; i++)
    {
        uint32 j;
        uint32 start = i;
        uint32 end = i;
        uint32 count = 0U;
        float sum = 0.0f;
        float point_speed_limit = v_max;
        float point_accel_limit = max_accel;
        float curvature_magnitude;

        while ((start > 0U)
               && ((back_get_point_progress(i)
                    - back_get_point_progress(start))
                   < BACK_INS_CURVATURE_FILTER_RADIUS_M))
        {
            start--;
        }

        while ((end < last_index)
               && ((back_get_point_progress(end)
                    - back_get_point_progress(i))
                   < BACK_INS_CURVATURE_FILTER_RADIUS_M))
        {
            end++;
        }

        for (j = start; j <= end; j++)
        {
            sum += g_back_raw_curvature[j];
            count++;
        }

        if (count > 0U)
        {
            g_back_curvature[i] = sum / (float)count;
        }

        curvature_magnitude = back_absf(g_back_curvature[i]);

        if (curvature_magnitude > BACK_INS_CURVATURE_EPSILON)
        {
            float radius = 1.0f / curvature_magnitude;
            float effective_radius;

            /*
             * The measured curve limit is defined at R=1.30 m.  Keep real
             * radii below 1.30 m so tighter recorded bends receive
             * lower speed.  Large radii stop rising at the dynamic radius
             * corresponding to the independent straight speed cap.
             */
            effective_radius = back_minf(
                radius,
                full_speed_radius);
            point_speed_limit = sqrtf(ay_limit * effective_radius);

            if (radius <= BACK_INS_CURVE_REFERENCE_RADIUS_M)
            {
                point_accel_limit = tight_curve_accel;
            }
            else if (radius < BACK_INS_ACCEL_RECOVERY_RADIUS_M)
            {
                float ratio =
                    (radius - BACK_INS_CURVE_REFERENCE_RADIUS_M)
                    / (BACK_INS_ACCEL_RECOVERY_RADIUS_M
                     - BACK_INS_CURVE_REFERENCE_RADIUS_M);

                point_accel_limit =
                    tight_curve_accel
                    + ratio * (max_accel - tight_curve_accel);
            }
            else
            {
                point_accel_limit = max_accel;
            }
        }

        point_speed_limit = back_clampf(
            point_speed_limit,
            BACK_INS_MIN_PATH_SPEED,
            v_max);

        point_accel_limit = back_clampf(
            point_accel_limit,
            tight_curve_accel,
            max_accel);

        g_back_speed_limit[i] = point_speed_limit;
        g_back_accel_limit_table[i] = point_accel_limit;
    }

    /* Propagate braking capability backwards so the vehicle has already
     * slowed before it reaches the next curvature-limited segment. */
    for (i = last_index; i > 0U; i--)
    {
        uint32 previous_index = i - 1U;
        float ds = back_get_point_progress(i)
                 - back_get_point_progress(previous_index);
        float v_next;
        float max_safe_v;

        if (ds < BACK_INS_MIN_SPEED_PLAN_DISTANCE_M)
        {
            ds = BACK_INS_MIN_SPEED_PLAN_DISTANCE_M;
        }

        v_next = g_back_speed_limit[i];
        max_safe_v = sqrtf(
            v_next * v_next + 2.0f * target_decel * ds);

        if (g_back_speed_limit[previous_index] > max_safe_v)
        {
            g_back_speed_limit[previous_index] = max_safe_v;
        }
    }

    /* Propagate acceleration capability along the actual return direction. */
    for (i = 1U; i <= last_index; i++)
    {
        float ds = back_get_point_progress(i)
                 - back_get_point_progress(i - 1U);
        float v_prev;
        float local_accel;
        float max_reachable_v;

        if (ds < BACK_INS_MIN_SPEED_PLAN_DISTANCE_M)
        {
            ds = BACK_INS_MIN_SPEED_PLAN_DISTANCE_M;
        }

        v_prev = g_back_speed_limit[i - 1U];
        local_accel = g_back_accel_limit_table[i];

        max_reachable_v = sqrtf(
            v_prev * v_prev + 2.0f * local_accel * ds);

        if (g_back_speed_limit[i] > max_reachable_v)
        {
            g_back_speed_limit[i] = max_reachable_v;
        }
    }

#if BACK_INS_SPEED_TABLE_DEBUG
    printf("IDX,DIST_M,KAPPA,RADIUS_M,SPEED_MPS,ACCEL_MPS2\r\n");

    for (i = 0U;
         i < g_back_total_points;
         i += BACK_INS_SPEED_TABLE_PRINT_STEP)
    {
        float radius_m = 9999.0f;

        if (back_absf(g_back_curvature[i])
            > BACK_INS_CURVATURE_EPSILON)
        {
            radius_m = 1.0f / back_absf(g_back_curvature[i]);
        }

        printf("%lu,%.3f,%.5f,%.3f,%.3f,%.3f\r\n",
               (unsigned long)i,
               back_get_point_progress(i),
               g_back_curvature[i],
               radius_m,
               g_back_speed_limit[i],
               g_back_accel_limit_table[i]);
    }
#endif

    if (print_summary)
    {
        printf("[BACK_INS_XY] Points=%lu, XYLength=%.3f, Vmax=%.3f, "
               "Vcurve=%.3f@R=%.3f, Ay=%.3f, Aacc=%.3f, "
               "Abrake=%.3f\r\n",
               (unsigned long)g_back_total_points,
               (double)g_back_total_dist_m,
               (double)v_max,
               (double)curve_limit_speed,
               (double)BACK_INS_CURVE_REFERENCE_RADIUS_M,
               (double)ay_limit,
               (double)max_accel,
               (double)target_decel);
    }

    return 1U;
}

/* ===================== 鐘舵€佹竻鐞?===================== */

static void back_clear_debug_values(void)
{
    g_back_target_yaw          = 0.0f;
    g_back_yaw_error           = 0.0f;
    g_back_run_dist            = 0.0f;
    g_back_path_progress       = 0.0f;
    g_back_remaining_dist      = 0.0f;
    g_back_dynamic_lookahead   = 0.0f;
    g_back_cross_track_error   = 0.0f;
    g_back_target_bearing      = 0.0f;
    g_back_alpha_error         = 0.0f;
    g_back_nearest_distance    = 0.0f;
    g_back_target_x            = 0.0f;
    g_back_target_y            = 0.0f;
    g_back_end_xy_distance     = 0.0f;
    g_back_end_longitudinal    = 0.0f;
    g_back_end_lateral         = 0.0f;
    g_back_end_yaw_error       = 0.0f;
    g_back_end_steer_correction = 0.0f;
    g_back_mileage_remaining   = 0.0f;
    g_back_end_steer_release_active = 0U;
    g_back_end_steer_release_start_m = 0.0f;
    g_back_end_steer_release_start_deg = 0.0f;
    g_back_current_curvature   = 0.0f;
    g_back_current_speed_limit = 0.0f;
    g_back_current_accel_limit = 0.0f;
    g_back_stop_speed_limit    = 0.0f;
}

/* ===================== 瀵瑰鎺ュ彛 ===================== */

uint8 back_ins_init(void)
{
    g_back_initialized = 0U;
    g_back_running = 0U;
    g_back_soft_start = 0U;
    g_back_result = BACK_INS_RESULT_IDLE;
    g_back_total_points = 0U;
    g_back_current_idx = 0U;
    g_back_target_idx = 0U;

    g_back_start_mileage = 0.0f;
    g_back_total_dist_m = 0.0f;
    g_back_control_total_dist_m = 0.0f;
    g_back_end_extension_target_m = 0.0f;
    g_back_end_extension_active = 0U;
    g_back_ref_x = 0.0f;
    g_back_ref_y = 0.0f;
    g_back_ref_yaw = 0.0f;
    g_back_start_x = 0.0f;
    g_back_start_y = 0.0f;
    g_back_start_yaw = 0.0f;
    g_back_alignment_yaw = 0.0f;

    back_clear_debug_values();

    if (flash_log_init() != FLASH_OK)
    {
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Flash init failed.\r\n");
        return 0U;
    }

    g_back_initialized = 1U;
    uart_write_string(UART_0, "\r\n[BACK_INS_XY] Init OK.\r\n");

    return 1U;
}

static uint8 back_ins_activate_loaded_path(uint8 silent_start)
{
    float target_decel;

    if (g_back_total_points < 3U)
    {
        return 0U;
    }

    /*
     * 娉ㄦ剰锛氳繖閲屼笉鑳?imu_ahrs_reset_position()銆?     * 褰撳墠璧风偣鍙兘浠嶅浜庡綍鍒跺潗鏍囩郴锛屼篃鍙兘鍥犻噸鏂颁笂鐢佃€屼粠闆跺紑濮嬶紱
     * back_prepare_xy_path() 浼氳嚜鍔ㄥ畬鎴愪袱绉嶆儏鍐电殑鍧愭爣瀵归綈銆?     */
    if (!back_prepare_xy_path())
    {
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Invalid or missing XY path.\r\n");
        return 0U;
    }

    if (!back_calculate_speed_profile((uint8)(!silent_start)))
    {
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Speed profile failed.\r\n");
        return 0U;
    }

    /*
     * Add a manually tunable open-loop mileage segment after the recorded
     * return path.  Capture it once at start so changing the tuning variable
     * while the vehicle is moving cannot move the stop point unexpectedly.
     */
    g_back_end_extension_target_m = back_clampf(
        g_back_end_extension_m,
        0.0f,
        BACK_INS_END_EXTENSION_MAX_M);
    g_back_control_total_dist_m =
        g_back_total_dist_m + g_back_end_extension_target_m;
    g_back_end_extension_active = 0U;

    g_back_start_mileage = encoder_get_total_mileage();

    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.target_ramp  = 0.0f;

    target_decel = back_maxf(g_back_target_decel, 0.10f);

    g_speed_mgr.accel_step =
        BACK_INS_REVERSE_ACCEL_LIMIT / CONTROL_FREQ;
    g_speed_mgr.decel_step = target_decel / CONTROL_FREQ;
    g_speed_mgr.mode = SPEED_MODE_REVERSE;

    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
    Motor_Control(0.0f, 0.0f);

    g_back_current_idx = 0U;
    g_back_target_idx = 0U;
    g_back_soft_start = 1U;

    back_clear_debug_values();
    g_back_remaining_dist = g_back_control_total_dist_m;
    g_back_current_curvature = g_back_curvature[0U];
    g_back_current_speed_limit = g_back_speed_limit[0U];
    g_back_current_accel_limit = g_back_accel_limit_table[0U];

    ins_lateral_error = 0.0f;
    ins_target_yaw = g_back_start_yaw;
    ins_target_speed = 0.0f;
    ins_gyro_z = 0.0f;

    g_back_result = BACK_INS_RESULT_RUNNING;
    g_back_running = 1U;

    if (!silent_start)
    {
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Reverse XY tracking started.\r\n");
        printf("[BACK_INS_XY] Path=%.3f, EndExtension=%.3f, "
               "StopMileage=%.3f\r\n",
               (double)g_back_total_dist_m,
               (double)g_back_end_extension_target_m,
               (double)g_back_control_total_dist_m);
    }

    return 1U;
}

uint8 back_ins_start(void)
{
    uint32 flash_count;

    if (!g_back_initialized)
    {
        if (!back_ins_init())
        {
            return 0U;
        }
    }

    if (g_back_running)
    {
        return 1U;
    }

    flash_count = flash_log_get_count();

    if (flash_count < 3U)
    {
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Not enough Flash records.\r\n");
        return 0U;
    }

    if (flash_count > FLASH_MAX_LOG_RECORDS)
    {
        flash_count = FLASH_MAX_LOG_RECORDS;
    }

    g_back_total_points = flash_log_read_reverse_batch(
        g_back_path,
        flash_count - 1U,
        flash_count);

    if (g_back_total_points < 3U)
    {
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Reverse Flash read failed.\r\n");
        return 0U;
    }

    return back_ins_activate_loaded_path(0U);
}

uint8 back_ins_start_from_ram(void)
{
    if (!g_back_initialized)
    {
        if (!back_ins_init())
        {
            return 0U;
        }
    }

    if (g_back_running)
    {
        return 1U;
    }

    g_back_total_points = imu_logger_read_buffer_reverse(
        g_back_path,
        FLASH_MAX_LOG_RECORDS);

    if (g_back_total_points < 3U)
    {
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Not enough RAM records.\r\n");
        return 0U;
    }

    return back_ins_activate_loaded_path(1U);
}

void back_ins_update(void)
{
    uint32 last_index;
    uint8 end_target_reached;
    float current_x;
    float current_y;
    float current_yaw;
    float current_speed;
    float previous_path_progress;
    float projected_path_x;
    float projected_path_y;
    float projected_path_yaw;
    float target_progress;
    float target_distance;
    float target_dx;
    float target_dy;
    float reverse_motion_yaw;
    float alpha_rad;
    float pp_angle_rad;
    float pp_angle_deg;
    float end_steer_release_span;
    float end_steer_release_ratio;
    float nearest_motion_yaw_rad;
    float vehicle_from_path_x;
    float vehicle_from_path_y;
    float odometer_progress;
    float control_remaining;
    float effective_end_dist;
    float path_speed_limit;
    float local_accel_limit;
    float target_decel;
    float planned_speed;
    ahrs_data_t imu_data;

    if (!g_back_running)
    {
        return;
    }

    last_index = g_back_total_points - 1U;

    current_x = imu_ahrs_get_pos_x();
    current_y = imu_ahrs_get_pos_y();
    current_yaw = imu_ahrs_get_yaw();

    current_speed =
        (back_absf(encoder_speed_left)
       + back_absf(encoder_speed_right)) * 0.5f;

    imu_data = imu_ahrs_get_data();
    ins_gyro_z = imu_data.gyro_z_dps;

    /* 缂栫爜鍣ㄩ噷绋嬩笓闂ㄧ敤浜庢湯灏剧偣鍑忛€熴€佸仠杞﹀拰瓒呯▼淇濇姢銆?*/
    g_back_run_dist =
        encoder_get_total_mileage() - g_back_start_mileage;
    if (g_back_run_dist < 0.0f)
    {
        g_back_run_dist = -g_back_run_dist;
    }

    odometer_progress = back_clampf(
        g_back_run_dist,
        0.0f,
        g_back_control_total_dist_m);

    g_back_mileage_remaining =
        g_back_control_total_dist_m - odometer_progress;

    if ((!g_back_end_extension_active)
        && (g_back_end_extension_target_m > 0.0f)
        && (g_back_run_dist >= g_back_total_dist_m))
    {
        g_back_end_extension_active = 1U;
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] End extension active.\r\n");
    }

    /* Project current XY onto the nearest path segment to obtain continuous,
     * monotonic progress rather than a jumping nearest-point index. */
    previous_path_progress = g_back_path_progress;
    g_back_current_idx = back_project_xy_to_path(
        current_x,
        current_y,
        g_back_current_idx,
        previous_path_progress,
        &g_back_path_progress,
        &projected_path_x,
        &projected_path_y,
        &projected_path_yaw,
        &g_back_nearest_distance);

    /* 瀹氫綅鏄庢樉澶辨晥鎴栧凡缁忚繙瓒呰矾寰勯暱搴︽椂锛岀珛鍗冲仠杞︺€?*/
    if (((!g_back_end_extension_active)
         && (g_back_nearest_distance > BACK_INS_LOST_PATH_DISTANCE_M))
        || (g_back_run_dist
            > g_back_control_total_dist_m
              + BACK_INS_MILEAGE_OVERRUN_MARGIN_M))
    {
        g_back_result = BACK_INS_RESULT_LOST_PATH;
        uart_write_string(
            UART_0,
            "\r\n[BACK_INS_XY] Lost path, emergency stop.\r\n");
        back_ins_stop();
        return;
    }

    /* 鏈熬鍙繚鐣欒褰曡埅鍚戣锛屼笉鍐嶈绠楃粓鐐?XY銆佺旱鍚戞垨妯悜璇樊銆?*/
    g_back_end_yaw_error = back_angle_diff(
        g_back_path[last_index].yaw_angle,
        current_yaw);

    target_decel = back_maxf(g_back_target_decel, 0.10f);

    /* 2. 鍔ㄦ€佸墠鐬伙紝骞跺鍓嶇灮璺緞鐐硅繘琛?XY 鎻掑€笺€?*/
    g_back_dynamic_lookahead =
        BACK_INS_LOOKAHEAD_DISTANCE
        + BACK_INS_LD_SPEED_COEFF * current_speed;

    g_back_dynamic_lookahead = back_clampf(
        g_back_dynamic_lookahead,
        BACK_INS_LD_MIN,
        BACK_INS_LD_MAX);

    target_progress =
        g_back_path_progress + g_back_dynamic_lookahead;

    end_target_reached = 0U;
    if (target_progress >= g_back_total_dist_m)
    {
        target_progress = g_back_total_dist_m;
        end_target_reached = 1U;
    }

    back_interpolate_target(
        target_progress,
        g_back_current_idx,
        &g_back_target_x,
        &g_back_target_y,
        &g_back_target_yaw,
        &g_back_target_idx);

    ins_target_yaw = g_back_target_yaw;
    g_back_yaw_error =
        back_angle_diff(g_back_target_yaw, current_yaw);

    /* 3. 鏍囧噯浜岀淮鍊掕溅 Pure Pursuit銆?*/
    target_dx = g_back_target_x - current_x;
    target_dy = g_back_target_y - current_y;
    target_distance = sqrtf(
        target_dx * target_dx + target_dy * target_dy);

    g_back_target_bearing = BACK_INS_RAD_TO_DEG(
        atan2f(target_dy, target_dx));

    /* 鍊掕溅鏃跺疄闄呰繍鍔ㄦ柟鍚戞槸杞﹁韩 yaw + 180掳銆?*/
    reverse_motion_yaw =
        back_normalize_angle(current_yaw + 180.0f);

    g_back_alpha_error = back_angle_diff(
        g_back_target_bearing,
        reverse_motion_yaw);

    alpha_rad = BACK_INS_DEG_TO_RAD(g_back_alpha_error);

    if (target_distance <= BACK_INS_TARGET_DISTANCE_MIN_M)
    {
        /* 鐩爣鐐逛笌杞﹁締宸茬粡閲嶅悎锛岄伩鍏?atan2(0,0) 閫犳垚鐬椂婊¤埖銆?*/
        pp_angle_deg = 0.0f;
    }
    else
    {
        target_distance = back_maxf(
            target_distance,
            BACK_INS_TARGET_DISTANCE_MIN_M);

        /*
         * 鍓嶈疆杞悜杞﹁締鍊掕溅鏃讹細kappa_motion = -tan(delta)/L锛?         * 鍥犳鏍囧噯 PP 杞蹇呴』鍙栧弽銆?         */
        pp_angle_rad = -atan2f(
            2.0f * WHEEL_BASE * sinf(alpha_rad),
            target_distance);

        pp_angle_deg = BACK_INS_RAD_TO_DEG(pp_angle_rad);
    }


    g_back_end_steer_correction = 0.0f;

    pp_angle_deg = back_clampf(
        pp_angle_deg,
        -BACK_INS_STEER_MAX_ANGLE_DEG,
        BACK_INS_STEER_MAX_ANGLE_DEG);

    /*
     * Once the lookahead reaches the recorded start point, stop using that
     * single XY point for steering.  Keep the last valid path steering angle
     * and release it smoothly to zero using only the remaining encoder
     * mileage.  This prevents the endpoint distance in Pure Pursuit from
     * collapsing and amplifying a small XY/yaw error into full steering lock.
     */
    if (end_target_reached && !g_back_end_steer_release_active)
    {
        g_back_end_steer_release_active = 1U;
        g_back_end_steer_release_start_m =
            back_maxf(g_back_mileage_remaining,
                      BACK_INS_END_STOP_DISTANCE_M);
        g_back_end_steer_release_start_deg = back_clampf(
            ins_lateral_error,
            -BACK_INS_STEER_MAX_ANGLE_DEG,
            BACK_INS_STEER_MAX_ANGLE_DEG);
    }

    if (g_back_end_steer_release_active)
    {
        end_steer_release_span =
            g_back_end_steer_release_start_m
            - BACK_INS_END_STOP_DISTANCE_M;

        if (end_steer_release_span > 0.001f)
        {
            end_steer_release_ratio = back_clampf(
                (g_back_mileage_remaining
                 - BACK_INS_END_STOP_DISTANCE_M)
                / end_steer_release_span,
                0.0f,
                1.0f);
        }
        else
        {
            end_steer_release_ratio = 0.0f;
        }

        g_back_end_steer_correction =
            g_back_end_steer_release_start_deg
            * end_steer_release_ratio
            - pp_angle_deg;
        pp_angle_deg =
            g_back_end_steer_release_start_deg
            * end_steer_release_ratio;
    }

    /*
     * Past the recorded endpoint there is no valid XY path to pursue.
     * Center the front wheels and finish only by the added encoder mileage.
     */
    if (g_back_end_extension_active)
    {
        pp_angle_deg = 0.0f;
        g_back_current_idx = last_index;
        g_back_target_idx = last_index;
        g_back_path_progress = g_back_total_dist_m;
        g_back_target_x = g_back_path[last_index].acc_x_g;
        g_back_target_y = g_back_path[last_index].acc_y_g;
        g_back_target_yaw = g_back_path[last_index].yaw_angle;
        ins_target_yaw = g_back_target_yaw;
        g_back_yaw_error = back_angle_diff(
            g_back_target_yaw,
            current_yaw);
        g_back_alpha_error = 0.0f;
        g_back_cross_track_error = 0.0f;
    }

    ins_lateral_error = pp_angle_deg;

    /* 妯悜璇樊锛氭鍊艰〃绀鸿溅杈嗕綅浜庤繑鑸矾寰勮繍鍔ㄦ柟鍚戠殑宸︿晶銆?*/
    nearest_motion_yaw_rad = BACK_INS_DEG_TO_RAD(
        back_normalize_angle(
            projected_path_yaw + 180.0f));

    vehicle_from_path_x =
        current_x - projected_path_x;
    vehicle_from_path_y =
        current_y - projected_path_y;

    g_back_cross_track_error =
        -vehicle_from_path_x * sinf(nearest_motion_yaw_rad)
        + vehicle_from_path_y * cosf(nearest_motion_yaw_rad);

    if (g_back_end_extension_active)
    {
        g_back_cross_track_error = 0.0f;
    }

    /* Interpolate speed, acceleration and curvature by continuous progress. */
    back_interpolate_profile(
        g_back_path_progress,
        g_back_current_idx,
        &path_speed_limit,
        &local_accel_limit,
        &g_back_current_curvature);

    local_accel_limit = back_maxf(
        local_accel_limit,
        0.10f);
    g_back_current_accel_limit = local_accel_limit;

    if (g_back_soft_start
        && (current_speed < BACK_INS_REVERSE_SOFT_RELEASE_SPEED))
    {
        g_speed_mgr.accel_step =
            BACK_INS_REVERSE_ACCEL_LIMIT / CONTROL_FREQ;
    }
    else
    {
        g_back_soft_start = 0U;
        g_speed_mgr.accel_step =
            local_accel_limit / CONTROL_FREQ;
    }

    g_speed_mgr.decel_step = target_decel / CONTROL_FREQ;

    /* 鏈熬鍑忛€熷拰鍋滆溅鍙娇鐢ㄥ綍鍒?杩旇埅缂栫爜鍣ㄩ噷绋嬨€?*/
    control_remaining = g_back_mileage_remaining;
    g_back_remaining_dist = control_remaining;

    if (control_remaining <= BACK_INS_END_STOP_DISTANCE_M)
    {
        if (g_back_result == BACK_INS_RESULT_RUNNING)
        {
            g_back_result = BACK_INS_RESULT_SUCCESS;
        }
        back_ins_stop();
        return;
    }

    /* 5. 鏇茬巼閫熷害涓婇檺涓庨噷绋嬬粓鐐瑰埗鍔ㄤ笂闄愬彇杈冨皬鍊笺€?*/
    effective_end_dist =
        control_remaining
        - current_speed * BACK_INS_END_CONTROL_DELAY_S
        - BACK_INS_END_BRAKE_MARGIN_M;

    if (effective_end_dist < 0.0f)
    {
        effective_end_dist = 0.0f;
    }

    g_back_stop_speed_limit = sqrtf(
        2.0f * target_decel * effective_end_dist);

    planned_speed = back_minf(
        path_speed_limit,
        g_back_stop_speed_limit);

    if (planned_speed < 0.0f)
    {
        planned_speed = 0.0f;
    }

    g_back_current_speed_limit = planned_speed;
    g_speed_mgr.target_final = -planned_speed;
    g_speed_mgr.mode = SPEED_MODE_REVERSE;
    ins_target_speed = -planned_speed;
}

void back_ins_stop(void)
{
    if (g_back_result == BACK_INS_RESULT_RUNNING)
    {
        g_back_result = BACK_INS_RESULT_ABORTED;
    }

    g_back_running = 0U;
    g_back_soft_start = 0U;
    g_back_end_extension_active = 0U;
    g_back_end_steer_release_active = 0U;
    g_back_end_steer_release_start_m = 0.0f;
    g_back_end_steer_release_start_deg = 0.0f;

    ins_lateral_error = 0.0f;
    ins_target_yaw = 0.0f;
    ins_target_speed = 0.0f;
    ins_gyro_z = 0.0f;

    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.target_ramp = 0.0f;
    g_speed_mgr.mode = SPEED_MODE_STOP;

    g_back_current_speed_limit = 0.0f;
    g_back_current_accel_limit = 0.0f;
    g_back_stop_speed_limit = 0.0f;

    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
    Motor_Control(0.0f, 0.0f);

    uart_write_string(
        UART_0,
        "\r\n[BACK_INS_XY] Reverse return stopped.\r\n");
}

uint8 back_ins_is_running(void)
{
    return g_back_running;
}

BackInsResult_e back_ins_get_result(void)
{
    return g_back_result;
}

/* ===================== 璋冭瘯鎺ュ彛 ===================== */

float back_ins_get_target_yaw(void)
{
    return g_back_target_yaw;
}

float back_ins_get_yaw_error(void)
{
    return g_back_yaw_error;
}

float back_ins_get_ref_yaw(void)
{
    return g_back_ref_yaw;
}

float back_ins_get_run_dist(void)
{
    return g_back_run_dist;
}

float back_ins_get_remaining_dist(void)
{
    return g_back_remaining_dist;
}

uint32 back_ins_get_current_idx(void)
{
    return g_back_current_idx;
}

uint32 back_ins_get_target_idx(void)
{
    return g_back_target_idx;
}

uint32 back_ins_get_total_points(void)
{
    return g_back_total_points;
}

float back_ins_get_current_curvature(void)
{
    return g_back_current_curvature;
}

float back_ins_get_current_speed_limit(void)
{
    return g_back_current_speed_limit;
}

float back_ins_get_current_accel_limit(void)
{
    return g_back_current_accel_limit;
}

float back_ins_get_stop_speed_limit(void)
{
    return g_back_stop_speed_limit;
}

float back_ins_get_path_progress(void)
{
    return g_back_path_progress;
}

float back_ins_get_dynamic_lookahead(void)
{
    return g_back_dynamic_lookahead;
}

float back_ins_get_cross_track_error(void)
{
    return g_back_cross_track_error;
}

float back_ins_get_target_bearing(void)
{
    return g_back_target_bearing;
}

float back_ins_get_alpha_error(void)
{
    return g_back_alpha_error;
}

float back_ins_get_nearest_distance(void)
{
    return g_back_nearest_distance;
}

float back_ins_get_end_xy_distance(void)
{
    return g_back_end_xy_distance;
}

float back_ins_get_end_longitudinal_error(void)
{
    return g_back_end_longitudinal;
}

float back_ins_get_end_lateral_error(void)
{
    return g_back_end_lateral;
}

float back_ins_get_end_yaw_error(void)
{
    return g_back_end_yaw_error;
}

float back_ins_get_end_steer_correction(void)
{
    return g_back_end_steer_correction;
}

float back_ins_get_mileage_remaining(void)
{
    return g_back_mileage_remaining;
}

float back_ins_get_target_x(void)
{
    return g_back_target_x;
}

float back_ins_get_target_y(void)
{
    return g_back_target_y;
}
/* ===================== Software blind-box task ===================== */

static void back_blind_zero_commands(void)
{
    ins_lateral_error = 0.0f;
    ins_target_speed = 0.0f;
    ins_target_yaw = 0.0f;

    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.target_ramp = 0.0f;
    g_speed_mgr.mode = SPEED_MODE_STOP;

    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
    Motor_Control(0.0f, 0.0f);
}

static void back_blind_finish(BackInsBlindResult_e result)
{
    g_back_blind_running = 0U;
    g_back_blind_result = result;
    g_back_blind_stage =
        (result == BACK_INS_BLIND_RESULT_SUCCESS)
        ? BACK_INS_BLIND_STAGE_COMPLETE
        : BACK_INS_BLIND_STAGE_FAILED;
    back_blind_zero_commands();
}

static uint8 back_blind_append_point(float x,
                                     float y,
                                     float yaw,
                                     float progress)
{
    if (g_back_blind_path_count >= BACK_INS_BLIND_PATH_MAX_POINTS)
    {
        return 0U;
    }

    if (g_back_blind_path_count > 0U)
    {
        float previous_progress =
            g_back_blind_path[g_back_blind_path_count - 1U].progress;

        if (progress <= (previous_progress + 0.001f))
        {
            return 1U;
        }
    }

    g_back_blind_path[g_back_blind_path_count].x = x;
    g_back_blind_path[g_back_blind_path_count].y = y;
    g_back_blind_path[g_back_blind_path_count].yaw = yaw;
    g_back_blind_path[g_back_blind_path_count].progress = progress;
    g_back_blind_path_count++;
    return 1U;
}

static uint8 back_blind_get_aligned_home(float *home_x,
                                         float *home_y,
                                         float *home_yaw)
{
    float align_rad;
    float align_cos;
    float align_sin;
    float relative_x;
    float relative_y;

    if ((!g_back_blind_home_valid)
        || (home_x == NULL)
        || (home_y == NULL)
        || (home_yaw == NULL))
    {
        return 0U;
    }

    align_rad = BACK_INS_DEG_TO_RAD(g_back_alignment_yaw);
    align_cos = cosf(align_rad);
    align_sin = sinf(align_rad);
    relative_x = g_back_blind_home_record_x - g_back_ref_x;
    relative_y = g_back_blind_home_record_y - g_back_ref_y;

    *home_x = g_back_start_x
            + align_cos * relative_x
            - align_sin * relative_y;
    *home_y = g_back_start_y
            + align_sin * relative_x
            + align_cos * relative_y;
    *home_yaw = back_normalize_angle(
        g_back_start_yaw
        + back_angle_diff(g_back_blind_home_record_yaw,
                          g_back_ref_yaw));

    return (uint8)(back_coordinate_is_valid(*home_x)
        && back_coordinate_is_valid(*home_y)
        && back_coordinate_is_valid(*home_yaw));
}

static uint8 back_blind_prepare_path(void)
{
    uint32 index;
    uint8 endpoint_added = 0U;
    float requested_distance;
    float progress = 0.0f;
    float previous_x;
    float previous_y;
    float previous_yaw;

    g_back_blind_path_count = 0U;
    g_back_blind_path_length = 0.0f;

    requested_distance = back_clampf(
        g_back_blind_path_distance_m,
        0.50f,
        5.50f);

    if ((g_back_total_points < 2U)
        || (!back_blind_get_aligned_home(
                &previous_x,
                &previous_y,
                &previous_yaw)))
    {
        return 0U;
    }

    if (!back_blind_append_point(
            previous_x,
            previous_y,
            previous_yaw,
            0.0f))
    {
        return 0U;
    }

    /* g_back_path is stored in return order.  Walk it backwards so the blind
     * path is garage -> track.  Geometric progress includes the independent
     * garage anchor and therefore does not need a forced recorder point. */
    index = g_back_total_points;
    while (index > 0U)
    {
        float point_x;
        float point_y;
        float point_yaw;
        float dx;
        float dy;
        float segment_length;

        index--;
        point_x = g_back_path[index].acc_x_g;
        point_y = g_back_path[index].acc_y_g;
        point_yaw = g_back_path[index].yaw_angle;
        dx = point_x - previous_x;
        dy = point_y - previous_y;
        segment_length = sqrtf(dx * dx + dy * dy);

        if (segment_length <= 0.001f)
        {
            continue;
        }

        if ((progress + segment_length) >= requested_distance)
        {
            float ratio = (requested_distance - progress) / segment_length;
            float endpoint_x = previous_x + ratio * dx;
            float endpoint_y = previous_y + ratio * dy;
            float endpoint_yaw = back_normalize_angle(
                previous_yaw
                + ratio * back_angle_diff(point_yaw, previous_yaw));

            if (!back_blind_append_point(
                    endpoint_x,
                    endpoint_y,
                    endpoint_yaw,
                    requested_distance))
            {
                return 0U;
            }
            endpoint_added = 1U;
            break;
        }

        progress += segment_length;
        if (!back_blind_append_point(
                point_x,
                point_y,
                point_yaw,
                progress))
        {
            return 0U;
        }

        previous_x = point_x;
        previous_y = point_y;
        previous_yaw = point_yaw;
    }

    if ((!endpoint_added) || (g_back_blind_path_count < 3U))
    {
        return 0U;
    }

    g_back_blind_path_length = requested_distance;
    return 1U;
}

static void back_blind_reanchor_to_live_pose(float current_x,
                                             float current_y,
                                             float current_yaw)
{
    uint32 i;
    float reference_x = g_back_blind_path[0U].x;
    float reference_y = g_back_blind_path[0U].y;
    float reference_yaw = g_back_blind_path[0U].yaw;
    float yaw_shift = back_clampf(
        back_angle_diff(current_yaw, reference_yaw),
        -BACK_INS_BLIND_MAX_YAW_REANCHOR_DEG,
        BACK_INS_BLIND_MAX_YAW_REANCHOR_DEG);
    float align_rad = BACK_INS_DEG_TO_RAD(yaw_shift);
    float align_cos = cosf(align_rad);
    float align_sin = sinf(align_rad);

    /* Translation is fully re-anchored so the blind task returns to its real
     * start.  Yaw re-anchoring is deliberately limited: a skewed return pose
     * must not rotate the whole 3.4 m route towards a cone. */
    for (i = 0U; i < g_back_blind_path_count; i++)
    {
        float relative_x = g_back_blind_path[i].x - reference_x;
        float relative_y = g_back_blind_path[i].y - reference_y;

        g_back_blind_path[i].x = current_x
            + align_cos * relative_x
            - align_sin * relative_y;
        g_back_blind_path[i].y = current_y
            + align_sin * relative_x
            + align_cos * relative_y;
        g_back_blind_path[i].yaw = back_normalize_angle(
            g_back_blind_path[i].yaw + yaw_shift);
    }
}

static void back_blind_interpolate(float progress,
                                   float *path_x,
                                   float *path_y,
                                   float *path_yaw)
{
    uint32 i;
    uint32 upper_index;
    uint32 lower_index;
    float lower_progress;
    float upper_progress;
    float ratio;

    progress = back_clampf(progress, 0.0f, g_back_blind_path_length);
    upper_index = g_back_blind_path_count - 1U;

    for (i = 1U; i < g_back_blind_path_count; i++)
    {
        if (g_back_blind_path[i].progress >= progress)
        {
            upper_index = i;
            break;
        }
    }

    lower_index = (upper_index > 0U) ? (upper_index - 1U) : upper_index;
    lower_progress = g_back_blind_path[lower_index].progress;
    upper_progress = g_back_blind_path[upper_index].progress;

    if ((upper_progress - lower_progress) > 0.0001f)
    {
        ratio = (progress - lower_progress)
              / (upper_progress - lower_progress);
        ratio = back_clampf(ratio, 0.0f, 1.0f);
    }
    else
    {
        ratio = 0.0f;
    }

    if (path_x != NULL)
    {
        *path_x = g_back_blind_path[lower_index].x
                + ratio * (g_back_blind_path[upper_index].x
                         - g_back_blind_path[lower_index].x);
    }
    if (path_y != NULL)
    {
        *path_y = g_back_blind_path[lower_index].y
                + ratio * (g_back_blind_path[upper_index].y
                         - g_back_blind_path[lower_index].y);
    }
    if (path_yaw != NULL)
    {
        *path_yaw = back_normalize_angle(
            g_back_blind_path[lower_index].yaw
            + ratio * back_angle_diff(
                g_back_blind_path[upper_index].yaw,
                g_back_blind_path[lower_index].yaw));
    }
}

static void back_blind_project(float current_x,
                               float current_y,
                               uint8 forward,
                               float previous_progress,
                               float *projected_progress,
                               float *projected_x,
                               float *projected_y,
                               float *projected_yaw,
                               float *nearest_distance)
{
    uint32 i;
    float best_distance_sq = 1.0e30f;
    float best_progress = previous_progress;
    float path_x;
    float path_y;

    for (i = 0U; i + 1U < g_back_blind_path_count; i++)
    {
        float segment_dx =
            g_back_blind_path[i + 1U].x - g_back_blind_path[i].x;
        float segment_dy =
            g_back_blind_path[i + 1U].y - g_back_blind_path[i].y;
        float segment_length_sq =
            segment_dx * segment_dx + segment_dy * segment_dy;
        float ratio = 0.0f;
        float candidate_x;
        float candidate_y;
        float dx;
        float dy;
        float distance_sq;

        if (segment_length_sq > 0.000001f)
        {
            ratio = ((current_x - g_back_blind_path[i].x) * segment_dx
                   + (current_y - g_back_blind_path[i].y) * segment_dy)
                  / segment_length_sq;
            ratio = back_clampf(ratio, 0.0f, 1.0f);
        }

        candidate_x = g_back_blind_path[i].x + ratio * segment_dx;
        candidate_y = g_back_blind_path[i].y + ratio * segment_dy;
        dx = candidate_x - current_x;
        dy = candidate_y - current_y;
        distance_sq = dx * dx + dy * dy;

        if (distance_sq < best_distance_sq)
        {
            best_distance_sq = distance_sq;
            best_progress = g_back_blind_path[i].progress
                          + ratio * (g_back_blind_path[i + 1U].progress
                                   - g_back_blind_path[i].progress);
        }
    }

    /* Progress is one-way and cannot jump across a nearby crossing. */
    if (forward)
    {
        best_progress = back_clampf(
            best_progress,
            previous_progress,
            previous_progress + BACK_INS_BLIND_MAX_PROGRESS_STEP_M);
    }
    else
    {
        best_progress = back_clampf(
            best_progress,
            previous_progress - BACK_INS_BLIND_MAX_PROGRESS_STEP_M,
            previous_progress);
    }
    best_progress = back_clampf(
        best_progress,
        0.0f,
        g_back_blind_path_length);

    back_blind_interpolate(
        best_progress,
        &path_x,
        &path_y,
        projected_yaw);
    best_distance_sq =
        (path_x - current_x) * (path_x - current_x)
      + (path_y - current_y) * (path_y - current_y);

    if (projected_progress != NULL) *projected_progress = best_progress;
    if (projected_x != NULL) *projected_x = path_x;
    if (projected_y != NULL) *projected_y = path_y;
    if (nearest_distance != NULL) *nearest_distance = sqrtf(best_distance_sq);
}

static uint8 back_blind_vehicle_stopped(void)
{
    return (uint8)(
        (back_absf(encoder_speed_left) <= BACK_INS_BLIND_STOP_SPEED_MPS)
        && (back_absf(encoder_speed_right) <= BACK_INS_BLIND_STOP_SPEED_MPS)
        && (back_absf(get_current_real_wheel_angle())
            <= BACK_INS_BLIND_STEER_READY_DEG));
}

static void back_blind_begin_motion(BackInsBlindStage_e stage)
{
    g_back_blind_stage = stage;
    g_back_blind_stage_start_ms = g_system_time_ms;
    g_back_blind_phase_start_mileage = encoder_get_total_mileage();
    g_back_blind_progress =
        (stage == BACK_INS_BLIND_STAGE_FORWARD_EXIT)
        ? 0.0f
        : g_back_blind_path_length;
    g_back_blind_remaining = g_back_blind_path_length;
    g_back_blind_stable_ticks = 0U;
    g_back_blind_lost_path_ticks = 0U;
    g_back_blind_arrival_candidate = 0U;
    g_back_blind_endpoint_crossed_latched = 0U;
    g_back_blind_last_long_valid = 0U;
    g_back_blind_end_steer_release_active = 0U;
    g_back_blind_end_steer_release_start_progress_m = 0.0f;
    g_back_blind_end_steer_release_start_deg = 0.0f;

    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.target_ramp = 0.0f;
    g_speed_mgr.accel_step = BACK_INS_BLIND_ACCEL_MPS2 / CONTROL_FREQ;
    g_speed_mgr.decel_step = BACK_INS_BLIND_DECEL_MPS2 / CONTROL_FREQ;
    g_speed_mgr.mode =
        (stage == BACK_INS_BLIND_STAGE_FORWARD_EXIT)
        ? SPEED_MODE_STRAIGHT
        : SPEED_MODE_REVERSE;
    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
}

static void back_blind_request_phase_stop(uint8 forward)
{
    ins_lateral_error = 0.0f;
    ins_target_speed = 0.0f;
    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.mode = SPEED_MODE_STOP;
    g_back_blind_stage = forward
                       ? BACK_INS_BLIND_STAGE_WAIT_REVERSE
                       : BACK_INS_BLIND_STAGE_FINAL_STOP;
    g_back_blind_stage_start_ms = g_system_time_ms;
    g_back_blind_stable_ticks = 0U;
}

static void back_blind_update_wait_state(void)
{
    ins_lateral_error = 0.0f;
    ins_target_speed = 0.0f;
    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.mode = SPEED_MODE_STOP;

    if ((g_system_time_ms - g_back_blind_stage_start_ms)
        > BACK_INS_BLIND_WAIT_TIMEOUT_MS)
    {
        back_blind_finish(BACK_INS_BLIND_RESULT_TIMEOUT);
        return;
    }

    if (back_blind_vehicle_stopped())
    {
        g_back_blind_stable_ticks++;
    }
    else
    {
        g_back_blind_stable_ticks = 0U;
    }

    if (g_back_blind_stable_ticks >= BACK_INS_BLIND_STOP_STABLE_TICKS)
    {
        if (g_back_blind_stage == BACK_INS_BLIND_STAGE_WAIT_FORWARD)
        {
            back_blind_begin_motion(BACK_INS_BLIND_STAGE_FORWARD_EXIT);
        }
        else if (g_back_blind_stage == BACK_INS_BLIND_STAGE_WAIT_REVERSE)
        {
            back_blind_begin_motion(BACK_INS_BLIND_STAGE_REVERSE_ENTER);
        }
        else
        {
            back_blind_finish(BACK_INS_BLIND_RESULT_SUCCESS);
        }
    }
}

static void back_blind_update_motion(uint8 forward)
{
    float current_x = imu_ahrs_get_pos_x();
    float current_y = imu_ahrs_get_pos_y();
    float current_yaw = imu_ahrs_get_yaw();
    float current_speed =
        (back_absf(encoder_speed_left)
       + back_absf(encoder_speed_right)) * 0.5f;
    float phase_distance =
        encoder_get_total_mileage() - g_back_blind_phase_start_mileage;
    float lookahead;
    float target_progress;
    float target_x;
    float target_y;
    float target_yaw;
    float target_dx;
    float target_dy;
    float target_distance;
    float target_bearing;
    float motion_yaw;
    float alpha;
    float wheel_angle_deg;
    float endpoint_x;
    float endpoint_y;
    float endpoint_yaw;
    float endpoint_yaw_rad;
    float endpoint_dx;
    float endpoint_dy;
    float effective_stop_distance;
    float stop_speed_limit;
    float planned_speed;
    float release_ratio;
    uint8 crossed_now = 0U;
    uint8 progress_near;
    uint8 inside_enter;
    uint8 outside_exit;

    if ((!back_coordinate_is_valid(current_x))
        || (!back_coordinate_is_valid(current_y))
        || (!back_coordinate_is_valid(current_yaw)))
    {
        back_blind_finish(BACK_INS_BLIND_RESULT_LOST_PATH);
        return;
    }

    if ((g_system_time_ms - g_back_blind_stage_start_ms)
        > BACK_INS_BLIND_MOTION_TIMEOUT_MS)
    {
        back_blind_finish(BACK_INS_BLIND_RESULT_TIMEOUT);
        return;
    }

    if (phase_distance < 0.0f)
    {
        phase_distance = -phase_distance;
    }
    if (phase_distance
        > (g_back_blind_path_length + BACK_INS_BLIND_MILEAGE_OVERRUN_M))
    {
        back_blind_finish(BACK_INS_BLIND_RESULT_OVERRUN);
        return;
    }

    back_blind_project(
        current_x,
        current_y,
        forward,
        g_back_blind_progress,
        &g_back_blind_progress,
        NULL,
        NULL,
        NULL,
        &g_back_blind_path_error);

    if (g_back_blind_path_error > BACK_INS_BLIND_LOST_PATH_M)
    {
        g_back_blind_lost_path_ticks++;
        if (g_back_blind_lost_path_ticks >= BACK_INS_BLIND_LOST_PATH_TICKS)
        {
            back_blind_finish(BACK_INS_BLIND_RESULT_LOST_PATH);
            return;
        }
    }
    else
    {
        g_back_blind_lost_path_ticks = 0U;
    }

    g_back_blind_remaining = forward
        ? (g_back_blind_path_length - g_back_blind_progress)
        : g_back_blind_progress;
    g_back_blind_remaining = back_clampf(
        g_back_blind_remaining,
        0.0f,
        g_back_blind_path_length);

    lookahead = BACK_INS_BLIND_LOOKAHEAD_BASE_M
              + BACK_INS_BLIND_LOOKAHEAD_SPEED_GAIN * current_speed;
    lookahead = back_clampf(
        lookahead,
        BACK_INS_BLIND_LOOKAHEAD_MIN_M,
        BACK_INS_BLIND_LOOKAHEAD_MAX_M);
    target_progress = forward
        ? (g_back_blind_progress + lookahead)
        : (g_back_blind_progress - lookahead);
    target_progress = back_clampf(
        target_progress,
        0.0f,
        g_back_blind_path_length);

    back_blind_interpolate(
        target_progress,
        &target_x,
        &target_y,
        &target_yaw);
    target_dx = target_x - current_x;
    target_dy = target_y - current_y;
    target_distance = sqrtf(
        target_dx * target_dx + target_dy * target_dy);
    target_bearing = BACK_INS_RAD_TO_DEG(atan2f(target_dy, target_dx));
    motion_yaw = forward
               ? current_yaw
               : back_normalize_angle(current_yaw + 180.0f);
    alpha = back_angle_diff(target_bearing, motion_yaw);

    if (target_distance <= BACK_INS_BLIND_TARGET_DISTANCE_MIN_M)
    {
        wheel_angle_deg = 0.0f;
    }
    else
    {
        float wheel_angle_rad = atan2f(
            2.0f * WHEEL_BASE * sinf(BACK_INS_DEG_TO_RAD(alpha)),
            target_distance);
        if (!forward)
        {
            wheel_angle_rad = -wheel_angle_rad;
        }
        wheel_angle_deg = BACK_INS_RAD_TO_DEG(wheel_angle_rad);
    }
    wheel_angle_deg = back_clampf(
        wheel_angle_deg,
        -BACK_INS_STEER_MAX_ANGLE_DEG,
        BACK_INS_STEER_MAX_ANGLE_DEG);

    /* For both directions, stop chasing the single endpoint once lookahead
     * reaches it.  Release the last valid path angle using coordinate progress. */
    if ((!g_back_blind_end_steer_release_active)
        && ((forward
             && (target_progress >= (g_back_blind_path_length - 0.0001f)))
            || ((!forward) && (target_progress <= 0.0001f))))
    {
        g_back_blind_end_steer_release_active = 1U;
        g_back_blind_end_steer_release_start_progress_m = back_maxf(
            g_back_blind_remaining,
            0.01f);
        g_back_blind_end_steer_release_start_deg = back_clampf(
            ins_lateral_error,
            -BACK_INS_STEER_MAX_ANGLE_DEG,
            BACK_INS_STEER_MAX_ANGLE_DEG);
    }

    if (g_back_blind_end_steer_release_active)
    {
        release_ratio = back_clampf(
            g_back_blind_remaining
            / g_back_blind_end_steer_release_start_progress_m,
            0.0f,
            1.0f);
        wheel_angle_deg =
            g_back_blind_end_steer_release_start_deg * release_ratio;
    }

    if (forward)
    {
        endpoint_x = g_back_blind_path[g_back_blind_path_count - 1U].x;
        endpoint_y = g_back_blind_path[g_back_blind_path_count - 1U].y;
        endpoint_yaw =
            g_back_blind_path[g_back_blind_path_count - 1U].yaw;
    }
    else
    {
        endpoint_x = g_back_blind_path[0U].x;
        endpoint_y = g_back_blind_path[0U].y;
        endpoint_yaw = g_back_blind_path[0U].yaw;
    }
    endpoint_dx = endpoint_x - current_x;
    endpoint_dy = endpoint_y - current_y;
    endpoint_yaw_rad = BACK_INS_DEG_TO_RAD(endpoint_yaw);
    g_back_blind_longitudinal_error =
        cosf(endpoint_yaw_rad) * endpoint_dx
      + sinf(endpoint_yaw_rad) * endpoint_dy;
    g_back_blind_lateral_error =
       -sinf(endpoint_yaw_rad) * endpoint_dx
      + cosf(endpoint_yaw_rad) * endpoint_dy;
    g_back_blind_yaw_error =
        back_angle_diff(endpoint_yaw, current_yaw);

    if (g_back_blind_last_long_valid)
    {
        if (forward)
        {
            crossed_now = (uint8)(
                (g_back_blind_last_longitudinal > 0.0f)
                && (g_back_blind_longitudinal_error <= 0.0f));
        }
        else
        {
            crossed_now = (uint8)(
                (g_back_blind_last_longitudinal < 0.0f)
                && (g_back_blind_longitudinal_error >= 0.0f));
        }
    }
    g_back_blind_last_longitudinal = g_back_blind_longitudinal_error;
    g_back_blind_last_long_valid = 1U;

    progress_near = (uint8)(
        g_back_blind_remaining <= BACK_INS_BLIND_PROGRESS_CAPTURE_M);
    inside_enter = (uint8)(
        (back_absf(g_back_blind_longitudinal_error)
            <= BACK_INS_BLIND_ENTER_LONG_M)
        && (back_absf(g_back_blind_lateral_error)
            <= BACK_INS_BLIND_ENTER_LAT_M)
        && (back_absf(g_back_blind_yaw_error)
            <= BACK_INS_BLIND_ENTER_YAW_DEG));
    outside_exit = (uint8)(
        (back_absf(g_back_blind_longitudinal_error)
            > BACK_INS_BLIND_EXIT_LONG_M)
        || (back_absf(g_back_blind_lateral_error)
            > BACK_INS_BLIND_EXIT_LAT_M)
        || (back_absf(g_back_blind_yaw_error)
            > BACK_INS_BLIND_EXIT_YAW_DEG));

    if (progress_near
        && crossed_now
        && (back_absf(g_back_blind_lateral_error)
            <= BACK_INS_BLIND_CROSS_LAT_M))
    {
        g_back_blind_endpoint_crossed_latched = 1U;
        g_back_blind_arrival_candidate = 1U;
    }
    else if (progress_near && inside_enter)
    {
        g_back_blind_arrival_candidate = 1U;
    }
    else if (g_back_blind_arrival_candidate
             && outside_exit
             && (!g_back_blind_endpoint_crossed_latched))
    {
        g_back_blind_arrival_candidate = 0U;
        g_back_blind_stable_ticks = 0U;
    }

    g_back_target_x = target_x;
    g_back_target_y = target_y;
    g_back_target_yaw = target_yaw;
    g_back_target_bearing = target_bearing;
    g_back_alpha_error = alpha;
    g_back_cross_track_error = g_back_blind_path_error;
    ins_target_yaw = target_yaw;
    ins_lateral_error = wheel_angle_deg;

    /* Once inside the capture box, command a stop and never chase the point
     * behind the vehicle.  Enter/exit hysteresis filters centimetre-level pose
     * noise; crossing the endpoint latches arrival unconditionally. */
    if (g_back_blind_arrival_candidate)
    {
        ins_lateral_error = 0.0f;
        ins_target_speed = 0.0f;
        g_speed_mgr.target_final = 0.0f;
        g_speed_mgr.mode = SPEED_MODE_STOP;

        if (current_speed <= BACK_INS_BLIND_STOP_SPEED_MPS)
        {
            g_back_blind_stable_ticks++;
        }
        else
        {
            g_back_blind_stable_ticks = 0U;
        }

        if (g_back_blind_stable_ticks
            >= BACK_INS_BLIND_STOP_STABLE_TICKS)
        {
            back_blind_request_phase_stop(forward);
        }
        return;
    }

    effective_stop_distance =
        g_back_blind_remaining
        - current_speed * BACK_INS_BLIND_CONTROL_DELAY_S
        - BACK_INS_BLIND_BRAKE_MARGIN_M;
    if (effective_stop_distance < 0.0f)
    {
        effective_stop_distance = 0.0f;
    }
    stop_speed_limit = sqrtf(
        2.0f * BACK_INS_BLIND_DECEL_MPS2 * effective_stop_distance);
    planned_speed = back_minf(
        back_clampf(g_back_blind_speed_mps, 0.10f, 1.50f),
        stop_speed_limit);

    if (g_back_blind_remaining <= BACK_INS_BLIND_APPROACH_DISTANCE_M)
    {
        planned_speed = back_minf(
            planned_speed,
            BACK_INS_BLIND_APPROACH_SPEED_MPS);
    }
    if (back_absf(wheel_angle_deg) > 12.0f)
    {
        planned_speed = back_minf(
            planned_speed,
            BACK_INS_BLIND_TURN_SPEED_MPS);
    }

    g_speed_mgr.accel_step = BACK_INS_BLIND_ACCEL_MPS2 / CONTROL_FREQ;
    g_speed_mgr.decel_step = BACK_INS_BLIND_DECEL_MPS2 / CONTROL_FREQ;
    g_speed_mgr.target_final = forward ? planned_speed : -planned_speed;
    g_speed_mgr.mode = forward ? SPEED_MODE_STRAIGHT : SPEED_MODE_REVERSE;
    ins_target_speed = g_speed_mgr.target_final;
}

void back_ins_blind_capture_home_pose(void)
{
    g_back_blind_home_record_x = imu_ahrs_get_pos_x();
    g_back_blind_home_record_y = imu_ahrs_get_pos_y();
    g_back_blind_home_record_yaw = imu_ahrs_get_yaw();
    g_back_blind_home_valid = (uint8)(
        back_coordinate_is_valid(g_back_blind_home_record_x)
        && back_coordinate_is_valid(g_back_blind_home_record_y)
        && back_coordinate_is_valid(g_back_blind_home_record_yaw));
}

uint8 back_ins_blind_start(void)
{
    float current_x;
    float current_y;
    float current_yaw;

    if (g_back_running || g_back_blind_running)
    {
        return 0U;
    }

    g_back_blind_result = BACK_INS_BLIND_RESULT_IDLE;
    g_back_blind_stage = BACK_INS_BLIND_STAGE_IDLE;
    if (!g_back_blind_home_valid)
    {
        g_back_blind_result = BACK_INS_BLIND_RESULT_HOME_INVALID;
        g_back_blind_stage = BACK_INS_BLIND_STAGE_FAILED;
        return 0U;
    }
    if (!back_blind_prepare_path())
    {
        g_back_blind_result = BACK_INS_BLIND_RESULT_PATH_TOO_SHORT;
        g_back_blind_stage = BACK_INS_BLIND_STAGE_FAILED;
        return 0U;
    }

    current_x = imu_ahrs_get_pos_x();
    current_y = imu_ahrs_get_pos_y();
    current_yaw = imu_ahrs_get_yaw();
    if ((!back_coordinate_is_valid(current_x))
        || (!back_coordinate_is_valid(current_y))
        || (!back_coordinate_is_valid(current_yaw)))
    {
        g_back_blind_result = BACK_INS_BLIND_RESULT_HOME_INVALID;
        g_back_blind_stage = BACK_INS_BLIND_STAGE_FAILED;
        return 0U;
    }

    back_blind_reanchor_to_live_pose(current_x, current_y, current_yaw);
    g_back_blind_progress = 0.0f;
    g_back_blind_remaining = g_back_blind_path_length;
    g_back_blind_path_error = 0.0f;
    g_back_blind_longitudinal_error = 0.0f;
    g_back_blind_lateral_error = 0.0f;
    g_back_blind_yaw_error = 0.0f;
    g_back_blind_stable_ticks = 0U;
    g_back_blind_lost_path_ticks = 0U;
    g_back_blind_arrival_candidate = 0U;
    g_back_blind_endpoint_crossed_latched = 0U;
    g_back_blind_last_long_valid = 0U;
    g_back_blind_result = BACK_INS_BLIND_RESULT_RUNNING;
    g_back_blind_stage = BACK_INS_BLIND_STAGE_WAIT_FORWARD;
    g_back_blind_stage_start_ms = g_system_time_ms;
    g_back_blind_running = 1U;
    back_blind_zero_commands();
    return 1U;
}

void back_ins_blind_update(void)
{
    if (!g_back_blind_running)
    {
        return;
    }

    switch (g_back_blind_stage)
    {
        case BACK_INS_BLIND_STAGE_WAIT_FORWARD:
        case BACK_INS_BLIND_STAGE_WAIT_REVERSE:
        case BACK_INS_BLIND_STAGE_FINAL_STOP:
            back_blind_update_wait_state();
            break;

        case BACK_INS_BLIND_STAGE_FORWARD_EXIT:
            back_blind_update_motion(1U);
            break;

        case BACK_INS_BLIND_STAGE_REVERSE_ENTER:
            back_blind_update_motion(0U);
            break;

        default:
            back_blind_finish(BACK_INS_BLIND_RESULT_ABORTED);
            break;
    }
}

void back_ins_blind_stop(void)
{
    if (g_back_blind_result == BACK_INS_BLIND_RESULT_RUNNING)
    {
        g_back_blind_result = BACK_INS_BLIND_RESULT_ABORTED;
    }
    g_back_blind_running = 0U;
    g_back_blind_stage = BACK_INS_BLIND_STAGE_FAILED;
    back_blind_zero_commands();
}

uint8 back_ins_blind_is_running(void)
{
    return g_back_blind_running;
}

BackInsBlindResult_e back_ins_blind_get_result(void)
{
    return g_back_blind_result;
}

BackInsBlindStage_e back_ins_blind_get_stage(void)
{
    return g_back_blind_stage;
}

float back_ins_blind_get_remaining_dist(void)
{
    return g_back_blind_remaining;
}

float back_ins_blind_get_path_error(void)
{
    return g_back_blind_path_error;
}

float back_ins_blind_get_progress(void)
{
    return g_back_blind_progress;
}

float back_ins_blind_get_longitudinal_error(void)
{
    return g_back_blind_longitudinal_error;
}

float back_ins_blind_get_lateral_error(void)
{
    return g_back_blind_lateral_error;
}

float back_ins_blind_get_yaw_error(void)
{
    return g_back_blind_yaw_error;
}
