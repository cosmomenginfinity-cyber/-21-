#ifndef PATHRECORDER_H
#define PATHRECORDER_H

#include "zf_common_typedef.h"
#include "zf_common_headfile.h"
#include "imu_ahrs.h"       // IMU坐标
#include "encoder.h"        // 单编码器里程
#include "flash.h"          // Flash存储
#include "zf_device_key.h"  // 按键检测

// 配置参数
#define PATH_RECORD_DISTANCE_THRESHOLD  0.20f   // 每20cm记录一次（0.20米）
#define PATH_MAX_POINT_NUM              FLASH_MAX_LOG_RECORDS // 与Flash最大记录数一致

// 路径点结构体
typedef struct {
    float pos_x;            // X坐标（米）
    float pos_y;            // Y坐标（米）
    uint32_t timestamp_ms;  // 时间戳
    uint8 valid;            // 有效标志
    uint8 reserved[3];      // 对齐
} path_point_t;

// 路径管理器
typedef struct {
    path_point_t points[PATH_MAX_POINT_NUM];
    uint32 current_point_idx; // 原来是uint8，超过255点会回绕
    float last_record_distance;
    uint8 is_recording;     // 1=正在录制，0=停止
} path_manager_t;

// 接口函数
void path_recorder_start(void);
void path_recorder_stop(void);
uint8 path_recorder_init(void);
void path_recorder_update(void);          // 主循环调用，检测按键 + 自动记录
uint8 path_recorder_force_final_ref_point(void);
void path_recorder_read_all_points(path_manager_t *out_manager);
uint8 path_recorder_clear_all_points(void);
uint32 path_recorder_get_record_count(void);
uint8 path_recorder_get_point_by_idx(uint32 idx, path_point_t *point);
uint8 path_recorder_is_working(void);
#endif // PATHRECORDER_H
