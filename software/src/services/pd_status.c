#include "pd_status.h"

#include <stddef.h>

#include "app_config.h"

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

void pd_status_init(pd_status_t *st)
{
    if (st == 0) {
        return;
    }
    st->supported = false;
    st->valid = false;
    st->link = PD_LINK_NONE;
    st->protocol = PD_PROTO_UNKNOWN;
    st->pdo_index = 0U;
    st->voltage_mv = 0U;
    st->current_ma = 0U;
    st->power_mw = 0U;
    st->last_update_ms = 0U;
}

static pd_protocol_t decode_protocol(uint8_t code)
{
    if (code > (uint8_t)PD_PROTO_BC12) {
        return PD_PROTO_UNKNOWN;
    }
    return (pd_protocol_t)code;
}

bool pd_status_poll(pd_status_t *st, const pd_backend_t *backend, uint32_t now_ms)
{
    if (st == 0) {
        return false;
    }
    if (backend == 0 || !backend->enabled || backend->read == 0) {
        st->supported = false;
        st->valid = false;
        st->link = PD_LINK_NONE;
        return false;
    }

    st->supported = true;

    if (st->valid &&
        (uint32_t)(now_ms - st->last_update_ms) > APP_PD_TIMEOUT_MS) {
        st->valid = false;
    }

    uint8_t buf[16];
    if (!backend->read(backend->ctx, backend->addr, backend->reg_base,
                       buf, (uint16_t)sizeof(buf))) {
        st->valid = false;
        return false;
    }

    if (buf[0] != 'P' || buf[1] != 'D' || buf[2] != 1U) {
        st->valid = false;
        return false;
    }

    uint8_t checksum = 0U;
    for (uint8_t i = 0U; i < 14U; ++i) {
        checksum ^= buf[i];
    }
    if (checksum != buf[14]) {
        st->valid = false;
        return false;
    }

    uint16_t mv = rd16(&buf[6]);
    uint16_t ma = rd16(&buf[8]);
    uint16_t mw = rd16(&buf[10]);
    uint8_t flags = buf[3];

    if (mv > 48000U || ma > 10000U) {
        st->valid = false;
        return false;
    }

    st->protocol = decode_protocol(buf[4]);
    st->pdo_index = buf[5];
    st->voltage_mv = mv;
    st->current_ma = ma;
    if (mw != 0U) {
        st->power_mw = mw;
    } else {
        uint32_t calc_mw = ((uint32_t)mv * ma + 500U) / 1000U;
        st->power_mw = (calc_mw > 65535U) ? 65535U : (uint16_t)calc_mw;
    }

    if ((flags & 0x01U) == 0U) {
        st->link = PD_LINK_NONE;
        st->voltage_mv = 0U;
        st->current_ma = 0U;
        st->power_mw = 0U;
    } else if ((flags & 0x02U) != 0U) {
        st->link = PD_LINK_SOURCE;
    } else {
        st->link = PD_LINK_SINK;
    }

    st->last_update_ms = now_ms;
    st->valid = (flags & 0x04U) != 0U;
    return true;
}

bool pd_status_is_fresh(const pd_status_t *st, uint32_t now_ms)
{
    if (st == 0 || !st->supported || !st->valid) {
        return false;
    }
    return (uint32_t)(now_ms - st->last_update_ms) <= APP_PD_TIMEOUT_MS;
}

const char *pd_protocol_name(pd_protocol_t protocol)
{
    switch (protocol) {
    case PD_PROTO_PD2:
        return "PD2.0";
    case PD_PROTO_PD3:
        return "PD3.0";
    case PD_PROTO_PPS:
        return "PPS";
    case PD_PROTO_QC2:
        return "QC2.0";
    case PD_PROTO_QC3:
        return "QC3.0";
    case PD_PROTO_AFC:
        return "AFC";
    case PD_PROTO_FCP:
        return "FCP";
    case PD_PROTO_SCP:
        return "SCP";
    case PD_PROTO_APPLE:
        return "APPLE";
    case PD_PROTO_BC12:
        return "BC1.2";
    case PD_PROTO_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

const char *pd_link_name(pd_link_t link)
{
    switch (link) {
    case PD_LINK_SINK:
        return "SINK";
    case PD_LINK_SOURCE:
        return "SOURCE";
    case PD_LINK_NONE:
        return "NONE";
    case PD_LINK_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}
