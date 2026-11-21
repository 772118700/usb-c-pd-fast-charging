/**
 * @file    balance.h
 * @brief   两节电池压差监测 (只监测, 不控制均衡)
 *
 * 2S BMS 的被动均衡电流通常只有 30~50 mA。本模块仅判断压差等级并
 * 在 OLED/故障位中提示, 不在 MCU 侧主动开启均衡, 避免与 BMS 冲突。
 */
#ifndef BALANCE_H
#define BALANCE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BALANCE_OK = 0,
    BALANCE_WARN,
    BALANCE_ALERT,
    BALANCE_INVALID
} balance_state_t;

typedef struct {
    uint16_t warn_mv;
    uint16_t alert_mv;
    uint32_t debounce_ms;
    uint16_t delta_mv;
    balance_state_t state;
    balance_state_t candidate_state;
    bool candidate;
    uint32_t candidate_since_ms;
} balance_monitor_t;

void balance_init(balance_monitor_t *mon,
                  uint16_t warn_mv,
                  uint16_t alert_mv,
                  uint32_t debounce_ms);

/**
 * @brief 更新压差状态。
 * @return 去抖后的状态。
 */
balance_state_t balance_update(balance_monitor_t *mon,
                               uint16_t cell1_mv,
                               uint16_t cell2_mv,
                               bool valid,
                               uint32_t now_ms);

const char *balance_state_name(balance_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* BALANCE_H */
