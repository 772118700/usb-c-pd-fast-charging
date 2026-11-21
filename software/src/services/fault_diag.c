#include "fault_diag.h"

#include <stddef.h>

#include "app_config.h"

void fault_config_default(fault_config_t *cfg)
{
    if (cfg == 0) {
        return;
    }
    cfg->cell_ov_trip_mv = APP_CELL_OV_TRIP_MV;
    cfg->cell_ov_clear_mv = APP_CELL_OV_CLEAR_MV;
    cfg->pack_ov_trip_mv = APP_PACK_OV_TRIP_MV;
    cfg->pack_ov_clear_mv = APP_PACK_OV_CLEAR_MV;
    cfg->cell_uv_trip_mv = APP_CELL_UV_TRIP_MV;
    cfg->cell_uv_clear_mv = APP_CELL_UV_CLEAR_MV;
    cfg->pack_uv_trip_mv = APP_PACK_UV_TRIP_MV;
    cfg->pack_uv_clear_mv = APP_PACK_UV_CLEAR_MV;
    cfg->oc_chg_trip_ma = APP_OC_CHG_TRIP_MA;
    cfg->oc_chg_clear_ma = APP_OC_CHG_CLEAR_MA;
    cfg->oc_dis_trip_ma = APP_OC_DIS_TRIP_MA;
    cfg->oc_dis_clear_ma = APP_OC_DIS_CLEAR_MA;
    cfg->imbalance_trip_mv = APP_IMBALANCE_TRIP_MV;
    cfg->imbalance_clear_mv = APP_IMBALANCE_CLEAR_MV;
    cfg->soc_low_pct = APP_SOC_LOW_PCT;

    cfg->debounce_ms[FAULT_BIT_OV] = APP_DEBOUNCE_OV_MS;
    cfg->debounce_ms[FAULT_BIT_UV] = APP_DEBOUNCE_UV_MS;
    cfg->debounce_ms[FAULT_BIT_OC_CHG] = APP_DEBOUNCE_OC_MS;
    cfg->debounce_ms[FAULT_BIT_OC_DIS] = APP_DEBOUNCE_OC_MS;
    cfg->debounce_ms[FAULT_BIT_TEMP] = APP_DEBOUNCE_TEMP_MS;
    cfg->debounce_ms[FAULT_BIT_TEMP_PD] = APP_DEBOUNCE_TEMP_MS;
    cfg->debounce_ms[FAULT_BIT_IMBALANCE] = APP_DEBOUNCE_IMBALANCE_MS;
    cfg->debounce_ms[FAULT_BIT_SENSOR_ADC] = APP_DEBOUNCE_SENSOR_MS;
    cfg->debounce_ms[FAULT_BIT_SENSOR_CURRENT] = APP_DEBOUNCE_SENSOR_MS;
    cfg->debounce_ms[FAULT_BIT_SENSOR_TEMP] = APP_DEBOUNCE_SENSOR_MS;
    cfg->debounce_ms[FAULT_BIT_PD_COMM] = APP_DEBOUNCE_PD_MS;
    cfg->debounce_ms[FAULT_BIT_SOC_LOW] = APP_DEBOUNCE_SOC_LOW_MS;
}

void fault_init(fault_manager_t *mgr, const fault_config_t *cfg)
{
    if (mgr == 0) {
        return;
    }
    if (cfg != 0) {
        mgr->cfg = *cfg;
    } else {
        fault_config_default(&mgr->cfg);
    }
    mgr->active = 0U;
    mgr->latched = 0U;
    mgr->thermal_inhibit = false;
    mgr->inhibit = false;
    for (uint8_t i = 0U; i < FAULT_BIT_COUNT; ++i) {
        mgr->since_ms[i] = 0U;
        mgr->timer_running[i] = false;
    }
}

static uint16_t max_cell_mv(const fault_input_t *in)
{
    return (in->cell_mv[0] > in->cell_mv[1]) ? in->cell_mv[0] : in->cell_mv[1];
}

static uint16_t min_cell_mv(const fault_input_t *in)
{
    return (in->cell_mv[0] < in->cell_mv[1]) ? in->cell_mv[0] : in->cell_mv[1];
}

/**
 * @brief 温度闭锁是否应触发全局 inhibit。
 *
 * 46 degC 只禁充时, 放电不应被全局 inhibit 切断; 只有正在充电才 inhibit。
 * 61 degC 充放电都禁时, 无论电流方向都 inhibit。
 * 注意: FAULT_BIT_TEMP 本身仍会置位用于告警, 与这里的 inhibit 判定解耦。
 */
static bool battery_temp_inhibit(const fault_input_t *in)
{
    bool chg_block = (in->thermal.flags & THERMAL_FLAG_CHARGE_BLOCK) != 0U;
    bool dis_block = (in->thermal.flags & THERMAL_FLAG_DISCHARGE_BLOCK) != 0U;

    if (chg_block && dis_block) {
        return true;
    }
    if (chg_block && in->current_ma > 50) {
        return true;
    }
    if (dis_block && in->current_ma < -50) {
        return true;
    }
    return false;
}

/**
 * @brief 返回当前是否应“置位/保持”该故障位。
 *
 * 已 active 的位使用清除门限, 形成回差, 避免在门限附近抖动。
 */
static bool fault_condition(uint8_t bit, const fault_config_t *cfg,
                            const fault_input_t *in, bool is_active)
{
    switch (bit) {
    case FAULT_BIT_OV: {
        if (!in->voltage_valid) {
            return false;
        }
        uint16_t cell = max_cell_mv(in);
        if (is_active) {
            return !(cell < cfg->cell_ov_clear_mv &&
                     in->pack_mv < cfg->pack_ov_clear_mv);
        }
        return (cell > cfg->cell_ov_trip_mv ||
                in->pack_mv > cfg->pack_ov_trip_mv);
    }
    case FAULT_BIT_UV: {
        if (!in->voltage_valid) {
            return false;
        }
        uint16_t cell = min_cell_mv(in);
        if (is_active) {
            return !(cell > cfg->cell_uv_clear_mv &&
                     in->pack_mv > cfg->pack_uv_clear_mv);
        }
        return (cell < cfg->cell_uv_trip_mv ||
                in->pack_mv < cfg->pack_uv_trip_mv);
    }
    case FAULT_BIT_OC_CHG:
        if (!in->current_valid) {
            return false;
        }
        if (is_active) {
            return in->current_ma > cfg->oc_chg_clear_ma;
        }
        return in->current_ma > cfg->oc_chg_trip_ma;
    case FAULT_BIT_OC_DIS:
        if (!in->current_valid) {
            return false;
        }
        if (is_active) {
            return in->current_ma < cfg->oc_dis_clear_ma;
        }
        return in->current_ma < cfg->oc_dis_trip_ma;
    case FAULT_BIT_TEMP:
        return (in->thermal.flags &
                (THERMAL_FLAG_CHARGE_BLOCK |
                 THERMAL_FLAG_DISCHARGE_BLOCK)) != 0U;
    case FAULT_BIT_TEMP_PD:
        return (in->thermal.flags & THERMAL_FLAG_PD_TRIP) != 0U;
    case FAULT_BIT_IMBALANCE:
        if (!in->balance_valid) {
            return false;
        }
        if (is_active) {
            return in->balance_delta_mv > cfg->imbalance_clear_mv;
        }
        return in->balance_delta_mv > cfg->imbalance_trip_mv;
    case FAULT_BIT_SENSOR_ADC:
        return !in->voltage_valid;
    case FAULT_BIT_SENSOR_CURRENT:
        return !in->current_valid;
    case FAULT_BIT_SENSOR_TEMP:
        return (in->thermal.flags & THERMAL_FLAG_SENSOR_FAULT) != 0U;
    case FAULT_BIT_PD_COMM:
        return in->pd_backend_enabled && !in->pd_fresh;
    case FAULT_BIT_SOC_LOW:
        return in->soc_valid && (in->soc_pct < cfg->soc_low_pct);
    default:
        return false;
    }
}

fault_snapshot_t fault_update(fault_manager_t *mgr,
                              const fault_input_t *in,
                              uint32_t now_ms)
{
    fault_snapshot_t snap = {0};
    if (mgr == 0 || in == 0) {
        return snap;
    }

    for (uint8_t bit = 0U; bit < FAULT_BIT_COUNT; ++bit) {
        const uint32_t mask = FAULT_MASK(bit);
        bool active = (mgr->active & mask) != 0U;
        bool cond = fault_condition(bit, &mgr->cfg, in, active);
        uint32_t debounce = mgr->cfg.debounce_ms[bit];

        if (cond) {
            if (!active) {
                if (!mgr->timer_running[bit]) {
                    mgr->timer_running[bit] = true;
                    mgr->since_ms[bit] = now_ms;
                } else if ((uint32_t)(now_ms - mgr->since_ms[bit]) >= debounce) {
                    mgr->active |= mask;
                    mgr->timer_running[bit] = false;
                    if ((FAULT_LATCH_MASK & mask) != 0U) {
                        mgr->latched |= mask;
                    }
                }
            } else {
                mgr->timer_running[bit] = false;
            }
        } else {
            if (active) {
                if (!mgr->timer_running[bit]) {
                    mgr->timer_running[bit] = true;
                    mgr->since_ms[bit] = now_ms;
                } else if ((uint32_t)(now_ms - mgr->since_ms[bit]) >= debounce) {
                    mgr->active &= ~mask;
                    mgr->timer_running[bit] = false;
                }
            } else {
                mgr->timer_running[bit] = false;
            }
        }
    }

    mgr->thermal_inhibit = battery_temp_inhibit(in) ||
                           ((mgr->active & FAULT_MASK(FAULT_BIT_TEMP_PD)) != 0U);
    mgr->inhibit = ((mgr->latched & FAULT_INHIBIT_MASK) != 0U) ||
                   mgr->thermal_inhibit;

    snap.active = mgr->active;
    snap.latched = mgr->latched;
    snap.inhibit = mgr->inhibit;
    return snap;
}

bool fault_reset(fault_manager_t *mgr)
{
    if (mgr == 0) {
        return false;
    }
    /* 故障仍处于 active 时不允许清锁存, 防止带故障强行恢复输出。 */
    if ((mgr->active & mgr->latched) != 0U) {
        return false;
    }
    mgr->latched = 0U;
    mgr->inhibit = mgr->thermal_inhibit;
    return true;
}

fault_snapshot_t fault_get_snapshot(const fault_manager_t *mgr)
{
    fault_snapshot_t snap = {0};
    if (mgr != 0) {
        snap.active = mgr->active;
        snap.latched = mgr->latched;
        snap.inhibit = mgr->inhibit;
    }
    return snap;
}

const char *fault_bit_name(uint8_t bit)
{
    switch (bit) {
    case FAULT_BIT_OV:
        return "OV";
    case FAULT_BIT_UV:
        return "UV";
    case FAULT_BIT_OC_CHG:
        return "OC_CHG";
    case FAULT_BIT_OC_DIS:
        return "OC_DIS";
    case FAULT_BIT_TEMP:
        return "TEMP";
    case FAULT_BIT_TEMP_PD:
        return "TEMP_PD";
    case FAULT_BIT_IMBALANCE:
        return "IMBAL";
    case FAULT_BIT_SENSOR_ADC:
        return "ADC";
    case FAULT_BIT_SENSOR_CURRENT:
        return "ISENS";
    case FAULT_BIT_SENSOR_TEMP:
        return "TSENS";
    case FAULT_BIT_PD_COMM:
        return "PD_COMM";
    case FAULT_BIT_SOC_LOW:
        return "SOC_LOW";
    default:
        return "?";
    }
}
