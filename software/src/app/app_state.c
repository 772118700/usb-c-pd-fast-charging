#include "app_state.h"

#include <string.h>

#include "board_port.h"

void app_state_init(app_state_t *st)
{
    if (st == 0) {
        return;
    }
    board_critical_enter();
    memset(&st->state, 0, sizeof(st->state));
    board_critical_exit();
}

void app_state_publish(app_state_t *st, const system_state_t *src)
{
    if (st == 0 || src == 0) {
        return;
    }
    board_critical_enter();
    st->state = *src;
    board_critical_exit();
}

void app_state_read(app_state_t *st, system_state_t *dst)
{
    if (st == 0 || dst == 0) {
        return;
    }
    board_critical_enter();
    *dst = st->state;
    board_critical_exit();
}
