/**
 * @file    bsp_adc.h
 * @brief   ADC 板级驱动接口 (STM32G071 ADC1)
 */
#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_ADC_CH_PACK = 0,
    BSP_ADC_CH_MID,
    BSP_ADC_CH_NTC_BATTERY,
    BSP_ADC_CH_NTC_PD,
    BSP_ADC_CH_COUNT
} bsp_adc_channel_t;

bool bsp_adc_init(void);

/**
 * @brief 读取一个通道的滤波后原始码。
 *
 * 内部做 8 次采样, 去掉最大/最小值后取平均, 以抑制开关电源噪声。
 */
bool bsp_adc_read_raw(bsp_adc_channel_t ch, uint16_t *raw);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ADC_H */
