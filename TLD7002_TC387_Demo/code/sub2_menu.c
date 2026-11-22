#include "menu.h"
#include "asr_audio.h"
#include "remote_control.h"
#include "sub2_navigation.h"
#include "sub2_waypoint_store.h"
#include "zf_device_dot_matrix_screen.h"
#include <stdio.h>
#include <string.h>

static const char *const sub2_wp_names[WP_NUM] = {
    "0.Start Area  ",
    "1.Gate 1 Left ",
    "2.Gate 1 Mid  ",
    "3.Gate 2 Mid  ",
    "4.Gate 3 Mid  ",
    "5.Gate 3 Right",
    "6.Action Area ",
    "7.Parking Area"
};
void Action_Sub2_Record(void)
{
    ips200_clear();
    ips200_show_string(0, 16*1, ">> SUB2 WAYPOINT <<");
    ips200_show_string(0, 16*3, "KEY1/2: Select Pt");
    ips200_show_string(0, 16*4, "KEY3  : Save Curr");
    ips200_show_string(10, 16*7, "KEY4  : Exit NO SV");


    flash_load_sub2_waypoints();

    remote_control_begin();
    //encoder_reset_all_data();
    //imu_ahrs_reset_position();

    int active_wp_idx = 0;

    while(1)
    {
        key_scan();

        remote_control_update();


        ips200_show_string(0, 16*2, sub2_wp_names[active_wp_idx]);
        char pos_buf[64];
        snprintf(pos_buf, sizeof(pos_buf), "X:%.2f  Y:%.2f", imu_ahrs_get_pos_x(), imu_ahrs_get_pos_y());
        ips200_show_string(0, 16*5, pos_buf);

        if (key2_action) {
            key2_action = 0;
            active_wp_idx = (active_wp_idx + 1) % WP_NUM;
        }
        if (key1_action) {
            key1_action = 0;
            active_wp_idx = (active_wp_idx == 0) ? WP_NUM - 1 : active_wp_idx - 1;
        }
        if (key3_action) {
            key3_action = 0;
            g_sub2_waypoints[active_wp_idx].x = imu_ahrs_get_pos_x();
            g_sub2_waypoints[active_wp_idx].y = imu_ahrs_get_pos_y();
            g_sub2_waypoints[active_wp_idx].yaw = imu_ahrs_get_yaw();
            g_sub2_waypoints[active_wp_idx].isValid = 1;

            flash_save_sub2_waypoints();

            ips200_show_string(0, 16*6, "WP Saved!       ");

            system_delay_ms(500);
            ips200_show_string(0, 16*6, "                ");
        }
        if (key4_action) {
            key4_action = 0;
            break;
        }
    }

    remote_control_end();
}

void Action_Sub2_Clear(void)
{
    ips200_clear();
    ips200_show_string(0, 16 * 3, "Clearing WPs...");


    flash_clear_sub2_waypoints();


    memset(g_sub2_waypoints, 0, sizeof(g_sub2_waypoints));

    ips200_clear();
    ips200_show_string(0, 16 * 4, "Cleared Success!");
    system_delay_ms(1200);
}


void Action_Sub2_PrintWPs(void)
{
    ips200_clear();
    ips200_show_string(0, 16*3, "Printing...");
    flash_load_sub2_waypoints();

    for(int i=0; i<WP_NUM; i++) {
        char out_buf[128];
        snprintf(out_buf, sizeof(out_buf), "WP[%d] (%s): x=%.3f, y=%.3f, yaw=%.1f, Valid:%d\r\n",
                 i, sub2_wp_names[i], g_sub2_waypoints[i].x, g_sub2_waypoints[i].y,
                 g_sub2_waypoints[i].yaw, g_sub2_waypoints[i].isValid);
        uart_write_string(UART_0, out_buf);
    }
    ips200_clear();
    ips200_show_string(0, 16*4, "Print Finished!");
    system_delay_ms(1000);
}
static const char *voice_network_error_text(void)
{
    switch(audio_network_error_code)
    {
        case AUDIO_NET_ERR_WIFI_INIT:      return "E1 WiFi Init    ";
        case AUDIO_NET_ERR_GET_TIME:       return "E2 Get Time     ";
        case AUDIO_NET_ERR_AUTH_URL:       return "E3 Auth URL     ";
        case AUDIO_NET_ERR_WS_CONNECT:     return "E4 WebSocket    ";
        case AUDIO_NET_ERR_START_SEND:     return "E5 Start Send   ";
        case AUDIO_NET_ERR_AUDIO_SEND:     return "E6 Audio Send   ";
        case AUDIO_NET_ERR_FINAL_SEND:     return "E7 Final  Send   ";
        case AUDIO_NET_ERR_RESULT_TIMEOUT: return "E8 Result Wait  ";
        default:                           return "E0 Unknown      ";
    }
}

void Action_Sub2_VoiceRun(void)
{
    static uint8 audio_initialized = 0U;

    system_delay_ms(200);
    key3_action = 0;
    g_voice_soft_trigger = 0;

    ips200_clear();

    if (!audio_initialized)
    {
        ips200_show_string(10, 16 * 3, "Voice Initializing");
        audio_init();
        audio_initialized = 1U;
    }

    // audio_init() enables the sampling PIT. Keep it stopped until the first
    // audio_session_begin(), so Subject 2 owns the complete sampling lifetime.
    pit_disable(ASR_PIT);

    flash_load_sub2_waypoints();

    dot_matrix_clear();

    g_active_voice_cmd = CMD_NONE;

    uint8 sub_stage = 0;
    uint32 state_timer = 0;
    uint8 ready_shown = 0;
    uint8 recording_started = 0;
    while(1)
    {
        key_scan();

        if (sub_stage == 0)
        {
            if (!ready_shown)
            {
                ready_shown = 1;
                ips200_clear();
                ips200_show_string(30, 16 * 3, "Ready to Start...");
                ips200_show_string(10, 16 * 7, "KEY4 to Exit");
                state_timer = g_system_time_ms;
            }


            if (g_system_time_ms - state_timer >= 1500)
            {
                ips200_clear();
                ips200_show_string(30, 16 * 3, "VAD: Connecting ");
                ips200_show_string(10, 16 * 7, "KEY4 to Exit");


                voice_text_clear();
                g_active_voice_cmd = CMD_NONE;

                audio_session_begin();
                g_voice_state = VOICE_STATE_LISTENING;
                sub_stage = 1;
                recording_started = 0;
                ready_shown = 0;
            }
        }

        else if (sub_stage == 1)
        {
            audio_loop();

            if (audio_network_error_flag)
            {
                ips200_show_string(30, 16 * 4, "VAD: Net Error  ");
                ips200_show_string(30, 16 * 5, voice_network_error_text());
                system_delay_ms(1500);
                sub_stage = 0;
            }
            else if (!recording_started)
            {
                if (audio_server_link_flag)
                {
                    recording_started = 1;
                    state_timer = g_system_time_ms;
                    ips200_show_string(30, 16 * 3, "VAD: Listening  ");
                }
            }
            else
            {
                uint32 elapsed = g_system_time_ms - state_timer;

                if (elapsed < 10000)
                {
                    char time_buf[32];
                    snprintf(time_buf, sizeof(time_buf), "Rec Time: %.1fs  ",
                             (10000.0f - elapsed) / 1000.0f);
                    ips200_show_string(30, 16 * 4, time_buf);
                }
                else if (audio_start_flag == 1)
                {
                    ips200_show_string(30, 16 * 4, "VAD: Analyzing  ");
                    audio_session_request_finish();
                }

                if (audio_start_flag == 0 && audio_need_net_flag == 0)
                {
                    if (g_active_voice_cmd != CMD_NONE)
                    {
                        g_voice_state = VOICE_STATE_EXECUTING;
                        sub_stage = 2;
                    }
                    else
                    {
                        ips200_show_string(30, 16 * 4, "VAD: No Match!  ");
                        ips200_show_string(30, 16 * 5, "Say Again...    ");
                        system_delay_ms(1500);
                        sub_stage = 0;
                    }
                }
            }
        }

               else if (sub_stage == 2)
               {
                   Sub2_Execute_Task_CPU0();


                   if (current_car_mode == MODE_STOP && g_active_voice_cmd == CMD_NONE)
                   {
                       sub_stage = 0;
                   }
               }


               if (key4_action == 1)
               {
                   key4_action = 0;

                   audio_session_cancel();
                   voice_text_clear();
                   g_active_voice_cmd = CMD_NONE;
                   dot_matrix_clear();

                   sub2_navigation_stop();
                   current_car_mode = MODE_STOP;
                   g_voice_state = VOICE_STATE_IDLE;
                   break;
               }
           }

           current_car_mode = MODE_STOP;
}
