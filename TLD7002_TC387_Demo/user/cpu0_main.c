#include "zf_common_headfile.h"
#include "zf_common_typedef.h"
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
#include "asr_ctrl.h"
#include "hmac_sha256.h"
#include "base64.h"
#include "asr_audio.h"
#include "zf_device_dot_matrix_screen.h" // 确保包含灯屏头文件
#pragma section all "cpu0_dsram"

int core0_main(void)
{
    clock_init();                   // 获取时钟频率<务必保留>
    debug_init();                   // 初始化默认调试串口


    Initialize();

    PID_Init(&pid_left,  1000.0f, 200.0f, 0.0f, -9500.0f, 9500.0f);
    PID_Init(&pid_right, 1000.0f, 200.0f, 0.0f, -9500.0f, 9500.0f);
    pit_ms_init(CCU60_CH0, 10);
    enableInterrupts();

    // 此处编写用户代码 例如外设初始化代码等
    cpu_wait_event_ready();         // 等待所有核心初始化完毕


    while (TRUE)
    {
        //play_two_tigers();
        key_scan();
        Menu();
    }
}



#pragma section all restore
// **************************** 代码区域 ****************************
