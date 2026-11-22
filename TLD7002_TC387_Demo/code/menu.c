#include "menu.h"
#include "pathrecorder.h"
#include "remote_control.h"
#include "sub2_menu.h"
#include <stdio.h>

// ==================== 引用外部全局变量 ====================
extern volatile float ins_cruise_speed;

// ================= 状态机变量 =================
int current_level = 0;
int current_index = 0;
int last_level = -1;
int last_index = -1;
int in_function = 0;

// 每层菜单包含的选项数量：
// [0] 欢迎页:1个
// [1] 主菜单:3个
// [2] 科目一:6项
// [3] 科目二:4项（TLD7002 点位/纯 PP）
int item_counts[] = {1, 3, 6, 4};

// 记录每一层的“上一层”
int parent_level[] = {-1, 0, 1, 1};

// 菜单数组：4 行 6 列，防止越界
MenuItem menu[4][6];

// ======================================================================
// 1. 各个层级的静态 UI 绘制
// ======================================================================
void display_level_0()
{
    ips200_show_string(60, 16 * 4, "W E L C O M E");
    ips200_show_string(30, 16 * 6, "Press KEY3 to Start");
}

void display_level_1()
{
    ips200_show_string(0, 0, "=== Main Menu ===");
    ips200_show_string(10, 16 * 2, "1. Subject 1 (INS)");
    ips200_show_string(10, 16 * 3, "2. Subject 2");
    ips200_show_string(10, 16 * 4, "3. Servo Center");
}

void display_level_2() // 科目一专属子菜单
{
    ips200_show_string(0, 0, "=== Subject 1 ===");
    ips200_show_string(10, 16 * 2, "1. Record Path (RC)");//遥控+记点
    ips200_show_string(10, 16 * 3, "2. Save to Flash");//保存
    ips200_show_string(10, 16 * 4, "3. Set Cruise Speed");//设定速度
    ips200_show_string(10, 16 * 5, "4. Playback Path");//发车
    ips200_show_string(10, 16 * 6, "5. Load from Flash");
    ips200_show_string(10, 16 * 7, "6. Clear Flash");
}

void display_level_3() // SUBJECT 2: TLD7002 pure-PP tasks
{
    ips200_show_string(0, 0, "=== Subject 2 ===");
    ips200_show_string(10, 16 * 2, "1. Record Waypoints");
    ips200_show_string(10, 16 * 3, "2. Clear Waypoints");
    ips200_show_string(10, 16 * 4, "3. Print Waypoints");
    ips200_show_string(10, 16 * 5, "4. Voice Control Run");
}

// ======================================================================
// 2. 具体功能函数逻辑 (动作层)
// ======================================================================

// 科目一专用的调速函数 (BPM 调整逻辑保持不变)
void Action_SetCruiseSpeed(void)
{
    ips200_clear();
    ips200_show_string(0, 16 * 2, ">> SET SPEED <<");
    ips200_show_string(0, 16 * 5, "KEY1: +0.50 m/s");
    ips200_show_string(0, 16 * 6, "KEY2: -0.50 m/s");
    ips200_show_string(10, 16 * 7, "KEY3/4: Save/Exit");
    float temp_speed = ins_cruise_speed;

    char speed_buf[32];
    while(1)
    {
        key_scan();
        snprintf(speed_buf, sizeof(speed_buf), "SPEED: %.2f m/s ", temp_speed);
        ips200_show_string(10, 16 * 4, speed_buf);

        if (key1_action)
        {
            key1_action = 0;
            temp_speed += 0.50f;
            if (temp_speed > 8.0f) temp_speed = 8.0f;
        }
        if (key2_action)
        {
            key2_action = 0;
            temp_speed -= 0.50f;
            if (temp_speed < 0.10f) temp_speed = 0.10f;
        }
        if (key3_action)
        {
            key3_action = 0;
            ins_cruise_speed = temp_speed;
            ips200_show_string(10, 16 * 4, " SAVED OK! ");
            system_delay_ms(800);
            break;
        }
        if (key4_action)
        {
            key4_action = 0;
            break;
        }
        system_delay_ms(10);
    }
}

//前轮回中 (严禁修改)
void Action_ServoCenter(void)
{
    ips200_clear();
    ips200_show_string(0, 16 * 3, ">> CENTERING <<");
    ips200_show_string(0, 16 * 5, "Please Wait...");
    ips200_show_string(0, 16 * 7, "KEY4 to Cancel");

    current_car_mode = MODE_CENTERING;
    return_to_mid_flag = 1;

    while(return_to_mid_flag)
    {
        key_scan();
        if (key4_action)
        {
            key4_action = 0;
            return_to_mid_flag = 0;
            set_motor_speed(0);
            break;
        }
    }
    current_car_mode = MODE_STOP;
    ips200_clear();
    ips200_show_string(0, 16 * 4, "Center Finished!");
    system_delay_ms(800);
}

void Action_Record(void)
{
    ips200_clear();
    ips200_show_string(0, 16*2, ">> RC DRIVING <<");
    ips200_show_string(0, 16*4, "KEY3: Mark Switch");
    ips200_show_string(0, 16*5, "RC Driving...");
    ips200_show_string(10, 16*7, "KEY4: Save End");

    uint8 mark_message_active = 0;
    uint32 mark_message_time = 0;
    uint8 end_mark_result = PATH_MARK_FAILED;

    // KEY3 is also used to enter this menu item. Clear the old edge so the
    // start point cannot be mistaken for a switch point.
    key3_action = 0;
    key4_action = 0;

    remote_control_begin();
    encoder_reset_all_data();
    imu_ahrs_reset_position();

    path_recorder_start();

    while(1)
    {
        key_scan();
        path_recorder_update();

        remote_control_update();

        if (key3_action == 1)
        {
            key3_action = 0;
            uint8 mark_result = path_recorder_mark_switch_point();

            if (mark_result == PATH_MARK_OK)
            {
                ips200_show_string(0, 16*5, "Switch Point OK!   ");
            }
            else if (mark_result == PATH_MARK_MOVING)
            {
                ips200_show_string(0, 16*5, "Stop Car First!    ");
            }
            else if (mark_result == PATH_MARK_ALREADY)
            {
                ips200_show_string(0, 16*5, "Switch Already Set ");
            }
            else
            {
                ips200_show_string(0, 16*5, "Switch Save Failed ");
            }

            mark_message_active = 1;
            mark_message_time = g_system_time_ms;
        }

        if (mark_message_active
            && ((uint32)(g_system_time_ms - mark_message_time) >= 800U))
        {
            ips200_show_string(0, 16*5, "RC Driving...       ");
            mark_message_active = 0;
        }

        if (key4_action == 1)
        {
            key4_action = 0;
            break;
        }
    }

    // Subject 1 must persist the exact physical endpoint instead of using
    // the last periodic 20cm sample as the path end.
    end_mark_result = path_recorder_force_end_point();
    path_recorder_stop();
    remote_control_end();

    if ((end_mark_result == PATH_MARK_OK)
        || (end_mark_result == PATH_MARK_ALREADY))
    {
        ips200_show_string(0, 16*5, "End Point Saved!    ");
    }
    else
    {
        ips200_show_string(0, 16*5, "End Save Failed!    ");
    }

    if (!path_recorder_subject1_is_complete())
    {
        ips200_show_string(0, 16*4, "WARNING: No Switch! ");
    }
    system_delay_ms(600);
}

void Action_Playback(void)
{
    ips200_clear();
    ips200_show_string(0, 16*2, ">> AUTO TRACK <<");
    ips200_show_string(0, 16*4, "Checking Data...");
    ips200_show_string(10, 16*7, "KEY4 to Stop");

    uint32 total_points = flash_log_get_count();
    if (total_points == 0)
    {
        ips200_show_string(0, 16*5, "NO FLASH DATA!");
        system_delay_ms(1500);
        return;
    }

    ips200_show_string(0, 16*4, "Running...      ");

    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
    encoder_reset_all_data();

    ins_start_navigation();

    current_car_mode = MODE_INS_TRACK;
    uint32 last_run_time = g_system_time_ms;

    while(1)
    {
        key_scan();
         if (g_system_time_ms - last_run_time >= 10)
         {
             last_run_time = g_system_time_ms;
             if (!ins_is_navigating())
             {
                 ips200_show_string(0, 16*4, "Arrived!");
                 break;
             }
         }
        if (key4_action == 1)
        {
            key4_action = 0;
            ips200_show_string(0, 16*5, "Emergency Stop!");
            break;
        }
    }

    ins_stop_navigation();
    current_car_mode = MODE_STOP;
    system_delay_ms(1000);
}

void Action_SaveFlash(void)
{
    ips200_clear();
    ips200_show_string(0, 3, "Saving to Flash...");

    if (!path_recorder_subject1_is_complete())
    {
        ips200_show_string(0, 16*6, "Switch/End Missing!");
        system_delay_ms(1500);
        return;
    }

    uint32 buffer_count = imu_logger_get_buffer_count();
    if (buffer_count > FLASH_MAX_LOG_RECORDS)
    {
        char capacity_text[32];
        snprintf(capacity_text, sizeof(capacity_text),
                 "Max Points: %lu", (unsigned long)FLASH_MAX_LOG_RECORDS);
        ips200_show_string(0, 16*5, "Path Too Long!  ");
        ips200_show_string(0, 16*6, capacity_text);
        system_delay_ms(1500);
        return;
    }

    // A Subject 1 recording replaces the previous path. Appending would
    // concatenate two routes and make playback find the old switch first.
    // Check capacity before clear, so an oversized new route cannot erase
    // the last valid route already stored in Flash.
    uint8 ret = FLASH_ERR;
    if ((buffer_count > 0U)
        && (imu_logger_clear_flash() == FLASH_OK))
    {
        ret = imu_logger_save_buffer_to_flash();
    }
    if (ret == FLASH_OK || ret == 1)
    {
        ips200_show_string(0, 16*6, "Save Success!   ");
    }
    else
    {
        ips200_show_string(0, 16*6, "Save FAILED!    ");
    }
    system_delay_ms(1500);
}

void Action_LoadFlash(void)
{
    ips200_clear();
    ips200_show_string(0, 16*2, ">> EXPORT CSV <<");
    ips200_show_string(0, 16*4, "Open PC Serial!");
    ips200_show_string(0, 16*5, "Exporting...");

    uint8 ret = imu_logger_export_csv(0, 99999);
    if (ret == 1)
    {
        ips200_show_string(0, 16*7, "Export Finish!  ");
    }
    else
    {
        ips200_show_string(0, 16*7, "No Data/Error!  ");
    }
    system_delay_ms(1500);
}

void Action_ClearFlash(void)
{
    ips200_clear();
    ips200_show_string(0, 16*2, ">> CLEAR FLASH <<");
    ips200_show_string(0, 16*4, "Formatting...");

    uint8 ret = imu_logger_clear_flash();
    if (ret == 1 || ret == FLASH_OK)
    {
        ips200_show_string(0, 16*6, "Clear Success!  ");
    }
    else
    {
        ips200_show_string(0, 16*6, "Clear FAILED!   ");
    }
    system_delay_ms(1500);
}

// ======================================================================
// 3. 菜单节点映射初始化
// ======================================================================
void Menu_Init()
{
    // [Level 0] 欢迎页
    menu[0][0].display = display_level_0;
    menu[0][0].action = NULL;
    menu[0][0].next_level = 1; // 确认键跳转 Level 1

    // [Level 1] 主菜单
    for(int i=0; i<item_counts[1]; i++) menu[1][i].display = display_level_1;
    menu[1][0].next_level = 2; // 选"科目1"，跳转 Level 2
    menu[1][1].next_level = 3; // 选"科目2"，跳转 Level 3
    menu[1][2].action = Action_ServoCenter;
    menu[1][2].next_level = -1;

    // [Level 2] 科目一 子菜单
    for(int i=0; i<item_counts[2]; i++) menu[2][i].display = display_level_2;
    menu[2][0].action = Action_Record;      menu[2][0].next_level = -1;
    menu[2][1].action = Action_SaveFlash;   menu[2][1].next_level = -1;
    menu[2][2].action = Action_SetCruiseSpeed; menu[2][2].next_level = -1;
    menu[2][3].action = Action_Playback;    menu[2][3].next_level = -1;
    menu[2][4].action = Action_LoadFlash;   menu[2][4].next_level = -1;
    menu[2][5].action = Action_ClearFlash;  menu[2][5].next_level = -1;

    // [Level 3] SUBJECT 2 actions: TLD7002 pure PP and voice control
    for(int i=0; i<item_counts[3]; i++) menu[3][i].display = display_level_3;
    menu[3][0].action = Action_Sub2_Record;    menu[3][0].next_level = -1;
    menu[3][1].action = Action_Sub2_Clear;     menu[3][1].next_level = -1;
    menu[3][2].action = Action_Sub2_PrintWPs;  menu[3][2].next_level = -1;
    menu[3][3].action = Action_Sub2_VoiceRun;  menu[3][3].next_level = -1;
}

// ======================================================================
// 4. 菜单状态机主引擎 (保持原样)
// ======================================================================
void Menu()
{
    if(in_function)
    {
        MenuItem* item = &menu[current_level][current_index];
        if(item->action != NULL)
        {
            item->action();
        }

        in_function = 0;
        system_delay_ms(200);

        key1_action = 0;
        key2_action = 0;
        key3_action = 0;
        key4_action = 0;

        ips200_clear();
        if(item->display) item->display();
        last_level = current_level;
        last_index = current_index;
    }
    else
    {
        if(key1_action) // 上翻
        {
            key1_action = 0;
            if(item_counts[current_level] > 1)
                current_index = (current_index == 0) ? item_counts[current_level] - 1 : current_index - 1;
        }

        if(key2_action) // 下翻
        {
            key2_action = 0;
            if(item_counts[current_level] > 1)
                current_index = (current_index + 1) % item_counts[current_level];
        }

        if(key3_action) // 确认
        {
            key3_action = 0;
            MenuItem* item = &menu[current_level][current_index];

            if(item->next_level != -1)
            {
                last_level = current_level;
                last_index = current_index;
                current_level = item->next_level;
                current_index = 0;
            }
            else
            {
                ips200_clear();
                in_function = 1;
            }
        }

        if(key4_action) // 返回
        {
            key4_action = 0;
            if(parent_level[current_level] != -1)
            {
                last_level = current_level;
                last_index = current_index;
                current_level = parent_level[current_level];
                current_index = 0;
            }
        }

        if(current_level != last_level || current_index != last_index)
        {
            MenuItem* item = &menu[current_level][current_index];
            ips200_clear();
            if(item->display) item->display();

            if(current_level > 0)
            {
                uint16 cursor_y = (uint16)(16 * (current_index + 2));
                ips200_show_string(180, cursor_y, "<-");
            }

            last_level = current_level;
            last_index = current_index;
        }
    }
}
