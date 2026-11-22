#include "sub2_navigation.h"
#include "sub2_waypoint_store.h"
#include "imu_ahrs.h"
#include "encoder.h"
#include "initialize.h"
#include <math.h>

// Subject 2 owns an independent navigation state. Subject 1 keeps ins.c.
static uint8 g_sub2_is_navigating = 0;
static float g_sub2_start_x = 0.0f;
static float g_sub2_start_y = 0.0f;
static float g_sub2_wp_target_yaw = 0.0f;
static uint8 g_sub2_wp_approach_stage = 0;
static float g_sub2_wp_preapproach_m = SUB2_WP_PREAPPROACH_M;
static float g_sub2_wp_terminal_start_mileage = 0.0f;
static float g_sub2_wp_min_distance = 1000.0f;
static float g_sub2_circle_last_yaw = 0.0f;
static float g_sub2_circle_accumulated_yaw = 0.0f;

static float sub2_angle_diff(float target, float current)
{
    float diff = target - current;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

void sub2_navigation_stop(void)
{
    g_sub2_is_navigating = 0;
    g_ins_mode = 0;
    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.target_ramp = 0.0f;
    ins_lateral_error = 0.0f;
}
// Start legacy point-to-point waypoint navigation.
void ins_start_waypoint_navigation_ex(float tx, float ty, float target_yaw,
                                      float preapproach_m) {
    g_wp_target_x = tx;
    g_wp_target_y = ty;
    g_sub2_wp_target_yaw = target_yaw;
    g_sub2_wp_preapproach_m = preapproach_m;
    if (g_sub2_wp_preapproach_m < SUB2_WP_PREAPPROACH_REACHED_M) {
        g_sub2_wp_preapproach_m = SUB2_WP_PREAPPROACH_REACHED_M;
    }
    if (g_sub2_wp_preapproach_m > 3.0f) g_sub2_wp_preapproach_m = 3.0f;
    g_sub2_wp_approach_stage = 0;
    g_sub2_wp_terminal_start_mileage = encoder_get_total_mileage();
    g_sub2_wp_min_distance = 1000.0f;
    g_wp_arrived = 0;
    g_sub2_is_navigating = 1;
    g_ins_mode = 1;
}

void ins_start_waypoint_navigation(float tx, float ty, float target_yaw) {
    ins_start_waypoint_navigation_ex(tx, ty, target_yaw,
                                     SUB2_WP_PREAPPROACH_M);
}
static float ins_clamp(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

void ins_update_waypoint_tracking(void) {
    if (!g_sub2_is_navigating) return;

    float current_x = imu_ahrs_get_pos_x();
    float current_y = imu_ahrs_get_pos_y();
    float current_yaw = imu_ahrs_get_yaw();
    float final_dx = g_wp_target_x - current_x;
    float final_dy = g_wp_target_y - current_y;
    float final_distance = sqrtf(final_dx * final_dx + final_dy * final_dy);
    float target_yaw_rad = DEG_TO_RAD_ANGLE(g_sub2_wp_target_yaw);
    float heading_x = cosf(target_yaw_rad);
    float heading_y = sinf(target_yaw_rad);
    float nav_x;
    float nav_y;

    if (0U == g_sub2_wp_approach_stage) {
        float pre_x = g_wp_target_x - g_sub2_wp_preapproach_m * heading_x;
        float pre_y = g_wp_target_y - g_sub2_wp_preapproach_m * heading_y;
        float pre_dx = pre_x - current_x;
        float pre_dy = pre_y - current_y;
        float pre_distance = sqrtf(pre_dx * pre_dx + pre_dy * pre_dy);
        uint8 prepoint_passed;

        if (pre_distance < g_sub2_wp_min_distance) g_sub2_wp_min_distance = pre_distance;
        prepoint_passed = (g_sub2_wp_min_distance <= 0.70f) &&
                          (pre_distance >= (g_sub2_wp_min_distance + 0.20f));

        if ((pre_distance <= SUB2_WP_PREAPPROACH_REACHED_M) ||
            (final_distance <= g_sub2_wp_preapproach_m) ||
            prepoint_passed) {
            g_sub2_wp_approach_stage = 1;
            g_sub2_wp_terminal_start_mileage = encoder_get_total_mileage();
            g_sub2_wp_min_distance = final_distance;
        } else {
            nav_x = pre_x;
            nav_y = pre_y;
        }
    }

    if (1U == g_sub2_wp_approach_stage) {
        float forward_error = final_dx * heading_x + final_dy * heading_y;
        float lateral_error = -final_dx * heading_y + final_dy * heading_x;
        float terminal_travel = encoder_get_total_mileage() - g_sub2_wp_terminal_start_mileage;
        uint8 strict_arrival;
        uint8 corridor_arrival;
        uint8 closest_pass;

        if (final_distance < g_sub2_wp_min_distance) g_sub2_wp_min_distance = final_distance;

        // Keep steering as pure PP. Position/corridor completion prevents the
        // car from chasing a passed target just to remove a small yaw error.
        strict_arrival = (final_distance <= SUB2_WP_POSITION_TOLERANCE_M);
        corridor_arrival = (forward_error <= 0.0f) &&
                           (fabsf(lateral_error) <= SUB2_WP_CORRIDOR_HALF_WIDTH_M);
        closest_pass = (g_sub2_wp_min_distance <= SUB2_WP_CORRIDOR_HALF_WIDTH_M) &&
                       (final_distance >= (g_sub2_wp_min_distance + 0.20f));

        if (strict_arrival || corridor_arrival || closest_pass) {
            sub2_navigation_stop();
            g_wp_arrived = 1;
            return;
        }

        if (terminal_travel >= SUB2_WP_TERMINAL_MAX_TRAVEL_M) {
            sub2_navigation_stop();
            if (g_sub2_wp_min_distance <= 0.70f) {
                g_wp_arrived = 1;
            } else {
                g_wp_arrived = 0;
                current_car_mode = MODE_STOP;
            }
            return;
        }

        nav_x = g_wp_target_x + SUB2_WP_EXIT_LOOKAHEAD_M * heading_x;
        nav_y = g_wp_target_y + SUB2_WP_EXIT_LOOKAHEAD_M * heading_y;
    }

    float nav_dx = nav_x - current_x;
    float nav_dy = nav_y - current_y;
    float nav_distance = sqrtf(nav_dx * nav_dx + nav_dy * nav_dy);
    float target_heading = atan2f(nav_dy, nav_dx) * (180.0f / PI);
    float yaw_error = sub2_angle_diff(target_heading, current_yaw);
    float lookahead_dist = ins_clamp(nav_distance, 0.45f, 1.40f);
    float pp_angle_rad = atan2f(2.0f * WHEEL_BASE * sinf(DEG_TO_RAD_ANGLE(yaw_error)),
                                lookahead_dist);

    ins_lateral_error = ins_clamp(pp_angle_rad * (180.0f / PI), -28.0f, 28.0f);
    ins_target_yaw = target_heading;

    ahrs_data_t imu_data = imu_ahrs_get_data();
    ins_gyro_z = imu_data.gyro_z_dps;

    if (0U == g_sub2_wp_approach_stage) {
        g_speed_mgr.target_final = 0.45f;
    } else if (final_distance < 0.70f) {
        g_speed_mgr.target_final = 0.20f;
    } else {
        g_speed_mgr.target_final = 0.32f;
    }
}
static void ins_update_sub2_snake(uint8 reverse, float current_mileage, float current_yaw) {
    float remaining = SUB2_SNAKE_PATH_DISTANCE_M - current_mileage;

    // The path has a zero-slope endpoint, so PP naturally restores the start
    // yaw. Stop by distance instead of passing the endpoint to force yaw.
    if (current_mileage >= (SUB2_SNAKE_PATH_DISTANCE_M - SUB2_STOP_DISTANCE_THRESHOLD_M)) {
        sub2_navigation_stop();
        current_car_mode = MODE_STOP;
        return;
    }

    float target_progress = current_mileage + SUB2_SNAKE_LOOKAHEAD_M;
    target_progress = ins_clamp(target_progress, 0.0f, SUB2_SNAKE_PATH_DISTANCE_M);
    float u = target_progress / SUB2_SNAKE_PATH_DISTANCE_M;
    float longitudinal = SUB2_SNAKE_FORWARD_LENGTH_M * u;
    float lateral = SUB2_SNAKE_LATERAL_GAIN_M *
                    sinf(2.0f * PI * u) * sinf(PI * u);

    if (reverse) longitudinal = -longitudinal;

    float start_yaw_rad = DEG_TO_RAD_ANGLE(g_start_yaw);
    float target_x = g_sub2_start_x + longitudinal * cosf(start_yaw_rad) - lateral * sinf(start_yaw_rad);
    float target_y = g_sub2_start_y + longitudinal * sinf(start_yaw_rad) + lateral * cosf(start_yaw_rad);
    float dx = target_x - imu_ahrs_get_pos_x();
    float dy = target_y - imu_ahrs_get_pos_y();
    float target_travel_yaw = atan2f(dy, dx) * (180.0f / PI);
    float current_travel_yaw = current_yaw + (reverse ? 180.0f : 0.0f);
    float alpha = sub2_angle_diff(target_travel_yaw, current_travel_yaw);
    float lookahead_dist = ins_clamp(sqrtf(dx * dx + dy * dy), 0.35f, 1.20f);
    float steer_angle = atan2f(2.0f * WHEEL_BASE * sinf(DEG_TO_RAD_ANGLE(alpha)),
                               lookahead_dist) * (180.0f / PI);

    if (reverse) steer_angle = -steer_angle;
    ins_lateral_error = ins_clamp(steer_angle, -30.0f, 30.0f);
    ins_target_yaw = target_travel_yaw + (reverse ? -180.0f : 0.0f);

    float base_speed = reverse ? 0.60f : 0.80f;
    if (remaining < 1.50f) {
        base_speed *= ins_clamp(remaining / 1.50f, 0.42f, 1.0f);
    }
    g_speed_mgr.target_final = reverse ? -base_speed : base_speed;
}
static void ins_update_sub2_circle(uint8 counter_clockwise, float current_yaw) {
    float direction = counter_clockwise ? 1.0f : -1.0f;
    float start_yaw_rad = DEG_TO_RAD_ANGLE(g_start_yaw);
    float center_x = g_sub2_start_x - direction * SUB2_CIRCLE_RADIUS_M * sinf(start_yaw_rad);
    float center_y = g_sub2_start_y + direction * SUB2_CIRCLE_RADIUS_M * cosf(start_yaw_rad);
    float current_x = imu_ahrs_get_pos_x();
    float current_y = imu_ahrs_get_pos_y();
    float radial_angle = atan2f(current_y - center_y, current_x - center_x);
    float lookahead_angle = radial_angle + direction *
                            (SUB2_CIRCLE_LOOKAHEAD_M / SUB2_CIRCLE_RADIUS_M);
    float target_x = center_x + SUB2_CIRCLE_RADIUS_M * cosf(lookahead_angle);
    float target_y = center_y + SUB2_CIRCLE_RADIUS_M * sinf(lookahead_angle);
    float dx = target_x - current_x;
    float dy = target_y - current_y;
    float target_heading = atan2f(dy, dx) * (180.0f / PI);
    float alpha = sub2_angle_diff(target_heading, current_yaw);
    float target_distance = ins_clamp(sqrtf(dx * dx + dy * dy), 0.45f, 1.20f);
    float steer_angle = atan2f(2.0f * WHEEL_BASE * sinf(DEG_TO_RAD_ANGLE(alpha)),
                               target_distance) * (180.0f / PI);

    if (counter_clockwise) {
        steer_angle = ins_clamp(steer_angle, 4.0f, 25.0f);
    } else {
        steer_angle = ins_clamp(steer_angle, -25.0f, -4.0f);
    }

    ins_lateral_error = steer_angle;
    ins_target_yaw = target_heading;
    g_speed_mgr.target_final = SUB2_CIRCLE_SPEED_MPS;

    float yaw_step = fabsf(sub2_angle_diff(current_yaw, g_sub2_circle_last_yaw));
    if (yaw_step < 15.0f) g_sub2_circle_accumulated_yaw += yaw_step;
    g_sub2_circle_last_yaw = current_yaw;

    float start_dx = current_x - g_sub2_start_x;
    float start_dy = current_y - g_sub2_start_y;
    float start_distance = sqrtf(start_dx * start_dx + start_dy * start_dy);
    float start_yaw_error = fabsf(sub2_angle_diff(g_start_yaw, current_yaw));

    if (((g_sub2_circle_accumulated_yaw >= SUB2_CIRCLE_MIN_RETURN_DEG) &&
         (start_distance <= SUB2_CIRCLE_RETURN_POS_TOL_M) &&
         (start_yaw_error <= SUB2_CIRCLE_RETURN_YAW_TOL_DEG)) ||
        (g_sub2_circle_accumulated_yaw >= SUB2_CIRCLE_MAX_DEG)) {
        sub2_navigation_stop();
        current_car_mode = MODE_STOP;
    }
}

void ins_sub2_navigation_state_machine(void) {
    if (!g_sub2_is_navigating) return;

    float current_mileage = encoder_get_total_mileage() - g_start_mileage_task;
    float current_yaw = imu_ahrs_get_yaw();

    switch (g_ins_mode) {
        // --------------------------------------------------
        case 1:
           ins_update_waypoint_tracking();
           if (g_wp_arrived && g_sub2_stage == 1)
           {
               g_sub2_stage = 2;
               ins_start_waypoint_navigation(g_sub2_waypoints[WP_ACTION].x, g_sub2_waypoints[WP_ACTION].y, g_sub2_waypoints[WP_ACTION].yaw);
           }
           else if (g_wp_arrived && g_sub2_stage == 2)
           {
               sub2_navigation_stop();
               current_car_mode = MODE_STOP;
           }
           else if (g_wp_arrived && g_sub2_stage == 3)
           {
               if (g_sub2_waypoints[WP_PARK].isValid != 1U)
               {
                   g_wp_arrived = 0;
                   sub2_navigation_stop();
                   current_car_mode = MODE_STOP;
                   return;
               }

               g_sub2_stage = 4;
               ins_start_waypoint_navigation_ex(g_sub2_waypoints[WP_PARK].x,
                                                 g_sub2_waypoints[WP_PARK].y,
                                                 g_sub2_waypoints[WP_PARK].yaw,
                                                 SUB2_PARK_PREAPPROACH_M);
           }
           else if (g_wp_arrived && g_sub2_stage == 4)
           {
                  sub2_navigation_stop();
                  current_car_mode = MODE_STOP;
           }
                    break;
        // --------------------------------------------------
        // --------------------------------------------------
        case 2: {
            float remaining = SUB2_STRAIGHT_DISTANCE_M - current_mileage;

            if (remaining <= 1.5f) {
                float app_speed = ins_clamp(remaining / 1.5f, 0.25f, 1.0f);
                g_speed_mgr.target_final = app_speed;
            } else {
                g_speed_mgr.target_final = 1.0f;
            }

            if (current_mileage >= SUB2_STRAIGHT_DISTANCE_M - SUB2_STOP_DISTANCE_THRESHOLD_M) {
                sub2_navigation_stop();
                current_car_mode = MODE_STOP;
                return;
            }

            float yaw_error = sub2_angle_diff(g_start_yaw, current_yaw);
            ins_lateral_error = ins_clamp(yaw_error * 1.5f, -15.0f, 15.0f);
            break;
        }

        case 3: {
            float remaining = SUB2_STRAIGHT_DISTANCE_M - current_mileage;

            if (remaining <= 1.5f) {
                float app_speed = 0.60f * ins_clamp(remaining / 1.5f, 0.42f, 1.0f);
                g_speed_mgr.target_final = -app_speed;
            } else {
                g_speed_mgr.target_final = -0.60f;
            }

            if (current_mileage >= SUB2_STRAIGHT_DISTANCE_M - SUB2_STOP_DISTANCE_THRESHOLD_M) {
                sub2_navigation_stop();
                current_car_mode = MODE_STOP;
                return;
            }

            float yaw_error = sub2_angle_diff(g_start_yaw, current_yaw);
            ins_lateral_error = ins_clamp(-(yaw_error * 1.5f), -15.0f, 15.0f);
            break;
        }

        case 4:
            ins_update_sub2_snake(0U, current_mileage, current_yaw);
            break;

        case 5:
            ins_update_sub2_snake(1U, current_mileage, current_yaw);
            break;
        case 6:
            ins_update_sub2_circle(1U, current_yaw);
            break;

        case 7:
            ins_update_sub2_circle(0U, current_yaw);
            break;
        case 8: {
            if (g_sub2_stage == 1) {
                g_speed_mgr.target_final = 0.8f;
                float yaw_error = sub2_angle_diff(g_start_yaw, current_yaw);
                ins_lateral_error = yaw_error * 1.5f;

                if (current_mileage >= 2.1f) {
                    g_sub2_stage = 2;
                    g_start_mileage_task = encoder_get_total_mileage();
                }
            }
            else if (g_sub2_stage == 2) {
                float target_yaw = sub2_angle_diff(g_start_yaw, -90.0f);
                g_speed_mgr.target_final = 0.5f;

                float yaw_error = sub2_angle_diff(target_yaw, current_yaw);
                float steer_angle = yaw_error * 1.5f;
                if (steer_angle > 40.0f)  steer_angle = 40.0f;
                if (steer_angle < -40.0f) steer_angle = -40.0f;
                ins_lateral_error = steer_angle;

                if (fabsf(yaw_error) < 2.0f) {
                    sub2_navigation_stop();
                    current_car_mode = MODE_STOP;
                }
            }
            break;
        }

        // --------------------------------------------------
        case 9: {
            if (g_sub2_stage == 1) {
                g_speed_mgr.target_final = 0.8f;
                float yaw_error = sub2_angle_diff(g_start_yaw, current_yaw);
                ins_lateral_error = yaw_error * 1.5f;

                if (current_mileage >= 2.1f) {
                    g_sub2_stage = 2;
                    g_start_mileage_task = encoder_get_total_mileage();
                }
            }
            else if (g_sub2_stage == 2) {
                float target_yaw = sub2_angle_diff(g_start_yaw, 90.0f);
                g_speed_mgr.target_final = 0.5f;

                float yaw_error = sub2_angle_diff(target_yaw, current_yaw);
                float steer_angle = yaw_error * 1.5f;
                if (steer_angle > 40.0f)  steer_angle = 40.0f;
                if (steer_angle < -40.0f) steer_angle = -40.0f;
                ins_lateral_error = steer_angle;

                if (fabsf(yaw_error) < 2.0f) {
                    sub2_navigation_stop();
                    current_car_mode = MODE_STOP;
                }
            }
            break;
        }

        default:
            break;
    }
}


void ins_start_sub2_task(uint8_t mode) {
    g_ins_mode = mode;
    g_start_mileage_task = encoder_get_total_mileage();
    g_start_yaw = imu_ahrs_get_yaw();
    g_start_time = g_system_time_ms;
    g_sub2_start_x = imu_ahrs_get_pos_x();
    g_sub2_start_y = imu_ahrs_get_pos_y();
    g_sub2_circle_last_yaw = g_start_yaw;
    g_sub2_circle_accumulated_yaw = 0.0f;
    g_wp_arrived = 0;
    g_sub2_stage = 0;
    g_sub2_is_navigating = 1;
}
