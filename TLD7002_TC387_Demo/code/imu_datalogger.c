#include "zf_common_typedef.h"
#include "zf_common_headfile.h"
#include "zf_driver_uart.h"
#include "zf_driver_delay.h"
#include "flash.h"
#include "encoder.h"
#include "imu_datalogger.h"
#include "imu_ahrs.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#include "global_vars.h"


static imu_encoder_log_t imu_log_buffer[IMU_LOG_BUFFER_SIZE];
static uint32_t buffer_index = 0;
static uint8_t logger_initialized = 0;

static void imu_logger_fill_record(imu_encoder_log_t *log_record) {
    if (log_record == NULL) return;

    ahrs_data_t imu_data = imu_ahrs_get_data();
    log_record->timestamp_ms = PATH_RECORD_EVENT_NONE;
    log_record->yaw_angle = imu_data.yaw_angle;

    //正确赋值位置数据
    log_record->acc_x_g = imu_ahrs_get_pos_x();
    log_record->acc_y_g = imu_ahrs_get_pos_y();

    // 【核心修改】：将“脉冲数”改为“累计里程”
    // 调用 encoder.c 里的接口获取总里程(m)，乘以 1000 转化成毫米(mm)存入
    log_record->main_encoder_cnt = (long long)(encoder_get_total_mileage() * 1000.0f);

    log_record->main_wheel_speed = (encoder_speed_left + encoder_speed_right) / 2.0f;
}

uint8_t imu_logger_init(void) {
    memset(imu_log_buffer, 0, sizeof(imu_log_buffer));
    buffer_index = 0;
    if (flash_log_init() != FLASH_OK) return 0;
    logger_initialized = 1;
    return 1;
}

uint8_t imu_logger_manual_write_event(uint32_t event_tag) {
    if (!logger_initialized || buffer_index >= IMU_LOG_BUFFER_SIZE) return 0;
    imu_logger_fill_record(&imu_log_buffer[buffer_index]);
    imu_log_buffer[buffer_index].timestamp_ms = event_tag;
    buffer_index++;
    return 1;
}

uint8_t imu_logger_manual_write(imu_encoder_log_t *log_record) {
    (void)log_record;
    return imu_logger_manual_write_event(PATH_RECORD_EVENT_NONE);
}

uint32_t imu_logger_get_buffer_count(void) {
    return buffer_index;
}

uint8_t imu_logger_save_buffer_to_flash(void) {
    if (!logger_initialized || buffer_index == 0) return 0;
    uint8_t ret = flash_log_store_batch(imu_log_buffer, buffer_index);
    if (ret == FLASH_OK) {
        buffer_index = 0;
        memset(imu_log_buffer, 0, sizeof(imu_log_buffer));
    }
    return ret;
}

uint8_t imu_logger_export_csv(uint32_t start_index, uint32_t count) {
    uint32_t flash_count = flash_log_get_count();
    if (flash_count == 0) return 0;

    uart_write_string(IMU_LOG_UART_CH, "Index,X,Y,Yaw,Mileage_m,Speed,Event\r\n");

    imu_encoder_log_t row;
    for (uint32_t i = 0; i < flash_count; i++) {
        if (flash_log_read_batch(&row, i, 1)) {
            char buf[128];

            // 【核心修改】：将毫米(mm)除以 1000.0 还原回米(m)进行打印
            // 这样 CSV 里的第五列就是 0.100, 0.200 这种直观的位移数据了
            double mileage_m = (double)row.main_encoder_cnt / 1000.0;

            snprintf(buf, sizeof(buf), "%lu,%.3f,%.3f,%.2f,%.3f,%.3f,0x%08lX\r\n",
                     (unsigned long)i,
                     (double)row.acc_x_g,
                     (double)row.acc_y_g,
                     (double)row.yaw_angle,
                     mileage_m,  // 打印还原后的物理里程
                     (double)row.main_wheel_speed,
                     (unsigned long)row.timestamp_ms);
            uart_write_string(IMU_LOG_UART_CH, buf);
        }
    }
    return 1;
}

uint8_t imu_logger_clear_flash(void) { return flash_log_clear_all(); }
