#include "ntc.h"

#include <math.h>

static bool ntc_config_valid(const ntc_config_t *cfg)
{
    return cfg != 0 && cfg->series_ohm > 0U && cfg->r25_ohm > 0U &&
           cfg->beta > 0U && cfg->vref_mv > 0U;
}

bool ntc_mv_to_temp(uint16_t adc_mv, const ntc_config_t *cfg, ntc_reading_t *out)
{
    if (out == 0 || !ntc_config_valid(cfg)) {
        return false;
    }

    out->temp_c_x10 = 0;
    out->adc_mv = adc_mv;
    out->resistance_ohm = 0U;
    out->valid = false;
    out->open_circuit = false;
    out->short_circuit = false;

    if (adc_mv >= (uint16_t)(cfg->vref_mv - 5U)) {
        out->open_circuit = true;
        return false;
    }
    if (adc_mv <= 5U) {
        out->short_circuit = true;
        return false;
    }
    if (adc_mv >= cfg->vref_mv) {
        out->open_circuit = true;
        return false;
    }

    double v = (double)adc_mv;
    double vref = (double)cfg->vref_mv;
    double r = (double)cfg->series_ohm * v / (vref - v);

    if (r > 1000000.0) {
        out->open_circuit = true;
        return false;
    }
    if (r < 20.0) {
        out->short_circuit = true;
        return false;
    }

    double inv_t = (1.0 / 298.15) + log(r / (double)cfg->r25_ohm) / (double)cfg->beta;
    if (inv_t <= 0.0) {
        return false;
    }
    double temp_c = (1.0 / inv_t) - 273.15;
    if (temp_c < -60.0 || temp_c > 150.0) {
        return false;
    }

    out->resistance_ohm = (uint32_t)(r + 0.5);
    out->temp_c_x10 = (int16_t)lround(temp_c * 10.0);
    out->valid = true;
    return true;
}

bool ntc_raw_to_temp(uint16_t raw, const ntc_config_t *cfg, ntc_reading_t *out)
{
    if (!ntc_config_valid(cfg) || raw > 4095U) {
        return false;
    }
    uint32_t mv = ((uint32_t)raw * cfg->vref_mv + 2047U) / 4095U;
    return ntc_mv_to_temp((uint16_t)mv, cfg, out);
}
