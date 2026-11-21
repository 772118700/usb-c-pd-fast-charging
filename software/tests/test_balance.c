#include "balance.h"

#include "test_util.h"

void test_balance_run(void)
{
    balance_monitor_t mon;
    balance_init(&mon, 50U, 100U, 3000U);

    CHECK(balance_update(&mon, 3700U, 3700U, true, 0U) == BALANCE_OK);

    /* 51 mV 超过告警门限, 但必须持续 3 s */
    CHECK(balance_update(&mon, 3751U, 3700U, true, 100U) == BALANCE_OK);
    CHECK(balance_update(&mon, 3751U, 3700U, true, 3099U) == BALANCE_OK);
    CHECK(balance_update(&mon, 3751U, 3700U, true, 3100U) == BALANCE_WARN);
    CHECK_NEAR(mon.delta_mv, 51, 0);

    /* 压差继续拉大 -> ALERT, 重新去抖 */
    CHECK(balance_update(&mon, 3801U, 3700U, true, 4000U) == BALANCE_WARN);
    CHECK(balance_update(&mon, 3801U, 3700U, true, 6999U) == BALANCE_WARN);
    CHECK(balance_update(&mon, 3801U, 3700U, true, 7000U) == BALANCE_ALERT);

    /* 压差恢复 -> 3 s 后回到 OK */
    CHECK(balance_update(&mon, 3740U, 3700U, true, 7100U) == BALANCE_ALERT);
    CHECK(balance_update(&mon, 3740U, 3700U, true, 10100U) == BALANCE_OK);

    /* 采样无效 */
    CHECK(balance_update(&mon, 0U, 0U, false, 10200U) == BALANCE_INVALID);
    CHECK_NEAR(mon.delta_mv, 0, 0);

    CHECK(balance_state_name(BALANCE_ALERT)[0] == 'A');
}
