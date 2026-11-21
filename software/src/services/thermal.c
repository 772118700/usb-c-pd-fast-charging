#include "thermal.h"

#include "app_config.h"

void thermal_defaults(thermal_limits_t *limits)
{
    if (limits == 0) {
        return;
    }
    limits->chg_min_c_x10 = APP_CHG_TEMP_MIN_CX10;
    limits->chg_min_recover_c_x10 = APP_CHG_TEMP_MIN_RECOVER_CX10;
    limits->chg_max_c_x10 = APP_CHG_TEMP_MAX_CX10;
    limits->chg_max_recover_c_x10 = APP_CHG_TEMP_MAX_RECOVER_CX10;
    limits->chg_warn_low_c_x10 = APP_CHG_TEMP_WARN_LOW_CX10;
    limits->chg_warn_high_c_x10 = APP_CHG_TEMP_WARN_HIGH_CX10;

    limits->dis_min_c_x10 = APP_DIS_TEMP_MIN_CX10;
    limits->dis_min_recover_c_x10 = APP_DIS_TEMP_MIN_RECOVER_CX10;
    limits->dis_max_c_x10 = APP_DIS_TEMP_MAX_CX10;
    limits->dis_max_recover_c_x10 = APP_DIS_TEMP_MAX_RECOVER_CX10;
    limits->dis_warn_low_c_x10 = APP_DIS_TEMP_WARN_LOW_CX10;
    limits->dis_warn_high_c_x10 = APP_DIS_TEMP_WARN_HIGH_CX10;

    limits->pd_warn_c_x10 = APP_PD_TEMP_WARN_CX10;
    limits->pd_trip_c_x10 = APP_PD_TEMP_TRIP_CX10;
    limits->pd_recover_c_x10 = APP_PD_TEMP_RECOVER_CX10;
}

void thermal_init(thermal_guard_t *guard, const thermal_limits_t *limits)
{
    if (guard == 0) {
        return;
    }
    if (limits != 0) {
        guard->limits = *limits;
    } else {
        thermal_defaults(&guard->limits);
    }
    guard->charge_blocked = false;
    guard->discharge_blocked = false;
    guard->pd_tripped = false;
}

static void update_battery_window(thermal_guard_t *g, int16_t temp)
{
    const thermal_limits_t *l = &g->limits;

    if (g->charge_blocked) {
        if (temp >= l->chg_min_recover_c_x10 && temp <= l->chg_max_recover_c_x10) {
            g->charge_blocked = false;
        }
    } else if (temp < l->chg_min_c_x10 || temp > l->chg_max_c_x10) {
        g->charge_blocked = true;
    }

    if (g->discharge_blocked) {
        if (temp >= l->dis_min_recover_c_x10 && temp <= l->dis_max_recover_c_x10) {
            g->discharge_blocked = false;
        }
    } else if (temp < l->dis_min_c_x10 || temp > l->dis_max_c_x10) {
        g->discharge_blocked = true;
    }
}

thermal_status_t thermal_eval(thermal_guard_t *guard,
                              int16_t battery_c_x10,
                              bool battery_valid,
                              int16_t pd_c_x10,
                              bool pd_valid,
                              int16_t current_ma)
{
    thermal_status_t st = {0};
    if (guard == 0) {
        st.flags = THERMAL_FLAG_SENSOR_FAULT;
        return st;
    }

    if (!battery_valid) {
        st.flags |= THERMAL_FLAG_SENSOR_FAULT;
        st.charge_allowed = false;
        st.discharge_allowed = false;
    } else {
        update_battery_window(guard, battery_c_x10);

        if (guard->charge_blocked) {
            st.flags |= THERMAL_FLAG_CHARGE_BLOCK;
        } else if (current_ma > 50 &&
                   (battery_c_x10 < guard->limits.chg_warn_low_c_x10 ||
                    battery_c_x10 > guard->limits.chg_warn_high_c_x10)) {
            st.flags |= THERMAL_FLAG_CHARGE_WARN;
        }

        if (guard->discharge_blocked) {
            st.flags |= THERMAL_FLAG_DISCHARGE_BLOCK;
        } else if (current_ma < -50 &&
                   (battery_c_x10 < guard->limits.dis_warn_low_c_x10 ||
                    battery_c_x10 > guard->limits.dis_warn_high_c_x10)) {
            st.flags |= THERMAL_FLAG_DISCHARGE_WARN;
        }

        st.charge_allowed = !guard->charge_blocked;
        st.discharge_allowed = !guard->discharge_blocked;
    }

    if (!pd_valid) {
        st.flags |= THERMAL_FLAG_SENSOR_FAULT;
    } else {
        if (guard->pd_tripped) {
            if (pd_c_x10 <= guard->limits.pd_recover_c_x10) {
                guard->pd_tripped = false;
            }
        } else if (pd_c_x10 >= guard->limits.pd_trip_c_x10) {
            guard->pd_tripped = true;
        }

        if (guard->pd_tripped) {
            st.flags |= THERMAL_FLAG_PD_TRIP;
        } else if (pd_c_x10 >= guard->limits.pd_warn_c_x10) {
            st.flags |= THERMAL_FLAG_PD_WARN;
        }
    }

    return st;
}
