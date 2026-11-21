#include "thermal.h"

#include "test_util.h"

void test_thermal_run(void)
{
    thermal_guard_t g;
    thermal_init(&g, 0);

    thermal_status_t s = thermal_eval(&g, 250, true, 300, true, 1000);
    CHECK(s.flags == 0U);
    CHECK(s.charge_allowed && s.discharge_allowed);

    /* 46 degC: 充电闭锁, 放电仍允许 */
    s = thermal_eval(&g, 460, true, 300, true, 1000);
    CHECK((s.flags & THERMAL_FLAG_CHARGE_BLOCK) != 0U);
    CHECK((s.flags & THERMAL_FLAG_DISCHARGE_BLOCK) == 0U);
    CHECK(!s.charge_allowed && s.discharge_allowed);

    /* 61 degC: 充放电都闭锁 */
    s = thermal_eval(&g, 610, true, 300, true, -1000);
    CHECK((s.flags & THERMAL_FLAG_CHARGE_BLOCK) != 0U);
    CHECK((s.flags & THERMAL_FLAG_DISCHARGE_BLOCK) != 0U);

    /* 回差: 41 degC 时充电恢复, 57 degC 以下放电恢复 */
    s = thermal_eval(&g, 410, true, 300, true, 1000);
    CHECK((s.flags & THERMAL_FLAG_CHARGE_BLOCK) == 0U);
    CHECK((s.flags & THERMAL_FLAG_DISCHARGE_BLOCK) == 0U);

    /* 低温: -1 degC 禁充, 放电允许 (放电下限 -20) */
    s = thermal_eval(&g, -10, true, 300, true, 1000);
    CHECK((s.flags & THERMAL_FLAG_CHARGE_BLOCK) != 0U);
    CHECK((s.flags & THERMAL_FLAG_DISCHARGE_BLOCK) == 0U);

    /* 重新初始化, 充电 43 degC: 告警但未闭锁 */
    thermal_init(&g, 0);
    s = thermal_eval(&g, 430, true, 300, true, 1000);
    CHECK((s.flags & THERMAL_FLAG_CHARGE_WARN) != 0U);
    CHECK((s.flags & THERMAL_FLAG_CHARGE_BLOCK) == 0U);

    /* 放电 56 degC: 告警 */
    s = thermal_eval(&g, 560, true, 300, true, -1000);
    CHECK((s.flags & THERMAL_FLAG_DISCHARGE_WARN) != 0U);
    CHECK((s.flags & THERMAL_FLAG_DISCHARGE_BLOCK) == 0U);

    /* PD NTC: 71 告警, 86 跳闸, 81 保持, 80 恢复 */
    s = thermal_eval(&g, 250, true, 710, true, 0);
    CHECK((s.flags & THERMAL_FLAG_PD_WARN) != 0U);

    s = thermal_eval(&g, 250, true, 860, true, 0);
    CHECK((s.flags & THERMAL_FLAG_PD_TRIP) != 0U);

    s = thermal_eval(&g, 250, true, 810, true, 0);
    CHECK((s.flags & THERMAL_FLAG_PD_TRIP) != 0U);

    s = thermal_eval(&g, 250, true, 800, true, 0);
    CHECK((s.flags & THERMAL_FLAG_PD_TRIP) == 0U);

    /* NTC 故障: 闭锁充放电, 但标记为传感器故障 */
    s = thermal_eval(&g, 250, false, 300, true, 1000);
    CHECK((s.flags & THERMAL_FLAG_SENSOR_FAULT) != 0U);
    CHECK(!s.charge_allowed && !s.discharge_allowed);

    s = thermal_eval(&g, 250, true, 300, false, 1000);
    CHECK((s.flags & THERMAL_FLAG_SENSOR_FAULT) != 0U);
    CHECK(s.charge_allowed && s.discharge_allowed);
}
