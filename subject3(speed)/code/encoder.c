#include "zf_common_headfile.h"
#include "initialize.h"
#include "encoder.h"
#include "imu_datalogger.h"
#include <string.h>
#include "global_vars.h"
// ================================================= 全局变量定义 =================================================
static encoder_data_t g_encoder;
static const float k_v = REAR_WHEEL_PERIMETER / ENCODER_PPR;
static float g_total_mileage = 0.0f;

float steer_sensor_to_wheel_angle(float sensor_angle)
{
    return sensor_angle * STEER_SENSOR_TO_WHEEL_RATIO;
}

float steer_wheel_to_sensor_angle(float wheel_angle)
{
    return wheel_angle / STEER_SENSOR_TO_WHEEL_RATIO;
}

float get_current_steer_sensor_angle(void)
{
    float raw_sensor = absolute_encoder_get_location();
    float sensor_delta = raw_sensor - DIR_MID;

    // 绝对编码器跨越0/4096边界时仍取最短角度差。
    if (sensor_delta > 2048.0f)
    {
        sensor_delta -= 4096.0f;
    }
    else if (sensor_delta < -2048.0f)
    {
        sensor_delta += 4096.0f;
    }

    return sensor_delta * (360.0f / 4096.0f);
}

float get_current_real_wheel_angle(void)
{
    return steer_sensor_to_wheel_angle(
        get_current_steer_sensor_angle());
}


// ================================================= 对外接口实现 =================================================
uint8 encoder_init(void) {
    encoder_dir_init(ENCODER_DIR_LEFT, ENCODER_DIR_PULSE_LEFT, ENCODER_DIR_DIR_LEFT);
    encoder_dir_init(ENCODER_DIR_RIGHT, ENCODER_DIR_PULSE_RIGHT, ENCODER_DIR_DIR_RIGHT);

    encoder_reset_all_data();

    return 1;
}

void encoder_update_all_data(void) {
    // 1. 直接读
    int32 l_raw = encoder_get_count(ENCODER_DIR_LEFT);
    int32 r_raw = encoder_get_count(ENCODER_DIR_RIGHT);
    encoder_clear_count(ENCODER_DIR_LEFT);
    encoder_clear_count(ENCODER_DIR_RIGHT);

    // 2. 左轮计算
    g_encoder.left.position += l_raw;
    float l_speed_raw = (float)l_raw * k_v * 100.0f;
    encoder_speed_left = FILTER_ALPHA * l_speed_raw + ( 1.0f - FILTER_ALPHA )* encoder_speed_left;

    // 3. 右轮计算
    g_encoder.right.position += r_raw;
    float r_speed_raw = -(float)r_raw * k_v * 100.0f;
    encoder_speed_right = FILTER_ALPHA * r_speed_raw + ( 1.0f - FILTER_ALPHA ) * encoder_speed_right;

    // 4. 更新全局位置
    encoder_count_left = g_encoder.left.position;
    encoder_count_right = -g_encoder.right.position;

    // 【修改】：不使用带滤波的速度，直接使用绝对无损的原始脉冲来累加总里程
    float l_dist_raw = fabsf((float)l_raw) * k_v; // 左轮本周期物理位移 (米)
    float r_dist_raw = fabsf((float)r_raw) * k_v; // 右轮本周期物理位移 (米)
    encoder_speed_raw_avg = ((l_speed_raw + r_speed_raw) / 2.0f);
    g_total_mileage += (l_dist_raw + r_dist_raw) / 2.0f;
}
//核心新增：提供给惯导算法的接口
float encoder_get_total_mileage(void) {
    return g_total_mileage;
}
void encoder_reset_total_mileage(void) {
    g_total_mileage = 0.0f;
}

void encoder_reset_all_data(void)
{
    memset((void*)&g_encoder, 0, sizeof(encoder_data_t));

    g_total_mileage = 0.0f;

    // 同时清空硬件计数，确保1024线编码器重置无残留
    encoder_clear_count(ENCODER_DIR_LEFT);
    encoder_clear_count(ENCODER_DIR_RIGHT);

    // 关键：重置全局计数变量
    encoder_count_right = 0;
    encoder_count_left = 0;
    encoder_speed_left = 0.0f;
    encoder_speed_right = 0.0f;
}
