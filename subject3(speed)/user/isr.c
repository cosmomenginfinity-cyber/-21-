/*********************************************************************************************************************
* TC387 Opensourec Library 即（TC387 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是 TC387 开源库的一部分
*
* TC387 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          isr
* 公司名称          成都逐飞科技有限公司
* 版本信息          查看 libraries/doc 文件夹内 version 文件 版本说明
* 开发环境          ADS v1.10.2
* 适用平台          TC387QP
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者                备注
* 2022-11-04       pudding            first version
********************************************************************************************************************/

#include "global_vars.h"
#include "initialize.h"
#include "../code/sub3_build_mode.h"
#include "PID.h"
#include "back_ins.h"
#include "motor.h"
#include "flash.h"
#include "encoder.h"
#include "imu_datalogger.h"
#include "imu_ahrs.h"
#include "encoder_motor_ctl.h"
#include "color_vision.h"
#include "isr_config.h"
#include "isr.h"
#if 0   // ================= 科目三专用：屏蔽科目一/科目二头文件 =================
#include "ins.h"
#endif
#include "asr_audio.h"
#include "zf_device_dot_matrix_screen.h"
#include "zf_device_tld7002.h"
// 对于TC系列默认是不支持中断嵌套的，希望支持中断嵌套需要在中断内使用 interrupt_global_enable(0); 来开启中断嵌套
// 简单点说实际上进入中断后TC系列的硬件自动调用了 interrupt_global_disable(); 来拒绝响应任何的中断，因此需要我们自己手动调用 interrupt_global_enable(0); 来开启中断的响应。
#if 0   // 科目一换向盲滑变量，科目三 back_ins 不使用
extern volatile uint8 g_ins_neutral_glide;
#endif
// **************************** PIT中断函数 ****************************
IFX_INTERRUPT(cc60_pit_ch0_isr, CCU6_0_CH0_INT_VECTAB_NUM, CCU6_0_CH0_ISR_PRIORITY)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    pit_clear_flag(CCU60_CH0);
    // =========================================================
    g_system_time_ms += 10;
#if SUB3_CAMERA_ONLY_MODE
    return;
#endif


    //ins_update();

    encoder_update_all_data();

    // Parking Task does not use yaw. Suspending SCHA634 SPI polling here keeps
    // CPU0 deterministic for Wi-Fi and the 8 kHz microphone data stream.
    if (g_voice_task_active == 0U)
    {
        imu_ahrs_update();
    }

    // 舵机内环使用绝对编码器角；底盘几何和差速使用等效真实轮角。
    float current_steer_sensor_angle =
        get_current_steer_sensor_angle();
    float current_real_wheel_angle =
        steer_sensor_to_wheel_angle(current_steer_sensor_angle);
    // 我们强制使用 tune_target_angle 作为目标，只跑舵机内环
    /*    int16 steer_pwm = calc_inner_servo_pwm(tune_target_angle, current_steer_angle);
        set_motor_speed(steer_pwm);*/
    // ==============================================================

    switch (current_car_mode)
    {
            // --------------------------------------------------
            // 模式 1: 遥控模式 (纯开环给角度，测试内环跟随)
            // --------------------------------------------------
            case MODE_RC:
            {
                float shadow_speed = 0.0f;
                float shadow_wheel_angle = 0.0f;

                /* Run the exact visual controller at its intended 100 Hz
                 * rate, but discard its outputs in remote-control mode. */
                if (color_vision_follow_is_enabled())
                {
                    (void)color_vision_follow_get_command(
                        &shadow_speed, &shadow_wheel_angle);
                }

                // 内环控制：遥控器角度 -> PWM (注意你全局变量名还是 angel，这里保持兼容)
                int16 steer_pwm = calc_inner_servo_pwm(
                    tune_target_angle,
                    current_steer_sensor_angle);
                set_motor_speed(steer_pwm);

                // 遥控目标保持编码器角单位，后轮差速只使用真实轮角。
                test_update_rear_wheels_for_ins(
                    tune_target_speed,
                    current_real_wheel_angle);
                //test_update_rear_wheels_for_ins(tune_target_speed, 0.0f);
                //test_update_rear_wheels_for_ins(4.0f, 0.0f);
                break;
            }

            // --------------------------------------------------
            // Mode 2: first-stage yellow target following
            // --------------------------------------------------
            case MODE_CAMERA:
            {
                float target_speed = 0.0f;
                float target_wheel_angle = 0.0f;
                float target_steer_sensor_angle;
                uint8 command_valid;
                int16 steer_pwm;

                /* CPU2 supplies the filtered visual command. */
                command_valid = color_vision_follow_get_command(
                    &target_speed, &target_wheel_angle);
                if (!command_valid)
                {
                    /* A stale/uncalibrated/edge-truncated target is a hard stop. */
                    set_motor_speed(0);
                    Motor_Control(0, 0);
                    PID_Reset(&pid_left);
                    PID_Reset(&pid_right);
                    g_speed_mgr.target_final = 0.0f;
                    g_speed_mgr.target_ramp = 0.0f;
                    target_speed = 0.0f;
                    target_wheel_angle = 0.0f;
                }

                if (target_wheel_angle > STEER_WHEEL_MAX_ANGLE_DEG)
                {
                    target_wheel_angle = STEER_WHEEL_MAX_ANGLE_DEG;
                }
                else if (target_wheel_angle < -STEER_WHEEL_MAX_ANGLE_DEG)
                {
                    target_wheel_angle = -STEER_WHEEL_MAX_ANGLE_DEG;
                }

                target_steer_sensor_angle =
                    steer_wheel_to_sensor_angle(target_wheel_angle);
                steer_pwm = calc_inner_servo_pwm(
                    target_steer_sensor_angle,
                    current_steer_sensor_angle);
                set_motor_speed(steer_pwm);

                if (command_valid)
                {
                    /* Existing ramp and wheel-speed PID form the speed loop. */
                    test_update_rear_wheels_for_ins(
                        target_speed,
                        current_real_wheel_angle);
                }
                break;
            }

            // --------------------------------------------------
            // 模式 2: 惯导自动驾驶模式 (算法同学对接区)
            // --------------------------------------------------
#if 0   // ================= 科目三专用：屏蔽科目一自动复现分支 =================
            case MODE_INS_TRACK:
            {
                ins_update();
                // [外环 PD]：路径偏差 -> 期望前轮角度
                float target_angle = ins_lateral_error;

                // 队长防线：防止算法出错给出离谱角度导致舵机卡死烧毁
                if (target_angle > 45.0f)  target_angle = 45.0f;
                if (target_angle < -45.0f) target_angle = -45.0f;

                // [内环 PD]：期望前轮角度 -> 舵机 PWM
                int16 steer_pwm = calc_inner_servo_pwm(target_angle, current_steer_angle);
                set_motor_speed(steer_pwm);
                // 【物理阻断】：如果正处于盲滑收敛期，绝不运行电机 PID，维持物理纯滚动！
                if (g_ins_neutral_glide == 1)
                {
                    Motor_Control(0, 0); // 强行空档
                }
                else
                {
                    // 正常行驶状态，运行差速驱动
                    test_update_rear_wheels_for_ins(g_speed_mgr.target_final, current_steer_angle);
                }

                break;
            }
            // --------------------------------------------------
            // 模式 3：科目三返航惯导
            // --------------------------------------------------
#endif  // ================= 科目一自动复现分支屏蔽结束 =================

            case MODE_INS_TRACK_THREE:
            {
                // 科目三返航算法更新
                // 这里会更新：
                // ins_lateral_error
                // ins_target_yaw
                // ins_gyro_z
                // g_speed_mgr.target_final
                if (back_ins_blind_is_running())
                {
                    back_ins_blind_update();
                }
                else
                {
                    back_ins_update();
                }

                // back_ins_update() 输出的是等效真实轮角。
                float target_wheel_angle = ins_lateral_error;

                if (target_wheel_angle > STEER_WHEEL_MAX_ANGLE_DEG)
                {
                    target_wheel_angle = STEER_WHEEL_MAX_ANGLE_DEG;
                }
                else if (target_wheel_angle < -STEER_WHEEL_MAX_ANGLE_DEG)
                {
                    target_wheel_angle = -STEER_WHEEL_MAX_ANGLE_DEG;
                }

                // 只在舵机边界换回编码器角，原内环 PD 参数无需按 0.59 重调。
                float target_steer_sensor_angle =
                    steer_wheel_to_sensor_angle(target_wheel_angle);
                int16 steer_pwm = calc_inner_servo_pwm(
                    target_steer_sensor_angle,
                    current_steer_sensor_angle);
                set_motor_speed(steer_pwm);
                // 科目三只使用 back_ins 的速度目标，不再依赖科目一的
                // g_ins_neutral_glide 变量，彻底与 ins.c 的运行逻辑解耦。
                test_update_rear_wheels_for_ins(
                    g_speed_mgr.target_final,
                    current_real_wheel_angle);
                break;
            }
            // --------------------------------------------------
            // 模式 3: 回中控制
             // --------------------------------------------------
            case MODE_CENTERING:

                return_to_middle();

                Motor_Control(0, 0);

                break;

            // --------------------------------------------------
            // 模式 0: 安全停车模式 (默认状态)
            // --------------------------------------------------
            case MODE_STOP:
            default:
            {
               set_motor_speed(0);      // 强制切断前轮动力
               Motor_Control(0, 0);     // 强制切断后轮动力

               PID_Reset(&pid_left);
               PID_Reset(&pid_right);

               g_speed_mgr.target_final = 0.0f;
               g_speed_mgr.target_ramp = 0.0f;

               return_to_mid_flag = 0;

               break;
            }
    }

    {
        ahrs_data_t diagnosis_imu = imu_ahrs_get_data();
        color_vision_diagnosis_publish(
            current_real_wheel_angle,
            (encoder_speed_left + encoder_speed_right) * 0.5f,
            diagnosis_imu.gyro_z_dps);
    }

}


IFX_INTERRUPT(cc60_pit_ch1_isr, CCU6_0_CH1_INT_VECTAB_NUM, CCU6_0_CH1_ISR_PRIORITY)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    pit_clear_flag(CCU60_CH1);




}

IFX_INTERRUPT(cc61_pit_ch0_isr, CCU6_1_CH0_INT_VECTAB_NUM, CCU6_1_CH0_ISR_PRIORITY)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    pit_clear_flag(CCU61_CH0);


#if !SUB3_CAMERA_ONLY_MODE
    audio_callback();
#endif

}

IFX_INTERRUPT(cc61_pit_ch1_isr, CCU6_1_CH1_INT_VECTAB_NUM, CCU6_1_CH1_ISR_PRIORITY)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    pit_clear_flag(CCU61_CH1);





}
// **************************** PIT中断函数 ****************************


// **************************** 外部中断函数 ****************************
IFX_INTERRUPT(exti_ch0_ch4_isr, EXTI_CH0_CH4_INT_VECTAB_NUM, EXTI_CH0_CH4_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    if(exti_flag_get(ERU_CH0_REQ0_P15_4))           // 通道0中断
    {
        exti_flag_clear(ERU_CH0_REQ0_P15_4);
		imu660rc_callback();     					// 660RC 模块 INT 更新中断


    }

    if(exti_flag_get(ERU_CH4_REQ13_P15_5))          // 通道4中断
    {
        exti_flag_clear(ERU_CH4_REQ13_P15_5);




    }
}

IFX_INTERRUPT(exti_ch1_ch5_isr, EXTI_CH1_CH5_INT_VECTAB_NUM, EXTI_CH1_CH5_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套

    if(exti_flag_get(ERU_CH1_REQ10_P14_3))          // 通道1中断
    {
        exti_flag_clear(ERU_CH1_REQ10_P14_3);

        if (g_lamp_scan_enabled && g_tld_ready)
        {
            dot_matrix_screen_scan();               // CPU2: Parking Task 专用同步扫描
        }

    }

    if(exti_flag_get(ERU_CH5_REQ1_P15_8))           // 通道5中断
    {
        exti_flag_clear(ERU_CH5_REQ1_P15_8);



    }
}

// 由于摄像头pclk引脚默认占用了 2通道，用于触发DMA，因此这里不再定义中断函数
// IFX_INTERRUPT(exti_ch2_ch6_isr, EXTI_CH2_CH6_INT_VECTAB_NUM, EXTI_CH2_CH6_INT_PRIO)
// {
//  interrupt_global_enable(0);                     // 开启中断嵌套
//  if(exti_flag_get(ERU_CH2_REQ7_P00_4))           // 通道2中断
//  {
//      exti_flag_clear(ERU_CH2_REQ7_P00_4);
//  }
//  if(exti_flag_get(ERU_CH6_REQ9_P20_0))           // 通道6中断
//  {
//      exti_flag_clear(ERU_CH6_REQ9_P20_0);
//  }
// }

IFX_INTERRUPT(exti_ch3_ch7_isr, EXTI_CH3_CH7_INT_VECTAB_NUM, EXTI_CH3_CH7_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    if(exti_flag_get(ERU_CH3_REQ6_P02_0))           // 通道3中断
    {
        exti_flag_clear(ERU_CH3_REQ6_P02_0);
        if (!g_voice_task_active)
        {
            camera_vsync_handler();                 // Parking 外正常启动摄像头 DMA 采集
        }
    }
    if(exti_flag_get(ERU_CH7_REQ16_P15_1))          // 通道7中断
    {
        exti_flag_clear(ERU_CH7_REQ16_P15_1);




    }
}
// **************************** 外部中断函数 ****************************


// **************************** DMA中断函数 ****************************
IFX_INTERRUPT(dma_ch5_isr, DMA_INT_VECTAB_NUM, DMA_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    camera_dma_handler();                           // 摄像头采集完成统一回调函数
}
// **************************** DMA中断函数 ****************************


// **************************** 串口中断函数 ****************************
// 串口0默认作为调试串口
IFX_INTERRUPT(uart0_tx_isr, UART0_INT_VECTAB_NUM, UART0_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}
IFX_INTERRUPT(uart0_rx_isr, UART0_INT_VECTAB_NUM, UART0_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套

#if DEBUG_UART_USE_INTERRUPT                        // 如果开启 debug 串口中断
        debug_interrupr_handler();                  // 调用 debug 串口接收处理函数 数据会被 debug 环形缓冲区读取
#endif                                              // 如果修改了 DEBUG_UART_INDEX 那这段代码需要放到对应的串口中断去
}


// 串口1默认连接到摄像头配置串口
IFX_INTERRUPT(uart1_tx_isr, UART1_INT_VECTAB_NUM, UART1_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套




}
IFX_INTERRUPT(uart1_rx_isr, UART1_INT_VECTAB_NUM, UART1_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    if (g_tld_uart_active)
    {
        tld7002_callback();                         // CPU0: stable TLD7002 UART service after the core barrier
    }
    else
    {
        camera_uart_handler();                      // CPU0 startup: SCC8660 configuration response
    }
}

// 串口2默认连接到无线转串口模块
IFX_INTERRUPT(uart2_tx_isr, UART2_INT_VECTAB_NUM, UART2_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart2_rx_isr, UART2_INT_VECTAB_NUM, UART2_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    wireless_uart_callback();                       // UART2 固定为无线调试串口




}
// 串口3默认连接到GPS定位模块
IFX_INTERRUPT(uart3_tx_isr, UART3_INT_VECTAB_NUM, UART3_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart3_rx_isr, UART3_INT_VECTAB_NUM, UART3_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}


IFX_INTERRUPT(uart4_tx_isr, UART4_INT_VECTAB_NUM, UART4_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart4_rx_isr, UART4_INT_VECTAB_NUM, UART4_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart5_tx_isr, UART5_INT_VECTAB_NUM, UART5_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart5_rx_isr, UART5_INT_VECTAB_NUM, UART5_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart6_tx_isr, UART6_INT_VECTAB_NUM, UART6_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart6_rx_isr, UART6_INT_VECTAB_NUM, UART6_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart8_tx_isr, UART8_INT_VECTAB_NUM, UART8_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart8_rx_isr, UART8_INT_VECTAB_NUM, UART8_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart9_tx_isr, UART9_INT_VECTAB_NUM, UART9_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart9_rx_isr, UART9_INT_VECTAB_NUM, UART9_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart10_tx_isr, UART10_INT_VECTAB_NUM, UART10_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart10_rx_isr, UART10_INT_VECTAB_NUM, UART10_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    uart_receiver_callback();                       // UART10 固定为遥控接收器



}

IFX_INTERRUPT(uart11_tx_isr, UART11_INT_VECTAB_NUM, UART11_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}

IFX_INTERRUPT(uart11_rx_isr, UART11_INT_VECTAB_NUM, UART11_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套



}
// 串口通讯错误中断
IFX_INTERRUPT(uart0_er_isr, UART0_INT_VECTAB_NUM, UART0_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart0_handle);
}
IFX_INTERRUPT(uart1_er_isr, UART1_INT_VECTAB_NUM, UART1_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart1_handle);
}
IFX_INTERRUPT(uart2_er_isr, UART2_INT_VECTAB_NUM, UART2_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart2_handle);
}
IFX_INTERRUPT(uart3_er_isr, UART3_INT_VECTAB_NUM, UART3_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart3_handle);
}
IFX_INTERRUPT(uart4_er_isr, UART4_INT_VECTAB_NUM, UART4_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart4_handle);
}
IFX_INTERRUPT(uart5_er_isr, UART5_INT_VECTAB_NUM, UART5_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart5_handle);
}
IFX_INTERRUPT(uart6_er_isr, UART6_INT_VECTAB_NUM, UART6_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart6_handle);
}
IFX_INTERRUPT(uart8_er_isr, UART8_INT_VECTAB_NUM, UART8_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart8_handle);
}
IFX_INTERRUPT(uart9_er_isr, UART9_INT_VECTAB_NUM, UART9_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart9_handle);
}
IFX_INTERRUPT(uart10_er_isr, UART10_INT_VECTAB_NUM, UART10_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart10_handle);
}
IFX_INTERRUPT(uart11_er_isr, UART11_INT_VECTAB_NUM, UART11_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     // 开启中断嵌套
    IfxAsclin_Asc_isrError(&uart11_handle);
}
// **************************** 串口中断函数 ****************************
