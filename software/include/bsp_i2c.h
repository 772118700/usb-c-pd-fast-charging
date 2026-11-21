/**
 * @file    bsp_i2c.h
 * @brief   I2C1 板级驱动接口 (OLED / INA226 / 可选 PD 状态)
 */
#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bsp_i2c_init(void);

bool bsp_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);
bool bsp_i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len);

/** OLED 连续写, data[0] 为 0x00 (命令) 或 0x40 (数据) 控制字节。 */
bool bsp_i2c_write_raw(uint8_t addr, const uint8_t *data, uint16_t len);

/** 总线异常后的恢复: 重新初始化 I2C 外设 (不改变引脚配置)。 */
bool bsp_i2c_recover(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_I2C_H */
