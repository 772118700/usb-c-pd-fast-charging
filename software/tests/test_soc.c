#include "soc.h"

#include "test_util.h"

void test_soc_run(void)
{
    CHECK_NEAR(soc_from_ocv_mv(3000U), 0, 0);
    CHECK_NEAR(soc_from_ocv_mv(4200U), 100, 0);
    CHECK_NEAR(soc_from_ocv_mv(3700U), 52, 1);
    CHECK_NEAR(soc_from_ocv_mv(3750U), 59, 1);
    CHECK_NEAR(soc_from_ocv_mv(2500U), 0, 0);

    soc_t soc;
    soc_init(&soc, 2500);
    CHECK_NEAR(soc.soc_pct, 50, 0);
    soc_seed_from_ocv(&soc, 4200U);
    CHECK_NEAR(soc.soc_pct, 100, 0);

    /* 2 A 放电 1 h: 2500 mAh 的 80% */
    for (int i = 0; i < 3600; ++i) {
        soc_update(&soc, -2000, 1000U, 3700U, true);
    }
    CHECK_NEAR(soc.soc_pct, 20, 1);

    /* 继续放电 -> 钳位 0 */
    for (int i = 0; i < 3600; ++i) {
        soc_update(&soc, -2000, 1000U, 3000U, true);
    }
    CHECK_NEAR(soc.soc_pct, 0, 0);

    /* 1 A 充电 1 h, 从 50% 起: +39.2% */
    soc_init(&soc, 2500);
    for (int i = 0; i < 3600; ++i) {
        soc_update(&soc, 1000, 1000U, 3800U, true);
    }
    CHECK(soc.soc_pct >= 88 && soc.soc_pct <= 90);

    /* 充电到顶 -> 钳位 100 */
    for (int i = 0; i < 7200; ++i) {
        soc_update(&soc, 2000, 1000U, 4200U, true);
    }
    CHECK_NEAR(soc.soc_pct, 100, 0);

    /* 静置修正: 50% 起, 3.80 V 对应 66%, 每 30 min 最多 +5% */
    soc_init(&soc, 2500);
    CHECK_NEAR(soc.soc_pct, 50, 0);
    for (int i = 0; i < 1800; ++i) {   /* 30 min, 1 s 步长 */
        soc_update(&soc, 0, 1000U, 3800U, true);
    }
    CHECK_NEAR(soc.soc_pct, 55, 1);

    /* 再静置 30 min -> 60% */
    for (int i = 0; i < 1800; ++i) {
        soc_update(&soc, 0, 1000U, 3800U, true);
    }
    CHECK_NEAR(soc.soc_pct, 60, 1);

    /* 有电流时静置计时清零: 100 mA 放电 30 min 约 -2% */
    soc_init(&soc, 2500);
    for (int i = 0; i < 1800; ++i) {
        soc_update(&soc, -100, 1000U, 3800U, true);
    }
    CHECK_NEAR(soc.soc_pct, 48, 1);

    /* dt 上限 5 s, 防止断点后跳变 */
    soc_init(&soc, 2500);
    soc_set_percent(&soc, 50);
    soc_update(&soc, -2000, 3600000U, 3700U, false);
    CHECK(soc.soc_pct >= 49 && soc.soc_pct <= 50);

    soc_set_percent(&soc, 120U);
    CHECK_NEAR(soc.soc_pct, 100, 0);
}
