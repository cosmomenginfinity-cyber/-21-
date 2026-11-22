#ifndef _KEY_MODE_H_
#define _KEY_MODE_H_
#include "global_vars.h"
#include "encoder.h"
#include "initialize.h"
#include "motor.h"
#include "PID.h"
void key_scan(void);                         // 按键扫描函数
void process_key_action(void);               // 按键动作处理

#endif
