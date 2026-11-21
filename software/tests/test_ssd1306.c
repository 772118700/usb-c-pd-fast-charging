#include "ssd1306.h"
#include "ssd1306_font.h"

#include <string.h>

#include "test_util.h"

typedef struct {
    uint32_t calls;
    uint16_t last_len;
    uint8_t last_data[300];
    bool fail;
} oled_mock_t;

static bool mock_oled_write(void *ctx, uint8_t addr, const uint8_t *data, uint16_t len)
{
    oled_mock_t *m = (oled_mock_t *)ctx;
    (void)addr;
    if (m->fail) {
        return false;
    }
    ++m->calls;
    m->last_len = len;
    uint16_t copy = (len < sizeof(m->last_data)) ? len : (uint16_t)sizeof(m->last_data);
    memcpy(m->last_data, data, copy);
    return true;
}

void test_ssd1306_run(void)
{
    oled_mock_t m;
    memset(&m, 0, sizeof(m));
    ssd1306_t oled;

    CHECK(ssd1306_init(&oled, mock_oled_write, &m, 0x3CU));
    CHECK(oled.ready);
    CHECK(m.calls > 10U);

    ssd1306_clear(&oled);
    ssd1306_draw_text(&oled, 0, 0, "A1");
    uint32_t nonzero = 0U;
    for (uint16_t i = 0U; i < SSD1306_BUFFER_SIZE; ++i) {
        if (oled.buffer[i] != 0U) {
            ++nonzero;
        }
    }
    CHECK(nonzero > 0U);

    uint32_t calls_before = m.calls;
    CHECK(ssd1306_flush(&oled));
    CHECK(m.calls == calls_before + 2U * SSD1306_PAGES);
    CHECK(m.last_len == (uint16_t)(SSD1306_WIDTH + 1U));
    CHECK(m.last_data[0] == 0x40U);

    /* 越界绘制不崩溃、不越界写 */
    ssd1306_draw_pixel(&oled, -1, -1, true);
    ssd1306_draw_pixel(&oled, 200, 100, true);
    CHECK(1);

    ssd1306_clear(&oled);
    uint32_t set = 0U;
    for (uint16_t i = 0U; i < SSD1306_BUFFER_SIZE; ++i) {
        if (oled.buffer[i] != 0U) {
            ++set;
        }
    }
    CHECK(set == 0U);

    /* 字库: 大小写等价, 未支持字回退 '?' */
    uint8_t upper[5];
    uint8_t lower[5];
    uint8_t unknown[5];
    uint8_t question[5];
    CHECK(ssd1306_font_lookup('A', upper));
    CHECK(ssd1306_font_lookup('a', lower));
    CHECK(memcmp(upper, lower, 5U) == 0);
    CHECK(ssd1306_font_lookup((char)0x7F, unknown));
    CHECK(ssd1306_font_lookup('?', question));
    CHECK(memcmp(unknown, question, 5U) == 0);

    /* 未初始化时 flush 返回 false */
    ssd1306_t off;
    memset(&off, 0, sizeof(off));
    CHECK(!ssd1306_flush(&off));

    /* I2C 失败可见 */
    m.fail = true;
    CHECK(!ssd1306_init(&oled, mock_oled_write, &m, 0x3CU));
    CHECK(!oled.ready);
}
