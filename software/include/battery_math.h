/**
 * @file    battery_math.h
 * @brief   高压分压采集 -> 电池组/单体电压换算 (纯函数, 可在主机测试)
 */
#ifndef BATTERY_MATH_H
#define BATTERY_MATH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t vref_mv;
    uint16_t full_scale;
    uint32_t top_ohm;
    uint32_t bottom_ohm;
} voltage_divider_t;

typedef struct {
    uint16_t pack_mv;
    uint16_t cell_mv[2];
    uint16_t cell_delta_mv;
    bool pack_valid;        /* 总压/分压链路有效 */
    bool cells_valid;       /* 两节单体都合理 */
} battery_voltage_t;

/**
 * @brief 将 ADC 原始码换算为还原后的被测电压。
 * @return false: 原始码贴边 (开路/短路) 或参数非法。
 */
bool divider_raw_to_mv(uint16_t raw, const voltage_divider_t *div, uint16_t *out_mv);

/**
 * @brief 由总压原始码与中点原始码计算两节单体电压。
 *
 * cell1 = V(mid), cell2 = V(pack) - V(mid)。
 * 两节均使用相同分压比时可直接相减; 本函数也允许两路使用不同分压比,
 * 但为热管理简单, 应用层固定使用相同阻值。
 */
bool battery_calc_from_raw(uint16_t raw_pack,
                           uint16_t raw_mid,
                           const voltage_divider_t *pack_div,
                           const voltage_divider_t *cell_div,
                           battery_voltage_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_MATH_H */
