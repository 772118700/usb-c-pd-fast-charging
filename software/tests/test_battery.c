#include "battery_math.h"

#include "test_util.h"

static voltage_divider_t pack_div(void)
{
    voltage_divider_t d = {3300U, 4095U, 100000U, 47000U};
    return d;
}

void test_battery_run(void)
{
    voltage_divider_t d = pack_div();
    battery_voltage_t v;

    /* 8.0 V 包电压, 4.0 V 中点 */
    CHECK(battery_calc_from_raw(3174U, 1588U, &d, &d, &v));
    CHECK_NEAR(v.pack_mv, 8000, 15);
    CHECK_NEAR(v.cell_mv[0], 4000, 15);
    CHECK_NEAR(v.cell_mv[1], 4000, 15);
    CHECK_NEAR(v.cell_delta_mv, 0, 10);
    CHECK(v.pack_valid && v.cells_valid);

    /* 中点偏低 -> 第二节偏高 */
    CHECK(battery_calc_from_raw(3174U, 1547U, &d, &d, &v));
    CHECK_NEAR(v.cell_mv[0], 3900, 20);
    CHECK_NEAR(v.cell_mv[1], 4100, 20);
    CHECK_NEAR(v.cell_delta_mv, 200, 20);

    /* 原始码贴边 -> 无效 */
    CHECK(!divider_raw_to_mv(0U, &d, &v.pack_mv));
    CHECK(!divider_raw_to_mv(4095U, &d, &v.pack_mv));
    CHECK(!battery_calc_from_raw(0U, 1588U, &d, &d, &v));
    CHECK(!v.pack_valid);

    /* 中点高于总压 -> 拓扑/采样错误 */
    CHECK(!battery_calc_from_raw(2000U, 3000U, &d, &d, &v));

    /* 分压参数非法 */
    voltage_divider_t bad = {0U, 4095U, 100000U, 47000U};
    uint16_t mv = 0U;
    CHECK(!divider_raw_to_mv(1000U, &bad, &mv));

    /* 8.4 V 包电压对应分压节点约 2686 mV, 换算后应还原为 8400 mV */
    voltage_divider_t div42 = {3300U, 4095U, 100000U, 47000U};
    uint16_t node_mv = 0U;
    CHECK(divider_raw_to_mv(3333U, &div42, &node_mv));
    CHECK_NEAR(node_mv, 8400, 15);
}
