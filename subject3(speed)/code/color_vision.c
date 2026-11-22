#include "color_vision.h"

#include "IfxCpu.h"
#include "IfxCpu_Intrinsics.h"
#include "global_vars.h"
#include "initialize.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

float g_color_follow_max_speed = 2.50f;

#define COLOR_VISION_MASK_ROW_BYTES    ((COLOR_VISION_FRAME_W + 7U) / 8U)
#define COLOR_VISION_MASK_BYTES        (COLOR_VISION_MASK_ROW_BYTES * COLOR_VISION_FRAME_H)
#define COLOR_VISION_DEBUG_HOLD_MS     (200U)
#define COLOR_VISION_STATUS_COLUMNS    (29U)

#define MASK_GET(mask, x, y) \
    (((mask)[((uint32)(y) * COLOR_VISION_MASK_ROW_BYTES) + ((uint32)(x) >> 3U)] >> \
      ((uint32)(x) & 7U)) & 1U)

#define MASK_SET(mask, x, y) \
    ((mask)[((uint32)(y) * COLOR_VISION_MASK_ROW_BYTES) + ((uint32)(x) >> 3U)] |= \
     (uint8)(1U << ((uint32)(x) & 7U)))

typedef struct
{
    volatile uint32 result_guard;
    volatile color_vision_result_t result;
    volatile uint8 camera_state;
    volatile uint8 debug_hold_request;
    volatile uint8 debug_hold_ack;
} color_vision_shared_t;

typedef struct
{
    uint32 area;
    uint32 sum_x;
    uint32 sum_y;
    uint16 x_min;
    uint16 y_min;
    uint16 x_max;
    uint16 y_max;
} color_vision_blob_accumulator_t;

/* CPU1 keeps the last selected target so each new frame can prefer the
 * spatially continuous component instead of the globally largest component. */
typedef struct
{
    uint8 valid;
    uint16 center_x;
    uint16 center_y;
    uint16 height;
    uint32 area;
} color_vision_target_track_t;

/*
 * This state is touched only by CPU0 (the menu and the 10 ms control ISR).
 * CPU1 publishes the camera result separately through g_color_vision_shared.
 */
typedef struct
{
    volatile uint8 enabled;
    uint8 initialized;
    uint8 valid_streak;
    uint8 command_valid;
    uint8 range_filter_initialized;
    uint8 lost_fade_active;
    uint32 last_frame_id;
    uint32 last_target_seen_ms;
    uint32 last_measurement_ms;
    float height_filtered_pixels;
    float center_filtered_pixels;
    float camera_range_m;
    float target_x_m;
    float target_y_m;
    float lookahead_m;
    float speed_cmd;
    float last_valid_speed_cmd;
    float wheel_angle_cmd;
    float range_filtered_m;
    float relative_speed_filtered_mps;
    float held_accel_cmd_mps2;
    float integrated_speed;
    float lost_fade_start_speed;
} color_vision_follow_state_t;

typedef struct
{
    volatile uint32 guard;
    volatile color_vision_diagnosis_t data;
} color_vision_diagnosis_shared_t;

IFX_ALIGN(4) static color_vision_follow_state_t g_color_vision_follow
    __attribute__((section(".bss.cpu0_dsram")));
IFX_ALIGN(4) static color_vision_diagnosis_shared_t
    g_color_vision_diagnosis_shared
    __attribute__((section(".bss.cpu0_dsram")));
IFX_ALIGN(4) static float g_color_vision_last_raw_error_px
    __attribute__((section(".bss.cpu0_dsram")));

/* CPU1 owns these buffers. The explicit section is required in a separate .c file. */
IFX_ALIGN(4) static uint16 g_color_vision_frame[COLOR_VISION_FRAME_H]
                                                   [COLOR_VISION_FRAME_W]
    __attribute__((section(".bss.cpu1_dsram")));
IFX_ALIGN(4) static uint8 g_color_vision_mask_a[COLOR_VISION_MASK_BYTES]
    __attribute__((section(".bss.cpu1_dsram")));
IFX_ALIGN(4) static uint8 g_color_vision_mask_b[COLOR_VISION_MASK_BYTES]
    __attribute__((section(".bss.cpu1_dsram")));
IFX_ALIGN(4) static uint32 g_color_vision_frame_id
    __attribute__((section(".bss.cpu1_dsram")));
IFX_ALIGN(4) static color_vision_target_track_t g_color_vision_target_track
    __attribute__((section(".bss.cpu1_dsram")));

/* CPU1 has 240 KB DSPR, so the private flood queue can stay in local RAM. */
IFX_ALIGN(4) static uint16 g_color_vision_flood_queue[COLOR_VISION_FRAME_PIXELS]
    __attribute__((section(".bss.cpu1_dsram")));
/* CPU0 owns display and shared state; CPU1 only writes these through SRI. */
IFX_ALIGN(4) static uint16 g_color_vision_mask_preview
    [COLOR_VISION_MASK_PREVIEW_H][COLOR_VISION_MASK_PREVIEW_W]
    __attribute__((section(".bss.cpu0_dsram")));
IFX_ALIGN(4) static color_vision_shared_t g_color_vision_shared
    __attribute__((section(".bss.cpu0_dsram")));

static uint16 color_vision_normalize_rgb565(uint16 pixel)
{
#if COLOR_VISION_RGB565_BYTE_SWAP
    pixel = (uint16)((pixel >> 8U) | (pixel << 8U));
#endif
    return pixel;
}
static void color_vision_follow_invalidate_tracking(void)
{
    g_color_vision_follow.initialized = 0U;
    g_color_vision_follow.valid_streak = 0U;
    g_color_vision_follow.command_valid = 0U;
    g_color_vision_follow.last_target_seen_ms = 0U;
    g_color_vision_follow.height_filtered_pixels = 0.0f;
    g_color_vision_follow.center_filtered_pixels = 0.0f;
    g_color_vision_follow.camera_range_m = 0.0f;
    g_color_vision_follow.target_x_m = 0.0f;
    g_color_vision_follow.target_y_m = 0.0f;
    g_color_vision_follow.lookahead_m = 0.0f;
    g_color_vision_follow.speed_cmd = 0.0f;
    g_color_vision_follow.last_valid_speed_cmd = 0.0f;
    g_color_vision_follow.wheel_angle_cmd = 0.0f;

    g_color_vision_follow.range_filter_initialized = 0U;
    g_color_vision_follow.lost_fade_active = 0U;
    g_color_vision_follow.last_measurement_ms = 0U;
    g_color_vision_follow.range_filtered_m = 0.0f;
    g_color_vision_follow.relative_speed_filtered_mps = 0.0f;
    g_color_vision_follow.held_accel_cmd_mps2 = 0.0f;
    g_color_vision_follow.integrated_speed = 0.0f;
    g_color_vision_follow.lost_fade_start_speed = 0.0f;
}
static void color_vision_decode_rgb565(uint16 pixel,
                                       uint8 *r,
                                       uint8 *g,
                                       uint8 *b)
{
    uint16 normalized = color_vision_normalize_rgb565(pixel);
    uint8 r5 = (uint8)((normalized >> 11U) & 0x1FU);
    uint8 g6 = (uint8)((normalized >> 5U) & 0x3FU);
    uint8 b5 = (uint8)(normalized & 0x1FU);

    *r = (uint8)((r5 << 3U) | (r5 >> 2U));
    *g = (uint8)((g6 << 2U) | (g6 >> 4U));
    *b = (uint8)((b5 << 3U) | (b5 >> 2U));

#if COLOR_VISION_SWAP_RED_BLUE
    {
        uint8 temporary = *r;
        *r = *b;
        *b = temporary;
    }
#endif
}

static void color_vision_rgb565_to_hsv(uint16 pixel,
                                        uint8 *h,
                                        uint8 *s,
                                        uint8 *v)
{
    uint8 r;
    uint8 g;
    uint8 b;
    uint8 maximum;
    uint8 minimum;
    uint8 delta;
    int32 hue;

    color_vision_decode_rgb565(pixel, &r, &g, &b);

    maximum = r;
    if (g > maximum)
    {
        maximum = g;
    }
    if (b > maximum)
    {
        maximum = b;
    }

    minimum = r;
    if (g < minimum)
    {
        minimum = g;
    }
    if (b < minimum)
    {
        minimum = b;
    }

    delta = (uint8)(maximum - minimum);
    *v = maximum;
    *s = (maximum == 0U) ? 0U :
         (uint8)((((uint16)delta * 255U) + (maximum / 2U)) / maximum);

    if (delta == 0U)
    {
        *h = 0U;
        return;
    }

    if (maximum == r)
    {
        hue = (30 * ((int32)g - (int32)b)) / (int32)delta;
    }
    else if (maximum == g)
    {
        hue = 60 + (30 * ((int32)b - (int32)r)) / (int32)delta;
    }
    else
    {
        hue = 120 + (30 * ((int32)r - (int32)g)) / (int32)delta;
    }

    if (hue < 0)
    {
        hue += 180;
    }
    else if (hue >= 180)
    {
        hue -= 180;
    }
    *h = (uint8)hue;
}

static uint8 color_vision_hue_in_range(uint8 h, uint8 h_min, uint8 h_max)
{
    if (h_min <= h_max)
    {
        return (uint8)((h >= h_min) && (h <= h_max));
    }

    /* Hue ranges such as red can cross OpenCV's 179 -> 0 boundary. */
    return (uint8)((h >= h_min) || (h <= h_max));
}

static uint8 color_vision_hsv_in_range(uint8 h,
                                       uint8 s,
                                       uint8 v,
                                       uint8 h_min,
                                       uint8 h_max,
                                       uint8 s_min,
                                       uint8 s_max,
                                       uint8 v_min,
                                       uint8 v_max)
{
    return (uint8)(color_vision_hue_in_range(h, h_min, h_max) &&
                   (s >= s_min) && (s <= s_max) &&
                   (v >= v_min) && (v <= v_max));
}

static uint8 color_vision_green_matches(uint8 h, uint8 s, uint8 v)
{
    return color_vision_hsv_in_range(h, s, v,
        COLOR_VISION_GREEN_H_MIN, COLOR_VISION_GREEN_H_MAX,
        COLOR_VISION_GREEN_S_MIN, COLOR_VISION_GREEN_S_MAX,
        COLOR_VISION_GREEN_V_MIN, COLOR_VISION_GREEN_V_MAX);
}

static void color_vision_build_green_mask(void)
{
    uint16 x;
    uint16 y;

    memset(g_color_vision_mask_a, 0, sizeof(g_color_vision_mask_a));
    for (y = 0U; y < COLOR_VISION_FRAME_H; y++)
    {
        for (x = 0U; x < COLOR_VISION_FRAME_W; x++)
        {
            uint8 h;
            uint8 s;
            uint8 v;
            color_vision_rgb565_to_hsv(g_color_vision_frame[y][x], &h, &s, &v);
            if (color_vision_green_matches(h, s, v))
            {
                MASK_SET(g_color_vision_mask_a, x, y);
            }
        }
    }
}

static void color_vision_erode_3x3(const uint8 *source, uint8 *destination)
{
    uint16 x;
    uint16 y;

    memset(destination, 0, COLOR_VISION_MASK_BYTES);
    for (y = 0U; y < COLOR_VISION_FRAME_H; y++)
    {
        for (x = 0U; x < COLOR_VISION_FRAME_W; x++)
        {
            uint8 keep = 1U;
            int16 offset_x;
            int16 offset_y;

            for (offset_y = -1; (offset_y <= 1) && keep; offset_y++)
            {
                for (offset_x = -1; offset_x <= 1; offset_x++)
                {
                    int16 neighbor_x = (int16)x + offset_x;
                    int16 neighbor_y = (int16)y + offset_y;
                    if ((neighbor_x < 0) ||
                        (neighbor_x >= (int16)COLOR_VISION_FRAME_W) ||
                        (neighbor_y < 0) ||
                        (neighbor_y >= (int16)COLOR_VISION_FRAME_H))
                    {
                        continue;
                    }
                    if (!MASK_GET(source,
                                  (uint16)neighbor_x,
                                  (uint16)neighbor_y))
                    {
                        keep = 0U;
                        break;
                    }
                }
            }
            if (keep)
            {
                MASK_SET(destination, x, y);
            }
        }
    }
}

static void color_vision_dilate_3x3(const uint8 *source, uint8 *destination)
{
    uint16 x;
    uint16 y;

    memset(destination, 0, COLOR_VISION_MASK_BYTES);
    for (y = 0U; y < COLOR_VISION_FRAME_H; y++)
    {
        for (x = 0U; x < COLOR_VISION_FRAME_W; x++)
        {
            uint8 set = 0U;
            int16 offset_x;
            int16 offset_y;

            for (offset_y = -1; (offset_y <= 1) && !set; offset_y++)
            {
                for (offset_x = -1; offset_x <= 1; offset_x++)
                {
                    int16 neighbor_x = (int16)x + offset_x;
                    int16 neighbor_y = (int16)y + offset_y;
                    if ((neighbor_x < 0) ||
                        (neighbor_x >= (int16)COLOR_VISION_FRAME_W) ||
                        (neighbor_y < 0) ||
                        (neighbor_y >= (int16)COLOR_VISION_FRAME_H))
                    {
                        continue;
                    }
                    if (MASK_GET(source,
                                 (uint16)neighbor_x,
                                 (uint16)neighbor_y))
                    {
                        set = 1U;
                        break;
                    }
                }
            }
            if (set)
            {
                MASK_SET(destination, x, y);
            }
        }
    }
}

static void color_vision_clean_green_mask(void)
{
#if COLOR_VISION_USE_MORPHOLOGY
    /* 3x3 opening removes isolated noise; 3x3 closing fills small holes. */
    color_vision_erode_3x3(g_color_vision_mask_a, g_color_vision_mask_b);
    color_vision_dilate_3x3(g_color_vision_mask_b, g_color_vision_mask_a);
    color_vision_dilate_3x3(g_color_vision_mask_a, g_color_vision_mask_b);
    color_vision_erode_3x3(g_color_vision_mask_b, g_color_vision_mask_a);
#endif
}

static void color_vision_build_mask_preview(void)
{
    uint16 preview_x;
    uint16 preview_y;

    for (preview_y = 0U; preview_y < COLOR_VISION_MASK_PREVIEW_H; preview_y++)
    {
        for (preview_x = 0U; preview_x < COLOR_VISION_MASK_PREVIEW_W; preview_x++)
        {
            uint16 source_x = (uint16)(preview_x * 2U);
            uint16 source_y = (uint16)(preview_y * 2U);
            uint8 selected = (uint8)(
                MASK_GET(g_color_vision_mask_a, source_x, source_y) ||
                MASK_GET(g_color_vision_mask_a, source_x + 1U, source_y) ||
                MASK_GET(g_color_vision_mask_a, source_x, source_y + 1U) ||
                MASK_GET(g_color_vision_mask_a, source_x + 1U, source_y + 1U));
            g_color_vision_mask_preview[preview_y][preview_x] =
                selected ? RGB565_WHITE : RGB565_BLACK;
        }
    }
}

static void color_vision_accumulate_pixel(color_vision_blob_accumulator_t *blob,
                                           uint16 x,
                                           uint16 y)
{
    blob->area++;
    blob->sum_x += x;
    blob->sum_y += y;
    if (x < blob->x_min)
    {
        blob->x_min = x;
    }
    if (x > blob->x_max)
    {
        blob->x_max = x;
    }
    if (y < blob->y_min)
    {
        blob->y_min = y;
    }
    if (y > blob->y_max)
    {
        blob->y_max = y;
    }
}

static uint32 color_vision_square_difference(uint16 first, uint16 second)
{
    int32 difference = (int32)first - (int32)second;
    return (uint32)(difference * difference);
}

static color_vision_blob_accumulator_t color_vision_find_tracked_blob(void)
{
    color_vision_blob_accumulator_t selected = {0U};
    uint32 selected_score = 0U;
    uint8 selected_valid = 0U;
    uint16 seed_x;
    uint16 seed_y;

    memset(g_color_vision_mask_b, 0, sizeof(g_color_vision_mask_b));
    for (seed_y = 0U; seed_y < COLOR_VISION_FRAME_H; seed_y++)
    {
        for (seed_x = 0U; seed_x < COLOR_VISION_FRAME_W; seed_x++)
        {
            color_vision_blob_accumulator_t current;
            uint16 queue_head;
            uint16 queue_tail;

            if (!MASK_GET(g_color_vision_mask_a, seed_x, seed_y) ||
                MASK_GET(g_color_vision_mask_b, seed_x, seed_y))
            {
                continue;
            }

            memset(&current, 0, sizeof(current));
            current.x_min = COLOR_VISION_FRAME_W;
            current.y_min = COLOR_VISION_FRAME_H;
            queue_head = 0U;
            queue_tail = 0U;
            MASK_SET(g_color_vision_mask_b, seed_x, seed_y);
            g_color_vision_flood_queue[queue_tail++] =
                (uint16)((seed_y * COLOR_VISION_FRAME_W) + seed_x);

            while (queue_head < queue_tail)
            {
                uint16 index = g_color_vision_flood_queue[queue_head++];
                uint16 x = (uint16)(index % COLOR_VISION_FRAME_W);
                uint16 y = (uint16)(index / COLOR_VISION_FRAME_W);
                int16 offset_x;
                int16 offset_y;

                color_vision_accumulate_pixel(&current, x, y);
                for (offset_y = -1; offset_y <= 1; offset_y++)
                {
                    for (offset_x = -1; offset_x <= 1; offset_x++)
                    {
                        int16 neighbor_x;
                        int16 neighbor_y;
                        if ((offset_x == 0) && (offset_y == 0))
                        {
                            continue;
                        }
                        neighbor_x = (int16)x + offset_x;
                        neighbor_y = (int16)y + offset_y;
                        if ((neighbor_x < 0) ||
                            (neighbor_x >= (int16)COLOR_VISION_FRAME_W) ||
                            (neighbor_y < 0) ||
                            (neighbor_y >= (int16)COLOR_VISION_FRAME_H))
                        {
                            continue;
                        }
                        if (MASK_GET(g_color_vision_mask_a,
                                     (uint16)neighbor_x,
                                     (uint16)neighbor_y) &&
                            !MASK_GET(g_color_vision_mask_b,
                                      (uint16)neighbor_x,
                                      (uint16)neighbor_y))
                        {
                            MASK_SET(g_color_vision_mask_b,
                                     (uint16)neighbor_x,
                                     (uint16)neighbor_y);
                            g_color_vision_flood_queue[queue_tail++] =
                                (uint16)(((uint16)neighbor_y * COLOR_VISION_FRAME_W) +
                                         (uint16)neighbor_x);
                        }
                    }
                }
            }

            if (current.area >= COLOR_VISION_MIN_AREA_PIXELS)
            {
                uint16 current_center_x =
                    (uint16)(current.sum_x / current.area);
                uint16 current_center_y =
                    (uint16)(current.sum_y / current.area);
                uint16 current_height =
                    (uint16)(current.y_max - current.y_min + 1U);
                uint32 current_score = 0U;

                if (g_color_vision_target_track.valid)
                {
                    /* Match the proven version: centre continuity is primary,
                     * and height prevents a nearby fragment taking the lock. */
                    current_score =
                        color_vision_square_difference(
                            current_center_x,
                            g_color_vision_target_track.center_x) +
                        color_vision_square_difference(
                            current_center_y,
                            g_color_vision_target_track.center_y) +
                        color_vision_square_difference(
                            current_height,
                            g_color_vision_target_track.height);

                    if (!selected_valid ||
                        (current_score < selected_score) ||
                        ((current_score == selected_score) &&
                         (current.area > selected.area)))
                    {
                        selected = current;
                        selected_score = current_score;
                        selected_valid = 1U;
                    }
                }
                else if (!selected_valid || (current.area > selected.area))
                {
                    selected = current;
                    selected_valid = 1U;
                }
            }
        }
    }

    if (selected_valid)
    {
        g_color_vision_target_track.valid = 1U;
        g_color_vision_target_track.center_x =
            (uint16)(selected.sum_x / selected.area);
        g_color_vision_target_track.center_y =
            (uint16)(selected.sum_y / selected.area);
        g_color_vision_target_track.height =
            (uint16)(selected.y_max - selected.y_min + 1U);
        g_color_vision_target_track.area = selected.area;
    }
    return selected;
}

static void color_vision_publish(const color_vision_result_t *result)
{
    g_color_vision_shared.result_guard++;
    __dsync();

    g_color_vision_shared.result.frame_id = result->frame_id;
    g_color_vision_shared.result.timestamp_ms = result->timestamp_ms;
    g_color_vision_shared.result.area = result->area;
    g_color_vision_shared.result.processing_ms = result->processing_ms;
    g_color_vision_shared.result.x_min = result->x_min;
    g_color_vision_shared.result.y_min = result->y_min;
    g_color_vision_shared.result.x_max = result->x_max;
    g_color_vision_shared.result.y_max = result->y_max;
    g_color_vision_shared.result.center_x = result->center_x;
    g_color_vision_shared.result.center_y = result->center_y;
    g_color_vision_shared.result.width = result->width;
    g_color_vision_shared.result.height = result->height;
    g_color_vision_shared.result.valid = result->valid;
    g_color_vision_shared.result.probe_h = result->probe_h;
    g_color_vision_shared.result.probe_s = result->probe_s;
    g_color_vision_shared.result.probe_v = result->probe_v;
    g_color_vision_shared.result.probe_raw = result->probe_raw;
    g_color_vision_shared.result.probe_r = result->probe_r;
    g_color_vision_shared.result.probe_g = result->probe_g;
    g_color_vision_shared.result.probe_b = result->probe_b;

    __dsync();
    g_color_vision_shared.result_guard++;
}

static void color_vision_process_work_frame(void)
{
    color_vision_blob_accumulator_t tracked_blob;
    color_vision_result_t result;
    uint32 process_started_ms = g_system_time_ms;

    memset(&result, 0, sizeof(result));
    result.frame_id = ++g_color_vision_frame_id;
    result.timestamp_ms = process_started_ms;
    result.probe_raw =
        g_color_vision_frame[COLOR_VISION_FRAME_H / 2U][COLOR_VISION_FRAME_W / 2U];
    color_vision_decode_rgb565(result.probe_raw,
                               &result.probe_r,
                               &result.probe_g,
                               &result.probe_b);
    color_vision_rgb565_to_hsv(result.probe_raw,
        &result.probe_h,
        &result.probe_s,
        &result.probe_v);

    color_vision_build_green_mask();
    color_vision_clean_green_mask();
    color_vision_build_mask_preview();
    tracked_blob = color_vision_find_tracked_blob();

    if (tracked_blob.area >= COLOR_VISION_MIN_AREA_PIXELS)
    {
        result.valid = 1U;
        result.area = tracked_blob.area;
        result.x_min = tracked_blob.x_min;
        result.y_min = tracked_blob.y_min;
        result.x_max = tracked_blob.x_max;
        result.y_max = tracked_blob.y_max;
        result.center_x = (uint16)(tracked_blob.sum_x / tracked_blob.area);
        result.center_y = (uint16)(tracked_blob.sum_y / tracked_blob.area);
        result.width = (uint16)(tracked_blob.x_max - tracked_blob.x_min + 1U);
        result.height = (uint16)(tracked_blob.y_max - tracked_blob.y_min + 1U);
    }
    result.processing_ms = (uint16)(g_system_time_ms - process_started_ms);
    color_vision_publish(&result);
}

uint8 color_vision_camera_init(void)
{
    uint8 init_state;

    memset((void *)&g_color_vision_shared, 0, sizeof(g_color_vision_shared));
    g_color_vision_shared.camera_state = COLOR_VISION_CAMERA_STARTING;
    __dsync();

    init_state = scc8660_init();
    scc8660_finish_flag = 0U;
    g_color_vision_shared.camera_state =
        (init_state == 0U) ? COLOR_VISION_CAMERA_READY : COLOR_VISION_CAMERA_ERROR;
    __dsync();
    return init_state;
}

void color_vision_service(void)
{
    const uint16 *camera_frame_address;
    volatile uint8 *camera_finish_flag =
        (volatile uint8 *)IFXCPU_GLB_ADDR_DSPR(
            0U, (uint32)&scc8660_finish_flag);

    if (g_color_vision_shared.debug_hold_request)
    {
        g_color_vision_shared.debug_hold_ack = 1U;
        __dsync();
        return;
    }

    if (g_color_vision_shared.debug_hold_ack)
    {
        g_color_vision_shared.debug_hold_ack = 0U;
        /* A frame may have completed while CPU1 was held for IPS drawing. */
        *camera_finish_flag = 0U;
        __dsync();
        return;
    }

    if ((g_color_vision_shared.camera_state != COLOR_VISION_CAMERA_READY) ||
        (*camera_finish_flag == 0U))
    {
        return;
    }

    /* Clear first so a later DMA completion remains visible after this copy. */
    *camera_finish_flag = 0U;
    __dsync();
    camera_frame_address = (const uint16 *)IFXCPU_GLB_ADDR_DSPR(
        0U, (uint32)&scc8660_image[0][0]);
    memcpy(g_color_vision_frame[0], camera_frame_address, SCC8660_IMAGE_SIZE);
    __dsync();
    color_vision_process_work_frame();

    /* Drop completions accumulated during processing. The next accepted flag
     * must be a new DMA completion, so the source cannot be mid-overwrite. */
    *camera_finish_flag = 0U;
    __dsync();
}

uint8 color_vision_get_result(color_vision_result_t *result)
{
    uint8 attempt;

    if (result == NULL)
    {
        return 0U;
    }

    for (attempt = 0U; attempt < 4U; attempt++)
    {
        uint32 guard_before = g_color_vision_shared.result_guard;
        uint32 guard_after;
        if (guard_before & 1U)
        {
            continue;
        }

        __dsync();
        result->frame_id = g_color_vision_shared.result.frame_id;
        result->timestamp_ms = g_color_vision_shared.result.timestamp_ms;
        result->area = g_color_vision_shared.result.area;
        result->processing_ms = g_color_vision_shared.result.processing_ms;
        result->x_min = g_color_vision_shared.result.x_min;
        result->y_min = g_color_vision_shared.result.y_min;
        result->x_max = g_color_vision_shared.result.x_max;
        result->y_max = g_color_vision_shared.result.y_max;
        result->center_x = g_color_vision_shared.result.center_x;
        result->center_y = g_color_vision_shared.result.center_y;
        result->width = g_color_vision_shared.result.width;
        result->height = g_color_vision_shared.result.height;
        result->valid = g_color_vision_shared.result.valid;
        result->probe_h = g_color_vision_shared.result.probe_h;
        result->probe_s = g_color_vision_shared.result.probe_s;
        result->probe_v = g_color_vision_shared.result.probe_v;
        result->probe_raw = g_color_vision_shared.result.probe_raw;
        result->probe_r = g_color_vision_shared.result.probe_r;
        result->probe_g = g_color_vision_shared.result.probe_g;
        result->probe_b = g_color_vision_shared.result.probe_b;
        __dsync();

        guard_after = g_color_vision_shared.result_guard;
        if ((guard_before == guard_after) && !(guard_after & 1U))
        {
            return 1U;
        }
    }
    return 0U;
}

static void color_vision_follow_reset_state(void)
{
    memset((void *)&g_color_vision_follow, 0, sizeof(g_color_vision_follow));
    color_vision_follow_invalidate_tracking();
}

void color_vision_follow_enable(void)
{
    color_vision_follow_reset_state();
    g_color_vision_follow.enabled = 1U;
    __dsync();
}

void color_vision_follow_disable(void)
{
    g_color_vision_follow.enabled = 0U;
    __dsync();
    color_vision_follow_reset_state();
}

uint8 color_vision_follow_is_enabled(void)
{
    return g_color_vision_follow.enabled;
}

static uint8 color_vision_follow_float_invalid(float value)
{
    /* TASKING builds used by this project do not require C99 isfinite(). */
    return (uint8)((value != value) ||
                   (value > 100000.0f) ||
                   (value < -100000.0f));
}

static float color_vision_follow_clampf(float value,
                                        float minimum,
                                        float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static uint8 color_vision_follow_range_from_height(float height_pixels,
                                                    float *range_m)
{
    float range;

    if ((range_m == NULL) ||
        color_vision_follow_float_invalid(height_pixels) ||
        (height_pixels <= 0.0f) ||
        (COLOR_FOLLOW_HEIGHT_CAL_DISTANCE_M <= 0.0f) ||
        (COLOR_FOLLOW_HEIGHT_CAL_PIXELS <= 0.0f))
    {
        return 0U;
    }

    range = COLOR_FOLLOW_HEIGHT_CAL_DISTANCE_M *
            COLOR_FOLLOW_HEIGHT_CAL_PIXELS / height_pixels;
    if (color_vision_follow_float_invalid(range) || (range <= 0.0f))
    {
        return 0U;
    }

    *range_m = range;
    return 1U;
}

/* Update the range/range-rate observer only when a new valid camera frame is
 * accepted.  Its dt is therefore the visual measurement interval, not the
 * 10 ms chassis-control interval. */
static void color_vision_follow_update_range_state(float measured_range_m,
                                                   uint32 measurement_ms)
{
    uint32 delta_ms;
    float dt_s;
    float predicted_range;
    float residual;

    if (!g_color_vision_follow.range_filter_initialized)
    {
        g_color_vision_follow.range_filter_initialized = 1U;
        g_color_vision_follow.range_filtered_m = measured_range_m;
        g_color_vision_follow.relative_speed_filtered_mps = 0.0f;
        g_color_vision_follow.last_measurement_ms = measurement_ms;
        return;
    }

    delta_ms = measurement_ms - g_color_vision_follow.last_measurement_ms;
    g_color_vision_follow.last_measurement_ms = measurement_ms;

    if ((delta_ms == 0U) ||
        (delta_ms > COLOR_FOLLOW_RANGE_FILTER_RESET_MS))
    {
        g_color_vision_follow.range_filtered_m = measured_range_m;
        g_color_vision_follow.relative_speed_filtered_mps = 0.0f;
        return;
    }

    dt_s = (float)delta_ms / 1000.0f;
    predicted_range = g_color_vision_follow.range_filtered_m +
                      g_color_vision_follow.relative_speed_filtered_mps * dt_s;
    residual = measured_range_m - predicted_range;

    g_color_vision_follow.range_filtered_m = predicted_range +
        COLOR_FOLLOW_RANGE_FILTER_ALPHA * residual;
    g_color_vision_follow.relative_speed_filtered_mps +=
        (COLOR_FOLLOW_REL_SPEED_FILTER_BETA / dt_s) * residual;
}

static void color_vision_follow_update_accel_command(void)
{
    float distance_error = g_color_vision_follow.range_filtered_m -
                           COLOR_FOLLOW_DESIRED_RANGE_M;
    float accel_cmd;

    if (fabsf(distance_error) < COLOR_FOLLOW_RANGE_DEADBAND_M)
    {
        distance_error = 0.0f;
    }

    accel_cmd = COLOR_FOLLOW_DISTANCE_KP_SQ * distance_error +
                COLOR_FOLLOW_REL_SPEED_KD_PER_S *
                g_color_vision_follow.relative_speed_filtered_mps;

    if (accel_cmd > COLOR_FOLLOW_ACCEL_MPS2)
    {
        accel_cmd = COLOR_FOLLOW_ACCEL_MPS2;
    }
    else if (accel_cmd < -COLOR_FOLLOW_DECEL_MPS2)
    {
        accel_cmd = -COLOR_FOLLOW_DECEL_MPS2;
    }
    g_color_vision_follow.held_accel_cmd_mps2 = accel_cmd;
}

/* Called at the fixed 100 Hz chassis rate.  This is the only speed ramp used
 * by camera follow; visual measurement timing is handled above. */
static float color_vision_follow_integrate_speed(void)
{
    g_color_vision_follow.integrated_speed +=
        g_color_vision_follow.held_accel_cmd_mps2 / CONTROL_FREQ;

    if (g_color_vision_follow.integrated_speed > g_color_follow_max_speed)
    {
        g_color_vision_follow.integrated_speed = g_color_follow_max_speed;
    }
    else if (g_color_vision_follow.integrated_speed < 0.0f)
    {
        g_color_vision_follow.integrated_speed = 0.0f;
    }
    return g_color_vision_follow.integrated_speed;
}

/* Return a continuous 0..1 blend for straight-line suppression.  The raw
 * frame error is used so a genuine turn immediately restores filtering and
 * steering response instead of waiting for the filtered centre to catch up. */
static float color_vision_follow_lateral_blend(float abs_error_pixels)
{
    float span;

    if (abs_error_pixels <= COLOR_FOLLOW_LATERAL_ERROR_SMALL_PX)
    {
        return 0.0f;
    }
    if (abs_error_pixels >= COLOR_FOLLOW_LATERAL_ERROR_LARGE_PX)
    {
        return 1.0f;
    }

    span = COLOR_FOLLOW_LATERAL_ERROR_LARGE_PX -
           COLOR_FOLLOW_LATERAL_ERROR_SMALL_PX;
    return (abs_error_pixels - COLOR_FOLLOW_LATERAL_ERROR_SMALL_PX) /
           span;
}

static uint8 color_vision_follow_calculate_geometry(
    const color_vision_result_t *result,
    float range,
    float center_x_pixels,
    float *range_m,
    float *target_x_m,
    float *target_y_m,
    float *wheel_angle_deg)
{
    float y_camera;
    float yaw_rad;
    float yaw_cos;
    float yaw_sin;
    float target_x;
    float target_y;
    float actual_vehicle_speed;
    float alpha;
    float lookahead;
    float angle_rad;

    if ((range_m == NULL) || (target_x_m == NULL) ||
        (target_y_m == NULL) || (wheel_angle_deg == NULL))
    {
        return 0U;
    }

    *range_m = 0.0f;
    *target_x_m = 0.0f;
    *target_y_m = 0.0f;
    *wheel_angle_deg = 0.0f;

    if (!result->valid ||
        color_vision_follow_float_invalid(range) ||
        (range <= 0.0f) ||
        (COLOR_FOLLOW_CAMERA_FX_PX <= 0.0f) ||
        (WHEEL_BASE <= 0.0f))
    {
        return 0U;
    }

    /* Image u grows to the right; vehicle Y is defined positive to the left. */
    y_camera = -(center_x_pixels - COLOR_FOLLOW_CAMERA_CX_PX) *
               range / COLOR_FOLLOW_CAMERA_FX_PX;

    yaw_rad = COLOR_FOLLOW_CAMERA_YAW_DEG *
              (3.14159265358979323846f / 180.0f);
    yaw_cos = cosf(yaw_rad);
    yaw_sin = sinf(yaw_rad);

    target_x = COLOR_FOLLOW_CAMERA_X_FROM_REAR_AXLE_M +
               (yaw_cos * range) - (yaw_sin * y_camera);
    target_y = COLOR_FOLLOW_CAMERA_Y_FROM_CENTER_M +
               (yaw_sin * range) + (yaw_cos * y_camera);

    if (color_vision_follow_float_invalid(target_x) ||
        color_vision_follow_float_invalid(target_y))
    {
        return 0U;
    }

    /* Proven direct-target PP. The marker bearing comes from calibrated
     * geometry; speed only changes the control lookahead, not the measured
     * person position. */
    alpha = atan2f(target_y, target_x);
    actual_vehicle_speed = fabsf(
        (encoder_speed_left + encoder_speed_right) * 0.5f);
    lookahead = COLOR_FOLLOW_LOOKAHEAD_BASE_M +
                COLOR_FOLLOW_LOOKAHEAD_SPEED_GAIN_S *
                actual_vehicle_speed;
    lookahead = color_vision_follow_clampf(
        lookahead,
        COLOR_FOLLOW_LOOKAHEAD_MIN_M,
        COLOR_FOLLOW_LOOKAHEAD_MAX_M);

    angle_rad = atan2f(2.0f * WHEEL_BASE * sinf(alpha),
                       lookahead);

    *wheel_angle_deg = COLOR_FOLLOW_PP_OUTPUT_SIGN *
                      (angle_rad * 180.0f / 3.14159265358979323846f);

    g_color_vision_follow.lookahead_m = lookahead;
    *range_m = range;
    *target_x_m = target_x;
    *target_y_m = target_y;
    return 1U;
}

uint8 color_vision_follow_get_command(float *speed_mps,
                                      float *wheel_angle_deg)
{
    color_vision_result_t result;
    uint32 lost_ms;
    float lost_fade;
    float height_pixels;
    float measured_range_m;
    float range_m;
    float target_x_m;
    float target_y_m;
    float wheel_angle_deg_local;
    float lateral_error_abs_pixels;
    float lateral_blend;
    float center_filter_alpha;
    float steer_gain;
    uint8 geometry_valid;
    //float abs_angle;                            // 转向角绝对值
    //float speed_factor;                         // 弯道降速系数

    if ((speed_mps == NULL) || (wheel_angle_deg == NULL))
    {
        return 0U;
    }

    *speed_mps = 0.0f;
    *wheel_angle_deg = 0.0f;

    if (!g_color_vision_follow.enabled)
    {
        return 0U;
    }
    if (color_vision_get_result(&result) && (result.frame_id != 0U) &&
        (result.frame_id != g_color_vision_follow.last_frame_id))
    {
        g_color_vision_follow.last_frame_id = result.frame_id;

        /* Update geometry once per accepted camera frame, not once per 10 ms
         * chassis tick. Invalid frames simply start the lost-frame timer; they
         * do not immediately clear the last continuous target command. */
        if (result.valid && (result.height != 0U))
        {
            height_pixels = (float)result.height;
            lateral_error_abs_pixels = fabsf(
                (float)result.center_x - COLOR_FOLLOW_CAMERA_CX_PX);
            lateral_blend = color_vision_follow_lateral_blend(
                lateral_error_abs_pixels);
            center_filter_alpha =
                COLOR_FOLLOW_CENTER_FILTER_ALPHA_SMALL +
                lateral_blend *
                (COLOR_FOLLOW_CENTER_FILTER_ALPHA -
                 COLOR_FOLLOW_CENTER_FILTER_ALPHA_SMALL);
            steer_gain = COLOR_FOLLOW_STEER_GAIN_SMALL +
                lateral_blend *
                (COLOR_FOLLOW_STEER_GAIN_LARGE -
                 COLOR_FOLLOW_STEER_GAIN_SMALL);

            if (!g_color_vision_follow.initialized)
            {
                g_color_vision_follow.initialized = 1U;
                g_color_vision_follow.height_filtered_pixels = height_pixels;
                g_color_vision_follow.center_filtered_pixels =
                    (float)result.center_x;
            }
            else
            {
                /* Longitudinal filtering is performed in metre space by the
                 * alpha-beta observer below. */
                g_color_vision_follow.height_filtered_pixels = height_pixels;
                g_color_vision_follow.center_filtered_pixels +=
                    center_filter_alpha *
                    ((float)result.center_x -
                     g_color_vision_follow.center_filtered_pixels);
            }

            geometry_valid = color_vision_follow_range_from_height(
                height_pixels, &measured_range_m);
            if (geometry_valid)
            {
                color_vision_follow_update_range_state(
                    measured_range_m, result.timestamp_ms);
                range_m = g_color_vision_follow.range_filtered_m;
                geometry_valid = color_vision_follow_calculate_geometry(
                    &result,
                    range_m,
                    g_color_vision_follow.center_filtered_pixels,
                    &range_m,
                    &target_x_m,
                    &target_y_m,
                    &wheel_angle_deg_local);
                if (geometry_valid)
                {
                    wheel_angle_deg_local *= steer_gain;
                }
            }
            if (geometry_valid)
            {
                g_color_vision_follow.camera_range_m = range_m;
                g_color_vision_follow.target_x_m = target_x_m;
                g_color_vision_follow.target_y_m = target_y_m;
                g_color_vision_follow.wheel_angle_cmd = wheel_angle_deg_local;
                g_color_vision_follow.last_target_seen_ms = g_system_time_ms;
                g_color_vision_follow.lost_fade_active = 0U;
                color_vision_follow_update_accel_command();
                if (g_color_vision_follow.valid_streak < 255U)
                {
                    g_color_vision_follow.valid_streak++;
                }
            }
        }
    }

    if (!COLOR_FOLLOW_GEOMETRY_CALIBRATED ||
        !g_color_vision_follow.initialized ||
        (g_color_vision_follow.valid_streak <
         COLOR_FOLLOW_START_VALID_FRAMES))
    {
        *wheel_angle_deg = 0.0f;
        return 0U;
    }

    lost_ms = g_system_time_ms - g_color_vision_follow.last_target_seen_ms;
    if (lost_ms >= COLOR_FOLLOW_LOST_TIMEOUT_MS)
    {
        g_color_vision_follow.command_valid = 0U;
        g_color_vision_follow.speed_cmd = 0.0f;
        g_color_vision_follow.last_valid_speed_cmd = 0.0f;
        color_vision_follow_invalidate_tracking();
        return 0U;
    }

    if (lost_ms > COLOR_FOLLOW_LOST_HOLD_MS)
    {
        if (!g_color_vision_follow.lost_fade_active)
        {
            g_color_vision_follow.lost_fade_active = 1U;
            g_color_vision_follow.lost_fade_start_speed =
                g_color_vision_follow.integrated_speed;
        }
        g_color_vision_follow.held_accel_cmd_mps2 = 0.0f;
        lost_fade = (float)(COLOR_FOLLOW_LOST_TIMEOUT_MS - lost_ms) /
                    (float)(COLOR_FOLLOW_LOST_TIMEOUT_MS -
                            COLOR_FOLLOW_LOST_HOLD_MS);
        *speed_mps = g_color_vision_follow.lost_fade_start_speed * lost_fade;
        g_color_vision_follow.integrated_speed = *speed_mps;
        g_color_vision_follow.last_valid_speed_cmd = *speed_mps;
    }
    else
    {
        if (lost_ms > COLOR_FOLLOW_ACCEL_FRESH_MS)
        {
            g_color_vision_follow.held_accel_cmd_mps2 = 0.0f;
        }
        *speed_mps = color_vision_follow_integrate_speed();
        g_color_vision_follow.last_valid_speed_cmd = *speed_mps;
    }

    *wheel_angle_deg = g_color_vision_follow.wheel_angle_cmd;
/*    // 【修改点 B】：新增弯道/大打角自动降速逻辑
        abs_angle = fabsf(*wheel_angle_deg);
        if (abs_angle > 10.0f) // 打角超过 10 度开始降速
        {
            speed_factor = 1.0f - 0.02f * (abs_angle - 10.0f);
            if (speed_factor < 0.60f)
            {
                speed_factor = 0.60f; // 最多降速到 60%
            }
            *speed_mps = (*speed_mps) * speed_factor; // 压低最终输出速度
        }*/
    g_color_vision_follow.speed_cmd = *speed_mps;
    g_color_vision_follow.command_valid = 1U;
    return 1U;
}

uint8 color_vision_follow_get_debug(color_vision_follow_debug_t *debug)
{
    if (debug == NULL)
    {
        return 0U;
    }

    debug->calibrated = (uint8)COLOR_FOLLOW_GEOMETRY_CALIBRATED;
    debug->enabled = g_color_vision_follow.enabled;
    debug->command_valid = g_color_vision_follow.command_valid;
    debug->camera_range_m = g_color_vision_follow.camera_range_m;
    debug->lookahead_m = g_color_vision_follow.lookahead_m;
    debug->relative_speed_mps =
        g_color_vision_follow.relative_speed_filtered_mps;
    debug->accel_cmd_mps2 = g_color_vision_follow.held_accel_cmd_mps2;
    debug->target_x_m = g_color_vision_follow.target_x_m;
    debug->target_y_m = g_color_vision_follow.target_y_m;
    debug->speed_mps = g_color_vision_follow.speed_cmd;
    debug->wheel_angle_deg = g_color_vision_follow.wheel_angle_cmd;
    return 1U;
}

void color_vision_diagnosis_publish(float actual_wheel_angle_deg,
                                    float actual_speed_mps,
                                    float yaw_rate_dps)
{
    color_vision_result_t result;
    color_vision_follow_debug_t follow_debug;
    color_vision_diagnosis_t diagnosis;
    uint32 age_ms = 0U;
    uint8 result_ok;

    memset((void *)&result, 0, sizeof(result));
    memset((void *)&follow_debug, 0, sizeof(follow_debug));
    memset((void *)&diagnosis, 0, sizeof(diagnosis));

    result_ok = color_vision_get_result(&result);
    (void)color_vision_follow_get_debug(&follow_debug);

    if (result_ok && (result.frame_id != 0U))
    {
        age_ms = g_system_time_ms - result.timestamp_ms;
        if (result.valid)
        {
            g_color_vision_last_raw_error_px =
                (float)result.center_x - COLOR_FOLLOW_CAMERA_CX_PX;
            diagnosis.vision_age_ms = (float)age_ms;
        }
        else
        {
            /* Preserve the last valid centre instead of creating a false
             * -CX jump; negative age explicitly marks this sample invalid. */
            diagnosis.vision_age_ms = -((float)age_ms + 1.0f);
        }
    }
    else
    {
        diagnosis.vision_age_ms = -1.0f;
    }

    diagnosis.raw_center_error_px = g_color_vision_last_raw_error_px;
    diagnosis.target_y_m = follow_debug.target_y_m;
    diagnosis.target_wheel_angle_deg = follow_debug.wheel_angle_deg;
    diagnosis.actual_wheel_angle_deg = actual_wheel_angle_deg;
    diagnosis.actual_speed_mps = actual_speed_mps;
    diagnosis.yaw_rate_dps = yaw_rate_dps;

    g_color_vision_diagnosis_shared.guard++;
    __dsync();
    g_color_vision_diagnosis_shared.data.raw_center_error_px =
        diagnosis.raw_center_error_px;
    g_color_vision_diagnosis_shared.data.target_y_m = diagnosis.target_y_m;
    g_color_vision_diagnosis_shared.data.target_wheel_angle_deg =
        diagnosis.target_wheel_angle_deg;
    g_color_vision_diagnosis_shared.data.actual_wheel_angle_deg =
        diagnosis.actual_wheel_angle_deg;
    g_color_vision_diagnosis_shared.data.actual_speed_mps =
        diagnosis.actual_speed_mps;
    g_color_vision_diagnosis_shared.data.yaw_rate_dps =
        diagnosis.yaw_rate_dps;
    g_color_vision_diagnosis_shared.data.vision_age_ms =
        diagnosis.vision_age_ms;
    __dsync();
    g_color_vision_diagnosis_shared.guard++;
}

uint8 color_vision_diagnosis_get_snapshot(
    color_vision_diagnosis_t *diagnosis)
{
    uint8 attempt;

    if (diagnosis == NULL)
    {
        return 0U;
    }

    for (attempt = 0U; attempt < 4U; attempt++)
    {
        uint32 guard_before = g_color_vision_diagnosis_shared.guard;
        uint32 guard_after;

        if (guard_before & 1U)
        {
            continue;
        }

        __dsync();
        diagnosis->raw_center_error_px =
            g_color_vision_diagnosis_shared.data.raw_center_error_px;
        diagnosis->target_y_m =
            g_color_vision_diagnosis_shared.data.target_y_m;
        diagnosis->target_wheel_angle_deg =
            g_color_vision_diagnosis_shared.data.target_wheel_angle_deg;
        diagnosis->actual_wheel_angle_deg =
            g_color_vision_diagnosis_shared.data.actual_wheel_angle_deg;
        diagnosis->actual_speed_mps =
            g_color_vision_diagnosis_shared.data.actual_speed_mps;
        diagnosis->yaw_rate_dps =
            g_color_vision_diagnosis_shared.data.yaw_rate_dps;
        diagnosis->vision_age_ms =
            g_color_vision_diagnosis_shared.data.vision_age_ms;
        __dsync();

        guard_after = g_color_vision_diagnosis_shared.guard;
        if ((guard_before == guard_after) && !(guard_after & 1U))
        {
            return 1U;
        }
    }

    return 0U;
}

color_vision_camera_state_t color_vision_get_camera_state(void)
{
    return (color_vision_camera_state_t)g_color_vision_shared.camera_state;
}

static uint8 color_vision_debug_acquire(void)
{
    uint16 wait_ms;

    /* Do not accept an acknowledgement left over from the previous draw. */
    g_color_vision_shared.debug_hold_request = 0U;
    __dsync();
    for (wait_ms = 0U;
         (wait_ms < COLOR_VISION_DEBUG_HOLD_MS) &&
         g_color_vision_shared.debug_hold_ack;
         wait_ms++)
    {
        system_delay_ms(1U);
    }

    g_color_vision_shared.debug_hold_request = 1U;
    __dsync();
    for (wait_ms = 0U; wait_ms < COLOR_VISION_DEBUG_HOLD_MS; wait_ms++)
    {
        if (g_color_vision_shared.debug_hold_ack)
        {
            __dsync();
            return 1U;
        }
        system_delay_ms(1U);
    }

    g_color_vision_shared.debug_hold_request = 0U;
    __dsync();
    return 0U;
}

static void color_vision_debug_release(void)
{
    g_color_vision_shared.debug_hold_request = 0U;
    __dsync();
}

static void color_vision_draw_box(const color_vision_result_t *result,
                                  uint16 origin_y,
                                  uint16 scale)
{
    uint16 x_min = (uint16)(result->x_min * scale);
    uint16 x_max = (uint16)(result->x_max * scale);
    uint16 y_min = (uint16)(origin_y + (result->y_min * scale));
    uint16 y_max = (uint16)(origin_y + (result->y_max * scale));
    uint16 center_x = (uint16)(result->center_x * scale);
    uint16 center_y = (uint16)(origin_y + (result->center_y * scale));
    uint16 cross_radius = (uint16)(3U * scale);
    uint16 cross_left = (center_x > cross_radius) ?
                        (uint16)(center_x - cross_radius) : 0U;
    uint16 cross_top = (center_y > (origin_y + cross_radius)) ?
                       (uint16)(center_y - cross_radius) : origin_y;
    uint16 cross_right = (uint16)(center_x + cross_radius);
    uint16 cross_bottom = (uint16)(center_y + cross_radius);
    uint16 image_x_max = (uint16)((COLOR_VISION_FRAME_W * scale) - 1U);
    uint16 image_y_max = (uint16)(origin_y +
                                  (COLOR_VISION_FRAME_H * scale) - 1U);

    if (cross_right > image_x_max)
    {
        cross_right = image_x_max;
    }
    if (cross_bottom > image_y_max)
    {
        cross_bottom = image_y_max;
    }

    ips200_draw_line(x_min, y_min, x_max, y_min, RGB565_GREEN);
    ips200_draw_line(x_min, y_max, x_max, y_max, RGB565_GREEN);
    ips200_draw_line(x_min, y_min, x_min, y_max, RGB565_GREEN);
    ips200_draw_line(x_max, y_min, x_max, y_max, RGB565_GREEN);
    ips200_draw_line(cross_left, center_y, cross_right, center_y, RGB565_RED);
    ips200_draw_line(center_x, cross_top, center_x, cross_bottom, RGB565_RED);
}

static void color_vision_show_status_line(uint16 y, const char *text)
{
    char display[COLOR_VISION_STATUS_COLUMNS + 1U];
    uint8 index = 0U;

    while ((index < COLOR_VISION_STATUS_COLUMNS) && ('\0' != text[index]))
    {
        display[index] = text[index];
        index++;
    }
    while (index < COLOR_VISION_STATUS_COLUMNS)
    {
        display[index] = ' ';
        index++;
    }
    display[COLOR_VISION_STATUS_COLUMNS] = '\0';

    /* 29 columns use pixels 0..231 with the default 8x16 font. */
    ips200_show_string(0U, y, display);
}

static int32 color_vision_debug_millimetres(float value)
{
    if (color_vision_follow_float_invalid(value))
    {
        return 0;
    }
    return (int32)(value * 1000.0f);
}

static int32 color_vision_debug_tenths(float value)
{
    if (color_vision_follow_float_invalid(value))
    {
        return 0;
    }
    return (int32)(value * 10.0f);
}

static void color_vision_show_unavailable(const char *state,
                                          const char *status)
{
    ips200_show_string(164U, 24U, state);
    ips200_show_string(164U, 84U, "R---");
    ips200_show_string(200U, 84U, "G---");
    ips200_show_string(164U, 102U, "B---");
    ips200_show_string(164U, 144U, "H---");
    ips200_show_string(164U, 162U, "S---");
    ips200_show_string(164U, 180U, "V---");
    color_vision_show_status_line(244U, status);
    color_vision_show_status_line(262U, "");
    color_vision_show_status_line(280U, "");
    color_vision_show_status_line(298U, "");
}

void color_vision_debug_begin(void)
{
    ips200_clear();
    ips200_show_string(168U, 4U, "RAW");
    ips200_show_string(168U, 124U, "MASK");
    ips200_show_string(164U, 44U, "KEY4");
    ips200_show_string(164U, 64U, "EXIT");
}

void color_vision_debug_update(void)
{
    color_vision_result_t result;
    color_vision_follow_debug_t follow_debug;
    color_vision_camera_state_t camera_state = color_vision_get_camera_state();
    const uint16 *frame_address;
    char line[48];
    char probe_line[12];
    int32 range_mm;
    int32 target_x_mm;
    int32 target_y_mm;
    int32 speed_mmps;
    int32 wheel_angle_tenths;

    if (!color_vision_debug_acquire())
    {
        color_vision_show_unavailable("STALE", "CPU1 VISION TIMEOUT");
        return;
    }

    if (!color_vision_get_result(&result) || (result.frame_id == 0U))
    {
        if (camera_state == COLOR_VISION_CAMERA_ERROR)
        {
            color_vision_show_unavailable("ERROR", "SCC8660 INIT ERROR");
        }
        else
        {
            color_vision_show_unavailable("WAIT ", "WAITING FOR CAMERA FRAME");
        }
        color_vision_debug_release();
        return;
    }

    frame_address = (const uint16 *)IFXCPU_GLB_ADDR_DSPR(
        2U, (uint32)&g_color_vision_frame[0][0]);
    ips200_show_rgb565_image(0U, 0U,
                            frame_address,
                            COLOR_VISION_FRAME_W,
                            COLOR_VISION_FRAME_H,
                            COLOR_VISION_FRAME_W,
                            COLOR_VISION_FRAME_H,
                            1U);
    ips200_show_rgb565_image(0U, 120U,
                            &g_color_vision_mask_preview[0][0],
                            COLOR_VISION_MASK_PREVIEW_W,
                            COLOR_VISION_MASK_PREVIEW_H,
                            COLOR_VISION_FRAME_W,
                            COLOR_VISION_FRAME_H,
                            0U);

    ips200_draw_line((COLOR_VISION_FRAME_W / 2U) - 3U,
                     COLOR_VISION_FRAME_H / 2U,
                     (COLOR_VISION_FRAME_W / 2U) + 3U,
                     COLOR_VISION_FRAME_H / 2U,
                     RGB565_CYAN);
    ips200_draw_line(COLOR_VISION_FRAME_W / 2U,
                     (COLOR_VISION_FRAME_H / 2U) - 3U,
                     COLOR_VISION_FRAME_W / 2U,
                     (COLOR_VISION_FRAME_H / 2U) + 3U,
                     RGB565_CYAN);

    if (result.valid)
    {
        color_vision_draw_box(&result, 0U, 1U);
        /* The 80x60 mask is enlarged to 160x120, so coordinates stay 1:1. */
        color_vision_draw_box(&result, 120U, 1U);
        ips200_show_string(164U, 24U, "LOCK ");
    }
    else
    {
        ips200_show_string(164U, 24U, "LOST ");
    }

    snprintf(probe_line, sizeof(probe_line), "R%03u",
             (unsigned int)result.probe_r);
    ips200_show_string(164U, 84U, probe_line);
    snprintf(probe_line, sizeof(probe_line), "G%03u",
             (unsigned int)result.probe_g);
    ips200_show_string(200U, 84U, probe_line);
    snprintf(probe_line, sizeof(probe_line), "B%03u",
             (unsigned int)result.probe_b);
    ips200_show_string(164U, 102U, probe_line);
    snprintf(probe_line, sizeof(probe_line), "H%03u",
             (unsigned int)result.probe_h);
    ips200_show_string(164U, 144U, probe_line);
    snprintf(probe_line, sizeof(probe_line), "S%03u",
             (unsigned int)result.probe_s);
    ips200_show_string(164U, 162U, probe_line);
    snprintf(probe_line, sizeof(probe_line), "V%03u",
             (unsigned int)result.probe_v);
    ips200_show_string(164U, 180U, probe_line);

    snprintf(line, sizeof(line), "F:%lu A:%lu",
             (unsigned long)result.frame_id,
             (unsigned long)result.area);
    color_vision_show_status_line(244U, line);
    snprintf(line, sizeof(line), "C:%03u,%03u WH:%03u,%03u",
             (unsigned int)result.center_x, (unsigned int)result.center_y,
             (unsigned int)result.width, (unsigned int)result.height);
    color_vision_show_status_line(262U, line);
    snprintf(line, sizeof(line), "P:%03u ms AGE:%04lu ms",
             (unsigned int)result.processing_ms,
             (unsigned long)(g_system_time_ms - result.timestamp_ms));
    color_vision_show_status_line(280U, line);

    color_vision_follow_get_debug(&follow_debug);
    range_mm = color_vision_debug_millimetres(follow_debug.camera_range_m);
    target_x_mm = color_vision_debug_millimetres(follow_debug.target_x_m);
    target_y_mm = color_vision_debug_millimetres(follow_debug.target_y_m);
    speed_mmps = color_vision_debug_millimetres(follow_debug.speed_mps);
    wheel_angle_tenths =
        color_vision_debug_tenths(follow_debug.wheel_angle_deg);

    if (follow_debug.enabled)
    {
        snprintf(line, sizeof(line), "Z%ld X%ld Y%ld mm",
                 (long)range_mm,
                 (long)target_x_mm,
                 (long)target_y_mm);
        color_vision_show_status_line(280U, line);
        if (follow_debug.calibrated)
        {
            snprintf(line, sizeof(line), "V%ld D%ld/10",
                     (long)speed_mmps,
                     (long)wheel_angle_tenths);
        }
        else
        {
            snprintf(line, sizeof(line), "PP CAL=0 SET HEADER");
        }
        color_vision_show_status_line(298U, line);
    }
    else
    {
        snprintf(line, sizeof(line), "H%03u-%03u S%03u V%03u",
                 (unsigned int)COLOR_VISION_GREEN_H_MIN,
                 (unsigned int)COLOR_VISION_GREEN_H_MAX,
                 (unsigned int)COLOR_VISION_GREEN_S_MIN,
                 (unsigned int)COLOR_VISION_GREEN_V_MIN);
        color_vision_show_status_line(298U, line);
    }

    color_vision_debug_release();
}

void color_vision_debug_end(void)
{
    color_vision_debug_release();
    ips200_clear();
}
