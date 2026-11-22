#include "key_mode.h"
#include "zf_common_headfile.h"

typedef struct
{
    uint8 stable_state;
    uint8 candidate_state;
    uint32 candidate_since_ms;
    uint8 armed;
} key_debounce_state_t;

static void key_scan_one(gpio_pin_enum pin,
                         key_debounce_state_t *state,
                         volatile int8 *action)
{
    uint8 current_state = gpio_get_level(pin);
    uint32 now_ms = g_system_time_ms;

    if (current_state != state->candidate_state)
    {
        state->candidate_state = current_state;
        state->candidate_since_ms = now_ms;
        return;
    }

    /* Require a stable released state before accepting the first press.
     * This prevents a power-on low-level glitch from creating a false event
     * and leaving the input state stuck at pressed. */
    if (!state->armed)
    {
        *action = 0;
        if ((current_state == 1U) &&
            ((uint32)(now_ms - state->candidate_since_ms) >=
             KEY_DEBOUNCE_TIME_MS))
        {
            state->stable_state = 1U;
            state->armed = 1U;
        }
        return;
    }

    if ((current_state != state->stable_state) &&
        ((uint32)(now_ms - state->candidate_since_ms) >=
         KEY_DEBOUNCE_TIME_MS))
    {
        state->stable_state = current_state;
        if (current_state == 0U)
        {
            *action = 1;
            state->armed = 0U;
        }
    }
}

// 按键扫描函数：四个实体按键统一使用真实的 60 ms 时间消抖。
void key_scan(void)
{
    static key_debounce_state_t key1_state = {1U, 1U, 0U, 0U};
    static key_debounce_state_t key2_state = {1U, 1U, 0U, 0U};
    static key_debounce_state_t key3_state = {1U, 1U, 0U, 0U};
    static key_debounce_state_t key4_state = {1U, 1U, 0U, 0U};

    key_scan_one(KEY1, &key1_state, &key1_action);
    key_scan_one(KEY2, &key2_state, &key2_action);
    key_scan_one(KEY3, &key3_state, &key3_action);
    key_scan_one(KEY4, &key4_state, &key4_action);
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
