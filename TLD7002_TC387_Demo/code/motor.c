#include "motor.h"
#include "initialize.h"
#include "global_vars.h"
#include <math.h>

uint16 MOTOR_MAX_SPEED = 8000;
uint16 MOTOR_MIN_SPEED = 10;

void set_motor_speed(float speed)
{
    // 1. 浮点限幅处理（兼容TC387的FPU）
    speed = (speed > MOTOR_MAX_SPEED) ? MOTOR_MAX_SPEED :
            (speed < -MOTOR_MAX_SPEED) ? -MOTOR_MAX_SPEED : speed;

    // 2. 死区处理与零速优化
    if (fabsf(speed) <= MOTOR_MIN_SPEED) {
        pwm_set_duty(PWM_S, 5000);  // 中位停转
        pwm_set_duty(PWM_N, 5000);
        //pwm_set_duty(PWM_R1, 5000);  // 中位停转
        //pwm_set_duty(PWM_R2, 5000);
        return;
    }

    // 3. 浮点转整数（利用TC387的32位寄存器）
    int_fast32_t duty = (int_fast32_t)speed;

    // 4. 算术移位优化（替代除法）
    int_fast32_t half = duty >> 1;  // TC387支持单周期移位

    // 5. 生成互补PWM（并行计算）
    int_fast32_t pwm_S = 5000 + half;
    int_fast32_t pwm_N = 5000 - half;
    //int_fast32_t pwm_R1 = 5000 + half;
    //int_fast32_t pwm_R2 = 5000 - half;

    // 6. SIMD风格饱和处理
    pwm_S = (pwm_S < 0) ? 0 : (pwm_S > 10000) ? 10000 : pwm_S;
    pwm_N = (pwm_N < 0) ? 0 : (pwm_N > 10000) ? 10000 : pwm_N;
    //pwm_R1 = (pwm_R1 < 0) ? 0 : (pwm_R1 > 10000) ? 10000 : pwm_R1;
    //pwm_R2 = (pwm_R2 < 0) ? 0 : (pwm_R2 > 10000) ? 10000 : pwm_R2;

    // 7. 设置PWM（强制内联）
    pwm_set_duty(PWM_S, (uint32_t)pwm_S);
    pwm_set_duty(PWM_N, (uint32_t)pwm_N);
    //pwm_set_duty(PWM_R1, (uint32_t)pwm_R1);
    //pwm_set_duty(PWM_R2, (uint32_t)pwm_R2);
}

void Motor_Control(float duty_left, float duty_right) {
    // 1. 限幅处理（直接比较）
    duty_left = (duty_left > 10000.0f) ? 10000.0f :
               (duty_left < -10000.0f) ? -10000.0f : duty_left;
    duty_right = (duty_right > 10000.0f) ? 10000.0f :
                (duty_right < -10000.0f) ? -10000.0f : duty_right;

    // 2. 浮点转整数（使用int_fast16_t提高效率）
    int_fast16_t dutyL = (int_fast16_t)duty_left;
    int_fast16_t dutyR = (int_fast16_t)duty_right;

    // 3. 用移位代替除法计算PWM（等效于除以2但更快）
    int_fast16_t halfL = dutyL >> 1;  // 算术右移保持符号
    int_fast16_t halfR = dutyR >> 1;

    // 4. 计算互补PWM值（并行计算四个值）
    int_fast16_t pwm_L1 = 5000 + halfL;
    int_fast16_t pwm_L2 = 5000 - halfL;
    int_fast16_t pwm_R1 = 5000 + halfR;
    int_fast16_t pwm_R2 = 5000 - halfR;

    // 5. 饱和处理（SIMD风格优化）
    pwm_L1 = (pwm_L1 < 0) ? 0 : (pwm_L1 > 10000) ? 10000 : pwm_L1;
    pwm_L2 = (pwm_L2 < 0) ? 0 : (pwm_L2 > 10000) ? 10000 : pwm_L2;
    pwm_R1 = (pwm_R1 < 0) ? 0 : (pwm_R1 > 10000) ? 10000 : pwm_R1;
    pwm_R2 = (pwm_R2 < 0) ? 0 : (pwm_R2 > 10000) ? 10000 : pwm_R2;

    // 6. 设置PWM（强制内联可能提升速度）
    pwm_set_duty(PWM_L1, (uint32_t)pwm_L1);
    pwm_set_duty(PWM_L2, (uint32_t)pwm_L2);
    pwm_set_duty(PWM_R1, (uint32_t)pwm_R1);
    pwm_set_duty(PWM_R2, (uint32_t)pwm_R2);
}
