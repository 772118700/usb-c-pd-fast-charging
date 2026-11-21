/**
 * @file    board_port.h
 * @brief   板级抽象: 时间、GPIO、临界区
 *
 * STM32G071CBT6 最小系统板建议引脚:
 *   PA0  ADC1_IN0  电池组总压分压
 *   PA1  ADC1_IN1  电池组中点分压
 *   PA4  ADC1_IN4  电池 NTC
 *   PA5  ADC1_IN5  PD/MOS NTC
 *   PB6  I2C1_SCL  OLED + INA226 (+ 可选 PD 状态)
 *   PB7  I2C1_SDA
 *   PA2  USART2_TX 调试串口 (可选)
 *   PA3  USART2_RX
 *   PA6  GPIO_OUT  故障 LED (高有效)
 *   PA7  GPIO_OUT  PD inhibit (可选, 默认不启用)
 *   PA13/PA14      SWD
 */
#ifndef BOARD_PORT_H
#define BOARD_PORT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void board_init(void);
uint32_t board_millis(void);
void board_delay_ms(uint32_t ms);

void board_set_fault_led(bool on);
void board_set_pd_inhibit(bool inhibit);

/* 任务上下文临界区; 主机测试中为空操作。 */
void board_critical_enter(void);
void board_critical_exit(void);

/* 可选用户按键: 未焊接时返回 false。 */
bool board_read_user_key(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_PORT_H */
