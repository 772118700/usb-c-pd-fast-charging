/**
 * @file    app_state.h
 * @brief   采集任务与显示/故障任务之间的系统快照
 *
 * 使用临界区保护的整结构双拷贝, 避免 32 位 MCU 上逐字段读取撕裂。
 */
#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "fault_diag.h"
#include "pd_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t timestamp_ms;
    uint16_t pack_mv;
    uint16_t cell_mv[2];
    uint16_t cell_delta_mv;
    int16_t current_ma;
    int32_t power_mw;
    int16_t temp_c_x10[2];
    uint8_t soc_pct;
    bool pack_valid;
    bool cells_valid;
    bool current_valid;
    bool temp_valid[2];
    uint8_t balance_state;
} battery_sample_t;

typedef struct {
    battery_sample_t battery;
    fault_snapshot_t fault;
    pd_status_t pd;
    uint32_t uptime_ms;
    uint32_t sample_count;
    bool ina_ready;
    bool oled_ready;
    bool pd_backend_enabled;
} system_state_t;

typedef struct {
    system_state_t state;
} app_state_t;

void app_state_init(app_state_t *st);
void app_state_publish(app_state_t *st, const system_state_t *src);
void app_state_read(app_state_t *st, system_state_t *dst);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
