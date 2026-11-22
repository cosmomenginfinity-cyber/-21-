#include "zf_device_scha634.h"

#include "zf_driver_delay.h"

/* Pre-calculated SCHA63x 32-bit SPI frames, including CRC. */
#define SCHA634_FRAME_READ_RATE_X          (0x040000F7U)
#define SCHA634_FRAME_READ_RATE_Y          (0x0C0000FBU)
#define SCHA634_FRAME_READ_RATE_Z          (0x040000F7U)
#define SCHA634_FRAME_READ_ACCEL_X         (0x100000E9U)
#define SCHA634_FRAME_READ_ACCEL_Y         (0x140000EFU)
#define SCHA634_FRAME_READ_ACCEL_Z         (0x180000E5U)
#define SCHA634_FRAME_READ_SUMMARY_STATUS  (0x380000D5U)
#define SCHA634_FRAME_READ_RATE_STATUS_1   (0x40000091U)
#define SCHA634_FRAME_READ_RATE_STATUS_2   (0x44000097U)
#define SCHA634_FRAME_READ_ACCEL_STATUS_1  (0x4800009DU)
#define SCHA634_FRAME_READ_COMMON_STATUS_1 (0x50000089U)
#define SCHA634_FRAME_READ_COMMON_STATUS_2 (0x5400008FU)
#define SCHA634_FRAME_READ_TRACEABILITY_2  (0x700000B9U)

#define SCHA634_FRAME_WRITE_RESET          (0xE000017CU)
#define SCHA634_FRAME_WRITE_BANK_0         (0xFC000073U)
#define SCHA634_FRAME_WRITE_NORMAL_MODE    (0xE4000067U)
#define SCHA634_FRAME_WRITE_RATE_FILTER    (0xD812129EU) /* 46 Hz */
#define SCHA634_FRAME_WRITE_ACCEL_FILTER   (0xE8022248U) /* 46 Hz */
#define SCHA634_FRAME_WRITE_EOI            (0xE000025BU)

#define SCHA634_RESPONSE_RS(frame)         (((frame) >> 24U) & 0x03U)
#define SCHA634_RESPONSE_ADDRESS(frame)    (((frame) >> 26U) & 0x3FU)
#define SCHA634_RESPONSE_DATA_U16(frame)   ((uint16)(((frame) >> 8U) & 0xFFFFU))
#define SCHA634_RESPONSE_DATA_S16(frame)   ((int16)SCHA634_RESPONSE_DATA_U16(frame))

#define SCHA634_RS_NORMAL_OPERATION        (0x01U)
#define SCHA634_RS_CHANNEL_ERROR           (0x03U)

#define SCHA634_RATE_X_ADDRESS             (0x01U)
#define SCHA634_RATE_Y_ADDRESS             (0x03U)
#define SCHA634_RATE_Z_ADDRESS             (0x01U)
#define SCHA634_ACCEL_X_ADDRESS            (0x04U)
#define SCHA634_ACCEL_Y_ADDRESS            (0x05U)
#define SCHA634_ACCEL_Z_ADDRESS            (0x06U)
#define SCHA634_SUMMARY_ADDRESS            (0x0EU)
#define SCHA634_TRACEABILITY_2_ADDRESS     (0x1CU)

#define SCHA634_ASIC_UNKNOWN               (0U)
#define SCHA634_ASIC_UNO                   (1U)
#define SCHA634_ASIC_DUE                   (2U)
#define SCHA634_ASIC_TYPE_SHIFT            (6U)
#define SCHA634_ASIC_TYPE_MASK             (0x03U)

#define SCHA634_INIT_ATTEMPTS              (2U)
/* Runtime reads execute inside the 100 Hz control ISR.  Never retry a whole
 * off-frame transaction there; a failed sample is retried on the next tick. */
#define SCHA634_READ_ATTEMPTS              (1U)

typedef struct
{
    gpio_pin_enum cs_pin;
    uint8 expected_type;
    uint8 actual_type;
    uint8 ready;
    uint32 last_response;
} scha634_device_t;

static scha634_device_t g_due;
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
static scha634_device_t g_uno;
#endif
static scha634_data_t g_last_data;
static scha634_diagnostics_t g_diagnostics;

static void scha634_prepare_device(scha634_device_t *device,
        gpio_pin_enum cs_pin, uint8 expected_type)
{
    device->cs_pin = cs_pin;
    device->expected_type = expected_type;
    device->actual_type = SCHA634_ASIC_UNKNOWN;
    device->ready = 0U;
    device->last_response = 0U;
}

static uint32 scha634_transfer(scha634_device_t *device, uint32 request)
{
    uint8 tx[4];
    uint8 rx[4] = {0U, 0U, 0U, 0U};
    uint32 response;

    /* Use four 8-bit transfers while CS remains low.  The TC387 library's
     * spi_transfer_16bit path loses the high byte of each word in this
     * project, so it must not be used for SCHA634's 32-bit frames. */
    tx[0] = (uint8)(request >> 24U);
    tx[1] = (uint8)(request >> 16U);
    tx[2] = (uint8)(request >> 8U);
    tx[3] = (uint8)request;

    gpio_low(device->cs_pin);
    spi_transfer_8bit(SCHA634_SPI, tx, rx, 4U);
    gpio_high(device->cs_pin);
    system_delay_us(10U);

    response = ((uint32)rx[0] << 24U)
             | ((uint32)rx[1] << 16U)
             | ((uint32)rx[2] << 8U)
             | (uint32)rx[3];
    device->last_response = response;
    return response;
}

static uint32 scha634_read_register_raw(scha634_device_t *device,
        uint32 request)
{
    /* SCHA634 uses an off-frame protocol: transfer 2 returns transfer 1. */
    (void)scha634_transfer(device, request);
    return scha634_transfer(device, request);
}

static uint8 scha634_crc8(uint32 frame)
{
    uint8 crc = 0xFFU;
    uint8 byte_index;
    uint8 bit_index;

    for(byte_index = 0U; byte_index < 3U; byte_index++)
    {
        crc ^= (uint8)(frame >> (24U - 8U * byte_index));
        for(bit_index = 0U; bit_index < 8U; bit_index++)
        {
            if((crc & 0x80U) != 0U)
            {
                crc = (uint8)((crc << 1U) ^ 0x1DU);
            }
            else
            {
                crc = (uint8)(crc << 1U);
            }
        }
    }

    return (uint8)(crc ^ 0xFFU);
}

static uint8 scha634_crc_is_valid(uint32 response)
{
    if((response == 0x00000000U) || (response == 0xFFFFFFFFU))
    {
        return 0U;
    }
    return (scha634_crc8(response) == (uint8)response) ? 1U : 0U;
}

static uint8 scha634_is_normal_for_address(uint32 response, uint32 address)
{
    return ((scha634_crc_is_valid(response) != 0U)
            && (SCHA634_RESPONSE_RS(response) == SCHA634_RS_NORMAL_OPERATION)
            && (SCHA634_RESPONSE_ADDRESS(response) == address)) ? 1U : 0U;
}

static void scha634_read_due_status(scha634_device_t *device)
{
    (void)scha634_transfer(device, SCHA634_FRAME_READ_SUMMARY_STATUS);
    g_diagnostics.due_summary_status = scha634_transfer(device,
            SCHA634_FRAME_READ_RATE_STATUS_1);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_RATE_STATUS_2);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_COMMON_STATUS_1);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_COMMON_STATUS_2);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_COMMON_STATUS_2);
    /* The following 10 ms control tick supplies the required settling time. */
}

#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
static void scha634_read_uno_status(scha634_device_t *device)
{
    (void)scha634_transfer(device, SCHA634_FRAME_READ_SUMMARY_STATUS);
    g_diagnostics.uno_summary_status = scha634_transfer(device,
            SCHA634_FRAME_READ_RATE_STATUS_1);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_ACCEL_STATUS_1);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_COMMON_STATUS_1);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_COMMON_STATUS_2);
    (void)scha634_transfer(device, SCHA634_FRAME_READ_COMMON_STATUS_2);
    /* The following 10 ms control tick supplies the required settling time. */
}
#endif

static void scha634_recover_status(scha634_device_t *device)
{
    if(device->expected_type == SCHA634_ASIC_DUE)
    {
        scha634_read_due_status(device);
    }
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    else
    {
        scha634_read_uno_status(device);
    }
#endif
}

static uint8 scha634_identify_device(scha634_device_t *device)
{
    uint32 response = scha634_read_register_raw(device,
            SCHA634_FRAME_READ_TRACEABILITY_2);
    uint16 traceability;

    if((scha634_crc_is_valid(response) == 0U)
            || (SCHA634_RESPONSE_ADDRESS(response)
                != SCHA634_TRACEABILITY_2_ADDRESS))
    {
        g_diagnostics.last_error = SCHA634_ERROR_COMMUNICATION;
        return 0U;
    }

    traceability = SCHA634_RESPONSE_DATA_U16(response);
    device->actual_type = (uint8)(((uint32)traceability
            >> SCHA634_ASIC_TYPE_SHIFT) & SCHA634_ASIC_TYPE_MASK);

    if(device->actual_type != device->expected_type)
    {
        g_diagnostics.last_error = SCHA634_ERROR_ASIC_ID;
        return 0U;
    }
    return 1U;
}

static uint8 scha634_check_startup_status(scha634_device_t *device)
{
    uint32 response;

    /* First read clears startup latches; validate a later summary frame. */
    (void)scha634_read_register_raw(device,
            SCHA634_FRAME_READ_SUMMARY_STATUS);
    system_delay_ms(3U);
    response = scha634_read_register_raw(device,
            SCHA634_FRAME_READ_SUMMARY_STATUS);

    if(device->expected_type == SCHA634_ASIC_DUE)
    {
        g_diagnostics.due_summary_status = response;
    }
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    else
    {
        g_diagnostics.uno_summary_status = response;
    }
#endif
    return scha634_is_normal_for_address(response, SCHA634_SUMMARY_ADDRESS);
}

static uint8 scha634_start_device(scha634_device_t *device)
{
    (void)scha634_transfer(device, SCHA634_FRAME_WRITE_BANK_0);
    (void)scha634_transfer(device, SCHA634_FRAME_WRITE_RESET);
    system_delay_ms(25U);

    if(scha634_identify_device(device) == 0U)
    {
        return 0U;
    }

    (void)scha634_transfer(device, SCHA634_FRAME_WRITE_NORMAL_MODE);
    /* DUE needs a second Mode write to start the Rate-Y signal path. */
    if(device->expected_type == SCHA634_ASIC_DUE)
    {
        (void)scha634_transfer(device, SCHA634_FRAME_WRITE_NORMAL_MODE);
    }

    system_delay_ms(1U);
    (void)scha634_transfer(device, SCHA634_FRAME_WRITE_RATE_FILTER);
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    if(device->expected_type == SCHA634_ASIC_UNO)
    {
        (void)scha634_transfer(device, SCHA634_FRAME_WRITE_ACCEL_FILTER);
    }
#endif

    /* The 46 Hz rate startup time can be as long as 500 ms. */
    system_delay_ms(550U);
    (void)scha634_transfer(device, SCHA634_FRAME_WRITE_EOI);

    if(scha634_check_startup_status(device) == 0U)
    {
        g_diagnostics.last_error = SCHA634_ERROR_STARTUP_STATUS;
        return 0U;
    }
    return 1U;
}

static uint8 scha634_start_with_retry(scha634_device_t *device,
        scha634_error_t final_error)
{
    uint8 attempt;

    for(attempt = 0U; attempt < SCHA634_INIT_ATTEMPTS; attempt++)
    {
        if(scha634_start_device(device) != 0U)
        {
            device->ready = 1U;
            return 1U;
        }
    }

    if((g_diagnostics.last_error == SCHA634_ERROR_NONE)
            || (g_diagnostics.last_error == SCHA634_ERROR_STARTUP_STATUS))
    {
        g_diagnostics.last_error = final_error;
    }
    return 0U;
}

static uint8 scha634_read_value(scha634_device_t *device, uint32 request,
        uint32 address, int16 *value)
{
    uint8 attempt;
    uint8 status_recovered = 0U;

    if((device->ready == 0U) || (value == 0))
    {
        return 0U;
    }

    for(attempt = 0U; attempt < SCHA634_READ_ATTEMPTS; attempt++)
    {
        uint32 response = scha634_read_register_raw(device, request);
        uint32 rs;

        if(scha634_is_normal_for_address(response, address) != 0U)
        {
            *value = SCHA634_RESPONSE_DATA_S16(response);
            return 1U;
        }

        if(scha634_crc_is_valid(response) == 0U)
        {
            g_diagnostics.crc_error_count++;
            continue;
        }

        rs = SCHA634_RESPONSE_RS(response);
        if(rs != SCHA634_RS_NORMAL_OPERATION)
        {
            g_diagnostics.return_status_error_count++;
            if((rs == SCHA634_RS_CHANNEL_ERROR)
                    && (status_recovered == 0U))
            {
                scha634_recover_status(device);
                g_diagnostics.status_recovery_count++;
                status_recovered = 1U;
            }
            continue;
        }

        g_diagnostics.address_error_count++;
    }

    return 0U;
}

uint8 scha634_init(void)
{
    g_last_data = (scha634_data_t){0};
    g_diagnostics = (scha634_diagnostics_t){0};
    scha634_prepare_device(&g_due, SCHA634_DUE_CS_PIN, SCHA634_ASIC_DUE);
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    scha634_prepare_device(&g_uno, SCHA634_UNO_CS_PIN, SCHA634_ASIC_UNO);
#endif

    spi_init(SCHA634_SPI, SPI_MODE0, SCHA634_SPI_SPEED,
            SCHA634_SPC_PIN, SCHA634_SDI_PIN, SCHA634_SDO_PIN, SPI_CS_NULL);
    gpio_init(SCHA634_DUE_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    gpio_init(SCHA634_UNO_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);
#endif
    system_delay_ms(30U);

    if(scha634_start_with_retry(&g_due, SCHA634_ERROR_DUE_INIT) == 0U)
    {
        return (uint8)g_diagnostics.last_error;
    }
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    if(scha634_start_with_retry(&g_uno, SCHA634_ERROR_UNO_INIT) == 0U)
    {
        return (uint8)g_diagnostics.last_error;
    }
#endif

    g_diagnostics.due_asic_type = g_due.actual_type;
    g_diagnostics.due_last_response = g_due.last_response;
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    g_diagnostics.uno_asic_type = g_uno.actual_type;
    g_diagnostics.uno_last_response = g_uno.last_response;
#endif
    g_diagnostics.initialized = 1U;
    g_diagnostics.last_error = SCHA634_ERROR_NONE;
    return 0U;
}

uint8 scha634_read(scha634_data_t *data)
{
    int16 gyro_y;
    int16 gyro_z;
    scha634_data_t next = {0};

#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    int16 gyro_x;
    int16 acc_x;
    int16 acc_y;
    int16 acc_z;
#endif

    if((data == 0) || (g_diagnostics.initialized == 0U))
    {
        g_diagnostics.last_error = SCHA634_ERROR_NOT_INITIALIZED;
        return (uint8)SCHA634_ERROR_NOT_INITIALIZED;
    }

    if((scha634_read_value(&g_due, SCHA634_FRAME_READ_RATE_Y,
                    SCHA634_RATE_Y_ADDRESS, &gyro_y) == 0U)
            || (scha634_read_value(&g_due, SCHA634_FRAME_READ_RATE_Z,
                    SCHA634_RATE_Z_ADDRESS, &gyro_z) == 0U)
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
            || (scha634_read_value(&g_uno, SCHA634_FRAME_READ_RATE_X,
                    SCHA634_RATE_X_ADDRESS, &gyro_x) == 0U)
            || (scha634_read_value(&g_uno, SCHA634_FRAME_READ_ACCEL_X,
                    SCHA634_ACCEL_X_ADDRESS, &acc_x) == 0U)
            || (scha634_read_value(&g_uno, SCHA634_FRAME_READ_ACCEL_Y,
                    SCHA634_ACCEL_Y_ADDRESS, &acc_y) == 0U)
            || (scha634_read_value(&g_uno, SCHA634_FRAME_READ_ACCEL_Z,
                    SCHA634_ACCEL_Z_ADDRESS, &acc_z) == 0U)
#endif
            )
    {
        g_diagnostics.read_error_count++;
        g_diagnostics.last_error = SCHA634_ERROR_DATA_FRAME;
        g_last_data.valid = 0U;
        *data = g_last_data;
        return (uint8)SCHA634_ERROR_DATA_FRAME;
    }

    next.gyro_y_lsb = gyro_y;
    next.gyro_z_lsb = gyro_z;
    next.gyro_y_dps = SCHA634_GYRO_Y_SIGN
            * ((float)gyro_y / SCHA634_GYRO_XY_SENSITIVITY);
    next.gyro_z_dps = SCHA634_GYRO_Z_SIGN
            * ((float)gyro_z / SCHA634_GYRO_Z_SENSITIVITY);

#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    next.gyro_x_lsb = gyro_x;
    next.acc_x_lsb = acc_x;
    next.acc_y_lsb = acc_y;
    next.acc_z_lsb = acc_z;
    next.gyro_x_dps = SCHA634_GYRO_X_SIGN
            * ((float)gyro_x / SCHA634_GYRO_XY_SENSITIVITY);
    next.acc_x_g = SCHA634_ACC_X_SIGN
            * ((float)acc_x / SCHA634_ACC_SENSITIVITY);
    next.acc_y_g = SCHA634_ACC_Y_SIGN
            * ((float)acc_y / SCHA634_ACC_SENSITIVITY);
    next.acc_z_g = SCHA634_ACC_Z_SIGN
            * ((float)acc_z / SCHA634_ACC_SENSITIVITY);
    next.accelerometer_valid = 1U;
#endif

    next.valid = 1U;
    g_last_data = next;
    g_diagnostics.sample_count++;
    g_diagnostics.due_last_response = g_due.last_response;
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    g_diagnostics.uno_last_response = g_uno.last_response;
#endif
    g_diagnostics.last_error = SCHA634_ERROR_NONE;
    *data = next;
    return 0U;
}

scha634_diagnostics_t scha634_get_diagnostics(void)
{
    return g_diagnostics;
}

uint8 scha634_has_accelerometer(void)
{
#if SCHA634_INTERFACE_MODE == SCHA634_INTERFACE_FULL_6DOF
    return 1U;
#else
    return 0U;
#endif
}
