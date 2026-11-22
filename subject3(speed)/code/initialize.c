#include "initialize.h"
#include "sub3_build_mode.h"
#include "global_vars.h"
#include "back_ins.h"
#include "color_vision.h"
#include "encoder.h"
#include "imu_ahrs.h"
#include "imu_datalogger.h"
#include "pathrecorder.h"

void Initialize(void)
{
#if !SUB3_CAMERA_ONLY_MODE
    // 1. 基础硬件初始化
    encoder_init();
    absolute_encoder_init();
#endif
    gpio_init(DRIVER_MOTOR, GPO, 1, GPO_PUSH_PULL);
    gpio_init(KEY1, GPI, 0, GPI_PULL_UP);
    gpio_init(KEY2, GPI, 0, GPI_PULL_UP);
    gpio_init(KEY3, GPI, 0, GPI_PULL_UP);
    gpio_init(KEY4, GPI, 0, GPI_PULL_UP);

    // 2. PWM 初始化 (舵机 + 后轮电机)
    pwm_init(PWM_N, 17000, 0);
    pwm_init(PWM_S, 17000, 0);
    pwm_init(PWM_L1, 21000, 0);
    pwm_init(PWM_L2, 21000, 0);
    pwm_init(PWM_R1, 21000, 0);
    pwm_init(PWM_R2, 21000, 0);

    // 3. 显示屏初始化 (建议先刷黑屏，防止花屏)
    ips200_set_dir(IPS200_PORTAIT);
    ips200_set_color(RGB565_BLACK, RGB565_WHITE);
    ips200_init(IPS200_TYPE);

    // Initialize UART10 first so SBUS is ready before the bounded IMU startup.
#if !SUB3_CAMERA_ONLY_MODE
    uart_receiver_init();
#endif

    // Complete zero-bias calibration before the 100 Hz PIT starts.  This keeps
    // repeated SPI reads and millisecond waits out of the control ISR.
    // Keep the vehicle completely stationary until this function returns.
#if !SUB3_CAMERA_ONLY_MODE
    imu_ahrs_init();

    // 5. 存储与逻辑模块初始化 (补齐这些！)
    imu_logger_init();      // 初始化 Flash 日志系统
    path_recorder_init();   // 初始化路径记录状态
#endif
#if 0   // ================= 科目三专用：屏蔽科目一初始化 =================
    ins_init();             // 科目一正向惯导，当前版本不运行
#endif  // ================= 科目一屏蔽结束 =================
#if !SUB3_CAMERA_ONLY_MODE
    back_ins_init();        // 初始化返航惯导参数 (科目三)
#endif

    Menu_Init();            // 初始化菜单节点

    // SCC8660 DMA and both camera ISRs are routed to CPU0 in isr_config.h.
    // Keep peripheral initialization on the same owner core; CPU2 processes frames.
    (void)color_vision_camera_init();

    // 7. 最后清理
#if !SUB3_CAMERA_ONLY_MODE
    encoder_reset_all_data(); // 确保所有计数值归零
#endif
}
