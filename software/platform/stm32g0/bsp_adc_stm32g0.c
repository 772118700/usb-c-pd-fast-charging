/**
 * @file    bsp_adc_stm32g0.c
 * @brief   STM32G071 ADC1 板级实现 (查询方式 + 8 次中值平均)
 *
 * CubeMX 配置:
 *   ADC1: 12 bit, 单次转换, 软件触发, 采样时间 160.5 cycles
 *   PA0 = ADC1_IN0 (PACK), PA1 = ADC1_IN1 (MID)
 *   PA4 = ADC1_IN4 (NTC_BAT), PA5 = ADC1_IN5 (NTC_PD)
 */
#include "bsp_adc.h"

#include "stm32g0xx_hal.h"

extern ADC_HandleTypeDef hadc1;

static uint32_t channel_of(bsp_adc_channel_t ch)
{
    switch (ch) {
    case BSP_ADC_CH_PACK:
        return ADC_CHANNEL_0;
    case BSP_ADC_CH_MID:
        return ADC_CHANNEL_1;
    case BSP_ADC_CH_NTC_BATTERY:
        return ADC_CHANNEL_4;
    case BSP_ADC_CH_NTC_PD:
        return ADC_CHANNEL_5;
    default:
        return 0xFFFFFFFFU;
    }
}

bool bsp_adc_init(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
        return false;
    }
    return true;
}

static bool adc_read_once(uint32_t channel, uint16_t *raw)
{
    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel = channel;
    cfg.Rank = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK) {
        return false;
    }
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return false;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 2U) != HAL_OK) {
        (void)HAL_ADC_Stop(&hadc1);
        return false;
    }
    uint32_t value = HAL_ADC_GetValue(&hadc1);
    (void)HAL_ADC_Stop(&hadc1);
    if (value > 4095U) {
        return false;
    }
    *raw = (uint16_t)value;
    return true;
}

bool bsp_adc_read_raw(bsp_adc_channel_t ch, uint16_t *raw)
{
    uint32_t channel = channel_of(ch);
    if (raw == 0 || channel == 0xFFFFFFFFU) {
        return false;
    }

    uint16_t samples[8];
    for (uint8_t i = 0U; i < 8U; ++i) {
        if (!adc_read_once(channel, &samples[i])) {
            return false;
        }
    }

    /* 插入排序 8 个样本, 去掉最大/最小后取平均。 */
    for (uint8_t i = 1U; i < 8U; ++i) {
        uint16_t key = samples[i];
        int8_t j = (int8_t)(i - 1U);
        while (j >= 0 && samples[j] > key) {
            samples[j + 1] = samples[j];
            --j;
        }
        samples[j + 1] = key;
    }

    uint32_t sum = 0U;
    for (uint8_t i = 1U; i < 7U; ++i) {
        sum += samples[i];
    }
    *raw = (uint16_t)((sum + 3U) / 6U);
    return true;
}

