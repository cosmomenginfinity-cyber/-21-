


#include "ins.h"
#include "imu_ahrs.h"
#include "encoder.h"
#include "flash.h"
#include "imu_datalogger.h"
#include "zf_driver_uart.h"
#include "PID.h" // 确保包含 PID 头文件以调用 PID_Reset()
#include <math.h>

// ins.c
#define SUBJECT3_ONLY  1
#if !SUBJECT3_ONLY

// 原来的完整科一代码
// 包括所有 g_path_cache、速度表和 ins_update()



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

// 4. 路径绝对曲率表（单位：1/m），用于曲率限速和正向/逆向速度传递
static float g_path_curvature[FLASH_MAX_LOG_RECORDS] __attribute__((section(".bss.lmubss")));

// ===================== 【速度决策物理参数】 =====================
// 路面有效附着系数：队内实测 μ = 0.57
// 横向安全系数：0.90
// 安全横向加速度 AY_MAX_SAFE = μ * g * safety = 0.57 * 9.81 * 0.90 ≈ 5.03 m/s^2
volatile float K_SAFE_CENTRIPETAL =
        INS_ROAD_FRICTION_MU
      * INS_GRAVITY_ACCEL
      * INS_LATERAL_SAFETY_FACTOR;

// 最大可靠规划减速度（m/s^2）
// 当前仍为初期参数，后续应根据实际停车距离标定
volatile float TARGET_DECEL = 1.5f;

// 最大可靠规划加速度（m/s^2）
// 当前仍为初期参数，后续应根据直线加速实验标定
volatile float START_ACCEL_LIMIT = 3.2f;

// ===================== 【换向状态控制变量】 =====================
uint32 g_reverse_switch_idx = 0;   // 换向点的绝对索引（为0代表本条赛道无倒车任务）
static uint32 g_wait_ticks = 0;    // 换向静止等待时钟计数器
static uint8  g_wait_finished = 0; // 换向静止等待结束标志位

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
    return 1;
}

// 换向扫描辅助函数
void ins_find_switch_point(void) {
    g_reverse_switch_idx = 0; // 扫描前清空历史缓存
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
    if (v_max < INS_MIN_PATH_SPEED)
    {
        v_max = INS_MIN_PATH_SPEED;
    }

    float ay_max_safe = K_SAFE_CENTRIPETAL;
    if (ay_max_safe < 0.10f)
    {
        ay_max_safe = 0.10f;
    }

    // 当 R >= R_full_speed 时，以 v_max 行驶仍不会超过安全横向加速度
    // R_full_speed = v_max^2 / AY_MAX_SAFE
    float full_speed_radius = (v_max * v_max) / ay_max_safe;

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
    // 第2步：对曲率做滑动平均，并由曲率计算曲率半径和弯道速度上限
    //
    // 曲率 kappa = 1 / R
    // 弯道安全速度 v = sqrt(AY_MAX_SAFE / |kappa|)
    // 等价于 v = sqrt(AY_MAX_SAFE * R)
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

        if (smoothed_curvature > INS_CURVATURE_EPSILON)
        {
            float radius = 1.0f / smoothed_curvature;

            if (radius < full_speed_radius)
            {
                point_speed_limit =
                    sqrtf(ay_max_safe * radius);
            }
            else
            {
                point_speed_limit = v_max;
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

        g_path_speed_limit[i] = point_speed_limit;
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

        float max_reachable_v =
            sqrtf(v_prev * v_prev
                + 2.0f * START_ACCEL_LIMIT * ds);

        if (g_path_speed_limit[i] > max_reachable_v)
        {
            g_path_speed_limit[i] = max_reachable_v;
        }
    }

#if INS_SPEED_TABLE_DEBUG
    printf("IDX,DIST_M,KAPPA,RADIUS_M,SPEED_MPS\r\n");

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

        printf("%lu,%.3f,%.5f,%.3f,%.3f\r\n",
               (unsigned long)i,
               distance_m,
               g_path_curvature[i],
               radius_m,
               g_path_speed_limit[i]);
    }
#endif

    printf("[速度决策] AYsafe=%.3f, Vmax=%.3f, Rfull=%.3f, "
           "逆向减速+正向加速传递完成！\r\n",
           ay_max_safe,
           v_max,
           full_speed_radius);
}

void ins_preprocess_path(void) {
    int window_size = 4;
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

    if (g_is_navigating) return;

    g_total_path_points = flash_log_get_count();
    if (g_total_path_points == 0) return;

    flash_log_read_batch(g_path_cache, 0, g_total_path_points);
    ins_preprocess_path();

    ins_find_switch_point();

    g_start_mileage = encoder_get_total_mileage();
    g_is_navigating = 1;

    last_idx = 0;
    last_yaw_error = 0;

    g_wait_ticks = 0;
    g_wait_finished = 0;

    // 将规划加速度、减速度换算成每个控制周期的速度增量
    g_speed_mgr.accel_step =
        START_ACCEL_LIMIT / CONTROL_FREQ;

    g_speed_mgr.decel_step =
        TARGET_DECEL / CONTROL_FREQ;
}

// 供菜单调用的停止函数
void ins_stop_navigation(void)
{
    // 1. 停止惯导更新
    g_is_navigating = 0;

    // 2. 退出换向过程中的空档盲滑状态
    g_ins_neutral_glide = 0;

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
    if(dynamic_Ld > 1.2f) dynamic_Ld = 1.2f;

    // 3. 计算当前路程进度 (单位：米)
    float current_run_dist = encoder_get_total_mileage() - g_start_mileage;
    float target_dist = current_run_dist + dynamic_Ld;

    // 发车瞬间重置索引
    if (current_run_dist < 0.05f) last_idx = 0;

    // ==================== 【核心解耦修改 1】 ====================
    static uint32 current_state_idx = 0;
    if (current_run_dist < 0.05f) current_state_idx = 0;

    PathState_e current_point_state = PATH_STATE_STRAIGHT;
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
        / 1000.0f;

    float remaining_dist =
        total_path_dist_m - current_run_dist;

    // 必须在所有 sqrtf() 运算之前检查终点，防止 remaining_dist 为负数
    if (remaining_dist <= INS_STOP_DISTANCE_THRESHOLD)
    {
        ins_stop_navigation();
        return;
    }

    // 检查本条路径是否存在需要换向倒车的物理拐点
    if (g_reverse_switch_idx != 0)
    {
        // 提取换向点的绝对物理位移（米）
        float switch_dist_m = (float)g_path_cache[g_reverse_switch_idx].main_encoder_cnt / 1000.0f;
        float dist_to_switch = switch_dist_m - current_run_dist;

        // ------------------ 【阶段 A：前行且逐渐逼近换向点】 ------------------
        if (g_wait_finished == 0)
        {
            if (dist_to_switch > 0.15f)
            {
                // 先计算由于换向引发的刹车降速
                float safe_approach_speed = ins_cruise_speed * (dist_to_switch / 1.5f);

                // 再去读取当前的弯道曲率极限限速值
                float curve_limit_speed = g_path_speed_limit[current_state_idx];

                // 取两者之中的极小值，实现双物理边界安全控制！
                float final_target = (safe_approach_speed < curve_limit_speed) ? safe_approach_speed : curve_limit_speed;

                if (final_target < 0.20f) {
                    final_target = 0.20f;
                }
                g_speed_mgr.target_final = final_target;
            }
            // ------------------ 【阶段 B：物理换向临界区（物理盲滑收敛与相位重对齐）】 ------------------
            else
            {
                g_speed_mgr.target_final = 0.0f;
                g_speed_mgr.target_ramp = 0.0f;

                // 盲滑等待：让车体在 250ms 内完全卸掉物理动能
                if (g_wait_ticks < 25) { // 在 100Hz 频率下，25 ticks = 250ms
                    g_wait_ticks++;

                    PID_Reset(&pid_left); // 强制清除历史累积的前向积分（I项）
                    PID_Reset(&pid_right);

                    g_ins_neutral_glide = 1; // 开启全局空档标志
                    Motor_Control(0, 0);     // 强行切断后轴动力输出，维持物理滚动
                    return; // 强行挂起退出本周期
                }

                g_ins_neutral_glide = 0; // 盲滑期结束，释放闭锁，允许电机运行

                // 相位重对齐：一笔抹平由于刹车拖地打滑造成的“少计里程”，坐标精度 100% 恢复！
                g_start_mileage = encoder_get_total_mileage() - switch_dist_m;

                // 限制起步斜坡斜率，倒车温柔起步
                g_speed_mgr.accel_step = 0.003f;

                g_wait_finished = 1; // 切换状态
            }
        }
        // ------------------ 【阶段 C：已彻底静止并顺利过渡到倒车状态】 ------------------
        else
        {
            // 已经安全起步，恢复正常规划加速度
            g_speed_mgr.accel_step =
                START_ACCEL_LIMIT / CONTROL_FREQ;

            // 倒车终点同样使用恒减速剖面控制进行精准停车
            float decel_start_dist_rev = (ins_cruise_speed * 0.5f * ins_cruise_speed * 0.5f) / (2.0f * TARGET_DECEL);

            if (remaining_dist <= decel_start_dist_rev)
            {
                float target_v_rev = sqrtf(2.0f * TARGET_DECEL * remaining_dist);
                if (target_v_rev < 0.25f) {
                    target_v_rev = 0.25f;
                }
                g_speed_mgr.target_final = -target_v_rev; // 倒车终点减速必须为负值！
            }
            else
            {
                g_speed_mgr.target_final = -ins_cruise_speed * 0.5f; // 常规倒车巡航
            }
        }
    }
    else
    {
        // ==================== 兼容情况：若整条路径没有倒车任务（无换向点） ====================
        float decel_start_dist_fwd = (ins_cruise_speed * ins_cruise_speed) / (2.0f * TARGET_DECEL);

        if (remaining_dist <= decel_start_dist_fwd)
        {
            float target_v_fwd = sqrtf(2.0f * TARGET_DECEL * remaining_dist);
            if (target_v_fwd < 0.25f) {
                target_v_fwd = 0.25f;
            }

            if (current_point_state == PATH_STATE_REVERSE) {
                g_speed_mgr.target_final = -target_v_fwd;
            } else {
                g_speed_mgr.target_final = target_v_fwd;
            }
        }
        else
        {
            // 正常巡航：直接从速度分配表（g_path_speed_limit）中提取最科学的物理速度！
            g_speed_mgr.target_final = g_path_speed_limit[current_state_idx];
        }
    }

    // 7. 计算角度偏差并输出
    float current_yaw = imu_ahrs_get_yaw();
    float yaw_error = ins_angle_diff(target_yaw, current_yaw); // 航向误差（度）

    // ==================== 纯 PP 算法计算目标打角 ====================
    float alpha_rad = DEG_TO_RAD_ANGLE(yaw_error);
    float pp_angle_rad = atan2f(2.0f * WHEEL_BASE * sinf(alpha_rad), dynamic_Ld);
    float pp_angle_deg = pp_angle_rad * (180.0f / PI); // 转换回度数

    // 倒车状态下的转角输出反向修正
    if (current_point_state == PATH_STATE_REVERSE) {
        pp_angle_deg = -pp_angle_deg;
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


#else

uint8 ins_init(void)
{
    return 1U;
}

void ins_update(void)
{
}

void ins_start_navigation(void)
{
}

void ins_stop_navigation(void)
{
}

uint8 ins_is_navigating(void)
{
    return 0U;
}

void ins_find_switch_point(void)
{
}

void ins_calculate_curvature_speed_limits(void)
{
}

void ins_preprocess_path(void)
{
}

void ins_pure_pursuit(float *left_speed, float *right_speed)
{
    if (left_speed != NULL)
    {
        *left_speed = 0.0f;
    }

    if (right_speed != NULL)
    {
        *right_speed = 0.0f;
    }
}
#endif
