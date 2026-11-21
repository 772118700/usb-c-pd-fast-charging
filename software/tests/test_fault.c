#include "fault_diag.h"

#include <string.h>

#include "test_util.h"

static fault_input_t base_input(void)
{
    fault_input_t in;
    memset(&in, 0, sizeof(in));
    in.pack_mv = 7400U;
    in.cell_mv[0] = 3700U;
    in.cell_mv[1] = 3700U;
    in.voltage_valid = true;
    in.current_valid = true;
    in.thermal.flags = 0U;
    in.thermal.charge_allowed = true;
    in.thermal.discharge_allowed = true;
    in.balance_valid = true;
    in.soc_valid = true;
    in.soc_pct = 50U;
    return in;
}

void test_fault_run(void)
{
    fault_manager_t mgr;
    fault_init(&mgr, 0);

    /* OV: 4300 mV 持续 500 ms -> active + latched + inhibit */
    fault_input_t in = base_input();
    in.cell_mv[0] = 4300U;
    in.pack_mv = 8000U;

    fault_snapshot_t s = fault_update(&mgr, &in, 0U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_OV)) == 0U);
    (void)fault_update(&mgr, &in, 499U);
    s = fault_update(&mgr, &in, 500U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_OV)) != 0U);
    CHECK((s.latched & FAULT_MASK(FAULT_BIT_OV)) != 0U);
    CHECK(s.inhibit);
    CHECK(!fault_reset(&mgr));   /* 故障仍 active, 不允许清 */

    /* 回到 4200 mV: 高于清除门限 4150, 保持 */
    in.cell_mv[0] = 4200U;
    in.pack_mv = 7900U;
    s = fault_update(&mgr, &in, 600U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_OV)) != 0U);
    s = fault_update(&mgr, &in, 1100U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_OV)) != 0U);

    /* 降到 4100 mV 以下并持续 500 ms -> active 清除, latched 保留 */
    in.cell_mv[0] = 4100U;
    in.pack_mv = 7800U;
    (void)fault_update(&mgr, &in, 1200U);
    s = fault_update(&mgr, &in, 1700U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_OV)) == 0U);
    CHECK((s.latched & FAULT_MASK(FAULT_BIT_OV)) != 0U); /* 仍锁存 */
    CHECK(s.inhibit);

    CHECK(fault_reset(&mgr));
    s = fault_get_snapshot(&mgr);
    CHECK(s.latched == 0U);
    CHECK(!s.inhibit);

    /* 过流充电 */
    fault_init(&mgr, 0);
    in = base_input();
    in.current_ma = 6000;
    (void)fault_update(&mgr, &in, 0U);
    s = fault_update(&mgr, &in, 200U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_OC_CHG)) != 0U);
    CHECK((s.latched & FAULT_MASK(FAULT_BIT_OC_CHG)) != 0U);

    /* 过流放电 */
    fault_init(&mgr, 0);
    in = base_input();
    in.current_ma = -7500;
    (void)fault_update(&mgr, &in, 0U);
    s = fault_update(&mgr, &in, 200U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_OC_DIS)) != 0U);

    /* 温度故障: 不锁存, 温度恢复后自动解除 */
    fault_init(&mgr, 0);
    in = base_input();
    in.current_ma = 2000;   /* 正在充电, 充电闭锁才构成 TEMP */
    in.thermal.flags = THERMAL_FLAG_CHARGE_BLOCK;
    in.thermal.charge_allowed = false;
    (void)fault_update(&mgr, &in, 0U);
    s = fault_update(&mgr, &in, 1000U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_TEMP)) != 0U);
    CHECK((s.latched & FAULT_MASK(FAULT_BIT_TEMP)) == 0U);
    CHECK(s.inhibit);

    in.thermal.flags = 0U;
    in.thermal.charge_allowed = true;
    (void)fault_update(&mgr, &in, 1100U);
    s = fault_update(&mgr, &in, 2100U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_TEMP)) == 0U);
    CHECK(!s.inhibit);

    /* 只禁充且放电: TEMP 告警置位, 但不因单侧闭锁切断放电 */
    fault_init(&mgr, 0);
    in = base_input();
    in.current_ma = -2000;  /* 放电 */
    in.thermal.flags = THERMAL_FLAG_CHARGE_BLOCK;
    in.thermal.charge_allowed = false;
    (void)fault_update(&mgr, &in, 0U);
    s = fault_update(&mgr, &in, 2000U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_TEMP)) != 0U);
    CHECK(!s.inhibit);

    /* 传感器故障: 告警但不直接闭锁 */
    fault_init(&mgr, 0);
    in = base_input();
    in.voltage_valid = false;
    (void)fault_update(&mgr, &in, 0U);
    s = fault_update(&mgr, &in, 1000U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_SENSOR_ADC)) != 0U);
    CHECK(!s.inhibit);

    /* 压差告警 */
    fault_init(&mgr, 0);
    in = base_input();
    in.balance_delta_mv = 110U;
    (void)fault_update(&mgr, &in, 0U);
    (void)fault_update(&mgr, &in, 2999U);
    s = fault_update(&mgr, &in, 3000U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_IMBALANCE)) != 0U);
    CHECK((s.latched & FAULT_MASK(FAULT_BIT_IMBALANCE)) == 0U);

    /* SOC 低 */
    fault_init(&mgr, 0);
    in = base_input();
    in.soc_pct = 4U;
    (void)fault_update(&mgr, &in, 0U);
    s = fault_update(&mgr, &in, 5000U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_SOC_LOW)) != 0U);

    /* PD 通讯超时 (仅后端启用时) */
    fault_init(&mgr, 0);
    in = base_input();
    in.pd_backend_enabled = true;
    in.pd_fresh = false;
    (void)fault_update(&mgr, &in, 0U);
    s = fault_update(&mgr, &in, 1000U);
    CHECK((s.active & FAULT_MASK(FAULT_BIT_PD_COMM)) != 0U);

    /* 名称 */
    CHECK(fault_bit_name(FAULT_BIT_OV)[0] == 'O');
}
