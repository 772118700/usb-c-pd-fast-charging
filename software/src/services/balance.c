#include "balance.h"

#include <stddef.h>

void balance_init(balance_monitor_t *mon,
                  uint16_t warn_mv,
                  uint16_t alert_mv,
                  uint32_t debounce_ms)
{
    if (mon == NULL) {
        return;
    }
    mon->warn_mv = warn_mv;
    mon->alert_mv = alert_mv;
    mon->debounce_ms = debounce_ms;
    mon->delta_mv = 0U;
    mon->state = BALANCE_OK;
    mon->candidate_state = BALANCE_OK;
    mon->candidate = false;
    mon->candidate_since_ms = 0U;
}

static balance_state_t classify(const balance_monitor_t *mon, uint16_t delta_mv)
{
    if (delta_mv > mon->alert_mv) {
        return BALANCE_ALERT;
    }
    if (delta_mv > mon->warn_mv) {
        return BALANCE_WARN;
    }
    return BALANCE_OK;
}

balance_state_t balance_update(balance_monitor_t *mon,
                               uint16_t cell1_mv,
                               uint16_t cell2_mv,
                               bool valid,
                               uint32_t now_ms)
{
    if (mon == NULL) {
        return BALANCE_INVALID;
    }
    if (!valid) {
        mon->delta_mv = 0U;
        mon->state = BALANCE_INVALID;
        mon->candidate = false;
        return mon->state;
    }

    uint16_t delta = (cell1_mv > cell2_mv)
                         ? (uint16_t)(cell1_mv - cell2_mv)
                         : (uint16_t)(cell2_mv - cell1_mv);
    mon->delta_mv = delta;

    balance_state_t desired = classify(mon, delta);
    if (desired == mon->state) {
        mon->candidate = false;
        return mon->state;
    }

    if (!mon->candidate) {
        mon->candidate = true;
        mon->candidate_state = desired;
        mon->candidate_since_ms = now_ms;
    } else if (mon->candidate_state != desired) {
        mon->candidate_state = desired;
        mon->candidate_since_ms = now_ms;
    } else if ((uint32_t)(now_ms - mon->candidate_since_ms) >= mon->debounce_ms) {
        mon->state = desired;
        mon->candidate = false;
        mon->candidate_since_ms = now_ms;
    }
    return mon->state;
}

const char *balance_state_name(balance_state_t state)
{
    switch (state) {
    case BALANCE_OK:
        return "OK";
    case BALANCE_WARN:
        return "WARN";
    case BALANCE_ALERT:
        return "ALERT";
    case BALANCE_INVALID:
    default:
        return "N/A";
    }
}
