#include "soc.h"

#include <stddef.h>

typedef struct {
    uint16_t ocv_mv;
    uint8_t soc_pct;
} soc_point_t;

/* 2S 单节 OCV-SOC 近似表 (25 degC, 三星 25R 类电芯), 必须单调递增。 */
static const soc_point_t k_soc_table[] = {
    {3000U, 0U},
    {3200U, 3U},
    {3300U, 6U},
    {3400U, 12U},
    {3500U, 22U},
    {3600U, 36U},
    {3700U, 52U},
    {3800U, 66U},
    {3900U, 78U},
    {4000U, 88U},
    {4100U, 95U},
    {4200U, 100U},
};

static int32_t abs_i32(int32_t v)
{
    return (v < 0) ? -v : v;
}

uint8_t soc_from_ocv_mv(uint16_t cell_mv)
{
    const size_t n = sizeof(k_soc_table) / sizeof(k_soc_table[0]);
    if (cell_mv <= k_soc_table[0].ocv_mv) {
        return 0U;
    }
    if (cell_mv >= k_soc_table[n - 1U].ocv_mv) {
        return 100U;
    }

    for (size_t i = 0U; i + 1U < n; ++i) {
        const soc_point_t *a = &k_soc_table[i];
        const soc_point_t *b = &k_soc_table[i + 1U];
        if (cell_mv >= a->ocv_mv && cell_mv <= b->ocv_mv) {
            uint32_t num = (uint32_t)(cell_mv - a->ocv_mv) *
                           (uint32_t)(b->soc_pct - a->soc_pct);
            uint32_t den = (uint32_t)(b->ocv_mv - a->ocv_mv);
            return (uint8_t)(a->soc_pct + (num + den / 2U) / den);
        }
    }
    return 0U;
}

void soc_init(soc_t *soc, int32_t capacity_mah)
{
    if (soc == NULL) {
        return;
    }
    if (capacity_mah < 100) {
        capacity_mah = 100;
    }
    soc->capacity_mah = capacity_mah;
    soc->capacity_ma_ms = (int64_t)capacity_mah * 3600000LL;
    soc->remaining_ma_ms = soc->capacity_ma_ms / 2;
    soc->soc_pct = 50U;
    soc->rest_ms = 0U;
    soc->ocv_corrected = false;
    soc->initialized = true;
}

static void soc_refresh_pct(soc_t *soc)
{
    if (soc->capacity_ma_ms <= 0) {
        soc->soc_pct = 0U;
        return;
    }
    int64_t scaled = soc->remaining_ma_ms * 100LL + soc->capacity_ma_ms / 2;
    int64_t pct = scaled / soc->capacity_ma_ms;
    if (pct < 0) {
        pct = 0;
    } else if (pct > 100) {
        pct = 100;
    }
    soc->soc_pct = (uint8_t)pct;
}

void soc_seed_from_ocv(soc_t *soc, uint16_t cell_mv)
{
    if (soc == NULL || !soc->initialized) {
        return;
    }
    uint8_t pct = soc_from_ocv_mv(cell_mv);
    soc->remaining_ma_ms = soc->capacity_ma_ms * (int64_t)pct / 100LL;
    soc->soc_pct = pct;
    soc->rest_ms = 0U;
    soc->ocv_corrected = false;
}

static void soc_apply_ocv_correction(soc_t *soc, uint16_t cell_mv)
{
    uint8_t target = soc_from_ocv_mv(cell_mv);
    int32_t diff = (int32_t)target - (int32_t)soc->soc_pct;

    soc->ocv_corrected = true;
    if (diff > (int32_t)5) {
        diff = 5;
    } else if (diff < -5) {
        diff = -5;
    }
    if (diff != 0) {
        soc->remaining_ma_ms += soc->capacity_ma_ms * (int64_t)diff / 100LL;
        if (soc->remaining_ma_ms < 0) {
            soc->remaining_ma_ms = 0;
        } else if (soc->remaining_ma_ms > soc->capacity_ma_ms) {
            soc->remaining_ma_ms = soc->capacity_ma_ms;
        }
        soc_refresh_pct(soc);
    }
}

void soc_update(soc_t *soc,
                int16_t current_ma,
                uint32_t dt_ms,
                uint16_t cell_mv,
                bool cell_valid)
{
    if (soc == NULL || !soc->initialized) {
        return;
    }
    if (dt_ms == 0U) {
        return;
    }
    if (dt_ms > 5000U) {
        dt_ms = 5000U;   /* 调度暂停/调试断点后避免一次性大跳变 */
    }

    int64_t delta = (int64_t)current_ma * (int64_t)dt_ms;
    if (delta > 0) {
        delta = delta * 98LL / 100LL;   /* 充电库仑效率 98% */
    }

    soc->remaining_ma_ms += delta;
    if (soc->remaining_ma_ms < 0) {
        soc->remaining_ma_ms = 0;
    } else if (soc->remaining_ma_ms > soc->capacity_ma_ms) {
        soc->remaining_ma_ms = soc->capacity_ma_ms;
    }
    soc_refresh_pct(soc);

    /* 静置 30 min 后, 用 OCV 做一次最多 ±5% 的慢修正。 */
    if (cell_valid && abs_i32(current_ma) <= 50) {
        if (soc->rest_ms < 0xFFFFFFFFU) {
            soc->rest_ms += dt_ms;
        }
        if (!soc->ocv_corrected && soc->rest_ms >= (30UL * 60UL * 1000UL)) {
            soc_apply_ocv_correction(soc, cell_mv);
            /* 允许再静置 30 min 后继续小步逼近 OCV, 避免一次跳变。 */
            soc->rest_ms = 0U;
            soc->ocv_corrected = false;
        }
    } else {
        soc->rest_ms = 0U;
        soc->ocv_corrected = false;
    }
}

void soc_set_percent(soc_t *soc, uint8_t percent)
{
    if (soc == NULL || !soc->initialized) {
        return;
    }
    if (percent > 100U) {
        percent = 100U;
    }
    soc->remaining_ma_ms = soc->capacity_ma_ms * (int64_t)percent / 100LL;
    soc->soc_pct = percent;
    soc->rest_ms = 0U;
    soc->ocv_corrected = false;
}
