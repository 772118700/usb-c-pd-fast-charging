/**
 * @file    soc.h
 * @brief   2S 锂电池 SOC: OCV 查表 + 安时积分 + 静置慢修正
 *
 * 约定: current_ma > 0 表示给电池充电, < 0 表示放电。
 * 电荷量内部使用 mA*ms 为单位, 避免浮点积分误差。
 */
#ifndef SOC_H
#define SOC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int64_t remaining_ma_ms;
    int64_t capacity_ma_ms;
    int32_t capacity_mah;
    uint8_t soc_pct;
    uint32_t rest_ms;
    bool ocv_corrected;
    bool initialized;
} soc_t;

/** 单节 OCV -> SOC 查表 (25 degC, 25R 近似)。 */
uint8_t soc_from_ocv_mv(uint16_t cell_mv);

/** 初始化, 未获得电压前默认 50%。 */
void soc_init(soc_t *soc, int32_t capacity_mah);

/** 用静置单体电压给初始 SOC 播种。 */
void soc_seed_from_ocv(soc_t *soc, uint16_t cell_mv);

/**
 * @brief 周期更新。
 * @param current_ma 带符号电流 (充电为正)
 * @param dt_ms      距上次更新毫秒数, 内部限制在 1..5000
 * @param cell_mv    单体平均电压 (用于静置修正)
 * @param cell_valid 电压是否有效
 */
void soc_update(soc_t *soc,
                int16_t current_ma,
                uint32_t dt_ms,
                uint16_t cell_mv,
                bool cell_valid);

/** 直接设置 SOC (产线校准/调试)。 */
void soc_set_percent(soc_t *soc, uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif /* SOC_H */
