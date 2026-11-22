#include "menu.h"

#include "back_ins.h"
#include "asr_audio.h"
#include "color_vision.h"
#include "encoder.h"
#include "global_vars.h"
#include "imu_ahrs.h"
#include "imu_datalogger.h"
#include "initialize.h"
#include "sub3_build_mode.h"
#include "motor.h"
#include "pathrecorder.h"
#include "PID.h"

#include <stdio.h>

#define SUB3_MENU_ITEM_COUNT             (9U)
#define SUB3_FOLLOW_SPEED_STEP_MPS       (0.5f) // 跟随速度上下调节步长 0.5m/s
#define SUB3_FOLLOW_SPEED_MIN_MPS        (0.2f) // 下限 0.2m/s
#define SUB3_FOLLOW_SPEED_MAX_MPS        (5.0f) // 上限 5.0m/s
#define SUB3_SPEED_STEP_MPS              (0.2f)
#define SUB3_RC_SPEED_STEP_MPS           (0.1f)
#define SUB3_RC_SPEED_MIN_MPS            (0.2f)
#define SUB3_RC_SPEED_MAX_MPS            (7.0f)
#define SUB3_RETURN_SPEED_MIN_MPS        (0.5f)
#define SUB3_RETURN_SPEED_MAX_MPS        (7.0f)

#define SUB3_RC_TIMEOUT_MS               (500U)
#define SUB3_RETURN_TRIGGER_VALUE        (192U)
#define SUB3_RETURN_STOP_SPEED_MPS       (0.10f)
#define SUB3_RETURN_STOP_STABLE_TICKS    (8U)
#define SUB3_RETURN_STOP_TIMEOUT_MS      (2500U)
#define SUB3_RETURN_DISPLAY_TICKS        (20U)

typedef enum
{
    SUB3_MENU_ACTION_TASK = 0,
    SUB3_MENU_PARKING_TASK,
    SUB3_MENU_REMOTE_CONTROL,
    SUB3_MENU_RC_SPEED,
    SUB3_MENU_STATIC_VISION,
    SUB3_MENU_FOLLOW_SPEED,
    SUB3_MENU_RETURN_SPEED,
    SUB3_MENU_STRAIGHT_SPEED,
    SUB3_MENU_SERVO_CENTER
} sub3_menu_item_t;
// 新增速度控制类型枚举：
typedef enum
{
    SUB3_SPEED_TYPE_RC = 0,
    SUB3_SPEED_TYPE_RETURN,
    SUB3_SPEED_TYPE_STRAIGHT,
    SUB3_SPEED_TYPE_FOLLOW
} sub3_speed_type_t;

typedef enum
{
    SUB3_RC_NO_FRAME = 0,
    SUB3_RC_VALID_FRAME,
    SUB3_RC_FAILSAFE
} sub3_rc_result_t;

static uint8 g_sub3_menu_index = 0U;
static uint8 g_sub3_menu_dirty = 1U;

static void sub3_clear_key_actions(void)
{
    key1_action = 0;
    key2_action = 0;
    key3_action = 0;
    key4_action = 0;
}

static void sub3_stop_vehicle(void)
{
    current_car_mode = MODE_STOP;
    tune_target_speed = 0.0f;
    tune_target_angle = 0.0f;
    g_speed_mgr.target_final = 0.0f;
    g_speed_mgr.target_ramp = 0.0f;
    set_motor_speed(0.0f);
    Motor_Control(0.0f, 0.0f);
}

static sub3_rc_result_t sub3_take_receiver_frame(
    uint8 apply_manual_drive,
    uint16 *switch_value,
    uint32 *last_valid_time)
{
    uint8 receiver_state;
    uint16 receiver_switch;
    uint16 receiver_speed;
    uint16 receiver_angle;

    if (uart_receiver.finsh_flag == 0U)
    {
        return SUB3_RC_NO_FRAME;
    }

    receiver_state = uart_receiver.state;
    receiver_switch = uart_receiver.channel[3];
    receiver_speed = uart_receiver.channel[1];
    receiver_angle = uart_receiver.channel[0];
    uart_receiver.finsh_flag = 0U;

    if (receiver_state != 1U)
    {
        sub3_stop_vehicle();
        return SUB3_RC_FAILSAFE;
    }

    *last_valid_time = g_system_time_ms;
    *switch_value = receiver_switch;

    if (apply_manual_drive)
    {
        tune_target_speed = map_sbus_to_speed(receiver_speed);
        tune_target_angle = map_sbus_to_angle(receiver_angle);
    }

    return SUB3_RC_VALID_FRAME;
}

static uint8 sub3_receiver_timed_out(uint32 last_valid_time)
{
    return (uint8)((g_system_time_ms - last_valid_time) > SUB3_RC_TIMEOUT_MS);
}

static void sub3_draw_main_menu(void)
{
    char line[32];
    /* Fit all nine entries above the bottom key-help line. */
    static const uint16 item_y[SUB3_MENU_ITEM_COUNT] =
    {
        26U, 52U, 78U, 104U, 130U, 156U, 182U, 208U, 234U
    };
    uint8 i;

    ips200_clear();
    ips200_show_string(24U, 4U, "=== COMPETITION ===");
    ips200_show_string(20U, item_y[0], "Action Task");
    ips200_show_string(20U, item_y[1], "Parking Task");
    ips200_show_string(20U, item_y[2], "Remote Control");

    snprintf(line, sizeof(line), "RC Max: %.1f m/s",
             (double)g_rc_max_speed);
    ips200_show_string(20U, item_y[3], line);

    ips200_show_string(20U, item_y[4], "Static Vision");

    snprintf(line, sizeof(line), "Follow: %.1f m/s",
             (double)g_color_follow_max_speed);
    ips200_show_string(20U, item_y[5], line);

    snprintf(line, sizeof(line), "Curve@1.3m: %.1f m/s",
             (double)g_back_curve_limit_speed_mps);
    ips200_show_string(20U, item_y[6], line);

    snprintf(line, sizeof(line), "Straight Max: %.1f m/s",
             (double)g_back_straight_max_speed_mps);
    ips200_show_string(20U, item_y[7], line);

    ips200_show_string(20U, item_y[8], "Servo Center");

    for (i = 0U; i < SUB3_MENU_ITEM_COUNT; i++)
    {
        ips200_show_string(4U, item_y[i],
                           (i == g_sub3_menu_index) ? ">" : " ");
    }

    ips200_show_string(8U, 285U, "KEY1/2 Select  KEY3 Enter");
}

static void sub3_draw_speed_value(const char *title,
                                  float value,
                                  uint8 symmetric,
                                  float step)
{
    char line[32];

    ips200_clear();
    ips200_show_string(16U, 32U, title);
    if (symmetric)
    {
        snprintf(line, sizeof(line), "LIMIT: +/-%.1f m/s", (double)value);
    }
    else
    {
        snprintf(line, sizeof(line), "SPEED: %.1f m/s", (double)value);
    }
    ips200_show_string(24U, 96U, line);
    snprintf(line, sizeof(line), "KEY1 +%.1f  KEY2 -%.1f", (double)step, (double)step);
    ips200_show_string(8U, 160U, line);
    ips200_show_string(8U, 192U, "KEY3 Save   KEY4 Cancel");
}

static void sub3_adjust_speed(sub3_speed_type_t speed_type)
{
    float minimum;
    float maximum;
    float step;
    float value;
    volatile float *setting;
    const char *title;
    uint8 symmetric = 0U;

    if (speed_type == SUB3_SPEED_TYPE_RC)
    {
        setting = &g_rc_max_speed;
        minimum = SUB3_RC_SPEED_MIN_MPS;
        maximum = SUB3_RC_SPEED_MAX_MPS;
        step = SUB3_RC_SPEED_STEP_MPS;
        title = "SET RC SPEED";
        symmetric = 1U;
    }
    else if (speed_type == SUB3_SPEED_TYPE_FOLLOW)
    {
        setting = &g_color_follow_max_speed;
        minimum = SUB3_FOLLOW_SPEED_MIN_MPS;
        maximum = SUB3_FOLLOW_SPEED_MAX_MPS;
        step = SUB3_FOLLOW_SPEED_STEP_MPS; // 上下 0.5 m/s 调节
        title = "SET FOLLOW SPEED";
    }
    else if (speed_type == SUB3_SPEED_TYPE_STRAIGHT)
    {
        setting = &g_back_straight_max_speed_mps;
        minimum = g_back_curve_limit_speed_mps;
        if (minimum < SUB3_RETURN_SPEED_MIN_MPS)
        {
            minimum = SUB3_RETURN_SPEED_MIN_MPS;
        }
        maximum = SUB3_RETURN_SPEED_MAX_MPS;
        step = SUB3_SPEED_STEP_MPS;
        title = "SET STRAIGHT MAX";
    }
    else
    {
        setting = &g_back_curve_limit_speed_mps;
        minimum = SUB3_RETURN_SPEED_MIN_MPS;
        maximum = g_back_straight_max_speed_mps;
        if (maximum > SUB3_RETURN_SPEED_MAX_MPS)
        {
            maximum = SUB3_RETURN_SPEED_MAX_MPS;
        }
        step = SUB3_SPEED_STEP_MPS;
        title = "SET CURVE@1.3M";
    }

    value = *setting;
    sub3_clear_key_actions();
    sub3_draw_speed_value(title, value, symmetric, step);

    while (TRUE)
    {
        key_scan();

        if (key1_action)
        {
            key1_action = 0;
            value += step;
            if (value > maximum)
            {
                value = maximum;
            }
            sub3_draw_speed_value(title, value, symmetric, step);
        }

        if (key2_action)
        {
            key2_action = 0;
            value -= step;
            if (value < minimum)
            {
                value = minimum;
            }
            sub3_draw_speed_value(title, value, symmetric, step);
        }

        if (key3_action)
        {
            key3_action = 0;
            *setting = value;
            ips200_show_string(48U, 240U, "SET FOR THIS RUN");
            system_delay_ms(300U);
            break;
        }

        if (key4_action)
        {
            key4_action = 0;
            break;
        }

        system_delay_ms(10U);
    }
}


static const char *sub3_voice_network_error_text(void)
{
    switch (audio_network_error_code)
    {
        case AUDIO_NET_ERR_WIFI_INIT:      return "E1 WIFI INIT";
        case AUDIO_NET_ERR_GET_TIME:       return "E2 GET TIME";
        case AUDIO_NET_ERR_AUTH_URL:       return "E3 AUTH URL";
        case AUDIO_NET_ERR_WS_CONNECT:     return "E4 WEBSOCKET";
        case AUDIO_NET_ERR_START_SEND:     return "E5 START SEND";
        case AUDIO_NET_ERR_AUDIO_SEND:     return "E6 AUDIO SEND";
        case AUDIO_NET_ERR_FINAL_SEND:     return "E7 FINAL SEND";
        case AUDIO_NET_ERR_RESULT_TIMEOUT: return "E8 RESULT TIMEOUT";
        default:                           return "E0 UNKNOWN";
    }
}

static void sub3_voice_parking_task(void)
{
    static uint8 audio_initialized = 0U;
    uint8 stage = 0U;
    uint8 recording_started = 0U;
    uint8 page_ready = 0U;
    uint8 exit_requested = 0U;
    uint8 countdown_second_shown = 0xFFU;
    uint32 stage_time_ms = g_system_time_ms;
    char countdown_line[32];

    sub3_stop_vehicle();
    sub3_clear_key_actions();
    g_voice_task_active = 1U;
    __dsync();
    ips200_clear();
    ips200_show_string(32U, 48U, "PARKING TASK");

    if (!audio_initialized)
    {
        ips200_show_string(16U, 96U, "VOICE INITIALIZING");
        audio_init();
        audio_initialized = 1U;
    }

    pit_disable(ASR_PIT);
    audio_session_cancel();
    voice_text_clear();
    g_active_voice_cmd = CMD_NONE;
    g_voice_state = VOICE_STATE_IDLE;

    while (!exit_requested)
    {
        key_scan();
        if (key4_action)
        {
            key4_action = 0;
            exit_requested = 1U;
            continue;
        }

        if (stage == 0U)
        {
            if (!page_ready)
            {
                ips200_clear();
                ips200_show_string(32U, 32U, "PARKING TASK");
                ips200_show_string(24U, 96U, "READY TO LISTEN");
                ips200_show_string(24U, 256U, "KEY4: EXIT");
                page_ready = 1U;
                stage_time_ms = g_system_time_ms;
            }

            if ((g_system_time_ms - stage_time_ms) >= 800U)
            {
                voice_text_clear();
                g_active_voice_cmd = CMD_NONE;
                audio_session_begin();
                g_voice_state = VOICE_STATE_LISTENING;
                recording_started = 0U;
                page_ready = 0U;
                stage = 1U;
                ips200_show_string(24U, 96U, "CONNECTING...");
            }
        }
        else if (stage == 1U)
        {
            audio_loop();

            if (audio_network_error_flag)
            {
                ips200_show_string(24U, 128U,
                                   sub3_voice_network_error_text());
                system_delay_ms(1200U);
                stage = 0U;
            }
            else if (!recording_started && audio_server_link_flag)
            {
                recording_started = 1U;
                stage_time_ms = g_system_time_ms;
                countdown_second_shown = 0xFFU;
            }
            else if (recording_started)
            {
                uint32 elapsed_ms = g_system_time_ms - stage_time_ms;
                uint8 seconds_left = (elapsed_ms >= 10000U) ? 0U :
                    (uint8)((10000U - elapsed_ms + 999U) / 1000U);

                if (seconds_left != countdown_second_shown)
                {
                    countdown_second_shown = seconds_left;
                    snprintf(countdown_line, sizeof(countdown_line),
                             "LISTENING: %2u s ",
                             (unsigned int)seconds_left);
                    ips200_show_string(24U, 96U, countdown_line);
                }

                if ((elapsed_ms >= 10000U) && audio_start_flag)
                {
                    ips200_show_string(24U, 96U, "ANALYZING...");
                    audio_session_request_finish();
                }

                if ((audio_start_flag == 0U) &&
                    (audio_need_net_flag == 0U))
                {
                    if (g_active_voice_cmd != CMD_NONE)
                    {
                        g_voice_state = VOICE_STATE_EXECUTING;
                        stage = 2U;
                    }
                    else
                    {
                        ips200_show_string(24U, 128U, "NO COMMAND MATCH");
                        system_delay_ms(1000U);
                        stage = 0U;
                    }
                }
            }
        }
        else
        {
            voice_action_service_cpu0();
            if ((g_voice_state == VOICE_STATE_IDLE) &&
                (g_active_voice_cmd == CMD_NONE) &&
                (g_light_command_sequence == g_light_ack_sequence))
            {
                ips200_show_string(24U, 128U, "VOICE COMPLETE");
                system_delay_ms(600U);
                stage = 0U;
            }
        }

        system_delay_ms(5U);
    }

    audio_session_cancel();
    voice_text_clear();
    g_active_voice_cmd = CMD_NONE;
    voice_light_clear_async();
    g_voice_state = VOICE_STATE_IDLE;
    g_voice_task_active = 0U;
    __dsync();
    sub3_stop_vehicle();
    sub3_clear_key_actions();
}

static void sub3_camera_vision_debug(void)
{
    uint8 exit_requested = 0U;

    sub3_stop_vehicle();
    sub3_clear_key_actions();
    color_vision_debug_begin();

    while (!exit_requested)
    {
        uint8 wait_step;
        color_vision_debug_update();

        /* Keep key debounce near 100 ms even when two images make IPS refresh slow. */
        for (wait_step = 0U; wait_step < 20U; wait_step++)
        {
            key_scan();
            if (key4_action)
            {
                key4_action = 0;
                exit_requested = 1U;
                break;
            }
            system_delay_ms(5U);
        }
    }

    color_vision_debug_end();
    sub3_clear_key_actions();
}

#if 0
static void sub3_camera_follow(void)
{
    uint8 exit_requested = 0U;
    uint8 debug_divider = 0U;

    sub3_stop_vehicle();
    sub3_clear_key_actions();
    color_vision_debug_begin();
    ips200_show_string(164U, 44U, "K2/K4");
    ips200_show_string(164U, 64U, "STOP");
    color_vision_follow_enable();
    current_car_mode = MODE_CAMERA;

    while (!exit_requested)
    {
        /* Full-frame IPS transfer briefly holds CPU2's camera buffers. */
        if (debug_divider == 0U)
        {
            color_vision_debug_update();
        }
        debug_divider++;
        if (debug_divider >= 10U)
        {
            debug_divider = 0U;
        }
        key_scan();

        /* KEY4 exits; KEY2 is also treated as an emergency stop here. */
        if (key4_action || key2_action)
        {
            key4_action = 0;
            key2_action = 0;
            exit_requested = 1U;
        }
        system_delay_ms(10U);
    }

    color_vision_follow_disable();
    color_vision_debug_end();
    sub3_stop_vehicle();
    sub3_clear_key_actions();
}
#endif

static float sub3_absf(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static uint8 sub3_wait_for_recorded_stop(void)
{
    uint32 start_time_ms = g_system_time_ms;
    uint8 stable_ticks = 0U;

    while ((g_system_time_ms - start_time_ms) <
           SUB3_RETURN_STOP_TIMEOUT_MS)
    {
        key_scan();
        path_recorder_update();

        if (key4_action || key2_action)
        {
            key4_action = 0;
            key2_action = 0;
            return 0U;
        }

        if ((sub3_absf(encoder_speed_left) <=
             SUB3_RETURN_STOP_SPEED_MPS) &&
            (sub3_absf(encoder_speed_right) <=
             SUB3_RETURN_STOP_SPEED_MPS))
        {
            stable_ticks++;
            if (stable_ticks >= SUB3_RETURN_STOP_STABLE_TICKS)
            {
                return 1U;
            }
        }
        else
        {
            stable_ticks = 0U;
        }

        system_delay_ms(10U);
    }

    return 0U;
}

static const char *sub3_return_result_text(BackInsResult_e result)
{
    switch (result)
    {
        case BACK_INS_RESULT_SUCCESS:
            return "RETURN COMPLETE";
        case BACK_INS_RESULT_POSE_INVALID:
            return "POSE INVALID STOP";
        case BACK_INS_RESULT_LOST_PATH:
            return "LOST PATH STOP";
        case BACK_INS_RESULT_CORRIDOR_VIOLATION:
            return "CORRIDOR STOP";
        case BACK_INS_RESULT_ALIGNMENT_FAILED:
            return "ALIGNMENT FAILED";
        default:
            return "RETURN ABORTED";
    }
}

static uint8 sub3_run_ram_return(void)
{
    uint8 abort_return = 0U;
    uint8 display_ticks = 0U;
    BackInsResult_e result;
    char line[32];

    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
    encoder_reset_all_data();

    if (!back_ins_start_from_ram())
    {
        ips200_clear();
        ips200_show_string(24U, 96U, "RAM PATH START FAIL");
        system_delay_ms(800U);
        return 0U;
    }

    sub3_clear_key_actions();
    ips200_clear();
    ips200_show_string(32U, 24U, "RAM RETURN");
    ips200_show_string(16U, 256U, "KEY2/4: ABORT");
    current_car_mode = MODE_INS_TRACK_THREE;

    while (back_ins_is_running())
    {
        key_scan();
        if (key4_action || key2_action)
        {
            key4_action = 0;
            key2_action = 0;
            abort_return = 1U;
            break;
        }

        display_ticks++;
        if (display_ticks >= SUB3_RETURN_DISPLAY_TICKS)
        {
            display_ticks = 0U;
            snprintf(line, sizeof(line), "POINT:%lu/%lu ",
                     (unsigned long)back_ins_get_current_idx(),
                     (unsigned long)back_ins_get_total_points());
            ips200_show_string(16U, 80U, line);

            snprintf(line, sizeof(line), "LEFT:%5.2f m ",
                     (double)back_ins_get_remaining_dist());
            ips200_show_string(16U, 120U, line);

            snprintf(line, sizeof(line), "SPEED:%5.2f m/s ",
                     (double)((encoder_speed_left +
                               encoder_speed_right) * 0.5f));
            ips200_show_string(16U, 160U, line);

            snprintf(line, sizeof(line), "XERR:%5.2f m ",
                     (double)back_ins_get_cross_track_error());
            ips200_show_string(16U, 200U, line);
        }

        system_delay_ms(5U);
    }

    if (abort_return)
    {
        back_ins_stop();
    }
    result = back_ins_get_result();
    sub3_stop_vehicle();

    ips200_clear();
    ips200_show_string(16U, 96U,
                       abort_return ? "RETURN ABORTED" :
                                      sub3_return_result_text(result));
    system_delay_ms((result == BACK_INS_RESULT_SUCCESS) ? 300U : 900U);

    return (uint8)((!abort_return) &&
                   (result == BACK_INS_RESULT_SUCCESS));
}

static const char *sub3_blind_stage_text(BackInsBlindStage_e stage)
{
    switch (stage)
    {
        case BACK_INS_BLIND_STAGE_WAIT_FORWARD:
            return "WAIT FORWARD";
        case BACK_INS_BLIND_STAGE_FORWARD_EXIT:
            return "FORWARD EXIT";
        case BACK_INS_BLIND_STAGE_WAIT_REVERSE:
            return "WAIT REVERSE";
        case BACK_INS_BLIND_STAGE_REVERSE_ENTER:
            return "REVERSE ENTER";
        case BACK_INS_BLIND_STAGE_FINAL_STOP:
            return "FINAL STOP";
        case BACK_INS_BLIND_STAGE_COMPLETE:
            return "BLIND COMPLETE";
        default:
            return "BLIND FAILED";
    }
}

static const char *sub3_blind_result_text(BackInsBlindResult_e result)
{
    switch (result)
    {
        case BACK_INS_BLIND_RESULT_SUCCESS:
            return "BLIND COMPLETE";
        case BACK_INS_BLIND_RESULT_PATH_TOO_SHORT:
            return "BLIND PATH SHORT";
        case BACK_INS_BLIND_RESULT_HOME_INVALID:
            return "BLIND HOME INVALID";
        case BACK_INS_BLIND_RESULT_LOST_PATH:
            return "BLIND LOST PATH";
        case BACK_INS_BLIND_RESULT_OVERRUN:
            return "BLIND OVERRUN";
        case BACK_INS_BLIND_RESULT_TIMEOUT:
            return "BLIND TIMEOUT";
        default:
            return "BLIND ABORTED";
    }
}

static uint8 sub3_run_blind_box(void)
{
    uint8 abort_blind = 0U;
    uint8 display_ticks = 0U;
    BackInsBlindResult_e result;
    char line[32];

    sub3_stop_vehicle();
    sub3_clear_key_actions();

    if (!back_ins_blind_start())
    {
        result = back_ins_blind_get_result();
        ips200_clear();
        ips200_show_string(8U, 96U, sub3_blind_result_text(result));
        system_delay_ms(900U);
        return 0U;
    }

    ips200_clear();
    ips200_show_string(32U, 16U, "SOFTWARE BLIND");
    ips200_show_string(16U, 280U, "KEY2/4: ABORT");
    current_car_mode = MODE_INS_TRACK_THREE;

    while (back_ins_blind_is_running())
    {
        key_scan();
        if (key4_action || key2_action)
        {
            key4_action = 0;
            key2_action = 0;
            abort_blind = 1U;
            break;
        }

        display_ticks++;
        if (display_ticks >= SUB3_RETURN_DISPLAY_TICKS)
        {
            display_ticks = 0U;
            ips200_show_string(
                16U,
                56U,
                sub3_blind_stage_text(back_ins_blind_get_stage()));

            snprintf(line, sizeof(line), "LEFT:%5.2f P:%5.2f",
                     (double)back_ins_blind_get_remaining_dist(),
                     (double)back_ins_blind_get_progress());
            ips200_show_string(16U, 96U, line);

            snprintf(line, sizeof(line), "EL:%5.2f ET:%5.2f",
                     (double)back_ins_blind_get_longitudinal_error(),
                     (double)back_ins_blind_get_lateral_error());
            ips200_show_string(16U, 136U, line);

            snprintf(line, sizeof(line), "YAW:%5.1f X:%5.2f",
                     (double)back_ins_blind_get_yaw_error(),
                     (double)back_ins_blind_get_path_error());
            ips200_show_string(16U, 176U, line);

            snprintf(line, sizeof(line), "SPEED:%5.2f m/s ",
                     (double)((encoder_speed_left
                             + encoder_speed_right) * 0.5f));
            ips200_show_string(16U, 216U, line);
        }

        system_delay_ms(5U);
    }

    if (abort_blind)
    {
        back_ins_blind_stop();
    }
    result = back_ins_blind_get_result();
    sub3_stop_vehicle();

    ips200_clear();
    ips200_show_string(
        8U,
        96U,
        abort_blind ? "BLIND ABORTED" : sub3_blind_result_text(result));
    system_delay_ms((result == BACK_INS_BLIND_RESULT_SUCCESS)
                    ? 300U : 900U);

    return (uint8)((!abort_blind)
        && (result == BACK_INS_BLIND_RESULT_SUCCESS));
}

static void sub3_camera_follow(void)
{
    uint8 exit_requested = 0U;
    uint8 return_requested = 0U;
    uint8 return_completed = 0U;
    uint8 display_divider = 0U;
    char line[32];

    sub3_stop_vehicle();
    sub3_clear_key_actions();
    encoder_reset_all_data();
    imu_ahrs_reset_position();
    back_ins_blind_capture_home_pose();
    path_recorder_start();

    if (!path_recorder_is_working())
    {
        ips200_clear();
        ips200_show_string(24U, 96U, "RAM RECORD FAIL");
        system_delay_ms(800U);
        return;
    }

    ips200_clear();
    ips200_show_string(32U, 16U, "ACTION TASK");
    ips200_show_string(16U, 48U, "FOLLOW + RAM RECORD");
    ips200_show_string(8U, 272U, "KEY3: RETURN  K2/4: ABORT");

    color_vision_follow_enable();
    current_car_mode = MODE_CAMERA;

    while (!exit_requested)
    {
        key_scan();
        path_recorder_update();

        if (key3_action)
        {
            key3_action = 0;
            return_requested = 1U;
            exit_requested = 1U;
        }
        else if (key4_action || key2_action)
        {
            key4_action = 0;
            key2_action = 0;
            exit_requested = 1U;
        }

        display_divider++;
        if (display_divider >= 20U)
        {
            color_vision_result_t result = {0};
            color_vision_follow_debug_t follow = {0};

            display_divider = 0U;
            (void)color_vision_get_result(&result);
            (void)color_vision_follow_get_debug(&follow);

            snprintf(line, sizeof(line), "LD:%4.2f R:%4.2f A:%4.2f",
                     (double)follow.lookahead_m,
                     (double)follow.relative_speed_mps,
                     (double)follow.accel_cmd_mps2);
            ips200_show_string(16U, 72U, line);

            snprintf(line, sizeof(line), "AREA:%5lu CX:%3u ",
                     (unsigned long)result.area,
                     (unsigned int)result.center_x);
            ips200_show_string(16U, 96U, line);

            snprintf(line, sizeof(line), "SPEED:%5.2f m/s ",
                     (double)follow.speed_mps);
            ips200_show_string(16U, 128U, line);

            snprintf(line, sizeof(line), "STEER:%6.2f deg ",
                     (double)follow.wheel_angle_deg);
            ips200_show_string(16U, 160U, line);

            snprintf(line, sizeof(line), "RANGE:%5.2f m ",
                     (double)follow.camera_range_m);
            ips200_show_string(16U, 192U, line);

            ips200_show_string(16U, 224U,
                follow.command_valid ? "TARGET: VALID  " : "TARGET: LOST   ");

            snprintf(line, sizeof(line), "POINTS:%4lu ",
                     (unsigned long)path_recorder_get_record_count());
            ips200_show_string(16U, 248U, line);
        }

        system_delay_ms(10U);
    }

    color_vision_follow_disable();
    sub3_stop_vehicle();

    if (return_requested)
    {
        uint8 final_point_ok;
        uint32 buffer_count;

        ips200_clear();
        ips200_show_string(24U, 80U, "WAITING FOR STOP");
        ips200_show_string(8U, 144U, "Recording coast path...");
        ips200_show_string(16U, 240U, "KEY2/4: ABORT");

        if (sub3_wait_for_recorded_stop())
        {
            final_point_ok = path_recorder_force_final_ref_point();
            path_recorder_stop();
            buffer_count = imu_logger_get_buffer_count();

            if ((final_point_ok == 1U) && (buffer_count >= 3U))
            {
                return_completed = sub3_run_ram_return();
            }
            else
            {
                ips200_clear();
                ips200_show_string(24U, 96U, "PATH TOO SHORT");
                system_delay_ms(800U);
            }
        }
        else
        {
            path_recorder_stop();
            ips200_clear();
            ips200_show_string(24U, 96U, "RETURN CANCELLED");
            system_delay_ms(500U);
        }
    }
    else
    {
        path_recorder_stop();
    }

    if (return_completed)
    {
        /* Advance only after the complete exit-and-enter sequence succeeds. */
        if (sub3_run_blind_box())
        {
            g_sub3_menu_index = SUB3_MENU_PARKING_TASK;
        }
    }

    sub3_clear_key_actions();
}

static void sub3_remote_control(void)
{
    uint8 exit_requested = 0U;
    uint8 display_divider = 0U;
    uint32 last_valid_time = g_system_time_ms;
    uint16 switch_value = 0U;
    char line[32];

    sub3_stop_vehicle();
    sub3_clear_key_actions();
    uart_receiver.finsh_flag = 0U;
    color_vision_follow_enable();

    ips200_clear();
    ips200_show_string(24U, 24U, "REMOTE CONTROL");
    ips200_show_string(16U, 64U, "WAITING FOR SBUS");
    ips200_show_string(16U, 224U, "VISION SHADOW: ON");
    ips200_show_string(16U, 256U, "KEY2/4: EXIT");

    while (!exit_requested)
    {
        sub3_rc_result_t rc_result;

        key_scan();
        if (key4_action || key2_action)
        {
            key4_action = 0;
            key2_action = 0;
            exit_requested = 1U;
            continue;
        }

        rc_result = sub3_take_receiver_frame(1U,
                                              &switch_value,
                                              &last_valid_time);

        if (rc_result == SUB3_RC_VALID_FRAME)
        {
            current_car_mode = MODE_RC;
        }
        else if ((rc_result == SUB3_RC_FAILSAFE) ||
                 sub3_receiver_timed_out(last_valid_time))
        {
            sub3_stop_vehicle();
        }

        display_divider++;
        if (display_divider >= 10U)
        {
            display_divider = 0U;
            snprintf(line, sizeof(line), "LIM:%3.1f ACT:%5.2f",
                     (double)g_rc_max_speed,
                     (double)((encoder_speed_left
                              + encoder_speed_right) * 0.5f));
            ips200_show_string(16U, 112U, line);
            snprintf(line, sizeof(line), "STEER:%6.2f deg ",
                     (double)get_current_real_wheel_angle());
            ips200_show_string(16U, 152U, line);
            ips200_show_string(16U, 192U,
                (current_car_mode == MODE_RC) ?
                "SBUS: CONNECTED   " : "SBUS: LOST        ");
        }

        system_delay_ms(10U);
    }

    color_vision_follow_disable();
    sub3_stop_vehicle();
    sub3_clear_key_actions();
}

static void sub3_servo_center(void)
{
    sub3_clear_key_actions();
    ips200_clear();
    ips200_show_string(40U, 80U, "CENTERING");
    ips200_show_string(24U, 144U, "KEY4 Cancel");

    current_car_mode = MODE_CENTERING;
    return_to_mid_flag = 1;

    while (return_to_mid_flag)
    {
        key_scan();
        if (key4_action)
        {
            key4_action = 0;
            return_to_mid_flag = 0;
        }
        system_delay_ms(10U);
    }

    sub3_stop_vehicle();
}

#if 0
static void sub3_clear_flash_confirmed(void)
{
    sub3_clear_key_actions();
    ips200_clear();
    ips200_show_string(24U, 64U, "CLEAR ALL PATH?");
    ips200_show_string(8U, 128U, "KEY3 Confirm");
    ips200_show_string(8U, 160U, "KEY4 Cancel");

    while (TRUE)
    {
        key_scan();
        if (key3_action)
        {
            uint8 clear_ok;
            key3_action = 0;
            clear_ok = imu_logger_clear_flash();
            path_recorder_clear_all_points();
            imu_logger_init();
            ips200_show_string(24U, 224U,
                               (clear_ok == FLASH_OK) ? "CLEAR OK" : "CLEAR FAIL");
            system_delay_ms(600U);
            break;
        }
        if (key4_action)
        {
            key4_action = 0;
            break;
        }
        system_delay_ms(10U);
    }
}

static uint8 sub3_stop_record_and_save(void)
{
    uint8 final_point_ok;
    uint8 save_ok = FLASH_ERR;
    uint32 buffer_count;

    sub3_stop_vehicle();
    final_point_ok = path_recorder_force_final_ref_point();
    path_recorder_stop();
    buffer_count = imu_logger_get_buffer_count();

    ips200_clear();
    ips200_show_string(24U, 64U, "SAVING PATH...");

    if ((final_point_ok == 1U) && (buffer_count >= 3U) &&
        (imu_logger_clear_flash() == FLASH_OK))
    {
        save_ok = imu_logger_save_buffer_to_flash();
    }

    if (save_ok == FLASH_OK)
    {
        ips200_show_string(32U, 128U, "SAVE SUCCESS");
        return 1U;
    }

    ips200_show_string(40U, 128U, "SAVE FAILED");
    ips200_show_string(8U, 192U, "Return disabled");
    return 0U;
}

static uint8 sub3_run_return(uint32 last_valid_time,
                             uint8 require_remote_switch,
                             uint8 use_ram_path)
{
    uint8 completed = 0U;
    uint8 abort_return = 0U;
    uint16 switch_value = 0U;
    BackInsResult_e back_result;
    const char *result_text = "RETURN ABORTED";
    char status_line[32];

    if (!use_ram_path)
    {
        ips200_clear();
        ips200_show_string(24U, 48U, "STARTING RETURN");
    }

    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
    encoder_reset_all_data();

    if ((!use_ram_path) && (!back_ins_init()))
    {
        ips200_show_string(24U, 112U, "FLASH INIT FAIL");
        system_delay_ms(1200U);
        return 0U;
    }

    if (use_ram_path)
    {
        if (!back_ins_start_from_ram())
        {
            return 0U;
        }
    }
    else if (!back_ins_start())
    {
        snprintf(status_line, sizeof(status_line), "FLASH POINTS: %lu",
                 (unsigned long)flash_log_get_count());
        ips200_show_string(24U, 112U, "BACK START FAIL");
        ips200_show_string(16U, 144U, status_line);
        system_delay_ms(1200U);
        return 0U;
    }

    current_car_mode = MODE_INS_TRACK_THREE;
    if (!use_ram_path)
    {
        ips200_show_string(32U, 112U, "RETURN RUNNING");
        if (require_remote_switch)
        {
            ips200_show_string(8U, 176U, "CH4 LOW: STOP");
        }
        else
        {
            ips200_show_string(8U, 176U, "Manual backup mode");
        }
        ips200_show_string(8U, 208U, "KEY4: STOP");
    }
    last_valid_time = g_system_time_ms;

    while (back_ins_is_running())
    {
        sub3_rc_result_t rc_result;
        key_scan();

        rc_result = SUB3_RC_NO_FRAME;
        if (require_remote_switch)
        {
            rc_result = sub3_take_receiver_frame(0U,
                                                  &switch_value,
                                                  &last_valid_time);
            if (rc_result == SUB3_RC_FAILSAFE)
            {
                abort_return = 1U;
            }
            else if (rc_result == SUB3_RC_VALID_FRAME)
            {
                if (switch_value <= SUB3_RETURN_TRIGGER_VALUE)
                {
                    abort_return = 1U;
                }
            }

            if (sub3_receiver_timed_out(last_valid_time))
            {
                abort_return = 1U;
            }
        }

        if (key4_action)
        {
            key4_action = 0;
            abort_return = 1U;
        }

        if (abort_return)
        {
            break;
        }

        system_delay_ms(5U);
    }

    back_ins_stop();
    back_result = back_ins_get_result();
    completed = (uint8)(
        (!abort_return) && (back_result == BACK_INS_RESULT_SUCCESS));

    if (abort_return)
    {
        result_text = "RETURN ABORTED";
    }
    else
    {
        switch (back_result)
        {
            case BACK_INS_RESULT_SUCCESS:
                result_text = "RETURN COMPLETE";
                break;
            case BACK_INS_RESULT_POSE_INVALID:
                result_text = "POSE INVALID STOP";
                break;
            case BACK_INS_RESULT_LOST_PATH:
                result_text = "LOST PATH STOP";
                break;
            case BACK_INS_RESULT_CORRIDOR_VIOLATION:
                result_text = "CORRIDOR STOP";
                break;
            case BACK_INS_RESULT_ALIGNMENT_FAILED:
                result_text = "ALIGNMENT FAILED";
                break;
            default:
                result_text = "RETURN ABORTED";
                break;
        }
    }

    sub3_stop_vehicle();

    ips200_clear();
    ips200_show_string(16U, 96U, result_text);
    if (!use_ram_path)
    {
        system_delay_ms(800U);
    }

    return completed;
}

static void sub3_manual_record(void)
{
    uint32 last_valid_time = g_system_time_ms;
    uint16 switch_value = 0U;
    uint8 stop_recording = 0U;
    uint8 final_point_ok;

    sub3_clear_key_actions();
    sub3_stop_vehicle();
    encoder_reset_all_data();
    imu_ahrs_reset_position();
    path_recorder_start();

    if (!path_recorder_is_working())
    {
        ips200_clear();
        ips200_show_string(32U, 96U, "RECORD INIT FAIL");
        system_delay_ms(600U);
        return;
    }

    current_car_mode = MODE_RC;
    ips200_clear();
    ips200_show_string(32U, 48U, "MANUAL RECORD");
    ips200_show_string(8U, 112U, "KEY4 Stop record");
    ips200_show_string(8U, 160U, "Then choose Manual Save");

    while (!stop_recording)
    {
        sub3_rc_result_t rc_result;
        key_scan();
        path_recorder_update();
        rc_result = sub3_take_receiver_frame(1U,
                                              &switch_value,
                                              &last_valid_time);

        if ((rc_result == SUB3_RC_FAILSAFE) ||
            sub3_receiver_timed_out(last_valid_time))
        {
            stop_recording = 1U;
        }
        if (key4_action)
        {
            key4_action = 0;
            stop_recording = 1U;
        }
        system_delay_ms(5U);
    }

    sub3_stop_vehicle();
    final_point_ok = path_recorder_force_final_ref_point();
    path_recorder_stop();

    ips200_clear();
    ips200_show_string(24U, 96U,
                       final_point_ok ? "BUFFER READY" : "END POINT FAILED");
    ips200_show_string(8U, 160U, "Use Manual Save next");
    system_delay_ms(700U);
}

static void sub3_manual_save(void)
{
    uint8 save_ok;

    sub3_clear_key_actions();
    save_ok = sub3_stop_record_and_save();
    if (save_ok)
    {
        ips200_show_string(8U, 192U, "Use Manual Return next");
    }
    system_delay_ms(700U);
}

static void sub3_manual_return(void)
{
    sub3_clear_key_actions();
    sub3_stop_vehicle();
    sub3_run_return(0U, 0U, 0U);
}

static void sub3_remote_mission(void)
{
    uint32 last_valid_time = g_system_time_ms;
    uint16 switch_value = 0U;
    uint8 mission_abort = 0U;
    uint8 final_point_ok;
    uint32 buffer_count;

    sub3_clear_key_actions();
    sub3_stop_vehicle();
    encoder_reset_all_data();
    imu_ahrs_reset_position();
    path_recorder_start();

    if (!path_recorder_is_working())
    {
        sub3_stop_vehicle();
        return;
    }

    current_car_mode = MODE_RC;
    last_valid_time = g_system_time_ms;

    ips200_clear();
    ips200_show_string(32U, 48U, "RC RECORDING");
    ips200_show_string(8U, 112U, "CH[3] > 192: RETURN");
    ips200_show_string(8U, 224U, "KEY4 Abort");

    while (!mission_abort && path_recorder_is_working())
    {
        sub3_rc_result_t rc_result;

        key_scan();
        path_recorder_update();
        rc_result = sub3_take_receiver_frame(1U,
                                              &switch_value,
                                              &last_valid_time);

        if ((rc_result == SUB3_RC_FAILSAFE) ||
            sub3_receiver_timed_out(last_valid_time) ||
            key4_action)
        {
            key4_action = 0;
            mission_abort = 1U;
        }
        else if ((rc_result == SUB3_RC_VALID_FRAME) &&
                 (switch_value > SUB3_RETURN_TRIGGER_VALUE))
        {
            /*
             * 竞速路径：不鸣笛、不等待、不擦写 Flash。
             * 补完终点后直接把 RAM 记录倒序交给返航控制器。
             */
            sub3_stop_vehicle();
            final_point_ok = path_recorder_force_final_ref_point();
            path_recorder_stop();
            buffer_count = imu_logger_get_buffer_count();

            if ((final_point_ok == 1U) && (buffer_count >= 3U))
            {
                (void)sub3_run_return(last_valid_time, 1U, 1U);
            }
            else
            {
                mission_abort = 1U;
            }
            break;
        }
    }

    if (mission_abort)
    {
        if (path_recorder_is_working())
        {
            path_recorder_stop();
        }
        sub3_stop_vehicle();
        ips200_clear();
        ips200_show_string(32U, 96U, "MISSION ABORTED");
    }

    sub3_clear_key_actions();
}
#endif

void Menu_Init(void)
{
    if ((g_rc_max_speed < SUB3_RC_SPEED_MIN_MPS) ||
        (g_rc_max_speed > SUB3_RC_SPEED_MAX_MPS))
    {
        g_rc_max_speed = 2.0f;
    }
    if ((g_back_curve_limit_speed_mps < SUB3_RETURN_SPEED_MIN_MPS) ||
        (g_back_curve_limit_speed_mps > SUB3_RETURN_SPEED_MAX_MPS))
    {
        g_back_curve_limit_speed_mps = 3.0f;
    }
    if ((g_back_straight_max_speed_mps < SUB3_RETURN_SPEED_MIN_MPS) ||
        (g_back_straight_max_speed_mps > SUB3_RETURN_SPEED_MAX_MPS))
    {
        g_back_straight_max_speed_mps = 4.0f;
    }
    if (g_back_straight_max_speed_mps < g_back_curve_limit_speed_mps)
    {
        g_back_straight_max_speed_mps = g_back_curve_limit_speed_mps;
    }
    if ((g_color_follow_max_speed < SUB3_FOLLOW_SPEED_MIN_MPS) ||
        (g_color_follow_max_speed > SUB3_FOLLOW_SPEED_MAX_MPS))
    {
        g_color_follow_max_speed = 2.50f;
    }

#if SUB3_CAMERA_ONLY_MODE
    g_sub3_menu_index = SUB3_MENU_STATIC_VISION;
#else
    g_sub3_menu_index = 0U;
#endif
    g_sub3_menu_dirty = 1U;
    sub3_draw_main_menu();
    g_sub3_menu_dirty = 0U;
}

void Menu(void)
{
    if (key1_action)
    {
        key1_action = 0;
        g_sub3_menu_index = (g_sub3_menu_index == 0U) ?
                            (SUB3_MENU_ITEM_COUNT - 1U) :
                            (g_sub3_menu_index - 1U);
        g_sub3_menu_dirty = 1U;
    }

    if (key2_action)
    {
        key2_action = 0;
        g_sub3_menu_index =
            (uint8)((g_sub3_menu_index + 1U) % SUB3_MENU_ITEM_COUNT);
        g_sub3_menu_dirty = 1U;
    }

    if (key3_action)
    {
        key3_action = 0;
#if SUB3_CAMERA_ONLY_MODE
        if (g_sub3_menu_index == SUB3_MENU_STATIC_VISION)
        {
            sub3_camera_vision_debug();
        }
#else
        switch ((sub3_menu_item_t)g_sub3_menu_index)
        {
            case SUB3_MENU_ACTION_TASK:
                sub3_camera_follow();
                break;
            case SUB3_MENU_PARKING_TASK:
                sub3_voice_parking_task();
                break;
            case SUB3_MENU_REMOTE_CONTROL:
                sub3_remote_control();
                break;
            case SUB3_MENU_RC_SPEED:
                sub3_adjust_speed(SUB3_SPEED_TYPE_RC);
                break;
            case SUB3_MENU_STATIC_VISION:
                sub3_camera_vision_debug();
                break;
            case SUB3_MENU_FOLLOW_SPEED: // 选中第 4 项并按下 KEY3（确认）
                sub3_adjust_speed(SUB3_SPEED_TYPE_FOLLOW);
                break;
            case SUB3_MENU_RETURN_SPEED: // 选中第 5 项并按下 KEY3
                sub3_adjust_speed(SUB3_SPEED_TYPE_RETURN);
                break;
            case SUB3_MENU_STRAIGHT_SPEED:
                sub3_adjust_speed(SUB3_SPEED_TYPE_STRAIGHT);
                break;
            case SUB3_MENU_SERVO_CENTER:
                sub3_servo_center();
                break;
            default:
                break;
        }
        sub3_clear_key_actions();
#endif
        g_sub3_menu_dirty = 1U;
    }

    if (key4_action)
    {
        key4_action = 0;
        sub3_stop_vehicle();
        g_sub3_menu_dirty = 1U;
    }

    if (g_sub3_menu_dirty)
    {
        sub3_draw_main_menu();
        g_sub3_menu_dirty = 0U;
    }

    system_delay_ms(5U);
}
