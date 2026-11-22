#ifndef MENU_H
#define MENU_H

#include "zf_common_headfile.h"
#include "global_vars.h"
#include "initialize.h"
#include "key_mode.h"
#include "PID.h"
#include "flash.h"
#include "encoder.h"
#include "ins.h"
#include "pathrecorder.h"
#include "imu_datalogger.h"
#include "imu_ahrs.h"
#include "encoder_motor_ctl.h"
// ================= 菜单结构定义 =================
typedef struct {
    void (*display)(void); // 函数指针：负责界面文字显示
    void (*action)(void);  // 函数指针：如果是最终功能页，执行具体操作
    int next_level;        // 核心跳转逻辑：-1代表到底了执行action，其余代表跳转到哪个Level
} MenuItem;

// ================= 菜单系统全局变量 =================
extern MenuItem menu[4][6];
extern int item_counts[];
extern int current_level;
extern int current_index;
extern int in_function;

// ================= 菜单核心API =================
void Menu_Init(void); // 在主函数初始化中调用一次
void Menu(void);      // 在主循环 while(1) 中循环调用

// ================= 各层级页面绘制函数 =================
void display_level_0(void); // 欢迎页
void display_level_1(void); // 主菜单 (选科目)
void display_level_2(void); // 科目一 子菜单
void display_level_3(void); // 科目二 子菜单

// ================= 科目一 动作执行函数 =================
void Action_Record(void);    // 遥控录制 (已集成你的遥控解析)
void Action_Playback(void);  // 惯导复现
void Action_SaveFlash(void); // 存入Flash
void Action_LoadFlash(void); // 读取Flash
void Action_SetCruiseSpeed(void);

#endif
