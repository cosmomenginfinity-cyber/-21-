#ifndef MOTOR_H
#define MOTOR_H
#include "zf_common_typedef.h"
#include <Ifx_Types.h>  // °üº¬__abs()º¯Êý
#include "global_vars.h"
extern uint16 MOTOR_MAX_SPEED;
extern uint16 MOTOR_MIN_SPEED;

void set_motor_speed(float speed);
void Motor_Control(float duty_left, float duty_right);

#endif
