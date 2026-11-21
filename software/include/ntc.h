/**
 * @file    ntc.h
 * @brief   10 kOhm B3950 NTC 温度换算与开路/短路诊断
 */
#ifndef NTC_H
#define NTC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t series_ohm;    /* 上拉电阻, NTC 接地 */
    uint16_t r25_ohm;
    uint16_t beta;
    uint16_t vref_mv;
} ntc_config_t;

typedef struct {
    int16_t temp_c_x10;     /* 0.1 degC */
    uint16_t adc_mv;
    uint32_t resistance_ohm;
    bool valid;
    bool open_circuit;
    bool short_circuit;
} ntc_reading_t;

/**
 * @brief 原始 ADC 码 -> 温度。
 * @param raw 0..4095
 */
bool ntc_raw_to_temp(uint16_t raw, const ntc_config_t *cfg, ntc_reading_t *out);

/**
 * @brief 节点电压 -> 温度 (便于单元测试/校准)。
 */
bool ntc_mv_to_temp(uint16_t adc_mv, const ntc_config_t *cfg, ntc_reading_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NTC_H */
