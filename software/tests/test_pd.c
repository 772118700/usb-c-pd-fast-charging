#include "pd_status.h"

#include <string.h>

#include "test_util.h"

typedef struct {
    uint8_t frame[16];
    bool fail;
    uint32_t reads;
} mock_pd_t;

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)(v >> 8);
}

static void frame_build(mock_pd_t *m, uint8_t flags, uint8_t proto,
                        uint8_t pdo, uint16_t mv, uint16_t ma, uint16_t mw)
{
    memset(m->frame, 0, sizeof(m->frame));
    m->frame[0] = 'P';
    m->frame[1] = 'D';
    m->frame[2] = 1U;
    m->frame[3] = flags;
    m->frame[4] = proto;
    m->frame[5] = pdo;
    put16(&m->frame[6], mv);
    put16(&m->frame[8], ma);
    put16(&m->frame[10], mw);
    uint8_t sum = 0U;
    for (uint8_t i = 0U; i < 14U; ++i) {
        sum ^= m->frame[i];
    }
    m->frame[14] = sum;
}

static bool mock_pd_read(void *ctx, uint8_t addr, uint8_t reg,
                         uint8_t *buf, uint16_t len)
{
    mock_pd_t *m = (mock_pd_t *)ctx;
    (void)addr;
    (void)reg;
    if (m->fail) {
        return false;
    }
    if (len != 16U) {
        return false;
    }
    memcpy(buf, m->frame, 16U);
    ++m->reads;
    return true;
}

void test_pd_run(void)
{
    mock_pd_t m;
    memset(&m, 0, sizeof(m));
    pd_backend_t backend = {mock_pd_read, &m, 0x2EU, 0x00U, true};
    pd_status_t st;

    pd_status_init(&st);
    CHECK(!st.supported && !st.valid);
    CHECK(!pd_status_is_fresh(&st, 0U));

    frame_build(&m, 0x07U, PD_PROTO_PD3, 3U, 20000U, 1500U, 30000U);
    CHECK(pd_status_poll(&st, &backend, 1000U));
    CHECK(st.supported && st.valid);
    CHECK(st.link == PD_LINK_SOURCE);
    CHECK(st.protocol == PD_PROTO_PD3);
    CHECK_NEAR(st.voltage_mv, 20000, 0);
    CHECK_NEAR(st.current_ma, 1500, 0);
    CHECK_NEAR(st.power_mw, 30000, 0);
    CHECK(pd_status_is_fresh(&st, 3000U));
    CHECK(!pd_status_is_fresh(&st, 3001U));

    /* 超时后轮询仍可恢复 */
    frame_build(&m, 0x07U, PD_PROTO_PPS, 4U, 15000U, 2000U, 30000U);
    CHECK(pd_status_poll(&st, &backend, 4000U));
    CHECK(st.valid);
    CHECK(st.protocol == PD_PROTO_PPS);

    /* 未连接 */
    frame_build(&m, 0x04U, PD_PROTO_PD3, 0U, 20000U, 0U, 0U);
    CHECK(pd_status_poll(&st, &backend, 5000U));
    CHECK(st.valid);
    CHECK(st.link == PD_LINK_NONE);
    CHECK_NEAR(st.voltage_mv, 0, 0);
    CHECK_NEAR(st.power_mw, 0, 0);

    /* 校验和错误 */
    frame_build(&m, 0x07U, PD_PROTO_PD3, 1U, 20000U, 1000U, 20000U);
    m.frame[14] ^= 0xFFU;
    CHECK(!pd_status_poll(&st, &backend, 6000U));
    CHECK(!st.valid);

    /* I2C 失败 */
    m.fail = true;
    CHECK(!pd_status_poll(&st, &backend, 7000U));
    m.fail = false;

    /* 后端未启用 */
    pd_backend_t off = {mock_pd_read, &m, 0x2EU, 0x00U, false};
    CHECK(!pd_status_poll(&st, &off, 8000U));
    CHECK(!st.supported);

    /* 协议码越界 -> UNKNOWN */
    frame_build(&m, 0x07U, 99U, 1U, 5000U, 1000U, 5000U);
    CHECK(pd_status_poll(&st, &backend, 9000U));
    CHECK(st.protocol == PD_PROTO_UNKNOWN);

    /* 名称 */
    CHECK(strcmp(pd_protocol_name(PD_PROTO_PD3), "PD3.0") == 0);
    CHECK(strcmp(pd_link_name(PD_LINK_SINK), "SINK") == 0);
}
