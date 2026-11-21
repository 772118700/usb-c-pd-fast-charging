/**
 * @file    stm32g0xx_hal.h
 * @brief   Host syntax-check stub for the STM32G0 HAL (NOT a real HAL)
 */
#ifndef STM32G0XX_HAL_H
#define STM32G0XX_HAL_H

#include <stdint.h>

typedef enum {
    HAL_OK = 0,
    HAL_ERROR,
    HAL_BUSY,
    HAL_TIMEOUT
} HAL_StatusTypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;

typedef struct {
    int dummy;
} GPIO_TypeDef;

typedef struct {
    int dummy;
} ADC_HandleTypeDef;

typedef struct {
    int dummy;
} I2C_HandleTypeDef;

typedef struct {
    uint32_t Channel;
    uint32_t Rank;
    uint32_t SamplingTime;
} ADC_ChannelConfTypeDef;

typedef enum {
    HAL_I2C_STATE_RESET = 0,
    HAL_I2C_STATE_READY,
    HAL_I2C_STATE_BUSY
} HAL_I2C_StateTypeDef;

extern GPIO_TypeDef *GPIOA;
extern GPIO_TypeDef *GPIOB;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;

#define GPIO_PIN_0  ((uint16_t)0x0001U)
#define GPIO_PIN_6  ((uint16_t)0x0040U)
#define GPIO_PIN_7  ((uint16_t)0x0080U)

#define ADC_CHANNEL_0 ((uint32_t)0U)
#define ADC_CHANNEL_1 ((uint32_t)1U)
#define ADC_CHANNEL_4 ((uint32_t)4U)
#define ADC_CHANNEL_5 ((uint32_t)5U)
#define ADC_REGULAR_RANK_1 ((uint32_t)1U)
#define ADC_SAMPLETIME_160CYCLES_5 ((uint32_t)7U)

#define I2C_MEMADD_SIZE_8BIT ((uint16_t)1U)

uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *hadc,
                                        ADC_ChannelConfTypeDef *config);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t addr,
                                   uint16_t mem, uint16_t mem_size,
                                   uint8_t *buf, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t addr,
                                    uint16_t mem, uint16_t mem_size,
                                    uint8_t *buf, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t addr,
                                          uint8_t *buf, uint16_t len, uint32_t timeout);
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c);

#endif /* STM32G0XX_HAL_H */
