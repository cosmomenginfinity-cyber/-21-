#include "ins.h"
#include "sub2_navigation.h"
#include "imu_ahrs.h"
#include "encoder.h"
#include "flash.h"
#include "imu_datalogger.h"
#include "zf_driver_uart.h"
#include "PID.h" // 确保包含 PID 头文件以调用 PID_Reset()
#include <math.h>

// ins.c

#ifndef INS_PI
#define INS_PI  3.14159265358979323846f
#endif

// ==================== 【内存保护】：彻底解除编译器 E112 爆板死锁 ====================

// 1. 原始路径缓存
static imu_encoder_log_t g_path_cache[FLASH_MAX_LOG_RECORDS] __attribute__((section(".bss.lmubss")));

// 2. 预处理后的状态表
PathState_e g_path_states[FLASH_MAX_LOG_RECORDS] __attribute__((section(".bss.lmubss")));

// 3. 高速全局曲率速度分配表
static float g_path_speed_limit[FLASH_MAX_LOG_RECORDS] __attribute__((section(".bss.lmubss")));

// 4. 路径绝对曲率表（单位：1/m），用于区分直道和曲率限速弯道
static float g_path_curvature[FLASH_MAX_LOG_RECORDS] __attribute__((section(".bss.lmubss")));

// 5. 前进阶段各路径点允许的加速度（单位：m/s^2）
static float g_path_accel_limit[FLASH_MAX_LOG_RECORDS] __attribute__((section(".bss.lmubss")));

// ===================== 【速度决策物理参数】 =====================
// 路面有效附着系数：队内实测 μ = 0.57
// 横向安全系数：0.90
// 实车观察：半径约1.3m时，2.5m/s已经接近弯道极限。
// 反推横向加速度约为 2.5^2/1.3 = 4.81m/s^2，规划值取4.80m/s^2。
volatile float K_SAFE_CENTRIPETAL =
        INS_CURVE_LATERAL_ACCEL_LIMIT;

// 统一规划减速度（m/s^2）
// 直行终点、倒车终点以及前进/倒车换向点均按 2.0m/s^2 生成动态制动距离。
volatile float TARGET_DECEL = 2.5f;

// 直道最大规划加速度（m/s^2）。弯道内会根据曲率自动降低，
// 半径逐渐接近2m时再平滑恢复到该值。
volatile float START_ACCEL_LIMIT = 3.0f;

// ===================== 【本文件所需的位置参数兜底】 =====================
// 参数原本也放在新版ins.h中。这里保留兜底定义，避免工程误用旧版ins.h时
// 出现“undeclared identifier”，并确保终点容差不会退回旧版的0.12m。
#ifdef INS_STOP_DISTANCE_THRESHOLD
#undef INS_STOP_DISTANCE_THRESHOLD
#endif
#define INS_STOP_DISTANCE_THRESHOLD        0.03f

#ifndef INS_SWITCH_POSITION_COMP_M
#define INS_SWITCH_POSITION_COMP_M         0.00f
#endif

#ifndef INS_END_POSITION_COMP_M
#define INS_END_POSITION_COMP_M            0.00f
#endif

#ifndef INS_SWITCH_POSITION_TOL_M
#define INS_SWITCH_POSITION_TOL_M          0.03f
#endif

#ifndef INS_SWITCH_CREEP_ZONE_M
#define INS_SWITCH_CREEP_ZONE_M            0.12f
#endif

#ifndef INS_SWITCH_CREEP_SPEED_MPS
#define INS_SWITCH_CREEP_SPEED_MPS         0.12f
#endif

#ifndef INS_SWITCH_2D_APPROACH_MIN_M
#define INS_SWITCH_2D_APPROACH_MIN_M        3.00f
#endif

#ifndef INS_SWITCH_2D_BRAKE_MARGIN_M
#define INS_SWITCH_2D_BRAKE_MARGIN_M        0.10f
#endif

#ifndef INS_SWITCH_2D_LATERAL_KP_DEG_PER_M
#define INS_SWITCH_2D_LATERAL_KP_DEG_PER_M  10.0f
#endif

#ifndef INS_SWITCH_2D_STEER_COMP_MAX_DEG
#define INS_SWITCH_2D_STEER_COMP_MAX_DEG    6.00f
#endif

#ifndef INS_SWITCH_2D_ACTIVATE_GUARD_M
#define INS_SWITCH_2D_ACTIVATE_GUARD_M       1.00f
#endif

#ifndef INS_SWITCH_2D_NEAREST_SEARCH_POINTS
#define INS_SWITCH_2D_NEAREST_SEARCH_POINTS  12U
#endif

#ifndef INS_END_CREEP_ZONE_M
#define INS_END_CREEP_ZONE_M               0.10f
#endif

#ifndef INS_END_CREEP_SPEED_MPS
#define INS_END_CREEP_SPEED_MPS            0.10f
#endif

// ===================== 【换向/终点动态制动参数】 =====================
// 控制、滤波和执行机构的等效响应延迟（秒）。
// 延迟只用于提前制动，不再把最终停车点固定平移到换向点之前。
#define INS_SWITCH_CONTROL_DELAY_S        0.20f
#define INS_END_CONTROL_DELAY_S           0.20f

// 实车静止判定阈值
#define INS_SWITCH_STOP_SPEED_MPS         0.05f
#define INS_SWITCH_STOP_GYRO_DPS          2.00f
#define INS_SWITCH_STABLE_TICKS           30U

// 终点实际静止判定阈值
#define INS_END_STOP_SPEED_MPS            0.05f
#define INS_END_STOP_GYRO_DPS             2.00f
#define INS_END_STABLE_TICKS              30U

// 倒车起步阶段限制加速度，防止换向后车轮打滑空转
#define INS_REVERSE_ACCEL_LIMIT           1.50f
#define INS_REVERSE_SOFT_RELEASE_SPEED    0.50f

// ===================== 【换向状态控制变量】 =====================
uint32 g_reverse_switch_idx = 0;   // 换向点的绝对索引（为0代表本条赛道无倒车任务）
static uint32 g_wait_ticks = 0;    // 换向静止等待时钟计数器
static uint8  g_wait_finished = 0; // 换向静止等待结束标志位
static uint8  g_reverse_soft_start = 0; // 倒车软启动阶段标志
static uint32 g_end_stable_ticks = 0; // 终点静止确认计数器
static uint8  g_switch_xy_available = 0; // Flash路径是否包含有效二维坐标

// 换向点二维进站调试量，可直接加入逐飞助手或ADS Watch。
volatile float g_ins_switch_longitudinal_error = 999.0f;
volatile float g_ins_switch_lateral_error = 0.0f;
volatile float g_ins_switch_xy_distance = 999.0f;
volatile uint8 g_ins_switch_2d_active = 0;

// 全局空档盲滑闭锁标志位
volatile uint8 g_ins_neutral_glide = 0;

// 引用外部全局变量
extern volatile float ins_cruise_speed;   // 引用主速度变量

// ===================== 局部参数定义 =====================
static uint32 g_total_path_points = 0;
static uint8  g_is_navigating = 0;
static float  g_start_mileage = 0.0f; // 启动导航时的初始里程
static uint32 last_idx = 0;

// 全局目标速度
float g_ins_left_speed = 0.0f;
float g_ins_right_speed = 0.0f;

// PID 参数 (针对 Yaw 误差)
float INS_YAW_KP = 0.8f;
float INS_YAW_KD = 0.1f;
static float last_yaw_error = 0.0f;

// ===================== 内部辅助函数 =====================

// 计算两个角度的最小差值 (处理 -180~180 突变)
static float ins_angle_diff(float target, float current) {
    float diff = target - current;
    while (diff > 180.0f)  diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

// ===================== 对外接口实现 =====================

uint8 ins_init(void) {
    g_is_navigating = 0;
    g_ins_left_speed = 0.0f;
    g_ins_right_speed = 0.0f;
    g_switch_xy_available = 0;
    g_ins_switch_2d_active = 0;
    g_ins_switch_longitudinal_error = 999.0f;
    g_ins_switch_lateral_error = 0.0f;
    g_ins_switch_xy_distance = 999.0f;
    return 1;
}

// 换向扫描辅助函数
void ins_find_switch_point(void) {
    g_reverse_switch_idx = 0; // 扫描前清空历史缓存

    // 新路径优先使用录制时由KEY3明确写入的换向事件。
    // 事件点不受20cm普通采样间隔和轮速滤波过零时刻影响。
    for (uint32 i = 0; i < g_total_path_points; i++) {
        if (g_path_cache[i].timestamp_ms == PATH_RECORD_EVENT_SWITCH) {
            g_reverse_switch_idx = i;
            return;
        }
    }

    // 兼容没有事件标记的旧路径。
    for (uint32 i = 0; i < g_total_path_points - 1; i++) {
        // 发现速度正负号反转，说明物理车模在这里需要停下并开始倒车
        if (g_path_cache[i].main_wheel_speed >= 0.0f && g_path_cache[i+1].main_wheel_speed < 0.0f) {
            g_reverse_switch_idx = i;
            break;
        }
    }
}

// 基于微分几何的全局曲率速度剖面计算引擎
void ins_calculate_curvature_speed_limits(void)
{
    // raw_curvature：跨点计算得到的原始绝对曲率，单位 1/m
    // 对曲率滤波而不是对半径滤波，避免直道大半径把弯道半径严重拉大
    static float raw_curvature[FLASH_MAX_LOG_RECORDS]
        __attribute__((section(".bss.lmubss")));

    if (g_total_path_points < 3U)
    {
        return;
    }

    float v_max = fabsf(ins_cruise_speed);

    // 使用实车测得的最大速度作为硬上限，防止菜单参数误设过大
    if (v_max > INS_MAX_SPEED)
    {
        v_max = INS_MAX_SPEED;
    }

    if (v_max < INS_MIN_PATH_SPEED)
    {
        v_max = INS_MIN_PATH_SPEED;
    }

    // ------------------------------------------------------------------
    // 第1步：跨多个路径点计算原始曲率
    //
    // 使用 i 左右各 INS_CURVATURE_SPAN 个点构成局部窗口：
    // kappa = 累计绝对航向变化(rad) / 累计路程(m)
    //
    // 使用“累计绝对航向变化”可以避免连续八字弯左右曲率在窗口内互相抵消。
    // ------------------------------------------------------------------
    for (uint32 i = 0U; i < g_total_path_points; i++)
    {
        uint32 left = (i > INS_CURVATURE_SPAN)
                    ? (i - INS_CURVATURE_SPAN)
                    : 0U;

        uint32 right = i + INS_CURVATURE_SPAN;
        if (right >= g_total_path_points)
        {
            right = g_total_path_points - 1U;
        }

        float ds =
            (float)(g_path_cache[right].main_encoder_cnt
                  - g_path_cache[left].main_encoder_cnt)
            / 1000.0f;

        float total_abs_yaw_rad = 0.0f;

        for (uint32 k = left; k < right; k++)
        {
            float diff_deg = fabsf(
                ins_angle_diff(
                    g_path_cache[k + 1U].yaw_angle,
                    g_path_cache[k].yaw_angle));

            // 很小的航向抖动按噪声处理，防止直道被误判为小半径弯道
            if (diff_deg >= INS_CURVATURE_YAW_DEADZONE_DEG)
            {
                total_abs_yaw_rad += diff_deg * (INS_PI / 180.0f);
            }
        }

        if (ds >= INS_MIN_CURVATURE_DISTANCE_M)
        {
            raw_curvature[i] = total_abs_yaw_rad / ds;
        }
        else
        {
            raw_curvature[i] = 0.0f;
        }
    }

    // ------------------------------------------------------------------
    // 第2步：对曲率做滑动平均，并按半径生成速度和加速度上限
    //
    // R <= 2m：v_curve=sqrt(a_y*R)，并限制最低弯速；
    //            加速度从急弯的1.2m/s^2逐步增大到直道加速度。
    // R >  2m：使用菜单设置的直道巡航速度和START_ACCEL_LIMIT。
    //
    // 这里只生成各路径点的原始速度上限；后续仍由逆向减速传递
    // 和正向加速传递把直道/弯道交界处理成连续速度曲线。
    // ------------------------------------------------------------------
    for (uint32 i = 0U; i < g_total_path_points; i++)
    {
        float curvature_sum = 0.0f;
        uint32 curvature_count = 0U;

        int32 start =
            (int32)i - (int32)INS_CURVATURE_FILTER_HALF_WIDTH;
        int32 end =
            (int32)i + (int32)INS_CURVATURE_FILTER_HALF_WIDTH;

        for (int32 j = start; j <= end; j++)
        {
            if ((j >= 0) && (j < (int32)g_total_path_points))
            {
                curvature_sum += raw_curvature[(uint32)j];
                curvature_count++;
            }
        }

        float smoothed_curvature = 0.0f;
        if (curvature_count > 0U)
        {
            smoothed_curvature =
                curvature_sum / (float)curvature_count;
        }

        g_path_curvature[i] = smoothed_curvature;

        float point_speed_limit = v_max;
        float point_accel_limit = START_ACCEL_LIMIT;

        if (smoothed_curvature > INS_CURVATURE_EPSILON)
        {
            float radius = 1.0f / smoothed_curvature;

            if (radius <= INS_CURVE_RADIUS_THRESHOLD_M)
            {
                // 曲率速度：半径1.3m时约2.50m/s，弯越缓允许速度越高。
                point_speed_limit =
                    sqrtf(K_SAFE_CENTRIPETAL * radius);

                if (point_speed_limit < INS_CURVE_MIN_SPEED)
                {
                    point_speed_limit = INS_CURVE_MIN_SPEED;
                }

                // 急弯内限制纵向加速度，避免横向附着已经接近极限时继续猛加速。
                // 半径1.3m到2m之间线性开放，进入直道时恢复完整加速度。
                if (radius <= INS_TIGHT_CURVE_RADIUS_M)
                {
                    point_accel_limit =
                        INS_TIGHT_CURVE_ACCEL_MPS2;
                }
                else
                {
                    float accel_ratio =
                        (radius - INS_TIGHT_CURVE_RADIUS_M)
                        / (INS_CURVE_RADIUS_THRESHOLD_M
                         - INS_TIGHT_CURVE_RADIUS_M);

                    point_accel_limit =
                        INS_TIGHT_CURVE_ACCEL_MPS2
                        + accel_ratio
                        * (START_ACCEL_LIMIT
                         - INS_TIGHT_CURVE_ACCEL_MPS2);
                }
            }
        }

        if (point_speed_limit > v_max)
        {
            point_speed_limit = v_max;
        }

        if (point_speed_limit < INS_MIN_PATH_SPEED)
        {
            point_speed_limit = INS_MIN_PATH_SPEED;
        }

        if (point_accel_limit > START_ACCEL_LIMIT)
        {
            point_accel_limit = START_ACCEL_LIMIT;
        }

        if (point_accel_limit < INS_TIGHT_CURVE_ACCEL_MPS2)
        {
            point_accel_limit = INS_TIGHT_CURVE_ACCEL_MPS2;
        }

        g_path_speed_limit[i] = point_speed_limit;
        g_path_accel_limit[i] = point_accel_limit;
    }

    // ------------------------------------------------------------------
    // 第3步：逆向速度传递
    // 从后向前传播低速要求，保证进入弯道之前能按 TARGET_DECEL 刹下来。
    // v_i <= sqrt(v_(i+1)^2 + 2 * A_BRAKE_MAX * ds)
    // ------------------------------------------------------------------
    for (int32 i = (int32)g_total_path_points - 2; i >= 0; i--)
    {
        float ds =
            (float)(g_path_cache[(uint32)i + 1U].main_encoder_cnt
                  - g_path_cache[(uint32)i].main_encoder_cnt)
            / 1000.0f;

        if (ds < INS_MIN_SPEED_PLAN_DISTANCE_M)
        {
            ds = INS_MIN_SPEED_PLAN_DISTANCE_M;
        }

        float v_next = g_path_speed_limit[(uint32)i + 1U];

        float max_safe_v =
            sqrtf(v_next * v_next
                + 2.0f * TARGET_DECEL * ds);

        if (g_path_speed_limit[(uint32)i] > max_safe_v)
        {
            g_path_speed_limit[(uint32)i] = max_safe_v;
        }
    }

    // ------------------------------------------------------------------
    // 第4步：正向加速传递
    // 从前向后传播加速能力，保证出弯后的速度上升是车辆可实现的。
    // v_i <= sqrt(v_(i-1)^2 + 2 * A_ACC_MAX * ds)
    //
    // 注意：起步仍由 Speed_Ramp_Update() 从0平滑拉升；
    // 本次正向传递主要约束“弯道低速点之后”的出弯加速。
    // ------------------------------------------------------------------
    for (uint32 i = 1U; i < g_total_path_points; i++)
    {
        float ds =
            (float)(g_path_cache[i].main_encoder_cnt
                  - g_path_cache[i - 1U].main_encoder_cnt)
            / 1000.0f;

        if (ds < INS_MIN_SPEED_PLAN_DISTANCE_M)
        {
            ds = INS_MIN_SPEED_PLAN_DISTANCE_M;
        }

        float v_prev = g_path_speed_limit[i - 1U];

        float local_accel_limit = g_path_accel_limit[i];

        float max_reachable_v =
            sqrtf(v_prev * v_prev
                + 2.0f * local_accel_limit * ds);

        if (g_path_speed_limit[i] > max_reachable_v)
        {
            g_path_speed_limit[i] = max_reachable_v;
        }
    }

#if INS_SPEED_TABLE_DEBUG
    printf("IDX,DIST_M,KAPPA,RADIUS_M,SPEED_MPS,ACCEL_MPS2\r\n");

    for (uint32 i = 0U;
         i < g_total_path_points;
         i += INS_SPEED_TABLE_PRINT_STEP)
    {
        float distance_m =
            (float)g_path_cache[i].main_encoder_cnt / 1000.0f;

        float radius_m = 9999.0f;
        if (g_path_curvature[i] > INS_CURVATURE_EPSILON)
        {
            radius_m = 1.0f / g_path_curvature[i];
        }

        printf("%lu,%.3f,%.5f,%.3f,%.3f,%.3f\r\n",
               (unsigned long)i,
               distance_m,
               g_path_curvature[i],
               radius_m,
               g_path_speed_limit[i],
               g_path_accel_limit[i]);
    }
#endif

    printf("[速度决策] Vmax=%.3f, CurveR=%.3f, Ay=%.3f, "
           "曲率速度+曲率加速度+逆向减速+正向加速传递完成！\r\n",
           v_max,
           INS_CURVE_RADIUS_THRESHOLD_M,
           K_SAFE_CENTRIPETAL);
}

void ins_preprocess_path(void) {
    uint32 window_size = 4U;
    float noise_deadzone = 0.3f;
    float curve_threshold = 20.0f;

    for (uint32 i = 0; i < g_total_path_points; i++) {
        if (g_path_cache[i].main_wheel_speed < -0.05f) {
            g_path_states[i] = PATH_STATE_REVERSE;
            continue;
        }

        uint32 start = (i < window_size) ? 0 : i - window_size;
        uint32 end = (i + window_size >= g_total_path_points) ? g_total_path_points - 1 : i + window_size;

        float total_variation = 0;
        for (uint32 k = start; k < end; k++) {
            float diff = fabsf(ins_angle_diff(g_path_cache[k+1].yaw_angle, g_path_cache[k].yaw_angle));
            if (diff > noise_deadzone) {
                total_variation += diff;
            }
        }

        if (total_variation > curve_threshold) {
            g_path_states[i] = PATH_STATE_CURVE;
        } else {
            g_path_states[i] = PATH_STATE_STRAIGHT;
        }
    }

    int slowdown_points = 8;
    for (int i = g_total_path_points - 2; i >= 0; i--) {
        if (g_path_states[i+1] == PATH_STATE_CURVE && g_path_states[i] == PATH_STATE_STRAIGHT) {
            for (int j = 0; j < slowdown_points && (i - j) >= 0; j++) {
                if (g_path_states[i-j] == PATH_STATE_STRAIGHT) {
                    g_path_states[i-j] = PATH_STATE_CURVE;
                }
            }
        }
    }

    g_reverse_switch_idx = 0;
    for (uint32 i = 0; i < g_total_path_points - 1; i++) {
        if (g_path_cache[i].main_wheel_speed >= 0.0f && g_path_cache[i+1].main_wheel_speed < 0.0f) {
            g_reverse_switch_idx = i;
            break;
        }
    }

    // 运行曲率控速引擎
    ins_calculate_curvature_speed_limits();
}

// 供菜单调用的启动函数
void ins_start_navigation(void) {

    // Subject 1 owns path replay. Clear any Subject 2 mode left by a
    // previous menu action before the 10 ms ISR starts dispatching.
    g_ins_mode = 0;

    if (g_is_navigating) return;

    g_total_path_points = flash_log_get_count();
    if (g_total_path_points == 0) return;

    flash_log_read_batch(g_path_cache, 0, g_total_path_points);
    ins_preprocess_path();

    ins_find_switch_point();

    // 录制和回放必须使用同一个二维坐标原点。
    // 录制开始前菜单已经清零坐标；这里在每次启动回放时再次清零。
    imu_ahrs_reset_position();

    // 旧Flash记录可能没有写入二维坐标。只要换向点之前任一点出现过
    // 非零坐标，就认为本条路径支持二维进站；否则自动保留原里程控制。
    g_switch_xy_available = 0;
    if (g_reverse_switch_idx != 0U)
    {
        for (uint32 i = 0U; i <= g_reverse_switch_idx; i++)
        {
            if ((fabsf(g_path_cache[i].acc_x_g) > 0.02f)
                || (fabsf(g_path_cache[i].acc_y_g) > 0.02f))
            {
                g_switch_xy_available = 1U;
                break;
            }
        }
    }

    g_start_mileage = encoder_get_total_mileage();
    g_is_navigating = 1;

    last_idx = 0;
    last_yaw_error = 0;

    g_wait_ticks = 0;
    g_wait_finished = 0;
    g_reverse_soft_start = 0;
    g_end_stable_ticks = 0;
    g_ins_switch_2d_active = 0;
    g_ins_switch_longitudinal_error = 999.0f;
    g_ins_switch_lateral_error = 0.0f;
    g_ins_switch_xy_distance = 999.0f;

    // 将规划加速度、减速度换算成每个控制周期的速度增量
    g_speed_mgr.accel_step =
        g_path_accel_limit[0] / CONTROL_FREQ;

    g_speed_mgr.decel_step =
        TARGET_DECEL / CONTROL_FREQ;
}

// 供菜单调用的停止函数
void ins_stop_navigation(void)
{
    // A later Subject 1 start must not re-enter a stale Subject 2 state.
    g_ins_mode = 0;

    // 1. 停止惯导更新
    g_is_navigating = 0;

    // 2. 退出换向过程中的空档盲滑状态
    g_ins_neutral_glide = 0;
    g_wait_ticks = 0;
    g_wait_finished = 0;
    g_reverse_soft_start = 0;
    g_end_stable_ticks = 0;
    g_switch_xy_available = 0;
    g_ins_switch_2d_active = 0;
    g_ins_switch_longitudinal_error = 999.0f;
    g_ins_switch_lateral_error = 0.0f;
    g_ins_switch_xy_distance = 999.0f;

    // 3. 清除速度决策目标和平滑目标
    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.target_ramp  = 0.0f;

    // 4. 清除旧版兼容速度变量
    g_ins_left_speed  = 0.0f;
    g_ins_right_speed = 0.0f;

    // 5. 清除惯导转向输出
    ins_lateral_error = 0.0f;
    ins_target_yaw    = 0.0f;
    ins_gyro_z        = 0.0f;

    // 6. 清除左右后轮速度 PID 的历史状态
    PID_Reset(&pid_left);
    PID_Reset(&pid_right);

    // 7. 立即关闭后轮 PWM 输出
    Motor_Control(0.0f, 0.0f);
}

void ins_update(void) {
    if (!g_is_navigating) return;

    // 1. 获取当前实时速度 (用于动态前瞻)
    float current_v = (encoder_speed_left + encoder_speed_right) / 2.0f;

    // 2. 计算动态前瞻距离 (单位：米)
    float dynamic_Ld = INS_LOOKAHEAD_DISTANCE + (INS_LD_SPEED_COEFF * fabsf(current_v));
    if(dynamic_Ld > 1.6f) dynamic_Ld = 1.6f;

    // 3. 计算当前路程进度 (单位：米)
    float current_run_dist = encoder_get_total_mileage() - g_start_mileage;
    float target_dist = current_run_dist + dynamic_Ld;

    // 前进阶段的前瞻点不得跨入倒车路径。
    // 否则车辆尚未到达换向点，就会提前追踪倒车段的目标航向。
    if ((g_reverse_switch_idx != 0U) && (g_wait_finished == 0U))
    {
        float recorded_switch_dist_m =
            (float)g_path_cache[g_reverse_switch_idx].main_encoder_cnt
            / 1000.0f;

        if (target_dist > recorded_switch_dist_m)
        {
            target_dist = recorded_switch_dist_m;
        }
    }

    // 发车瞬间重置索引
    if (current_run_dist < 0.05f) last_idx = 0;

    // ==================== 【核心解耦修改 1】 ====================
    static uint32 current_state_idx = 0;
    if (current_run_dist < 0.05f) current_state_idx = 0;

    // 默认沿用路径最后一个状态，保证使用正终点补偿、里程略超出
    // 最后记录点时，倒车转向符号不会错误恢复成前进状态。
    PathState_e current_point_state =
        g_path_states[g_total_path_points - 1U];
    for (uint32 i = current_state_idx; i < g_total_path_points; i++) {
        float point_progress_m = (float)g_path_cache[i].main_encoder_cnt / 1000.0f;
        if (point_progress_m >= current_run_dist) {
            current_point_state = g_path_states[i];
            current_state_idx = i;
            break;
        }
    }

    // ==================== 【核心解耦修改 2】 ====================
    float target_yaw = g_path_cache[g_total_path_points - 1].yaw_angle;
    for (uint32 i = last_idx; i < g_total_path_points; i++) {
        float point_progress_m = (float)g_path_cache[i].main_encoder_cnt / 1000.0f;
        if (point_progress_m >= target_dist) {
            target_yaw = g_path_cache[i].yaw_angle;
            last_idx = i;
            break;
        }
    }

    ins_target_yaw = target_yaw;

    // ======================================================================
    // 5. 根据状态规划目标速度 (【恒减速运动学剖面控制 - 全局线性可调】)
    // ======================================================================
    extern Speed_Manager_t g_speed_mgr;
    extern PID_Controller pid_left;
    extern PID_Controller pid_right;

    // 计算当前距离物理终点还有多少米
    float total_path_dist_m =
        (float)g_path_cache[g_total_path_points - 1].main_encoder_cnt
        / 1000.0f
        + INS_END_POSITION_COMP_M;

    float remaining_dist =
        total_path_dist_m - current_run_dist;

    // 仅在换向点前的专属进站区使用，离开该区仍保持原PP路径跟踪。
    float switch_lateral_for_steer = 0.0f;
    uint8 switch_position_comp_enabled = 0U;

    // 检查本条路径是否存在需要换向倒车的物理拐点
    if (g_reverse_switch_idx != 0)
    {
        // 录制路径中的换向里程，以及独立、可标定的位置补偿。
        // 正补偿表示实车需要比当前软件里程继续多走一段后再换向。
        float switch_dist_m =
            (float)g_path_cache[g_reverse_switch_idx].main_encoder_cnt
            / 1000.0f;
        float corrected_switch_dist_m =
            switch_dist_m + INS_SWITCH_POSITION_COMP_M;
        float route_dist_to_switch =
            corrected_switch_dist_m - current_run_dist;

        // 默认仍以沿程里程控制。二维坐标通过有效性检查并进入专属进站区后，
        // 才用换向点坐标系中的纵向误差替代它。
        float control_dist_to_switch = route_dist_to_switch;

        float actual_switch_speed =
            (fabsf(encoder_speed_left)
            + fabsf(encoder_speed_right)) * 0.5f;

        float dynamic_brake_distance =
            (actual_switch_speed * actual_switch_speed)
            / (2.0f * TARGET_DECEL)
            + actual_switch_speed * INS_SWITCH_CONTROL_DELAY_S
            + INS_SWITCH_2D_BRAKE_MARGIN_M;

        float switch_approach_distance = INS_SWITCH_2D_APPROACH_MIN_M;
        if (switch_approach_distance < dynamic_brake_distance)
        {
            switch_approach_distance = dynamic_brake_distance;
        }

        if ((g_wait_finished == 0U) && g_switch_xy_available)
        {
            float switch_yaw_rad =
                g_path_cache[g_reverse_switch_idx].yaw_angle
                * (INS_PI / 180.0f);
            float switch_yaw_cos = cosf(switch_yaw_rad);
            float switch_yaw_sin = sinf(switch_yaw_rad);

            // INS_SWITCH_POSITION_COMP_M同时作用于里程目标和二维坐标目标。
            float switch_target_x =
                g_path_cache[g_reverse_switch_idx].acc_x_g
                + INS_SWITCH_POSITION_COMP_M * switch_yaw_cos;
            float switch_target_y =
                g_path_cache[g_reverse_switch_idx].acc_y_g
                + INS_SWITCH_POSITION_COMP_M * switch_yaw_sin;

            float current_pos_x = imu_ahrs_get_pos_x();
            float current_pos_y = imu_ahrs_get_pos_y();
            float switch_dx = switch_target_x - current_pos_x;
            float switch_dy = switch_target_y - current_pos_y;

            // 将目标点误差投影到录制换向点的航向坐标系：
            // longitudinal只负责换向点减速和停车。
            float switch_longitudinal =
                switch_dx * switch_yaw_cos
                + switch_dy * switch_yaw_sin;
            float switch_xy_distance =
                sqrtf(switch_dx * switch_dx + switch_dy * switch_dy);

            // 横向修正不能直接追换向点，否则弯曲进站段会被切弯。
            // 在当前里程索引附近寻找离车辆最近的录制二维点，再计算
            // 车辆相对原路径的横向误差，保持原PP所描述的路径形状。
            uint32 nearest_center_idx = current_state_idx;
            if (nearest_center_idx > g_reverse_switch_idx)
            {
                nearest_center_idx = g_reverse_switch_idx;
            }

            uint32 nearest_start_idx =
                (nearest_center_idx > INS_SWITCH_2D_NEAREST_SEARCH_POINTS)
                ? (nearest_center_idx - INS_SWITCH_2D_NEAREST_SEARCH_POINTS)
                : 0U;
            uint32 nearest_end_idx =
                nearest_center_idx + INS_SWITCH_2D_NEAREST_SEARCH_POINTS;
            if (nearest_end_idx > g_reverse_switch_idx)
            {
                nearest_end_idx = g_reverse_switch_idx;
            }

            uint32 nearest_path_idx = nearest_center_idx;
            float nearest_distance_sq = 1.0e30f;
            for (uint32 j = nearest_start_idx; j <= nearest_end_idx; j++)
            {
                float path_dx = g_path_cache[j].acc_x_g - current_pos_x;
                float path_dy = g_path_cache[j].acc_y_g - current_pos_y;
                float path_distance_sq = path_dx * path_dx + path_dy * path_dy;
                if (path_distance_sq < nearest_distance_sq)
                {
                    nearest_distance_sq = path_distance_sq;
                    nearest_path_idx = j;
                }
            }

            float nearest_yaw_rad =
                g_path_cache[nearest_path_idx].yaw_angle
                * (INS_PI / 180.0f);
            float nearest_dx =
                g_path_cache[nearest_path_idx].acc_x_g - current_pos_x;
            float nearest_dy =
                g_path_cache[nearest_path_idx].acc_y_g - current_pos_y;
            float path_lateral_error =
                -nearest_dx * sinf(nearest_yaw_rad)
                + nearest_dy * cosf(nearest_yaw_rad);

            g_ins_switch_longitudinal_error = switch_longitudinal;
            g_ins_switch_lateral_error = path_lateral_error;
            g_ins_switch_xy_distance = switch_xy_distance;

            // 使用“坐标直线距离与沿程剩余距离”的差值做保护。
            // 坐标未清零、旧Flash无坐标或坐标系明显错位时，不启用二维补偿。
            float positive_route_dist = route_dist_to_switch;
            if (positive_route_dist < 0.0f)
            {
                positive_route_dist = 0.0f;
            }
            float coordinate_disagreement =
                fabsf(switch_xy_distance - positive_route_dist);

            if ((g_ins_switch_2d_active == 0U)
                && (route_dist_to_switch <= switch_approach_distance)
                && (coordinate_disagreement
                    <= INS_SWITCH_2D_ACTIVATE_GUARD_M))
            {
                // 一旦进入便锁存，避免坐标噪声造成控制模式反复切换。
                g_ins_switch_2d_active = 1U;
            }

            if (g_ins_switch_2d_active)
            {
                control_dist_to_switch = switch_longitudinal;
                switch_lateral_for_steer = path_lateral_error;
                switch_position_comp_enabled = 1U;
            }
        }

        // ------------------ 【阶段 A：前行且逐渐逼近换向点】 ------------------
        if (g_wait_finished == 0)
        {
            if (control_dist_to_switch > INS_SWITCH_POSITION_TOL_M)
            {
                // 二维进站时额外扣除安全余量，促使车辆更早降速；
                // 未启用二维坐标时保持原来的里程减速特性。
                float brake_margin = g_ins_switch_2d_active
                    ? INS_SWITCH_2D_BRAKE_MARGIN_M
                    : 0.0f;
                float effective_brake_dist =
                    control_dist_to_switch
                    - actual_switch_speed * INS_SWITCH_CONTROL_DELAY_S
                    - brake_margin;

                if (effective_brake_dist < 0.0f)
                {
                    effective_brake_dist = 0.0f;
                }

                float safe_approach_speed =
                    sqrtf(2.0f * TARGET_DECEL * effective_brake_dist);

                // 当前路径点的弯道/直道速度上限。
                float curve_limit_speed = g_path_speed_limit[current_state_idx];

                // 仅前进阶段按当前曲率限制加速度；倒车软启动逻辑保持原样。
                g_speed_mgr.accel_step =
                    g_path_accel_limit[current_state_idx]
                    / CONTROL_FREQ;

                // 只有进入动态制动距离后才施加换向停车速度约束；
                // 进入之前继续服从曲率速度表。
                float final_target = curve_limit_speed;
                if ((control_dist_to_switch <= dynamic_brake_distance)
                    && (safe_approach_speed < final_target))
                {
                    final_target = safe_approach_speed;
                }

                // 进入换向点附近后保持小速度爬行，防止静摩擦使车辆提前停住。
                // 一旦进入位置容差，下一阶段会立即把目标速度置零。
                if ((control_dist_to_switch <= INS_SWITCH_CREEP_ZONE_M)
                    && (final_target < INS_SWITCH_CREEP_SPEED_MPS))
                {
                    final_target = INS_SWITCH_CREEP_SPEED_MPS;
                }

                g_speed_mgr.target_final = final_target;
                g_speed_mgr.decel_step = TARGET_DECEL / CONTROL_FREQ;
                g_wait_ticks = 0;
            }
            // ------------------ 【阶段 B：零速闭环与物理静止确认】 ------------------
            else
            {
                g_speed_mgr.target_final = 0.0f;
                g_speed_mgr.decel_step = TARGET_DECEL / CONTROL_FREQ;

                // 保持速度闭环工作，不再提前清空target_ramp、PID并空档滑行。
                // 只有实际轮速和车体角速度连续稳定后，才允许进入倒车阶段。
                g_ins_neutral_glide = 0;

                float actual_switch_speed =
                    (fabsf(encoder_speed_left)
                    + fabsf(encoder_speed_right)) * 0.5f;
                ahrs_data_t switch_imu_data = imu_ahrs_get_data();

                if ((actual_switch_speed <= INS_SWITCH_STOP_SPEED_MPS)
                    && (fabsf(switch_imu_data.gyro_z_dps)
                        <= INS_SWITCH_STOP_GYRO_DPS))
                {
                    if (g_wait_ticks < INS_SWITCH_STABLE_TICKS)
                    {
                        g_wait_ticks++;
                    }
                }
                else
                {
                    g_wait_ticks = 0;
                }

                if (g_wait_ticks < INS_SWITCH_STABLE_TICKS)
                {
                    return;
                }

                // 已确认物理静止，此时才允许清理速度斜坡与PID历史量。
                g_speed_mgr.target_ramp = 0.0f;
                PID_Reset(&pid_left);
                PID_Reset(&pid_right);

                // 将累计里程的路径进度对齐到记录的换向点。
                // 此时车辆已经到达“录制点+标定补偿”附近，不再凭空跳过15cm。
                g_start_mileage = encoder_get_total_mileage() - switch_dist_m;
                last_idx = g_reverse_switch_idx;

                // 倒车起步阶段持续使用较小加速度，避免只限制一个控制周期。
                g_speed_mgr.accel_step =
                    INS_REVERSE_ACCEL_LIMIT / CONTROL_FREQ;
                g_reverse_soft_start = 1;

                g_wait_finished = 1; // 切换状态
                g_ins_switch_2d_active = 0U;
            }
        }
        // ------------------ 【阶段 C：已彻底静止并顺利过渡到倒车状态】 ------------------
        else
        {
            // 倒车实际速度达到阈值前保持软启动，防止换向瞬间打滑空转。
            float actual_reverse_speed =
                (fabsf(encoder_speed_left)
                + fabsf(encoder_speed_right)) * 0.5f;

            if (g_reverse_soft_start
                && (actual_reverse_speed < INS_REVERSE_SOFT_RELEASE_SPEED))
            {
                g_speed_mgr.accel_step =
                    INS_REVERSE_ACCEL_LIMIT / CONTROL_FREQ;
            }
            else
            {
                g_reverse_soft_start = 0;
                g_speed_mgr.accel_step =
                    START_ACCEL_LIMIT / CONTROL_FREQ;
            }

            // 倒车终点：继续按剩余距离动态降速，不再在剩余12cm时直接退出。
            if (remaining_dist > INS_STOP_DISTANCE_THRESHOLD)
            {
                float effective_end_dist =
                    remaining_dist
                    - actual_reverse_speed * INS_END_CONTROL_DELAY_S;

                if (effective_end_dist < 0.0f)
                {
                    effective_end_dist = 0.0f;
                }

                float target_v_rev =
                    sqrtf(2.0f * TARGET_DECEL * effective_end_dist);
                float reverse_cruise_limit = fabsf(ins_cruise_speed) * 0.5f;

                if (target_v_rev > reverse_cruise_limit)
                {
                    target_v_rev = reverse_cruise_limit;
                }

                // 最后10cm低速逼近，避免静摩擦导致停车不足。
                if ((remaining_dist <= INS_END_CREEP_ZONE_M)
                    && (target_v_rev < INS_END_CREEP_SPEED_MPS))
                {
                    target_v_rev = INS_END_CREEP_SPEED_MPS;
                }

                g_speed_mgr.target_final = -target_v_rev;
                g_speed_mgr.decel_step = TARGET_DECEL / CONTROL_FREQ;
                g_end_stable_ticks = 0;
            }
            else
            {
                // 已进入终点位置容差：目标置零，并等待车体真实静止。
                g_speed_mgr.target_final = 0.0f;
                g_speed_mgr.decel_step = TARGET_DECEL / CONTROL_FREQ;

                ahrs_data_t end_imu_data = imu_ahrs_get_data();
                if ((actual_reverse_speed <= INS_END_STOP_SPEED_MPS)
                    && (fabsf(end_imu_data.gyro_z_dps)
                        <= INS_END_STOP_GYRO_DPS))
                {
                    if (g_end_stable_ticks < INS_END_STABLE_TICKS)
                    {
                        g_end_stable_ticks++;
                    }
                }
                else
                {
                    g_end_stable_ticks = 0;
                }

                if (g_end_stable_ticks >= INS_END_STABLE_TICKS)
                {
                    ins_stop_navigation();
                }
                return;
            }
        }
    }
    else
    {
        // ==================== 无倒车任务：同样精确处理前进终点 ====================
        float actual_end_speed =
            (fabsf(encoder_speed_left)
            + fabsf(encoder_speed_right)) * 0.5f;

        if (remaining_dist > INS_STOP_DISTANCE_THRESHOLD)
        {
            // 无换向任务的前进阶段同样使用曲率加速度表。
            g_speed_mgr.accel_step =
                g_path_accel_limit[current_state_idx]
                / CONTROL_FREQ;

            float effective_end_dist =
                remaining_dist
                - actual_end_speed * INS_END_CONTROL_DELAY_S;

            if (effective_end_dist < 0.0f)
            {
                effective_end_dist = 0.0f;
            }

            float target_v_fwd =
                sqrtf(2.0f * TARGET_DECEL * effective_end_dist);
            float path_speed_limit =
                fabsf(g_path_speed_limit[current_state_idx]);

            if (target_v_fwd > path_speed_limit)
            {
                target_v_fwd = path_speed_limit;
            }

            if ((remaining_dist <= INS_END_CREEP_ZONE_M)
                && (target_v_fwd < INS_END_CREEP_SPEED_MPS))
            {
                target_v_fwd = INS_END_CREEP_SPEED_MPS;
            }

            if (current_point_state == PATH_STATE_REVERSE)
            {
                g_speed_mgr.target_final = -target_v_fwd;
            }
            else
            {
                g_speed_mgr.target_final = target_v_fwd;
            }

            g_speed_mgr.decel_step = TARGET_DECEL / CONTROL_FREQ;
            g_end_stable_ticks = 0;
        }
        else
        {
            g_speed_mgr.target_final = 0.0f;
            g_speed_mgr.decel_step = TARGET_DECEL / CONTROL_FREQ;

            ahrs_data_t end_imu_data = imu_ahrs_get_data();
            if ((actual_end_speed <= INS_END_STOP_SPEED_MPS)
                && (fabsf(end_imu_data.gyro_z_dps)
                    <= INS_END_STOP_GYRO_DPS))
            {
                if (g_end_stable_ticks < INS_END_STABLE_TICKS)
                {
                    g_end_stable_ticks++;
                }
            }
            else
            {
                g_end_stable_ticks = 0;
            }

            if (g_end_stable_ticks >= INS_END_STABLE_TICKS)
            {
                ins_stop_navigation();
            }
            return;
        }
    }

    // 7. 计算角度偏差并输出
    float current_yaw = imu_ahrs_get_yaw();
    float yaw_error = ins_angle_diff(target_yaw, current_yaw); // 航向误差（度）

    // ==================== 纯 PP 算法计算目标打角 ====================
    float alpha_rad = DEG_TO_RAD_ANGLE(yaw_error);
    float pp_angle_rad = atan2f(2.0f * WHEEL_BASE * sinf(alpha_rad), dynamic_Ld);
    float pp_angle_deg = pp_angle_rad * (180.0f / PI); // 转换回度数

    // 转向符号由真实换向状态机决定，而不是仅依赖路径索引。
    // 这样位置补偿使里程暂时越过录制换向索引时，也不会提前反转方向。
    uint8 is_reverse_phase =
        (g_reverse_switch_idx != 0U)
        ? g_wait_finished
        : (current_point_state == PATH_STATE_REVERSE);

    if (is_reverse_phase) {
        pp_angle_deg = -pp_angle_deg;
    }

    // 二维坐标不接管原PP，只在换向点前的专属进站区叠加小幅横向修正。
    // 正横向误差表示目标在录制航向左侧，因此补偿方向与正航向误差一致。
    if (switch_position_comp_enabled && (g_wait_finished == 0U))
    {
        float position_steer_comp =
            INS_SWITCH_2D_LATERAL_KP_DEG_PER_M
            * switch_lateral_for_steer;

        if (position_steer_comp > INS_SWITCH_2D_STEER_COMP_MAX_DEG)
        {
            position_steer_comp = INS_SWITCH_2D_STEER_COMP_MAX_DEG;
        }
        else if (position_steer_comp < -INS_SWITCH_2D_STEER_COMP_MAX_DEG)
        {
            position_steer_comp = -INS_SWITCH_2D_STEER_COMP_MAX_DEG;
        }

        pp_angle_deg += position_steer_comp;
    }

    ins_lateral_error = pp_angle_deg;

    ahrs_data_t imu_data = imu_ahrs_get_data();
    ins_gyro_z = imu_data.gyro_z_dps;
}

uint8 ins_is_navigating(void) {
    return g_is_navigating;
}

// ==========================================
// 附加在 ins.c 的最下面：供外部获取计算好的电机速度
// ==========================================
void ins_pure_pursuit(float *left_speed, float *right_speed) {
    if (left_speed)  *left_speed = g_ins_left_speed;
    if (right_speed) *right_speed = g_ins_right_speed;
}




