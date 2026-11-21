/**
 * @file    board_port_stm32g0.c
 * @brief   STM32G071 板级 GPIO/时间/临界区实现
 *
 * 引脚:
 *   PA6  故障 LED (高亮)
 *   PA7  PD inhibit / 负载开关 (可选, 高 = 禁止输出)
 *   PB0  用户按键 (低有效, 内部/外部上拉)
 */
#include "board_port.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32g0xx_hal.h"

#include "app_config.h"

void board_init(void)
{
    board_set_fault_led(false);
    board_set_pd_inhibit(false);
}

uint32_t board_millis(void)
{
    return HAL_GetTick();
}

void board_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void board_set_fault_led(bool on)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void board_set_pd_inhibit(bool inhibit)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,
                      inhibit ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void board_critical_enter(void)
{
    taskENTER_CRITICAL();
}

void board_critical_exit(void)
{
    taskEXIT_CRITICAL();
}

bool board_read_user_key(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET;
}
