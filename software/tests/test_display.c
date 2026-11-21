#include "display_model.h"

#include <string.h>

#include "test_util.h"

static display_input_t sample_input(void)
{
    display_input_t in;
    memset(&in, 0, sizeof(in));
    in.pack_mv = 7840U;
    in.current_ma = 1230;
    in.power_mw = 9660U;
    in.soc_pct = 76U;
    in.cell_delta_mv = 8U;
    in.temp_c_x10[0] = 284;
    in.temp_c_x10[1] = 312;
    in.temp_valid[0] = true;
    in.temp_valid[1] = true;
    in.balance_state = 0U;
    in.ina_ready = true;
    in.oled_ready = true;
    in.page = 0U;
    in.uptime_s = 123U;
    return in;
}

void test_display_run(void)
{
    display_input_t in = sample_input();
    char lines[DISPLAY_LINES][DISPLAY_LINE_LEN];

    display_render(&in, lines);
    for (uint8_t i = 0U; i < DISPLAY_LINES; ++i) {
        CHECK(strlen(lines[i]) <= 21U);
    }
    CHECK(strstr(lines[0], "PACK") != 0);
    CHECK(strstr(lines[0], "7.84") != 0);
    CHECK(strstr(lines[0], "76%") != 0);
    CHECK(strstr(lines[1], "+1.23A") != 0);
    CHECK(strstr(lines[1], "9.6W") != 0);
    CHECK(strstr(lines[2], "28.4C") != 0);
    CHECK(strstr(lines[3], "008mV") != 0);
    CHECK(strstr(lines[3], "OK") != 0);

    /* 负电流/负温度 */
    in.current_ma = -4100;
    in.power_mw = -30340;
    in.temp_c_x10[0] = -53;
    in.page = 0U;
    display_render(&in, lines);
    CHECK(strstr(lines[1], "-4.10A") != 0);
    CHECK(strstr(lines[1], "30.3W") != 0);
    CHECK(strstr(lines[2], "-5.3C") != 0);

    /* PD 不支持 (IP5389 黑盒默认) */
    in.page = 1U;
    in.pd_supported = false;
    display_render(&in, lines);
    CHECK(strstr(lines[0], "N/A") != 0);
    CHECK(strstr(lines[1], "IP5389") != 0);

    /* PD 有效帧 */
    in.pd_supported = true;
    in.pd_valid = true;
    in.pd_protocol = 2U;   /* PD3.0 */
    in.pd_pdo_index = 3U;
    in.pd_mv = 20000U;
    in.pd_ma = 1500U;
    in.pd_mw = 30000U;
    in.pd_link = 2U;       /* SOURCE */
    display_render(&in, lines);
    CHECK(strstr(lines[0], "PD3.0") != 0);
    CHECK(strstr(lines[0], "PDO#3") != 0);
    CHECK(strstr(lines[1], "20.00V") != 0);
    CHECK(strstr(lines[2], "30.0W") != 0);
    CHECK(strstr(lines[2], "SRC") != 0);

    /* 故障页 */
    in.page = 2U;
    in.fault_active = 0x00000401U;
    in.fault_latched = 0x00000001U;
    in.inhibit = true;
    display_render(&in, lines);
    CHECK(strstr(lines[0], "FAULT ACT") != 0);
    CHECK(strstr(lines[2], "INHIBIT YES") != 0);
    CHECK(strstr(lines[3], "INA OK") != 0);

    /* 页码越界回落到第 0 页 */
    in.page = 9U;
    display_render(&in, lines);
    CHECK(strstr(lines[0], "PACK") != 0);

    /* 空输入不崩溃 */
    display_render(0, lines);
    CHECK(lines[0][0] == '\0');
}
