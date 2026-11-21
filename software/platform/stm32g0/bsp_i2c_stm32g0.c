/**
 * @file    bsp_i2c_stm32g0.c
 * @brief   STM32G071 I2C1 板级实现 (OLED + INA226 + 可选 PD 状态)
 *
 * CubeMX 配置:
 *   I2C1 400 kHz, 7-bit, PB6=SCL, PB7=SDA
 *   4.7k 上拉到 3.3 V, 不启用内部上拉
 */
#include "bsp_i2c.h"

#include "stm32g0xx_hal.h"

#include "app_config.h"

extern I2C_HandleTypeDef hi2c1;

bool bsp_i2c_init(void)
{
    return HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_RESET;
}

bool bsp_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (buf == 0 || len == 0U) {
        return false;
    }
    return HAL_I2C_Mem_Read(&hi2c1, (uint16_t)addr << 1, reg,
                            I2C_MEMADD_SIZE_8BIT, buf, len,
                            APP_I2C_TIMEOUT_MS) == HAL_OK;
}

bool bsp_i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if (buf == 0 || len == 0U) {
        return false;
    }
    return HAL_I2C_Mem_Write(&hi2c1, (uint16_t)addr << 1, reg,
                             I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len,
                             APP_I2C_TIMEOUT_MS) == HAL_OK;
}

bool bsp_i2c_write_raw(uint8_t addr, const uint8_t *data, uint16_t len)
{
    if (data == 0 || len == 0U) {
        return false;
    }
    return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)addr << 1,
                                   (uint8_t *)data, len,
                                   APP_I2C_TIMEOUT_MS) == HAL_OK;
}

bool bsp_i2c_recover(void)
{
    /* 只复位外设, 不改变 CubeMX 配置的 GPIO/时钟。 */
    if (HAL_I2C_DeInit(&hi2c1) != HAL_OK) {
        return false;
    }
    return HAL_I2C_Init(&hi2c1) == HAL_OK;
}
