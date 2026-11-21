/**
 * @file    display_model.h
 * @brief   OLED 页面内容生成 (纯函数, 不依赖 I2C)
 */
#ifndef DISPLAY_MODEL_H
#define DISPLAY_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_LINES       4U
#define DISPLAY_LINE_LEN    22U
#define DISPLAY_PAGE_COUNT  3U

typedef struct {
    uint16_t pack_mv;
    int16_t current_ma;
    int32_t power_mw;
    uint8_t soc_pct;
    uint16_t cell_delta_mv;
    int16_t temp_c_x10[2];
    bool temp_valid[2];
    uint8_t balance_state;      /* balance_state_t */
    bool ina_ready;
    bool oled_ready;

    bool pd_supported;
    bool pd_valid;
    uint8_t pd_protocol;
    uint8_t pd_pdo_index;
    uint16_t pd_mv;
    uint16_t pd_ma;
    uint16_t pd_mw;
    uint8_t pd_link;

    uint32_t fault_active;
    uint32_t fault_latched;
    bool inhibit;
    uint8_t page;
    uint32_t uptime_s;
} display_input_t;

void display_render(const display_input_t *in,
                    char lines[DISPLAY_LINES][DISPLAY_LINE_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_MODEL_H */
