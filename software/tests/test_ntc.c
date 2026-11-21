#include "ntc.h"

#include "test_util.h"

void test_ntc_run(void)
{
    ntc_config_t cfg = {10000U, 10000U, 3950U, 3300U};
    ntc_reading_t r;

    /* 25 degC: NTC = R25, 分压 = 1650 mV */
    CHECK(ntc_mv_to_temp(1650U, &cfg, &r));
    CHECK(r.valid);
    CHECK_NEAR(r.temp_c_x10, 250, 8);
    CHECK_NEAR(r.resistance_ohm, 10000, 60);

    /* 0 degC 附近: R ~= 33.6 kOhm, V ~= 2544 mV */
    CHECK(ntc_mv_to_temp(2544U, &cfg, &r));
    CHECK_NEAR(r.temp_c_x10, 0, 15);

    /* 45 degC 附近: R ~= 4.35 kOhm, V ~= 1000 mV */
    CHECK(ntc_mv_to_temp(1000U, &cfg, &r));
    CHECK_NEAR(r.temp_c_x10, 450, 20);

    /* 开路/短路 */
    CHECK(!ntc_mv_to_temp(3299U, &cfg, &r));
    CHECK(r.open_circuit);
    CHECK(!ntc_mv_to_temp(2U, &cfg, &r));
    CHECK(r.short_circuit);

    /* 原始码路径 */
    uint16_t raw_25c = (uint16_t)((1650UL * 4095UL + 1650UL) / 3300UL);
    CHECK(ntc_raw_to_temp(raw_25c, &cfg, &r));
    CHECK_NEAR(r.temp_c_x10, 250, 12);

    ntc_config_t bad = {0U, 10000U, 3950U, 3300U};
    CHECK(!ntc_raw_to_temp(2000U, &bad, &r));
}
