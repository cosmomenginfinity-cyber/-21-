#include "zf_common_headfile.h"
#include "global_vars.h"
#include "initialize.h"
#include "../code/sub3_build_mode.h"
#include "back_ins.h"
#include "motor.h"
#include "encoder.h"
#include "imu_ahrs.h"
#include "encoder_motor_ctl.h"
#include "color_vision.h"
#include "asr_audio.h"
#pragma section all "cpu3_dsram"

void core3_main(void)
{
    disable_Watchdog();                     // 关闭看门狗
    interrupt_global_enable(0);             // 打开全局中断

#if SUB3_CAMERA_ONLY_MODE
    cpu_wait_event_ready();
    while (TRUE)
    {
        system_delay_ms(10U);
    }
#else
    seekfree_assistant_oscilloscope_struct oscilloscope_data = {0};
    color_vision_diagnosis_t vision_diagnosis = {0};

    // CPU3 owns the auxiliary wireless debug channel. The send loop is paused
    // during Parking Task to keep all avoidable bus traffic away from CPU2's
    // time-critical TLD7002 scan.
    wireless_uart_init();
    seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIRELESS_UART);
    oscilloscope_data.channel_num = 7U;
    cpu_wait_event_ready();                 // 等待所有核心初始化完毕

    while (TRUE)
    {
        if (g_voice_task_active)
        {
            system_delay_ms(10U);
            continue;
        }

        if (((current_car_mode == MODE_CAMERA) ||
             ((current_car_mode == MODE_RC) &&
              color_vision_follow_is_enabled())) &&
            color_vision_diagnosis_get_snapshot(&vision_diagnosis))
        {
            // Vision diagnosis; MODE_RC uses shadow follow.
            oscilloscope_data.data[0] = vision_diagnosis.raw_center_error_px;
            oscilloscope_data.data[1] = vision_diagnosis.target_y_m;
            oscilloscope_data.data[2] = vision_diagnosis.target_wheel_angle_deg;
            oscilloscope_data.data[3] = vision_diagnosis.actual_wheel_angle_deg;
            oscilloscope_data.data[4] = vision_diagnosis.actual_speed_mps;
            oscilloscope_data.data[5] = vision_diagnosis.yaw_rate_dps;
            oscilloscope_data.data[6] = vision_diagnosis.vision_age_ms;
        }
        else
        {
            // Preserve the original return-navigation diagnostic channels.
            oscilloscope_data.data[0] = imu_ahrs_get_yaw();
            oscilloscope_data.data[1] = ins_target_yaw;
            oscilloscope_data.data[2] = ins_lateral_error;
            oscilloscope_data.data[3] = get_current_real_wheel_angle();
            oscilloscope_data.data[4] =
                (encoder_speed_left + encoder_speed_right) / 2.0f;
            oscilloscope_data.data[5] = g_speed_mgr.target_ramp;
            oscilloscope_data.data[6] =
                back_ins_get_end_steer_correction();
        }

        seekfree_assistant_oscilloscope_send(&oscilloscope_data);
        system_delay_ms(10U);
    }
#endif
}

#pragma section all restore
