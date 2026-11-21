#include "fault_diag.h"
#include "soc.h"
#include "thermal.h"

#include "test_util.h"

static fault_input_t base_input(void)
{
    fault_input_t in = {0};
    in.pack_mv = 7400U;
    in.cell_mv[0] = 3700U;
    in.cell_mv[1] = 3700U;
    in.voltage_valid = true;
    in.current_valid = true;
    in.balance_valid = true;
    in.soc_valid = true;
    in.soc_pct = 50U;
    return in;
}

void test_scenarios_run(void)
{
    /* 场景 1: 30 W 放电 (2S, 约 4.1 A) 30 min, 从 100% 开始 */
    soc_t soc;
    soc_init(&soc, 2500);
    soc_set_percent(&soc, 100U);
    for (int i = 0; i < 1800; ++i) {
        soc_update(&soc, -4100, 1000U, 3700U, true);
    }
    CHECK(soc.soc_pct >= 17 && soc.soc_pct <= 19);

    /* 场景 2: 30 W 充电 (约 3.6 A) 30 min, 从 20% 开始 */
    soc_init(&soc, 2500);
    soc_set_percent(&soc, 20U);
    for (int i = 0; i < 1800; ++i) {
        soc_update(&soc, 3600, 1000U, 3900U, true);
    }
    CHECK(soc.soc_pct >= 88 && soc.soc_pct <= 92);

    /* 场景 3: 温度从 25 -> 60 degC, 2 A 充电。
       46 degC 起充电闭锁, 50 degC 起 PD 告警, 61 degC 时故障任务应报 TEMP。 */
    thermal_guard_t guard;
    thermal_init(&guard, 0);
    fault_manager_t faults;
    fault_init(&faults, 0);

    fault_snapshot_t snap = {0};
    for (int step = 0; step <= 360; ++step) {
        int16_t temp = (int16_t)(250 + step * 10);   /* 25.0 -> 61.0 */
        thermal_status_t th = thermal_eval(&guard, temp, true, 300, true, 2000);
        fault_input_t in = base_input();
        in.current_ma = 2000;
        in.thermal = th;
        in.soc_pct = 50U;
        snap = fault_update(&faults, &in, (uint32_t)step * 100U);
    }
    CHECK(snap.active != 0U);
    CHECK((snap.active & FAULT_MASK(FAULT_BIT_TEMP)) != 0U);
    CHECK(snap.inhibit);

    /* 场景 4: 过压 -> 锁存, 即使电压恢复也不自动解除 */
    fault_init(&faults, 0);
    fault_input_t in = base_input();
    in.cell_mv[0] = 4300U;
    in.pack_mv = 8000U;
    (void)fault_update(&faults, &in, 0U);
    (void)fault_update(&faults, &in, 600U);
    in.cell_mv[0] = 3700U;
    in.pack_mv = 7400U;
    (void)fault_update(&faults, &in, 2000U);   /* 启动清除去抖 */
    snap = fault_update(&faults, &in, 2500U);   /* 500 ms 到期 */
    CHECK((snap.latched & FAULT_MASK(FAULT_BIT_OV)) != 0U);
    CHECK(snap.inhibit);
    CHECK(fault_reset(&faults));
}
