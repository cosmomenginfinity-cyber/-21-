#ifndef COLOR_VISION_H
#define COLOR_VISION_H

#include "zf_common_headfile.h"

/*
 * Field tuning area. Normal track-side tuning should only require changing
 * the two RGB layout switches and the single green HSV range below.
 *
 * SCC8660 format 0 sends the high byte first, while TC387 stores uint16 in
 * little-endian order. Keep BYTE_SWAP=1 for the stock driver. If the mask hue
 * is obviously wrong, test BYTE_SWAP first. Use SWAP_RED_BLUE only when red
 * and blue are reversed after the byte order is correct.
 *
 * H uses the same range as OpenCV: 0..179. S and V use 0..255. The six HSV
 * values printed by the Python tool can therefore be copied here directly.
 */
#define COLOR_VISION_RGB565_BYTE_SWAP       (1U)
#define COLOR_VISION_SWAP_RED_BLUE          (0U)

/*
 * SCC8660 HSV range for the bright-green A4 marker. Keep the values that were
 * already verified with the real camera, then re-check sun/shade/backlight.
 */
#define COLOR_VISION_GREEN_H_MIN            (36U)//36//24
#define COLOR_VISION_GREEN_H_MAX            (70U)//80//63
#define COLOR_VISION_GREEN_S_MIN            (190U)//50//180
#define COLOR_VISION_GREEN_S_MAX            (255U)
#define COLOR_VISION_GREEN_V_MIN            (55U)//50//105
#define COLOR_VISION_GREEN_V_MAX            (255U)

#define COLOR_VISION_MIN_AREA_PIXELS        (20U)
#define COLOR_VISION_USE_MORPHOLOGY         (1U)

/*
 * Follow geometry.
 *
 * Range is derived from the bounding-box height of a landscape A4 green
 * marker. Its physical vertical dimension is 210 mm.
 *
 * Coordinate convention: vehicle origin is the rear-axle centre, X is
 * forward and Y is left. Camera yaw is positive when pointing to the left.
 * Distances are metres and image coordinates/focal length are pixels.
 */
#define COLOR_FOLLOW_GEOMETRY_CALIBRATED          (1U)

/*
 * 35 px was measured for the landscape A4 marker's 210 mm height at 0.5 m.
 * Re-measure with the marker worn on the runner before competition use.
 */
#define COLOR_FOLLOW_HEIGHT_CAL_DISTANCE_M         (0.50f)
#define COLOR_FOLLOW_HEIGHT_CAL_PIXELS             (35.0f)
#define COLOR_FOLLOW_CAMERA_FX_PX                  (40.91f)
#define COLOR_FOLLOW_CAMERA_CX_PX                  ((COLOR_VISION_FRAME_W - 1U) * 0.5f)

/* TODO_MEASURE: fixed camera/chassis geometry, all from the rear-axle centre. */
#define COLOR_FOLLOW_CAMERA_X_FROM_REAR_AXLE_M     (0.90f)
#define COLOR_FOLLOW_CAMERA_Y_FROM_CENTER_M        (0.00f)
#define COLOR_FOLLOW_CAMERA_YAW_DEG                (0.00f)

/*
 * Runtime tuning. Speed is calculated from camera-to-marker range, not area.
 * PP_OUTPUT_SIGN should normally stay +1: image-right is converted to
 * vehicle-right exactly once in color_vision.c.
 */
#define COLOR_FOLLOW_PP_OUTPUT_SIGN                (1.0f)
#define COLOR_FOLLOW_LOOKAHEAD_BASE_M               (0.70f)
#define COLOR_FOLLOW_LOOKAHEAD_SPEED_GAIN_S         (0.40f)
#define COLOR_FOLLOW_LOOKAHEAD_MIN_M                (1.00f)
#define COLOR_FOLLOW_LOOKAHEAD_MAX_M                (1.80f)

/*
 * Visual direct-target PP uses a more conservative compromise than the INS
 * path-point selector:
 * Ld = clamp(0.70 + 0.40 * |vehicle_speed|, 1.00, 1.80) m.
 * This preserves tighter turns than the original long-lookahead version while
 * avoiding the overly aggressive low-speed response of the raw INS values.
 * There is deliberately no breadcrumb path or curvature-derived speed ceiling.
 */
#define COLOR_FOLLOW_DESIRED_RANGE_M               (1.00f)
#define COLOR_FOLLOW_RANGE_DEADBAND_M              (0.12f)
#define COLOR_FOLLOW_DISTANCE_KP_SQ                 (1.20f)
#define COLOR_FOLLOW_REL_SPEED_KD_PER_S             (1.50f)
#define COLOR_FOLLOW_ACCEL_MPS2                    (2.00f)
#define COLOR_FOLLOW_DECEL_MPS2                    (2.00f)
#define COLOR_FOLLOW_RANGE_FILTER_ALPHA            (0.35f)
#define COLOR_FOLLOW_REL_SPEED_FILTER_BETA         (0.06f)
#define COLOR_FOLLOW_RANGE_FILTER_RESET_MS         (120U)
#define COLOR_FOLLOW_ACCEL_FRESH_MS                (70U)
#define COLOR_FOLLOW_LATERAL_ERROR_SMALL_PX        (4.0f)
#define COLOR_FOLLOW_LATERAL_ERROR_LARGE_PX        (18.0f)
#define COLOR_FOLLOW_CENTER_FILTER_ALPHA_SMALL     (0.25f)
#define COLOR_FOLLOW_CENTER_FILTER_ALPHA           (0.50f)
#define COLOR_FOLLOW_STEER_GAIN_SMALL              (0.50f)
#define COLOR_FOLLOW_STEER_GAIN_LARGE              (1.00f)
#define COLOR_FOLLOW_LOST_HOLD_MS                  (100U)
#define COLOR_FOLLOW_LOST_TIMEOUT_MS               (300U)
#define COLOR_FOLLOW_START_VALID_FRAMES            (1U)

extern float g_color_follow_max_speed; // 全局跟随最大速度定义

#define COLOR_VISION_FRAME_W                (SCC8660_W)
#define COLOR_VISION_FRAME_H                (SCC8660_H)
#define COLOR_VISION_FRAME_PIXELS           (COLOR_VISION_FRAME_W * COLOR_VISION_FRAME_H)
#define COLOR_VISION_MASK_PREVIEW_W         (COLOR_VISION_FRAME_W / 2U)
#define COLOR_VISION_MASK_PREVIEW_H         (COLOR_VISION_FRAME_H / 2U)

typedef enum
{
    COLOR_VISION_CAMERA_STARTING = 0,
    COLOR_VISION_CAMERA_READY,
    COLOR_VISION_CAMERA_ERROR
} color_vision_camera_state_t;

typedef struct
{
    uint32 frame_id;
    uint32 timestamp_ms;
    uint32 area;
    uint16 processing_ms;
    uint16 x_min;
    uint16 y_min;
    uint16 x_max;
    uint16 y_max;
    uint16 center_x;
    uint16 center_y;
    uint16 width;
    uint16 height;
    uint8 valid;
    uint8 probe_h;
    uint8 probe_s;
    uint8 probe_v;
    uint16 probe_raw;
    uint8 probe_r;
    uint8 probe_g;
    uint8 probe_b;
} color_vision_result_t;

typedef struct
{
    uint8 calibrated;
    uint8 enabled;
    uint8 command_valid;
    float camera_range_m;
    float lookahead_m;
    float relative_speed_mps;
    float accel_cmd_mps2;
    float target_x_m;
    float target_y_m;
    float speed_mps;
    float wheel_angle_deg;
} color_vision_follow_debug_t;

/*
 * Coherent 100 Hz diagnosis snapshot. CPU0 publishes this after completing
 * the current chassis-control tick; CPU1 only reads and transmits it. A
 * negative vision_age_ms means that the current camera result is invalid.
 */
typedef struct
{
    float raw_center_error_px;
    float target_y_m;
    float target_wheel_angle_deg;
    float actual_wheel_angle_deg;
    float actual_speed_mps;
    float yaw_rate_dps;
    float vision_age_ms;
} color_vision_diagnosis_t;

/* Called once by CPU0. Camera DMA and its two ISRs are owned by CPU0. */
uint8 color_vision_camera_init(void);

/* Called repeatedly by the CPU1 main loop. Never call it from an ISR. */
void color_vision_service(void);

/* Coherent result snapshot for a future CPU0 control loop. */
uint8 color_vision_get_result(color_vision_result_t *result);
color_vision_camera_state_t color_vision_get_camera_state(void);

/* CPU0-only follow controller state and command generation. */
void color_vision_follow_enable(void);
void color_vision_follow_disable(void);
uint8 color_vision_follow_is_enabled(void);
uint8 color_vision_follow_get_command(float *speed_mps,
                                      float *wheel_angle_deg);
uint8 color_vision_follow_get_debug(color_vision_follow_debug_t *debug);

/* CPU0 publishes; CPU1 reads through the guarded snapshot. */
void color_vision_diagnosis_publish(float actual_wheel_angle_deg,
                                    float actual_speed_mps,
                                    float yaw_rate_dps);
uint8 color_vision_diagnosis_get_snapshot(
    color_vision_diagnosis_t *diagnosis);

/* CPU0-only IPS200 debug page API. */
void color_vision_debug_begin(void);
void color_vision_debug_update(void);
void color_vision_debug_end(void);

#endif
