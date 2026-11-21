/**
 * @file    thermal.h
 * @brief   电池充放电温度窗口 + PD 模块 NTC 过温保护 (带回差)
 */
#ifndef THERMAL_H
#define THERMAL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THERMAL_FLAG_CHARGE_BLOCK       (1U << 0)
#define THERMAL_FLAG_DISCHARGE_BLOCK    (1U << 1)
#define THERMAL_FLAG_CHARGE_WARN        (1U << 2)
#define THERMAL_FLAG_DISCHARGE_WARN     (1U << 3)
#define THERMAL_FLAG_PD_WARN            (1U << 4)
#define THERMAL_FLAG_PD_TRIP            (1U << 5)
#define THERMAL_FLAG_SENSOR_FAULT       (1U << 6)

typedef struct {
    int16_t chg_min_c_x10;
    int16_t chg_min_recover_c_x10;
    int16_t chg_max_c_x10;
    int16_t chg_max_recover_c_x10;
    int16_t chg_warn_low_c_x10;
    int16_t chg_warn_high_c_x10;

    int16_t dis_min_c_x10;
    int16_t dis_min_recover_c_x10;
    int16_t dis_max_c_x10;
    int16_t dis_max_recover_c_x10;
    int16_t dis_warn_low_c_x10;
    int16_t dis_warn_high_c_x10;

    int16_t pd_warn_c_x10;
    int16_t pd_trip_c_x10;
    int16_t pd_recover_c_x10;
} thermal_limits_t;

typedef struct {
    thermal_limits_t limits;
    bool charge_blocked;
    bool discharge_blocked;
    bool pd_tripped;
} thermal_guard_t;

typedef struct {
    uint16_t flags;
    bool charge_allowed;
    bool discharge_allowed;
} thermal_status_t;

void thermal_defaults(thermal_limits_t *limits);
void thermal_init(thermal_guard_t *guard, const thermal_limits_t *limits);

/**
 * @brief 评估电池 NTC 与 PD/MOS NTC。
 *
 * 温度窗口按“当前是充电还是放电”给出告警; 但充/放电闭锁始终同时计算,
 * 这样静置时插入充电器也能立即拒绝超温充电。
 */
thermal_status_t thermal_eval(thermal_guard_t *guard,
                              int16_t battery_c_x10,
                              bool battery_valid,
                              int16_t pd_c_x10,
                              bool pd_valid,
                              int16_t current_ma);

#ifdef __cplusplus
}
#endif

#endif /* THERMAL_H */
