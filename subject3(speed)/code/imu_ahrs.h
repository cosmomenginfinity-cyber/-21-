#ifndef IMU_AHRS_H
#define IMU_AHRS_H
// 引入逐飞库基础类型（必须，对齐TC387平台的uint8/int16等类型定义）
#include "zf_common_typedef.h"
// SCHA634-D03 DUE interface (Rate-Y and Rate-Z)
#include "zf_device_scha634.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#define RAD_TO_DEG (180.0f/M_PI)  // 弧度转角度
#define DEG_TO_RAD (M_PI/180.0f)  // 角度转弧度

// 数据结构声明（仅一次）
typedef struct {
    float roll_angle;  // 横滚角（°）
    float pitch_angle; // 俯仰角（°）
    float yaw_angle;   // 航向角（°）
    float acc_x_g;     // X轴加速度（g）
    float acc_y_g;     // Y轴加速度（g）
    float acc_z_g;     // Z轴加速度（g）
    float gyro_x_dps;  // X轴陀螺仪（°/s）
    float gyro_y_dps;  // Y轴陀螺仪（°/s）
    float gyro_z_dps;  // Z轴陀螺仪（°/s）
} ahrs_data_t;

typedef struct {
    float x; // X坐标（米）
    float y; // Y坐标（米）
} imu_position_t;

// 对外接口声明（C99标准类型）
uint8_t imu_ahrs_init(void);
void imu_ahrs_update(void);
ahrs_data_t imu_ahrs_get_data(void);
float imu_ahrs_get_yaw(void);
imu_position_t imu_ahrs_get_position(void);
float imu_ahrs_get_pos_x(void);
float imu_ahrs_get_pos_y(void);
void imu_ahrs_reset_position(void);

#endif // __IMU_AHRS_H__
