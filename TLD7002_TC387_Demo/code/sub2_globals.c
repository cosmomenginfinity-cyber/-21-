#include "sub2_navigation.h"

Waypoint_t g_sub2_waypoints[WP_NUM];
volatile uint8 g_ins_mode = 0;
volatile uint8 g_wp_arrived = 0;
volatile float g_wp_target_x = 0.0f;
volatile float g_wp_target_y = 0.0f;
volatile float g_wp_target_yaw = 0.0f;
volatile WaypointIndex_e g_target_wp_idx = WP_GATE1;
volatile float g_start_mileage_task = 0.0f;
volatile float g_start_yaw = 0.0f;
volatile uint32 g_start_time = 0;
volatile uint8 g_sub2_stage = 0;
volatile uint8 g_voice_soft_trigger = 0;
