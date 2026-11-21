#include "ssd1306.h"

#include <stddef.h>
#include <string.h>

#include "ssd1306_font.h"

static bool write_cmd(ssd1306_t *dev, uint8_t cmd)
{
    uint8_t b[2] = {0x00U, cmd};
    return dev->write(dev->ctx, dev->addr, b, (uint16_t)sizeof(b));
}

static bool write_cmd2(ssd1306_t *dev, uint8_t cmd, uint8_t arg)
{
    uint8_t b[3] = {0x00U, cmd, arg};
    return dev->write(dev->ctx, dev->addr, b, (uint16_t)sizeof(b));
}

bool ssd1306_init(ssd1306_t *dev, ssd1306_write_fn write, void *ctx, uint8_t addr)
{
    if (dev == 0 || write == 0) {
        return false;
    }
    dev->write = write;
    dev->ctx = ctx;
    dev->addr = addr;
    dev->ready = false;
    memset(dev->buffer, 0, sizeof(dev->buffer));

    if (!write_cmd(dev, 0xAEU)) {            /* display off */
        return false;
    }
    if (!write_cmd2(dev, 0xD5U, 0x80U) ||    /* clock divide */
        !write_cmd2(dev, 0xA8U, 0x3FU) ||    /* multiplex 64 */
        !write_cmd2(dev, 0xD3U, 0x00U) ||    /* display offset */
        !write_cmd(dev, 0x40U) ||            /* start line 0 */
        !write_cmd2(dev, 0x8DU, 0x14U) ||    /* charge pump on */
        !write_cmd2(dev, 0x20U, 0x00U) ||    /* horizontal addressing */
        !write_cmd(dev, 0xA1U) ||            /* segment remap */
        !write_cmd(dev, 0xC8U) ||            /* COM scan dec */
        !write_cmd2(dev, 0xDAU, 0x12U) ||    /* COM pins */
        !write_cmd2(dev, 0x81U, 0xCFU) ||    /* contrast */
        !write_cmd2(dev, 0xD9U, 0xF1U) ||    /* pre-charge */
        !write_cmd2(dev, 0xDBU, 0x40U) ||    /* VCOMH */
        !write_cmd(dev, 0xA4U) ||            /* resume RAM display */
        !write_cmd(dev, 0xA6U) ||            /* normal */
        !write_cmd(dev, 0x2EU)) {            /* deactivate scroll */
        return false;
    }
    if (!write_cmd(dev, 0xAFU)) {            /* display on */
        return false;
    }

    dev->ready = true;
    return true;
}

void ssd1306_clear(ssd1306_t *dev)
{
    if (dev != 0) {
        memset(dev->buffer, 0, sizeof(dev->buffer));
    }
}

void ssd1306_draw_pixel(ssd1306_t *dev, int16_t x, int16_t y, bool on)
{
    if (dev == 0 || x < 0 || y < 0 ||
        x >= (int16_t)SSD1306_WIDTH || y >= (int16_t)SSD1306_HEIGHT) {
        return;
    }
    uint16_t index = (uint16_t)((y / 8) * (int16_t)SSD1306_WIDTH + x);
    uint8_t mask = (uint8_t)(1U << (y & 7));
    if (on) {
        dev->buffer[index] |= mask;
    } else {
        dev->buffer[index] &= (uint8_t)~mask;
    }
}

void ssd1306_draw_char(ssd1306_t *dev, int16_t x, int16_t y, char c)
{
    if (dev == 0) {
        return;
    }
    if (c == '\n') {
        return;
    }
    uint8_t glyph[5];
    (void)ssd1306_font_lookup(c, glyph);
    for (uint8_t col = 0U; col < 5U; ++col) {
        for (uint8_t row = 0U; row < 7U; ++row) {
            if ((glyph[col] & (uint8_t)(1U << row)) != 0U) {
                ssd1306_draw_pixel(dev, (int16_t)(x + col), (int16_t)(y + row), true);
            }
        }
    }
}

void ssd1306_draw_text(ssd1306_t *dev, int16_t x, int16_t y, const char *text)
{
    if (dev == 0 || text == 0) {
        return;
    }
    int16_t cx = x;
    int16_t cy = y;
    while (*text != '\0') {
        if (*text == '\n') {
            cx = x;
            cy = (int16_t)(cy + 8);
        } else {
            ssd1306_draw_char(dev, cx, cy, *text);
            cx = (int16_t)(cx + 6);
        }
        ++text;
    }
}

bool ssd1306_flush(ssd1306_t *dev)
{
    if (dev == 0 || !dev->ready) {
        return false;
    }

    for (uint8_t page = 0U; page < SSD1306_PAGES; ++page) {
        uint8_t cmd[4] = {0x00U, (uint8_t)(0xB0U + page), 0x00U, 0x10U};
        if (!dev->write(dev->ctx, dev->addr, cmd, (uint16_t)sizeof(cmd))) {
            return false;
        }

        uint8_t data[1U + SSD1306_WIDTH];
        data[0] = 0x40U;
        memcpy(&data[1], &dev->buffer[(uint16_t)page * SSD1306_WIDTH], SSD1306_WIDTH);
        if (!dev->write(dev->ctx, dev->addr, data, (uint16_t)sizeof(data))) {
            return false;
        }
    }
    return true;
}
