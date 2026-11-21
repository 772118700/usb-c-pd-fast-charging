#include "display_model.h"

#include <stdio.h>
#include <string.h>

#include "balance.h"
#include "pd_status.h"

static void fmt_temp(char *buf, size_t len, int16_t c_x10, bool valid)
{
    if (!valid) {
        (void)snprintf(buf, len, "N/A");
        return;
    }
    int32_t v = c_x10;
    char sign = '+';
    if (v < 0) {
        sign = '-';
        v = -v;
    }
    (void)snprintf(buf, len, "%c%ld.%ldC", sign, (long)(v / 10), (long)(v % 10));
}

static void fmt_current(char *buf, size_t len, int16_t ma)
{
    int32_t v = ma;
    char sign = (v < 0) ? '-' : '+';
    if (v < 0) {
        v = -v;
    }
    (void)snprintf(buf, len, "%c%lu.%02luA", sign,
                   (unsigned long)(v / 1000), (unsigned long)((v % 1000) / 10));
}

static void render_main(const display_input_t *in,
                        char lines[DISPLAY_LINES][DISPLAY_LINE_LEN])
{
    uint16_t pack = in->pack_mv;
    unsigned soc = in->soc_pct;
    (void)snprintf(lines[0], DISPLAY_LINE_LEN, "PACK %u.%02uV SOC %3u%%",
                   (unsigned)(pack / 1000U), (unsigned)((pack % 1000U) / 10U), soc);

    char cur[12];
    fmt_current(cur, sizeof(cur), in->current_ma);
    uint32_t p = (in->power_mw < 0) ? (uint32_t)(-in->power_mw)
                                    : (uint32_t)in->power_mw;
    (void)snprintf(lines[1], DISPLAY_LINE_LEN, "I%s P%lu.%01luW", cur,
                   (unsigned long)(p / 1000U), (unsigned long)((p % 1000U) / 100U));

    char t1[12];
    char t2[12];
    fmt_temp(t1, sizeof(t1), in->temp_c_x10[0], in->temp_valid[0]);
    fmt_temp(t2, sizeof(t2), in->temp_c_x10[1], in->temp_valid[1]);
    (void)snprintf(lines[2], DISPLAY_LINE_LEN, "T1%7s T2%7s", t1, t2);

    (void)snprintf(lines[3], DISPLAY_LINE_LEN, "dCELL %03umV %s",
                   (unsigned)in->cell_delta_mv,
                   balance_state_name((balance_state_t)in->balance_state));
}

static const char *short_link(uint8_t link)
{
    switch ((pd_link_t)link) {
    case PD_LINK_SOURCE:
        return "SRC";
    case PD_LINK_SINK:
        return "SNK";
    case PD_LINK_NONE:
        return "NONE";
    case PD_LINK_UNKNOWN:
    default:
        return "?";
    }
}

static void render_pd(const display_input_t *in,
                      char lines[DISPLAY_LINES][DISPLAY_LINE_LEN])
{
    if (!in->pd_supported) {
        (void)snprintf(lines[0], DISPLAY_LINE_LEN, "PD N/A (BLACK BOX)");
        (void)snprintf(lines[1], DISPLAY_LINE_LEN, "IP5389 NO I2C LINK");
        (void)snprintf(lines[2], DISPLAY_LINE_LEN, "READ-ONLY VIA PCBA");
        (void)snprintf(lines[3], DISPLAY_LINE_LEN, "CHECK VENDOR DOC");
        return;
    }
    if (!in->pd_valid) {
        (void)snprintf(lines[0], DISPLAY_LINE_LEN, "PD LINK STALE");
        (void)snprintf(lines[1], DISPLAY_LINE_LEN, "CHECK I2C/ADAPTER");
        (void)snprintf(lines[2], DISPLAY_LINE_LEN, "NO PD CONTROL");
        (void)snprintf(lines[3], DISPLAY_LINE_LEN, "CHECK ADAPTER");
        return;
    }

    (void)snprintf(lines[0], DISPLAY_LINE_LEN, "%s PDO#%u",
                   pd_protocol_name((pd_protocol_t)in->pd_protocol),
                   (unsigned)in->pd_pdo_index);
    (void)snprintf(lines[1], DISPLAY_LINE_LEN, "%u.%02uV %u.%02uA",
                   (unsigned)(in->pd_mv / 1000U),
                   (unsigned)((in->pd_mv % 1000U) / 10U),
                   (unsigned)(in->pd_ma / 1000U),
                   (unsigned)((in->pd_ma % 1000U) / 10U));
    (void)snprintf(lines[2], DISPLAY_LINE_LEN, "%u.%01uW LINK %s",
                   (unsigned)(in->pd_mw / 1000U),
                   (unsigned)((in->pd_mw % 1000U) / 100U),
                   short_link(in->pd_link));
    (void)snprintf(lines[3], DISPLAY_LINE_LEN, "PD READ-ONLY");
}

static void render_fault(const display_input_t *in,
                         char lines[DISPLAY_LINES][DISPLAY_LINE_LEN])
{
    (void)snprintf(lines[0], DISPLAY_LINE_LEN, "FAULT ACT %08lX",
                   (unsigned long)in->fault_active);
    (void)snprintf(lines[1], DISPLAY_LINE_LEN, "FAULT LAT %08lX",
                   (unsigned long)in->fault_latched);
    (void)snprintf(lines[2], DISPLAY_LINE_LEN, "INHIBIT %s T%lus",
                   in->inhibit ? "YES" : "NO ", (unsigned long)in->uptime_s);
    (void)snprintf(lines[3], DISPLAY_LINE_LEN, "INA %s OLED %s",
                   in->ina_ready ? "OK" : "ERR",
                   in->oled_ready ? "OK" : "ERR");
}

void display_render(const display_input_t *in,
                    char lines[DISPLAY_LINES][DISPLAY_LINE_LEN])
{
    if (lines == 0) {
        return;
    }
    for (uint8_t i = 0U; i < DISPLAY_LINES; ++i) {
        lines[i][0] = '\0';
    }
    if (in == 0) {
        return;
    }

    uint8_t page = in->page;
    if (page >= DISPLAY_PAGE_COUNT) {
        page = 0U;
    }

    switch (page) {
    case 1U:
        render_pd(in, lines);
        break;
    case 2U:
        render_fault(in, lines);
        break;
    case 0U:
    default:
        render_main(in, lines);
        break;
    }
}
