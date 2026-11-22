#include "zf_common_headfile.h"
#include "color_vision.h"
#include "asr_audio.h"
#pragma section all "cpu1_dsram"

void core1_main(void)
{
    disable_Watchdog();                     // 关闭看门狗
    interrupt_global_enable(0);             // 打开全局中断
    cpu_wait_event_ready();                 // 等待所有核心初始化完毕

    // CPU1 exclusively owns RGB/HSV conversion, mask processing and target
    // detection. Parking Task pauses vision so CPU2 can scan the lamp board
    // without camera DMA or image-processing bus traffic.
    while (TRUE)
    {
        if (!g_voice_task_active)
        {
            color_vision_service();
        }
        else
        {
            system_delay_ms(1U);
        }
    }
}

#pragma section all restore
