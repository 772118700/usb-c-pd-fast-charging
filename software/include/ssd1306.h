/**
 * @file    ssd1306.h
 * @brief   SSD1306/SSD1315 128x64 I2C OLED 最小驱动 (带 1 KB 帧缓存)
 */
#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_WIDTH       128U
#define SSD1306_HEIGHT      64U
#define SSD1306_PAGES       (SSD1306_HEIGHT / 8U)
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_PAGES)

typedef bool (*ssd1306_write_fn)(void *ctx, uint8_t addr,
                                 const uint8_t *data, uint16_t len);

typedef struct {
    ssd1306_write_fn write;
    void *ctx;
    uint8_t addr;
    uint8_t buffer[SSD1306_BUFFER_SIZE];
    bool ready;
} ssd1306_t;

bool ssd1306_init(ssd1306_t *dev, ssd1306_write_fn write, void *ctx, uint8_t addr);
void ssd1306_clear(ssd1306_t *dev);
void ssd1306_draw_pixel(ssd1306_t *dev, int16_t x, int16_t y, bool on);
void ssd1306_draw_char(ssd1306_t *dev, int16_t x, int16_t y, char c);
void ssd1306_draw_text(ssd1306_t *dev, int16_t x, int16_t y, const char *text);
bool ssd1306_flush(ssd1306_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* SSD1306_H */
