#include "pathrecorder.h"
#include "imu_datalogger.h"
#include "encoder.h"
#include "imu_ahrs.h"
#include <math.h>
#include <stdio.h>

static path_manager_t g_path_manager;
static uint8 g_path_initialized = 0;
// 防止同一条路线重复 Save 时反复往 buffer 里补 final 点
static uint8 g_final_ref_written = 0;

static void path_recorder_fill_local_point(uint32 idx)
{
    if (idx >= PATH_MAX_POINT_NUM)
    {
        return;
    }

    g_path_manager.points[idx].pos_x = imu_ahrs_get_pos_x();
    g_path_manager.points[idx].pos_y = imu_ahrs_get_pos_y();
    g_path_manager.points[idx].timestamp_ms = 0;
    g_path_manager.points[idx].valid = 1;
}
// 统一的“记录当前点”函数。
// 成功后会：
// 1. 写入 imu_logger buffer
// 2. 更新 path_manager 本地点
// 3. current_point_idx++
static uint8 path_recorder_write_current_point(void)
{
    if (!g_path_initialized)
    {
        return 0;
    }

    if (g_path_manager.current_point_idx >= PATH_MAX_POINT_NUM)
    {
        uart_write_string(UART_0, "\r\n>>> [PATH] Point buffer full.\r\n");
        return 0;
    }

    if (!imu_logger_manual_write(NULL))
    {
        uart_write_string(UART_0, "\r\n>>> [PATH] Logger write failed.\r\n");
        return 0;
    }

    path_recorder_fill_local_point(g_path_manager.current_point_idx);

    g_path_manager.current_point_idx++;

    return 1;
}
uint8 path_recorder_init(void) {
    memset(&g_path_manager, 0, sizeof(path_manager_t));
    g_path_initialized = 1;
    return 1;
}

// ===================== pathrecorder.c 彻底重构 =====================

// 供菜单调用的【开始记录】接口
void path_recorder_start(void) {
    if (g_path_manager.is_recording) return;

    // Flash 已在系统初始化时完成；竞速录制只清 RAM，避免重复访问 Flash。
    if (!imu_logger_reset_buffer()) return;

    g_path_manager.is_recording = 1;
    g_path_manager.current_point_idx = 0;

    // 每次重新录制都允许重新补写停车区 final_yaw 参考点
    g_final_ref_written = 0;

    // 使用绝对总里程作为起始参考
    g_path_manager.last_record_distance = encoder_get_total_mileage();

    // 记录初始点，同时正确更新 current_point_idx
    if (!path_recorder_write_current_point())
    {
        g_path_manager.is_recording = 0;
        uart_write_string(UART_0, "\r\n>>> [RECORD START] Initial point failed.\r\n");
        return;
    }
}

// 供菜单调用的【停止记录】接口
void path_recorder_stop(void) {
    if (!g_path_manager.is_recording) return;

    g_path_manager.is_recording = 0;
    /*if (g_path_manager.current_point_idx > 0) {
        imu_logger_save_buffer_to_flash();
        uart_write_string(UART_0, "\r\n>>> [RECORD STOP] Saved.\r\n");
    }*/
}
// 科目三专用：强制记录停车区 final_yaw 参考点
//
// 新版重点：
// 允许在 path_recorder_stop() 之后调用。
// 因为当前菜单流程是：
// Subject 3 -> Record Path -> KEY4 Stop
// Subject 3 -> Save to Flash
//
// 这个 final 点主要用于 back_ins：
// target_yaw = recorded_yaw - final_yaw
uint8 path_recorder_force_final_ref_point(void)
{
    if (!g_path_initialized)
    {
        uart_write_string(UART_0, "\r\n>>> [SUB3] Path not initialized.\r\n");
        return 0;
    }

    // 没有任何记录点，说明没有先执行 Record Path
    if (g_path_manager.current_point_idx == 0)
    {
        uart_write_string(UART_0, "\r\n>>> [SUB3] No record data.\r\n");
        return 0;
    }

    // 防止同一条路线重复补 final 点
    if (g_final_ref_written)
    {
        uart_write_string(UART_0, "\r\n>>> [SUB3] Final ref already saved.\r\n");
        return 1;
    }

    if (g_path_manager.current_point_idx >= PATH_MAX_POINT_NUM)
    {
        uart_write_string(UART_0, "\r\n>>> [SUB3] Point buffer full.\r\n");
        return 0;
    }

    // 强制记录当前点。
    // 这个点作为停车区 final_yaw 参考点。
    // 注意：这里不再要求 is_recording == 1。
    if (!path_recorder_write_current_point())
    {
        uart_write_string(UART_0, "\r\n>>> [SUB3] Final ref save failed.\r\n");
        return 0;
    }

    g_path_manager.last_record_distance = encoder_get_total_mileage();

    g_final_ref_written = 1;

    return 1;
}
// 修改后的自动更新函数，放在定时器或主循环，不再包含按键
void path_recorder_update(void) {
    if (!g_path_initialized || !g_path_manager.is_recording) return;

    float current_dist = encoder_get_total_mileage();

    if (fabs(current_dist - g_path_manager.last_record_distance) >= PATH_RECORD_DISTANCE_THRESHOLD) {
        if (g_path_manager.current_point_idx < PATH_MAX_POINT_NUM) {
            // 统一写入 Logger 和本地路径表，避免两套计数不同步
            if (path_recorder_write_current_point())
            {
                g_path_manager.last_record_distance = current_dist;
            }
        }
    }
}
uint32 path_recorder_get_record_count(void) { return g_path_manager.current_point_idx; }
uint8 path_recorder_is_working(void) { return g_path_manager.is_recording; }
void path_recorder_read_all_points(path_manager_t *out_manager) { memcpy(out_manager, &g_path_manager, sizeof(path_manager_t)); }
uint8 path_recorder_clear_all_points(void)
{
    g_path_manager.current_point_idx = 0;
    g_final_ref_written = 0;
    return 1;
}
