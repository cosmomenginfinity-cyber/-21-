#include "pathrecorder.h"
#include "imu_datalogger.h"
#include "encoder.h"
#include "imu_ahrs.h"
#include <math.h>
#include <stdio.h>

static path_manager_t g_path_manager;
static uint8 g_path_initialized = 0;
// 防止同一条路线重复 Save 时反复往 buffer 里补 final 点
static uint8 g_switch_point_written = 0;
static uint8 g_end_point_written = 0;

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
static uint8 path_recorder_write_current_point(uint32 event_tag)
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

    if (!imu_logger_manual_write_event(event_tag))
    {
        uart_write_string(UART_0, "\r\n>>> [PATH] Logger write failed.\r\n");
        return 0;
    }

    path_recorder_fill_local_point(g_path_manager.current_point_idx);

    g_path_manager.current_point_idx++;
    g_path_manager.last_record_distance = encoder_get_total_mileage();

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

    imu_logger_init();

    g_path_manager.is_recording = 1;
    g_path_manager.current_point_idx = 0;
    g_switch_point_written = 0;
    g_end_point_written = 0;

    // 使用绝对总里程作为起始参考
    g_path_manager.last_record_distance = encoder_get_total_mileage();

    // 记录初始点
    path_recorder_write_current_point(PATH_RECORD_EVENT_NONE);
    uart_write_string(UART_0, "\r\n>>> [RECORD START]\r\n");
}

// 供菜单调用的【停止记录】接口
void path_recorder_stop(void) {
    if (!g_path_manager.is_recording) return;

    g_path_manager.is_recording = 0;
    /*if (g_path_manager.current_point_idx > 0) {
        imu_logger_save_buffer_to_flash();
        uart_write_string(UART_0, "\r\n>>> [RECORD STOP] Saved.\r\n");
    }*/
    uart_write_string(UART_0, "\r\n>>> [RECORD STOP] Data in Buffer.\r\n");
}

uint8 path_recorder_mark_switch_point(void)
{
    if (!g_path_initialized || !g_path_manager.is_recording)
    {
        return PATH_MARK_FAILED;
    }

    if (g_switch_point_written)
    {
        return PATH_MARK_ALREADY;
    }

    ahrs_data_t imu_data = imu_ahrs_get_data();
    if ((fabsf(encoder_speed_left) > 0.05f)
        || (fabsf(encoder_speed_right) > 0.05f)
        || (fabsf(imu_data.gyro_z_dps) > 2.0f))
    {
        return PATH_MARK_MOVING;
    }

    if (!path_recorder_write_current_point(PATH_RECORD_EVENT_SWITCH))
    {
        return PATH_MARK_FAILED;
    }

    g_switch_point_written = 1;
    uart_write_string(UART_0, "\r\n>>> [PATH] Switch point saved.\r\n");
    return PATH_MARK_OK;
}

uint8 path_recorder_force_end_point(void)
{
    if (!g_path_initialized || !g_path_manager.is_recording)
    {
        return PATH_MARK_FAILED;
    }

    if (g_end_point_written)
    {
        return PATH_MARK_ALREADY;
    }

    if (!path_recorder_write_current_point(PATH_RECORD_EVENT_END))
    {
        return PATH_MARK_FAILED;
    }

    g_end_point_written = 1;
    uart_write_string(UART_0, "\r\n>>> [PATH] End point saved.\r\n");
    return PATH_MARK_OK;
}

uint8 path_recorder_subject1_is_complete(void)
{
    return (g_switch_point_written && g_end_point_written) ? 1U : 0U;
}
// 修改后的自动更新函数，放在定时器或主循环，不再包含按键
void path_recorder_update(void) {
    if (!g_path_initialized || !g_path_manager.is_recording) return;

    float current_dist = encoder_get_total_mileage();

    if (fabs(current_dist - g_path_manager.last_record_distance) >= PATH_RECORD_DISTANCE_THRESHOLD) {
        if (g_path_manager.current_point_idx < PATH_MAX_POINT_NUM) {
            if (path_recorder_write_current_point(PATH_RECORD_EVENT_NONE)) {
                uart_write_string(UART_0, "*");
            }
        }
    }
}
uint32 path_recorder_get_record_count(void) { return g_path_manager.current_point_idx; }
uint8 path_recorder_is_working(void) { return g_path_manager.is_recording; }
void path_recorder_read_all_points(path_manager_t *out_manager) { memcpy(out_manager, &g_path_manager, sizeof(path_manager_t)); }
uint8 path_recorder_clear_all_points(void) {
    g_path_manager.current_point_idx = 0;
    g_switch_point_written = 0;
    g_end_point_written = 0;
    return 1;
}
