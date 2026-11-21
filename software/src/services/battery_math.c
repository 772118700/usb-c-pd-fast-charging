#include "battery_math.h"

static bool divider_params_valid(const voltage_divider_t *div)
{
    return div != 0 && div->vref_mv > 0U && div->full_scale > 0U &&
           div->top_ohm > 0U && div->bottom_ohm > 0U;
}

bool divider_raw_to_mv(uint16_t raw, const voltage_divider_t *div, uint16_t *out_mv)
{
    if (out_mv == 0 || !divider_params_valid(div)) {
        return false;
    }
    if (raw == 0U || raw >= div->full_scale) {
        return false;   /* 贴边: 开路、短路或 ADC 饱和 */
    }

    uint64_t numerator = (uint64_t)raw * div->vref_mv *
                         ((uint64_t)div->top_ohm + div->bottom_ohm);
    uint64_t denominator = (uint64_t)div->full_scale * div->bottom_ohm;
    uint64_t mv = (numerator + denominator / 2U) / denominator;

    if (mv > 65535U) {
        return false;
    }
    *out_mv = (uint16_t)mv;
    return true;
}

bool battery_calc_from_raw(uint16_t raw_pack,
                           uint16_t raw_mid,
                           const voltage_divider_t *pack_div,
                           const voltage_divider_t *cell_div,
                           battery_voltage_t *out)
{
    if (out == 0) {
        return false;
    }

    out->pack_mv = 0U;
    out->cell_mv[0] = 0U;
    out->cell_mv[1] = 0U;
    out->cell_delta_mv = 0U;
    out->pack_valid = false;
    out->cells_valid = false;

    uint16_t pack_mv = 0U;
    uint16_t mid_mv = 0U;
    if (!divider_raw_to_mv(raw_pack, pack_div, &pack_mv) ||
        !divider_raw_to_mv(raw_mid, cell_div, &mid_mv)) {
        return false;
    }
    if (mid_mv > pack_mv) {
        return false;
    }

    out->pack_mv = pack_mv;
    out->cell_mv[0] = mid_mv;
    out->cell_mv[1] = (uint16_t)(pack_mv - mid_mv);
    out->cell_delta_mv = (out->cell_mv[0] > out->cell_mv[1])
                             ? (uint16_t)(out->cell_mv[0] - out->cell_mv[1])
                             : (uint16_t)(out->cell_mv[1] - out->cell_mv[0]);

    out->pack_valid = (pack_mv >= 1000U && pack_mv <= 10000U);
    out->cells_valid = (out->cell_mv[0] >= 500U && out->cell_mv[0] <= 5000U &&
                        out->cell_mv[1] >= 500U && out->cell_mv[1] <= 5000U);
    return out->pack_valid && out->cells_valid;
}
