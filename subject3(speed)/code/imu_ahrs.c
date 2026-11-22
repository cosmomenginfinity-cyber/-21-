#include "imu_ahrs.h"

#include "zf_device_scha634.h"
#include "zf_driver_delay.h"
#include "global_vars.h"

#include <math.h>

/* DUE-only SCHA634-D03 configuration.  DUE supplies Rate-Y and Rate-Z; the
 * vehicle yaw path uses Rate-Z pure integration at the 100 Hz PIT rate. */
#define GYRO_CALIB_SAMPLES      (2000U)
/* Calibration is completed before the 100 Hz PIT is started.  Bound the
 * attempt count so a missing sensor cannot block startup forever. */
#define GYRO_CALIB_MAX_ATTEMPTS (GYRO_CALIB_SAMPLES * 2U)
#define ZERO_THRESHOLD_DPS      (1.50f)
#define GYRO_BIAS_ALPHA         (0.995f)
#define GYRO_BIAS_BETA          (1.0f - GYRO_BIAS_ALPHA)
#define IMU_NOMINAL_DT_S        (0.01f)
#define IMU_MAX_VALID_DT_MS     (30U)

static ahrs_data_t g_ahrs = {0};
static float gyro_y_bias = 0.0f;
static float gyro_z_bias = 0.0f;
static uint8_t imu_initialized = 0U;
static uint32_t last_update_time_ms = 0U;
static float integral_yaw = 0.0f;
static float g_pos_x = 0.0f;
static float g_pos_y = 0.0f;

static uint8_t imu_is_static(float gyro_z_dps)
{
    return (fabsf(gyro_z_dps) < ZERO_THRESHOLD_DPS) ? 1U : 0U;
}

uint8_t imu_ahrs_init(void)
{
    scha634_data_t sample;
    uint32 valid_samples = 0U;
    uint32 attempt_count = 0U;
    float gyro_y_sum = 0.0f;
    float gyro_z_sum = 0.0f;

    imu_initialized = 0U;
    g_ahrs = (ahrs_data_t){0};
    gyro_y_bias = 0.0f;
    gyro_z_bias = 0.0f;
    integral_yaw = 0.0f;
    g_pos_x = 0.0f;
    g_pos_y = 0.0f;

    if(scha634_init() != 0U)
    {
        return 0U;
    }

    while((valid_samples < GYRO_CALIB_SAMPLES)
            && (attempt_count < GYRO_CALIB_MAX_ATTEMPTS))
    {
        attempt_count++;
        if((scha634_read(&sample) == 0U) && (sample.valid != 0U))
        {
            gyro_y_sum += sample.gyro_y_dps;
            gyro_z_sum += sample.gyro_z_dps;
            valid_samples++;
            system_delay_ms(1U);
        }
        else
        {
            system_delay_ms(3U);
        }
    }

    if(valid_samples < GYRO_CALIB_SAMPLES)
    {
        return 0U;
    }

    gyro_y_bias = gyro_y_sum / (float)valid_samples;
    gyro_z_bias = gyro_z_sum / (float)valid_samples;
    imu_initialized = 1U;
    last_update_time_ms = g_system_time_ms;
    return 1U;
}

void imu_ahrs_update(void)
{
    scha634_data_t sample;
    uint32 now_ms;
    uint32 elapsed_ms;
    float dt;
    float gyro_y_dps;
    float gyro_z_dps;
    float yaw_rad;

    if(imu_initialized == 0U)
    {
        return;
    }

    if((scha634_read(&sample) != 0U) || (sample.valid == 0U))
    {
        return;
    }

    now_ms = g_system_time_ms;
    elapsed_ms = now_ms - last_update_time_ms;
    last_update_time_ms = now_ms;
    if((elapsed_ms == 0U) || (elapsed_ms > IMU_MAX_VALID_DT_MS))
    {
        dt = IMU_NOMINAL_DT_S;
    }
    else
    {
        dt = (float)elapsed_ms * 0.001f;
    }

    gyro_y_dps = sample.gyro_y_dps - gyro_y_bias;
    gyro_z_dps = sample.gyro_z_dps - gyro_z_bias;

    /* Bias tracking is allowed only while the vehicle is commanded to stop
     * and both wheel encoders confirm that it is physically stationary. */
    if((current_car_mode == MODE_STOP)
            && (fabsf(encoder_speed_left) < 0.01f)
            && (fabsf(encoder_speed_right) < 0.01f)
            && (imu_is_static(gyro_z_dps) != 0U))
    {
        gyro_z_bias = GYRO_BIAS_ALPHA * gyro_z_bias
                + GYRO_BIAS_BETA * sample.gyro_z_dps;
        gyro_z_dps = 0.0f;
    }

    integral_yaw += gyro_z_dps * dt;
    while(integral_yaw > 180.0f)
    {
        integral_yaw -= 360.0f;
    }
    while(integral_yaw < -180.0f)
    {
        integral_yaw += 360.0f;
    }

    g_ahrs.roll_angle = 0.0f;
    g_ahrs.pitch_angle = 0.0f;
    g_ahrs.yaw_angle = integral_yaw;
    g_ahrs.acc_x_g = 0.0f;
    g_ahrs.acc_y_g = 0.0f;
    g_ahrs.acc_z_g = 0.0f;
    g_ahrs.gyro_x_dps = 0.0f;
    g_ahrs.gyro_y_dps = gyro_y_dps;
    g_ahrs.gyro_z_dps = gyro_z_dps;

    yaw_rad = integral_yaw * DEG_TO_RAD;
    g_pos_x += encoder_speed_raw_avg * cosf(yaw_rad) * dt;
    g_pos_y += encoder_speed_raw_avg * sinf(yaw_rad) * dt;
}

ahrs_data_t imu_ahrs_get_data(void)
{
    return g_ahrs;
}

float imu_ahrs_get_yaw(void)
{
    return g_ahrs.yaw_angle;
}

imu_position_t imu_ahrs_get_position(void)
{
    imu_position_t pos = {g_pos_x, g_pos_y};
    return pos;
}

float imu_ahrs_get_pos_x(void)
{
    return g_pos_x;
}

float imu_ahrs_get_pos_y(void)
{
    return g_pos_y;
}

void imu_ahrs_reset_position(void)
{
    g_pos_x = 0.0f;
    g_pos_y = 0.0f;
}
