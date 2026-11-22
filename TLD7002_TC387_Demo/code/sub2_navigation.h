#ifndef SUB2_NAVIGATION_H
#define SUB2_NAVIGATION_H

#include "zf_common_typedef.h"
#include "global_vars.h"
/* Subject 2 has its own waypoint state so it cannot collide with Subject 1. */
typedef struct
{
    float x;
    float y;
    float yaw;
    uint8_t isValid;
} Waypoint_t;

typedef enum
{
    WP_START = 0,
    WP_GATE1_L,
    WP_GATE1,
    WP_GATE2,
    WP_GATE3,
    WP_GATE3_R,
    WP_ACTION,
    WP_PARK,
    WP_NUM
} WaypointIndex_e;

extern Waypoint_t g_sub2_waypoints[WP_NUM];
extern volatile uint8 g_ins_mode;
extern volatile uint8 g_wp_arrived;
extern volatile float g_wp_target_x;
extern volatile float g_wp_target_y;
extern volatile float g_wp_target_yaw;
extern volatile WaypointIndex_e g_target_wp_idx;
extern volatile float g_start_mileage_task;
extern volatile float g_start_yaw;
extern volatile uint32 g_start_time;
extern volatile uint8 g_sub2_stage;
extern volatile uint8 g_voice_soft_trigger;

/*
 * Subject 2 motion geometry. These values are intentionally isolated from
 * the Subject 1 path replay constants in ins.h.
 */
#define SUB2_STRAIGHT_DISTANCE_M       10.0f
#define SUB2_STOP_DISTANCE_THRESHOLD_M  0.12f /* TLD7002 pure-PP value */
#define SUB2_SNAKE_PATH_DISTANCE_M     10.0f
#define SUB2_SNAKE_FORWARD_LENGTH_M     8.5f
#define SUB2_SNAKE_LATERAL_GAIN_M       1.5f
#define SUB2_SNAKE_LOOKAHEAD_M          0.75f
#define SUB2_CIRCLE_RADIUS_M            3.2f

#define SUB2_WP_PREAPPROACH_M           1.2f
#define SUB2_PARK_PREAPPROACH_M         0.60f
#define SUB2_WP_PREAPPROACH_REACHED_M   0.60f
#define SUB2_WP_POSITION_TOLERANCE_M     0.20f
#define SUB2_WP_EXIT_LOOKAHEAD_M         0.80f
#define SUB2_WP_CORRIDOR_HALF_WIDTH_M    0.45f
#define SUB2_WP_TERMINAL_MAX_TRAVEL_M    2.50f

#define SUB2_CIRCLE_LOOKAHEAD_M          0.80f
#define SUB2_CIRCLE_SPEED_MPS            0.65f
#define SUB2_CIRCLE_RETURN_POS_TOL_M     0.75f
#define SUB2_CIRCLE_RETURN_YAW_TOL_DEG  12.0f
#define SUB2_CIRCLE_MIN_RETURN_DEG      355.0f
#define SUB2_CIRCLE_MAX_DEG             400.0f

void sub2_navigation_stop(void);
void ins_start_waypoint_navigation_ex(float tx, float ty, float target_yaw,
                                      float preapproach_m);
void ins_start_waypoint_navigation(float tx, float ty, float target_yaw);
void ins_update_waypoint_tracking(void);
void ins_sub2_navigation_state_machine(void);
void ins_start_sub2_task(uint8_t mode);

#endif
