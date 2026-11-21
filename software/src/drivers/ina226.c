#include "ina226.h"

#include <stddef.h>

static bool read_u16(ina226_t *dev, uint8_t reg, uint16_t *out)
{
    uint8_t b[2];
    if (!dev->read(dev->ctx, dev->addr, reg, b, 2U)) {
        return false;
    }
    *out = (uint16_t)((uint16_t)b[0] << 8 | b[1]);   /* INA226 为大端 */
    return true;
}

static bool write_u16(ina226_t *dev, uint8_t reg, uint16_t value)
{
    uint8_t b[2] = {(uint8_t)(value >> 8), (uint8_t)(value & 0xFFU)};
    return dev->write(dev->ctx, dev->addr, reg, b, 2U);
}

bool ina226_init(ina226_t *dev,
                 ina226_read_fn read,
                 ina226_write_fn write,
                 void *ctx,
                 uint8_t addr,
                 uint32_t shunt_uohm,
                 uint32_t max_current_ma,
                 int8_t current_sign)
{
    if (dev == 0 || read == 0 || write == 0 || shunt_uohm == 0U ||
        max_current_ma == 0U || (current_sign != 1 && current_sign != -1)) {
        return false;
    }

    dev->read = read;
    dev->write = write;
    dev->ctx = ctx;
    dev->addr = addr;
    dev->shunt_uohm = shunt_uohm;
    dev->current_sign = current_sign;
    dev->initialized = false;

    /* Current_LSB = 量程 / 32768, 向上取整到 1 uA。 */
    uint32_t lsb_ua = (max_current_ma * 1000U + 32767U) / 32768U;
    if (lsb_ua == 0U) {
        lsb_ua = 1U;
    }

    /* Cal = 0.00512 / (Current_LSB[A] * R[Ohm]) = 5.12e9 / (LSB[uA]*R[uOhm]) */
    uint64_t denom = (uint64_t)lsb_ua * shunt_uohm;
    if (denom == 0U) {
        return false;
    }
    uint64_t cal = (5120000000ULL + denom / 2U) / denom;
    if (cal == 0U || cal > 65535U) {
        return false;
    }

    dev->current_lsb_ua = lsb_ua;
    dev->calibration = (uint16_t)cal;

    if (!write_u16(dev, INA226_REG_CONFIG, INA226_CONFIG_VALUE)) {
        return false;
    }
    if (!write_u16(dev, INA226_REG_CALIBRATION, dev->calibration)) {
        return false;
    }

    dev->initialized = true;
    return true;
}

bool ina226_check_id(ina226_t *dev)
{
    if (dev == 0 || !dev->initialized) {
        return false;
    }
    uint16_t manuf = 0U;
    uint16_t die = 0U;
    if (!read_u16(dev, INA226_REG_MANUF_ID, &manuf) ||
        !read_u16(dev, INA226_REG_DIE_ID, &die)) {
        return false;
    }
    return manuf == INA226_MANUF_ID && die == INA226_DIE_ID;
}

bool ina226_read(ina226_t *dev, ina226_sample_t *out)
{
    if (dev == 0 || out == 0 || !dev->initialized) {
        return false;
    }

    uint16_t shunt_u = 0U;
    uint16_t bus_u = 0U;
    uint16_t current_u = 0U;
    if (!read_u16(dev, INA226_REG_SHUNT, &shunt_u) ||
        !read_u16(dev, INA226_REG_BUS, &bus_u) ||
        !read_u16(dev, INA226_REG_CURRENT, &current_u)) {
        out->valid = false;
        return false;
    }

    int16_t shunt_raw = (int16_t)shunt_u;
    uint16_t bus_raw = bus_u;
    int16_t current_raw = (int16_t)current_u;

    int32_t shunt_uv = ((int32_t)shunt_raw * 5) / 2;      /* 2.5 uV/LSB */
    uint32_t bus_mv = ((uint32_t)bus_raw * 5U) / 4U;      /* 1.25 mV/LSB */
    int64_t current_ua = (int64_t)current_raw *
                         (int64_t)dev->current_lsb_ua *
                         (int64_t)dev->current_sign;
    int32_t current_ma = (int32_t)(current_ua / 1000);

    out->shunt_uv = shunt_uv;
    out->bus_mv = bus_mv;
    out->current_ma = current_ma;
    out->power_mw = (int32_t)(((int64_t)bus_mv * current_ma) / 1000);

    /* 合理性检查: 2S 电池母线应在 0..36 V; 电流不应超过配置量程太多。 */
    if (bus_mv == 0U || bus_mv > 36000U || current_raw == INT16_MIN) {
        out->valid = false;
        return false;
    }
    out->valid = true;
    return true;
}

bool ina226_read_bus_mv(ina226_t *dev, uint32_t *bus_mv)
{
    if (dev == 0 || bus_mv == 0 || !dev->initialized) {
        return false;
    }
    uint16_t raw = 0U;
    if (!read_u16(dev, INA226_REG_BUS, &raw)) {
        return false;
    }
    *bus_mv = ((uint32_t)raw * 5U) / 4U;
    return true;
}
