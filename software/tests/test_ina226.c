#include "ina226.h"

#include "test_util.h"

typedef struct {
    uint16_t regs[256];
    bool fail;
    uint8_t last_reg;
} mock_ina_t;

static bool mock_read(void *ctx, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    mock_ina_t *m = (mock_ina_t *)ctx;
    (void)addr;
    if (m->fail) {
        return false;
    }
    if (len != 2U) {
        return false;   /* INA226 每个寄存器 16 位, 驱动逐寄存器读取 */
    }
    m->last_reg = reg;
    buf[0] = (uint8_t)(m->regs[reg] >> 8);
    buf[1] = (uint8_t)(m->regs[reg] & 0xFFU);
    return true;
}

static bool mock_write(void *ctx, uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    mock_ina_t *m = (mock_ina_t *)ctx;
    (void)addr;
    if (m->fail) {
        return false;
    }
    if (len != 2U) {
        return false;
    }
    m->regs[reg] = (uint16_t)((uint16_t)buf[0] << 8 | buf[1]);
    return true;
}

static void put_u16(mock_ina_t *m, uint8_t reg, uint16_t v)
{
    m->regs[reg] = v;
}

void test_ina226_run(void)
{
    mock_ina_t m;
    for (uint32_t i = 0; i < 256U; ++i) {
        m.regs[i] = 0U;
    }
    m.fail = false;
    m.last_reg = 0U;

    put_u16(&m, INA226_REG_MANUF_ID, INA226_MANUF_ID);
    put_u16(&m, INA226_REG_DIE_ID, INA226_DIE_ID);

    ina226_t dev;
    CHECK(ina226_init(&dev, mock_read, mock_write, &m, 0x40U, 10000U, 8192U, 1));
    CHECK(dev.initialized);
    CHECK_NEAR(dev.current_lsb_ua, 250, 0);
    CHECK_NEAR(dev.calibration, 2048, 0);
    CHECK(ina226_check_id(&dev));

    /* 7.4 V, +1 A, 10 mOhm -> 10 mV 分流 */
    put_u16(&m, INA226_REG_SHUNT, (uint16_t)(int16_t)4000);
    put_u16(&m, INA226_REG_BUS, 5920U);          /* 5920 * 1.25 = 7400 mV */
    put_u16(&m, INA226_REG_POWER, 0U);
    put_u16(&m, INA226_REG_CURRENT, (uint16_t)(int16_t)4000); /* 4000*250uA=1A */

    ina226_sample_t s;
    CHECK(ina226_read(&dev, &s));
    CHECK(s.valid);
    CHECK_NEAR(s.bus_mv, 7400, 0);
    CHECK_NEAR(s.current_ma, 1000, 1);
    CHECK_NEAR(s.power_mw, 7400, 10);
    CHECK_NEAR(s.shunt_uv, 10000, 2);

    /* 反向 -1 A */
    put_u16(&m, INA226_REG_CURRENT, (uint16_t)(int16_t)-4000);
    put_u16(&m, INA226_REG_SHUNT, (uint16_t)(int16_t)-4000);
    CHECK(ina226_read(&dev, &s));
    CHECK_NEAR(s.current_ma, -1000, 1);
    CHECK_NEAR(s.power_mw, -7400, 10);

    /* 符号取反配置 */
    ina226_t dev_inv;
    CHECK(ina226_init(&dev_inv, mock_read, mock_write, &m, 0x40U, 10000U, 8192U, -1));
    CHECK(ina226_read(&dev_inv, &s));
    CHECK_NEAR(s.current_ma, 1000, 1);

    /* 总线为 0 -> 无效 */
    put_u16(&m, INA226_REG_BUS, 0U);
    CHECK(!ina226_read(&dev, &s));
    CHECK(!s.valid);

    /* I2C 失败 */
    m.fail = true;
    CHECK(!ina226_read(&dev, &s));
    m.fail = false;

    /* 参数非法 */
    ina226_t bad;
    CHECK(!ina226_init(&bad, mock_read, mock_write, &m, 0x40U, 0U, 8192U, 1));
    CHECK(!ina226_init(&bad, mock_read, mock_write, &m, 0x40U, 10000U, 8192U, 0));

    /* 校准值溢出: 极小分流 + 大量程 */
    CHECK(!ina226_init(&bad, mock_read, mock_write, &m, 0x40U, 1U, 30000U, 1));
}
