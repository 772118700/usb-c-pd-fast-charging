/**
 * @file    ssd1306_font.h
 * @brief   5x7 ASCII font lookup (internal)
 */
#ifndef SSD1306_FONT_H
#define SSD1306_FONT_H

#include <stdbool.h>
#include <stdint.h>

bool ssd1306_font_lookup(char c, uint8_t out[5]);

#endif /* SSD1306_FONT_H */
