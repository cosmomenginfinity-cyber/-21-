#include "key_mode.h"
#include "zf_common_headfile.h"

// 按键扫描函数
void key_scan(void)
{
    static uint8 key1_last_state = 1;  // 初始状态为释放
    static uint8 key1_debounce = 0;
    static uint8 key2_last_state = 1;  // 初始状态为释放
    static uint8 key2_debounce = 0;
    static uint8 key3_last_state = 1;  // 初始状态为释放
    static uint8 key3_debounce = 0;
    static uint8 key4_last_state = 1;  // 初始状态为释放
    static uint8 key4_debounce = 0;

    // 扫描KEY1
    uint8 key1_current = gpio_get_level(KEY1);
    if (key1_current != key1_last_state)
    {
        key1_debounce++;
        if (key1_debounce >= KEY_DEBOUNCE_TIME)
        {
            key1_last_state = key1_current;
            key1_debounce = 0;

            if (key1_current == 0)  // 按键按下
            {
                key1_action = 1;  // 标记按键动作
            }
        }
    }
    else
    {
        key1_debounce = 0;
    }

    // 扫描KEY2
    uint8 key2_current = gpio_get_level(KEY2);
    if (key2_current != key2_last_state)
    {
        key2_debounce++;
        if (key2_debounce >= KEY_DEBOUNCE_TIME)
        {
            key2_last_state = key2_current;
            key2_debounce = 0;

            if (key2_current == 0)  // 按键按下
            {
                key2_action = 1;  // 标记按键动作
            }
        }
    }
    else
    {
        key2_debounce = 0;
    }
    // 扫描KEY3
    uint8 key3_current = gpio_get_level(KEY3);
    if (key3_current != key3_last_state)
        {
            key3_debounce++;
            if (key3_debounce >= KEY_DEBOUNCE_TIME)
            {
                key3_last_state = key3_current;
                key3_debounce = 0;

                if (key3_current == 0)  // 按键按下
                {
                    key3_action = 1;  // 标记按键动作
                }
            }
        }
        else
        {
            key3_debounce = 0;
        }
    // 扫描KEY4
        uint8 key4_current = gpio_get_level(KEY4);
        if (key4_current != key4_last_state)
            {
                key4_debounce++;
                if (key4_debounce >= KEY_DEBOUNCE_TIME)
                {
                    key4_last_state = key4_current;
                    key4_debounce = 0;

                    if (key4_current == 0)  // 按键按下
                    {
                        key4_action = 1;  // 标记按键动作
                    }
                }
            }
            else
            {
                key4_debounce = 0;
            }
}
// 处理按键动作
// 处理按键动作函数
void process_key_action(void)
{
    // ================== KEY1：启动/停止阶跃测试 ==================
    if (key1_action)
    {
        key1_action = 0; // 消耗按键动作

        if (tune_target_speed == 0.0f)
        {
            // 如果当前是停止状态，则给定 2.0 m/s 的目标速度，开启阶跃响应
            tune_target_speed = 2.0f;
            // 建议在每次启动前清零 PID 内部状态，保证每次测试的起点一致
            PID_Reset(&pid_left);
            PID_Reset(&pid_right);
        }
        else
        {
            // 如果已经在运行，再次按下则停止
            tune_target_speed = 0.0f;
        }
    }

    // ================== KEY2：紧急停止 ==================
    if (key2_action)
    {
        key2_action = 0; // 消耗按键动作

        tune_target_speed = 0.0f; // 强制目标速度归零

        // 紧急停止时重置PID积分和微分历史，让电机彻底失去动力
        PID_Reset(&pid_left);
        PID_Reset(&pid_right);

        // 直接下发 PWM 为 0，确保第一时间断电
        Motor_Control(0, 0);
    }

    // ================== KEY3：前轮(舵机)回中测试 ==================
    if (key3_action)
    {
        key3_action = 0; // 消耗按键动作

        // 触发回中任务 (你的 main 函数里有 if(return_to_mid_flag) return_to_middle(); )
        return_to_mid_flag = 1;
    }
}
