#include "zf_common_headfile.h"
#include <stdio.h>
#include <string.h>

#include "zf_driver_uart.h"
#include "zf_driver_delay.h"
#include "global_vars.h"
#include "initialize.h"
#include "PID.h"
#include "motor.h"
#include "flash.h"
#include "encoder.h"
#include "imu_datalogger.h"
#include "imu_ahrs.h"
#include "encoder_motor_ctl.h"

extern volatile float g_steer_wheel_angle_snapshot;

#pragma section all "cpu1_dsram"
//uint8_t i = 0;
uint8_t i_c1 = 0;
//static char send_str[128] = {0}; // 扩容至 128 字节，防止 sprintf 溢出导致死机
//static char send_str_c1[128] = {0};
void core1_main(void)
{
    disable_Watchdog();                     // 关闭看门狗
    interrupt_global_enable(0);             // 打开全局中断
    // 此处编写用户代码 例如外设初始化代码等

    // 无线逐飞助手代码保留备用，需要恢复时同时取消循环内代码的注释。
    /*
    wireless_uart_init();
    seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIRELESS_UART);
    seekfree_assistant_oscilloscope_struct oscilloscope_data;
    oscilloscope_data.channel_num = 6;
    */

    // 此处编写用户代码 例如外设初始化代码等
    cpu_wait_event_ready();                 // 等待所有核心初始化完毕
    printf("yaw_deg,target_yaw_deg,pp_wheel_deg,wheel_deg,speed_mps,target_speed_mps\r\n");

    while (TRUE)
    {
        float yaw = imu_ahrs_get_yaw();
        float target_yaw = ins_target_yaw;
        float pp_wheel_angle = ins_lateral_error;
        float wheel_angle = g_steer_wheel_angle_snapshot;
        float speed = (encoder_speed_left + encoder_speed_right) / 2.0f;
        float target_speed = g_speed_mgr.target_ramp;

        printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r\n",
               yaw,
               target_yaw,
               pp_wheel_angle,
               wheel_angle,
               speed,
               target_speed);

        /*
        oscilloscope_data.data[0] = yaw;
        oscilloscope_data.data[1] = target_yaw;
        oscilloscope_data.data[2] = pp_wheel_angle;
        oscilloscope_data.data[3] = wheel_angle;
        oscilloscope_data.data[4] = speed;
        oscilloscope_data.data[5] = target_speed;
        seekfree_assistant_oscilloscope_send(&oscilloscope_data);
        */

        // 20 Hz足够观察控制曲线，也不会塞满115200波特率调试串口。
        system_delay_ms(50);
    }
}
#pragma section all restore
