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
#pragma section all "cpu0_dsram"
#include "zf_common_headfile.h"


int core0_main(void)
{
    clock_init();                   // 获取时钟频率<务必保留>
    debug_init();                   // 初始化默认调试串口

    //audio_init();
    // 科目三专用初始化：保留编码器、IMU、Flash、记录器、back_ins、遥控、菜单
    Initialize();

    // 修改后：留出 5% 的低半管导通时间，强制给自举电容“续命充电”，彻底解决 100% 占空比陷阱
    PID_Init(&pid_left,  1000.0f, 200.0f, 0.0f, -9500.0f, 9500.0f);
    PID_Init(&pid_right, 1000.0f, 200.0f, 0.0f, -9500.0f, 9500.0f);
    pit_ms_init(CCU60_CH0, 10);

    cpu_wait_event_ready();         // 等待所有核心初始化完毕
    while (TRUE)
    {
        key_scan();

        Menu();
    }


}


#pragma section all restore
// **************************** 代码区域 ****************************
