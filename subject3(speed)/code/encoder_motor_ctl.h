#ifndef ENCODER_MOTOR_CTL_H
#define ENCODER_MOTOR_CTL_H

#include "zf_common_headfile.h"
#include "global_vars.h"   // 依赖全局变量
#include "PID.h"           // 依赖PID模块
#include "motor.h"   // 依赖电机驱动模块

void Speed_Ramp_Update(Speed_Manager_t *mgr);
void test_update_rear_wheels_for_ins(
    float target_base_speed,
    float current_wheel_angle);
void System_Background_Tasks(void);
#endif // ENCODER_MOTOR_CTL_H
