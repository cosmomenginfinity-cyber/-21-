#ifndef _ENCODER_H_
#define _ENCODER_H_
#include "global_vars.h"
#include "zf_common_headfile.h"

// 转向绝对编码器角度与等效前轮角的低速粗标定。
// 32°是修改限位结构后的真实前轮机械工作上限；执行前仍按0.59
// 换算成转向传感器目标角，约为54.24°。
#define STEER_SENSOR_TO_WHEEL_RATIO   0.59f
#define STEER_WHEEL_MAX_ANGLE_DEG     32.0f
#define STEER_SENSOR_MAX_ANGLE_DEG    \
    (STEER_WHEEL_MAX_ANGLE_DEG / STEER_SENSOR_TO_WHEEL_RATIO)

extern float get_current_steer_sensor_angle(void);
extern float get_current_real_wheel_angle(void);
extern float steer_sensor_to_wheel_angle(float sensor_angle);
extern float steer_wheel_to_sensor_angle(float wheel_angle);
typedef struct
{
    int32 raw_value;          // 原始计数
    float filtered_value;     // 滤波后浮点值
    int32 filtered_int;       // 滤波后整数值
    int32 position;           // 累计位置
    float velocity;           // 速度
    int32 last_position;      // 上一次位置
    uint32 last_calc_time;    // 上一次计算时间
} single_encoder_data_t;

typedef struct
{
    single_encoder_data_t left;   // 左编码器数据
    single_encoder_data_t right;  // 右编码器数据
    single_encoder_data_t avg;    // 平均数据
} encoder_data_t;

//extern int32 left_encoder_count;       // 左编码器累计计数
//extern int32 right_encoder_count;      // 右编码器累计计数

// ================================================= 对外函数声明 =================================================
uint8 encoder_init(void);
void encoder_update_all_data(void);
encoder_data_t encoder_get_full_data(void);
single_encoder_data_t encoder_get_avg_data(void);
void encoder_reset_all_data(void);
float encoder_get_total_mileage(void);
void encoder_reset_total_mileage(void);
#endif
