#include "remote_control.h"

#include "global_vars.h"

void remote_control_begin(void)
{
    tune_target_speed = 0.0f;
    tune_target_angle = 0.0f;
    uart_receiver.finsh_flag = 0;
    current_car_mode = MODE_RC;
}

void remote_control_update(void)
{
    if (uart_receiver.finsh_flag != 1)
    {
        return;
    }

    if (uart_receiver.state == 1)
    {
        tune_target_speed = map_sbus_to_speed(uart_receiver.channel[1]);
        tune_target_angle = map_sbus_to_angle(uart_receiver.channel[0]);
    }
    else
    {
        tune_target_speed = 0.0f;
        tune_target_angle = 0.0f;
    }

    uart_receiver.finsh_flag = 0;
}

void remote_control_end(void)
{
    tune_target_speed = 0.0f;
    tune_target_angle = 0.0f;
    current_car_mode = MODE_STOP;
}
