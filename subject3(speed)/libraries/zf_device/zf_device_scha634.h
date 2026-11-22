#ifndef _zf_device_scha634_h_
#define _zf_device_scha634_h_

#include "zf_common_typedef.h"
#include "zf_driver_gpio.h"
#include "zf_driver_spi.h"

/*
 * SCHA634 has two independent ASIC SPI interfaces:
 *   DUE: Y/Z gyroscopes
 *   UNO: X gyroscope and X/Y/Z accelerometers
 *
 * This vehicle can connect only the DUE ASIC.  DUE_CSB reuses the old
 * IMU660RA chip-select wire on P20.13.  DUE provides Rate-Y and Rate-Z; the
 * UNO-only Rate-X and acceleration channels are deliberately not required.
 */
#define SCHA634_INTERFACE_DUE_ONLY        (0U)
#define SCHA634_INTERFACE_FULL_6DOF       (1U)

#ifndef SCHA634_INTERFACE_MODE
#define SCHA634_INTERFACE_MODE            SCHA634_INTERFACE_DUE_ONLY
#endif

/* 1 MHz is sufficient for the 100 Hz control loop and is tolerant of jumper
 * wiring. */
#define SCHA634_SPI_SPEED                 (1U * 1000U * 1000U)
#define SCHA634_SPI                       (SPI_0)
#define SCHA634_SPC_PIN                   (SPI0_SCLK_P20_11)
#define SCHA634_SDI_PIN                   (SPI0_MOSI_P20_14)
#define SCHA634_SDO_PIN                   (SPI0_MISO_P20_12)
#define SCHA634_DUE_CS_PIN                (P20_13)

#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
#define SCHA634_UNO_CS_PIN                (P20_8)
#endif

/* Select the exact suffix printed on the component.  The supplied reference
 * project used 80 LSB/(deg/s) on every axis, which corresponds to D03. */
#define SCHA634_VARIANT_D01                (1U)
#define SCHA634_VARIANT_D02                (2U)
#define SCHA634_VARIANT_D03                (3U)

#ifndef SCHA634_VARIANT
#define SCHA634_VARIANT                    SCHA634_VARIANT_D03
#endif

#if SCHA634_VARIANT == SCHA634_VARIANT_D01
#define SCHA634_GYRO_XY_SENSITIVITY        (160.0f)
#define SCHA634_GYRO_Z_SENSITIVITY         (160.0f)
#elif SCHA634_VARIANT == SCHA634_VARIANT_D02
#define SCHA634_GYRO_XY_SENSITIVITY        (80.0f)
#define SCHA634_GYRO_Z_SENSITIVITY         (160.0f)
#elif SCHA634_VARIANT == SCHA634_VARIANT_D03
#define SCHA634_GYRO_XY_SENSITIVITY        (80.0f)
#define SCHA634_GYRO_Z_SENSITIVITY         (80.0f)
#else
#error "Unsupported SCHA634_VARIANT"
#endif

#define SCHA634_ACC_SENSITIVITY            (4905.0f)

/* Mounting direction hooks.  Keep +1 when the sensor axes agree with the old
 * IMU axes; set an individual value to -1 when that body axis is reversed. */
#ifndef SCHA634_GYRO_X_SIGN
#define SCHA634_GYRO_X_SIGN                (1.0f)
#endif
#ifndef SCHA634_GYRO_Y_SIGN
#define SCHA634_GYRO_Y_SIGN                (1.0f)
#endif
#ifndef SCHA634_GYRO_Z_SIGN
#define SCHA634_GYRO_Z_SIGN                (1.0f)
#endif
#ifndef SCHA634_ACC_X_SIGN
#define SCHA634_ACC_X_SIGN                 (1.0f)
#endif
#ifndef SCHA634_ACC_Y_SIGN
#define SCHA634_ACC_Y_SIGN                 (1.0f)
#endif
#ifndef SCHA634_ACC_Z_SIGN
#define SCHA634_ACC_Z_SIGN                 (1.0f)
#endif

typedef enum
{
    SCHA634_ERROR_NONE = 0,
    SCHA634_ERROR_NOT_INITIALIZED,
    SCHA634_ERROR_DUE_INIT,
    SCHA634_ERROR_UNO_INIT,
    SCHA634_ERROR_COMMUNICATION,
    SCHA634_ERROR_ASIC_ID,
    SCHA634_ERROR_STARTUP_STATUS,
    SCHA634_ERROR_DATA_FRAME
} scha634_error_t;

typedef struct
{
    int16 acc_x_lsb;
    int16 acc_y_lsb;
    int16 acc_z_lsb;
    int16 gyro_x_lsb;
    int16 gyro_y_lsb;
    int16 gyro_z_lsb;
    int16 temp_due_lsb;
    int16 temp_uno_lsb;

    float acc_x_g;
    float acc_y_g;
    float acc_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float temp_due_c;
    float temp_uno_c;

    uint8 accelerometer_valid;
    uint8 valid;
} scha634_data_t;

typedef struct
{
    uint32 sample_count;
    uint32 read_error_count;
    uint32 crc_error_count;
    uint32 return_status_error_count;
    uint32 address_error_count;
    uint32 status_recovery_count;
    uint32 due_summary_status;
    uint32 uno_summary_status;
    uint32 due_last_response;
    uint32 uno_last_response;
    uint8 due_asic_type;
    uint8 uno_asic_type;
    uint8 initialized;
    scha634_error_t last_error;
} scha634_diagnostics_t;

/* Return value follows the Seekfree device convention: 0 is success. */
uint8 scha634_init(void);
uint8 scha634_read(scha634_data_t *data);
scha634_diagnostics_t scha634_get_diagnostics(void);
uint8 scha634_has_accelerometer(void);

#endif
