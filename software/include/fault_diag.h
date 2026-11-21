/**
 * @file    fault_diag.h
 * @brief   集中故障诊断、去抖、锁存与输出闭锁请求
 *
 * 故障分级:
 *   - OV / UV / OC_CHG / OC_DIS: 去抖后置位并锁存, 需显式复位。
 *   - TEMP / TEMP_PD: 去抖后置位, 温度恢复后自动清除, 不锁存。
 *   - IMBALANCE / SENSOR_* / PD_COMM / SOC_LOW: 告警类, 不锁存。
 *
 * 重要: MCU 是“监督者”, 不是锂电主保护。即使 inhibit=1, 也必须依赖
 * BMS 的过充/过放/过流/短路保护与 IP5389 自身的保护。
 */
#ifndef FAULT_DIAG_H
#define FAULT_DIAG_H

#include <stdbool.h>
#include <stdint.h>

#include "thermal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FAULT_BIT_OV            0U
#define FAULT_BIT_UV            1U
#define FAULT_BIT_OC_CHG        2U
#define FAULT_BIT_OC_DIS        3U
#define FAULT_BIT_TEMP          4U
#define FAULT_BIT_TEMP_PD       5U
#define FAULT_BIT_IMBALANCE     6U
#define FAULT_BIT_SENSOR_ADC    7U
#define FAULT_BIT_SENSOR_CURRENT 8U
#define FAULT_BIT_SENSOR_TEMP   9U
#define FAULT_BIT_PD_COMM       10U
#define FAULT_BIT_SOC_LOW       11U
#define FAULT_BIT_COUNT         12U

#define FAULT_MASK(b)           (1UL << (b))
#define FAULT_LATCH_MASK        (FAULT_MASK(FAULT_BIT_OV) | \
                                 FAULT_MASK(FAULT_BIT_UV) | \
                                 FAULT_MASK(FAULT_BIT_OC_CHG) | \
                                 FAULT_MASK(FAULT_BIT_OC_DIS))
#define FAULT_INHIBIT_MASK      (FAULT_LATCH_MASK | \
                                 FAULT_MASK(FAULT_BIT_TEMP) | \
                                 FAULT_MASK(FAULT_BIT_TEMP_PD))

typedef struct {
    uint16_t cell_ov_trip_mv;
    uint16_t cell_ov_clear_mv;
    uint16_t pack_ov_trip_mv;
    uint16_t pack_ov_clear_mv;
    uint16_t cell_uv_trip_mv;
    uint16_t cell_uv_clear_mv;
    uint16_t pack_uv_trip_mv;
    uint16_t pack_uv_clear_mv;
    int16_t oc_chg_trip_ma;
    int16_t oc_chg_clear_ma;
    int16_t oc_dis_trip_ma;
    int16_t oc_dis_clear_ma;
    uint16_t imbalance_trip_mv;
    uint16_t imbalance_clear_mv;
    uint8_t soc_low_pct;
    uint32_t debounce_ms[FAULT_BIT_COUNT];
} fault_config_t;

typedef struct {
    uint16_t pack_mv;
    uint16_t cell_mv[2];
    bool voltage_valid;
    int16_t current_ma;
    bool current_valid;
    thermal_status_t thermal;
    uint16_t balance_delta_mv;
    bool balance_valid;
    bool pd_backend_enabled;
    bool pd_fresh;
    bool soc_valid;
    uint8_t soc_pct;
} fault_input_t;

typedef struct {
    uint32_t active;
    uint32_t latched;
    bool inhibit;
} fault_snapshot_t;

typedef struct {
    fault_config_t cfg;
    uint32_t active;
    uint32_t latched;
    uint32_t since_ms[FAULT_BIT_COUNT];
    bool timer_running[FAULT_BIT_COUNT];
    bool thermal_inhibit;
    bool inhibit;
} fault_manager_t;

void fault_config_default(fault_config_t *cfg);
void fault_init(fault_manager_t *mgr, const fault_config_t *cfg);
fault_snapshot_t fault_update(fault_manager_t *mgr,
                              const fault_input_t *in,
                              uint32_t now_ms);

/**
 * @brief 清除已锁存故障。
 * @return false: 仍有锁存位处于 active 状态, 必须先排除故障。
 */
bool fault_reset(fault_manager_t *mgr);

fault_snapshot_t fault_get_snapshot(const fault_manager_t *mgr);
const char *fault_bit_name(uint8_t bit);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_DIAG_H */
