// 引入依赖头文件（逐飞库+自定义头文件）
#include "imu_ahrs.h"
#include "zf_device_imu660ra.h"
#include "zf_driver_delay.h"
#include "zf_driver_timer.h"
#include <math.h>
#include "global_vars.h"

// ==================== 1. 硬件参数修正 ====================
#define DEBUG_UART              (uint8_t)UART_0
#define GYRO_SCALE_FACTOR       16.384f
#define ACC_SCALE_FACTOR        2048.0f


// ==================== 2. 算法参数 ====================
#define GYRO_CALIB_SAMPLES      2000
#define MAHONY_KP               0.8f
#define MAHONY_KI               0.01f
#define ZERO_THRESHOLD          1.50f
#define GYRO_BIAS_ALPHA         0.995f
#define GYRO_BIAS_BETA          (1.0f - GYRO_BIAS_ALPHA)
#define IMU_UPDATE_DT  0.01f

// ==================== 3. 串口打印 ====================

// ==================== 4. 内部全局变量 ====================
static ahrs_data_t g_ahrs = {0};
static float gyro_z_bias = 0.0f;
static float gyro_x_bias = 0.0f;
static float gyro_y_bias = 0.0f;
static uint8_t is_calibrated = 0;
static uint32_t last_update_time = 0;
static float integral_yaw = 0.0f;
static float g_pos_x = 0.0f; // 坐标 X
static float g_pos_y = 0.0f; // 坐标 Y

// Mahony四元数
typedef struct { float q0, q1, q2, q3; } mahony_q_t;
static mahony_q_t g_q = {1.0f, 0.0f, 0.0f, 0.0f};
static float ex_int = 0.0f, ey_int = 0.0f, ez_int = 0.0f;

// ==================== 5. 辅助函数 ====================
static uint32_t get_system_ms(void) {
    return g_system_time_ms; // 完美解决 MODULE_STM0 报错
}

static uint8_t imu_is_static(float gz) {
    return (fabs(gz) < ZERO_THRESHOLD) ? 1 : 0;
}

// ==================== 6. Mahony更新 (Roll/Pitch) ====================
static void mahony_update(float ax, float ay, float az, float gx, float gy, float gz, float dt) {
    float norm, vx, vy, vz, ex, ey, ez;
    norm = sqrtf(ax*ax + ay*ay + az*az);
    if (norm < 0.1f) return;
    ax /= norm; ay /= norm; az /= norm;

    vx = 2.0f * (g_q.q1 * g_q.q3 - g_q.q0 * g_q.q2);
    vy = 2.0f * (g_q.q0 * g_q.q1 + g_q.q2 * g_q.q3);
    vz = g_q.q0 * g_q.q0 - g_q.q1 * g_q.q1 - g_q.q2 * g_q.q2 + g_q.q3 * g_q.q3;

    ex = (ay * vz - az * vy);
    ey = (az * vx - ax * vz);
    ez = (ax * vy - ay * vx);

    ex_int += ex * MAHONY_KI * dt;
    ey_int += ey * MAHONY_KI * dt;
    ez_int += ez * MAHONY_KI * dt;

    gx += MAHONY_KP * ex + ex_int;
    gy += MAHONY_KP * ey + ey_int;
    gz += MAHONY_KP * ez + ez_int;

    float delta_q0 = 0.5f * (-g_q.q1 * gx - g_q.q2 * gy - g_q.q3 * gz);
    float delta_q1 = 0.5f * ( g_q.q0 * gx + g_q.q2 * gz - g_q.q3 * gy);
    float delta_q2 = 0.5f * ( g_q.q0 * gy - g_q.q1 * gz + g_q.q3 * gx);
    float delta_q3 = 0.5f * ( g_q.q0 * gz + g_q.q1 * gy - g_q.q2 * gx);

    g_q.q0 += delta_q0 * dt; g_q.q1 += delta_q1 * dt;
    g_q.q2 += delta_q2 * dt; g_q.q3 += delta_q3 * dt;

    norm = sqrtf(g_q.q0*g_q.q0 + g_q.q1*g_q.q1 + g_q.q2*g_q.q2 + g_q.q3*g_q.q3);
    g_q.q0 /= norm; g_q.q1 /= norm; g_q.q2 /= norm; g_q.q3 /= norm;
}

// ==================== 7. 对外接口实现 ====================

uint8_t imu_ahrs_init(void) {
    if (imu660ra_init() != 0) return 0;
    float sx = 0, sy = 0, sz = 0;
    for(int i=0; i<GYRO_CALIB_SAMPLES; i++) {
        imu660ra_get_gyro();
        sx += (float)imu660ra_gyro_x / GYRO_SCALE_FACTOR;
        sy += (float)imu660ra_gyro_y / GYRO_SCALE_FACTOR;
        sz += (float)imu660ra_gyro_z / GYRO_SCALE_FACTOR;
        system_delay_ms(1);
    }
    gyro_x_bias = sx / GYRO_CALIB_SAMPLES;
    gyro_y_bias = sy / GYRO_CALIB_SAMPLES;
    gyro_z_bias = sz / GYRO_CALIB_SAMPLES;
    last_update_time = get_system_ms();
    is_calibrated = 1;
    g_pos_x = 0; g_pos_y = 0; integral_yaw = 0;
    return 1;
}

void imu_ahrs_update(void) {
    if (!is_calibrated) return;

    imu660ra_get_acc();
    imu660ra_get_gyro();

    float ax = (float)imu660ra_acc_x / ACC_SCALE_FACTOR;
    float ay = (float)imu660ra_acc_y / ACC_SCALE_FACTOR;
    float az = (float)imu660ra_acc_z / ACC_SCALE_FACTOR;
    az -=1.000f;

    float gx_dps = ((float)imu660ra_gyro_x / GYRO_SCALE_FACTOR) - gyro_x_bias;
    float gy_dps = ((float)imu660ra_gyro_y / GYRO_SCALE_FACTOR) - gyro_y_bias;
    float gz_dps = ((float)imu660ra_gyro_z / GYRO_SCALE_FACTOR) - gyro_z_bias;

    float dt = IMU_UPDATE_DT;

    // ==================== 【致命 Bug 修复】 ====================
    // 只有当车模处于安全停止模式，且编码器物理速度确实为 0 时，才允许动态校准零偏！
    // 这样可以彻底避免运动过程中的角速度被误当成零偏吸收。
    if (current_car_mode == MODE_STOP &&
            fabsf(encoder_speed_left) < 0.01f &&
            fabsf(encoder_speed_right) < 0.01f)
        {
            if (imu_is_static(gz_dps))
            {
                float raw_gz = (float)imu660ra_gyro_z / GYRO_SCALE_FACTOR;
                gyro_z_bias = GYRO_BIAS_ALPHA * gyro_z_bias + GYRO_BIAS_BETA * raw_gz;
                gz_dps = 0;
            }
        }

    // 1. 更新 Yaw 角度
    integral_yaw += gz_dps * dt;
    if (integral_yaw > 180.0f)  integral_yaw -= 360.0f;
    if (integral_yaw < -180.0f) integral_yaw += 360.0f;
    g_ahrs.yaw_angle = integral_yaw;

    // 2. 更新 Roll/Pitch (Mahony)
    mahony_update(ax, ay, az, gx_dps*DEG_TO_RAD, gy_dps*DEG_TO_RAD, gz_dps*DEG_TO_RAD, dt);
    g_ahrs.roll_angle = atan2f(2.0f*(g_q.q0*g_q.q1 + g_q.q2*g_q.q3), 1.0f - 2.0f*(g_q.q1*g_q.q1 + g_q.q2*g_q.q2)) * RAD_TO_DEG;
    g_ahrs.pitch_angle = asinf(2.0f*(g_q.q0*g_q.q2 - g_q.q3*g_q.q1)) * RAD_TO_DEG;

    // ==================== 核心修改：惯导坐标计算 ====================
    // 将角度转为弧度
    float yaw_rad = g_ahrs.yaw_angle * DEG_TO_RAD;
    // 修复：使用物理无损原始速度进行投影累加，彻底消除 2D 坐标系在提速时的延迟漂移
    g_pos_x += encoder_speed_raw_avg * cosf(yaw_rad) * dt;
    g_pos_y += encoder_speed_raw_avg * sinf(yaw_rad) * dt;
    // =============================================================

    g_ahrs.acc_x_g = ax; g_ahrs.acc_y_g = ay; g_ahrs.acc_z_g = az;
    g_ahrs.gyro_x_dps = gx_dps; g_ahrs.gyro_y_dps = gy_dps; g_ahrs.gyro_z_dps = gz_dps;
}

ahrs_data_t imu_ahrs_get_data(void) { return g_ahrs; }
float imu_ahrs_get_yaw(void) { return g_ahrs.yaw_angle; }
imu_position_t imu_ahrs_get_position(void) {
    imu_position_t pos = {g_pos_x, g_pos_y};
    return pos;
}
float imu_ahrs_get_pos_x(void) { return g_pos_x; }
float imu_ahrs_get_pos_y(void) { return g_pos_y; }
void imu_ahrs_reset_position(void) { g_pos_x = 0; g_pos_y = 0; }
