#include "initialize.h"
#include "global_vars.h"
#include "sub2_waypoint_store.h"
#include "zf_device_dot_matrix_screen.h" // 确保包含灯屏头文件

void Initialize(void)
{
    // 1. 基础硬件初始化
    encoder_init();

    // 【修复 1】：取消注释！必须初始化绝对值电位器，否则舵机闭环瘫痪
    absolute_encoder_init();

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
    //dot_matrix_screen_init();

    // 4. 核心感知层初始化 (陀螺仪校准就在这里)
    // 注意：imu_ahrs_init 内部通常有 2000 次采样校准，
    // 执行到这里时，车子必须保持绝对静止，否则校准出来的零偏是错的！
    imu_ahrs_init();

    // 5. 存储与逻辑模块初始化 (补齐这些！)
    imu_logger_init();      // 初始化 Flash 日志系统
    path_recorder_init();   // 初始化路径记录状态
    ins_init();             // 初始化惯导导航参数
    flash_load_sub2_waypoints();

    // 6. 通信与菜单
    uart_receiver_init();   // 接收遥控器信号
    Menu_Init();            // 初始化菜单节点

    // 7. 最后清理
    encoder_reset_all_data(); // 确保所有计数值归零
}
