#ifndef _INITIALIZE_H_
#define _INITIALIZE_H_
#include "IfxStm.h"
#include "global_vars.h"
#include "menu.h"

/*#define ENCODER_DIR_LEFT                (TIM6_ENCODER)
#define ENCODER_DIR_PULSE_LEFT          (TIM6_ENCODER_CH1_P20_3)
#define ENCODER_DIR_DIR_LEFT            (TIM6_ENCODER_CH2_P20_0)*/
#define ENCODER_DIR_RIGHT                 (TIM2_ENCODER)
#define ENCODER_DIR_PULSE_RIGHT           (TIM2_ENCODER_CH1_P33_7)
#define ENCODER_DIR_DIR_RIGHT             (TIM2_ENCODER_CH2_P33_6)
#define ENCODER_DIR_LEFT                  (TIM4_ENCODER)
#define ENCODER_DIR_PULSE_LEFT            (TIM4_ENCODER_CH1_P02_8)
#define ENCODER_DIR_DIR_LEFT              (TIM4_ENCODER_CH2_P00_9)

#define PWM_S              (ATOM1_CH4_P02_4)
#define PWM_N              (ATOM1_CH5_P02_5)
#define PWM_L1              (ATOM0_CH1_P21_3)
#define PWM_L2              (ATOM1_CH6_P20_10)
#define PWM_R2              (ATOM0_CH3_P21_5)
#define PWM_R1              (ATOM0_CH2_P21_4)

/*#define KEY1                    (P20_6)
#define KEY2                    (P20_7)
#define KEY3                    (P20_8)
#define KEY4                    (P20_9)*/

#define KEY1                    (P20_6)
#define KEY2                    (P20_7)
#define KEY3                    (P11_2)
#define KEY4                    (P11_3)



#define DRIVER_MOTOR            (P21_6)

#define SUB2_HORN_PWM_PIN       (ATOM3_CH4_P33_8)

#define IPS200_TYPE     (IPS200_TYPE_SPI)
//按键参数
#define RAMP_STEP  5.0f
#define KEY_DEBOUNCE_TIME           20      // 消抖时间（20ms
// ================= 车辆物理参数 (全部统一为 米 m) =================
#define DEG_TO_RAD_ANGLE(x)        ((x) * PI / 180.0f)

#define WHEEL_BASE           0.58f      // 前后轴距 60cm -> 0.60m
#define TRACK_WIDTH          0.50f      // 左右轮距 50cm -> 0.50m

// 轮胎参数 (基于后轮计算测速)
#define REAR_WHEEL_DIAMETER  0.24f      // 后轮直径 24cm -> 0.24m
// 注意：修复了你原代码里 REAR_WHEEL_DIAMETER_M 的命名笔误
#define REAR_WHEEL_PERIMETER (REAR_WHEEL_DIAMETER * PI) // 后轮周长约 0.7539m

// 控制系统参数
#define ENCODER_PPR          2048.0f    // 轮子转一圈的脉冲数 (请确认这是否考虑了减速比)
#define CONTROL_FREQ         100.0f     // 控制频率 100Hz (对应 10ms 定时器)
#define MAX_SPEED_M_S        10.0f       // 最大限制速度 3m/s

// 其他
#define DIR_MID              1744.0f    // 舵机中位      2. 放入源文件（ .c ）的控制函数
void Initialize(void);

#endif
