#ifndef __IMU_DATALOGGER_H__
#define __IMU_DATALOGGER_H__

// ================================= 基础依赖（补充IMU核心头文件） =================================
#include "zf_common_typedef.h"
#include "zf_common_headfile.h"
#include "zf_driver_uart.h"
#include "imu_ahrs.h"  // 关键：添加IMU核心逻辑头文件，关联解算数据
#include "encoder.h"   // 保留编码器依赖

#define IMU_LOG_BUFFER_SIZE     1000
#define IMU_LOG_UART_CH         UART_0

// timestamp_ms is currently unused by the path logger. Reuse it as a
// persistent path-event tag without changing the Flash record layout.
#define PATH_RECORD_EVENT_NONE      0x00000000U
#define PATH_RECORD_EVENT_SWITCH    0x53574954U  // "SWIT"
#define PATH_RECORD_EVENT_END       0x454E4421U  // "END!"

// 标准结构体，不加任何 packed 属性，确保 TASKING 编译通过
typedef struct _imu_encoder_log_t
{
    uint32_t timestamp_ms;

    // IMU 原始数据
    int16_t imu_gyro_x;
    int16_t imu_gyro_y;
    int16_t imu_gyro_z;
    int16_t imu_acc_x;
    int16_t imu_acc_y;
    int16_t imu_acc_z;

    // IMU 解算数据
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float acc_x_g;                  // 借用来存 X 坐标
    float acc_y_g;                  // 借用来存 Y 坐标
    float acc_z_g;
    float roll_angle;
    float pitch_angle;
    float yaw_angle;

    // 编码器数据
    int64_t main_encoder_cnt;
    float   main_wheel_speed;
    float   linear_velocity;
} imu_encoder_log_t;

typedef struct
{
    uint32_t flash_records;
    uint8_t  initialized;
} imu_log_stats_t;

uint8_t flash_log_init(void);
uint8_t flash_log_store_batch(imu_encoder_log_t *data, uint32_t count);
uint32_t flash_log_read_batch(imu_encoder_log_t *buf, uint32_t start, uint32_t count);
uint32_t flash_log_get_count(void);
uint8_t flash_log_clear_all(void);

uint8_t imu_logger_init(void);
uint8_t imu_logger_manual_write(imu_encoder_log_t *log_record);
uint8_t imu_logger_manual_write_event(uint32_t event_tag);
uint32_t imu_logger_get_buffer_count(void);
uint8_t imu_logger_save_buffer_to_flash(void);
uint8_t imu_logger_export_csv(uint32_t start_index, uint32_t count);
uint8_t imu_logger_clear_flash(void);

#endif
