/**
 * @file    ina226.h
 * @brief   INA226 双向电压/电流/功率监测驱动 (I2C, 与 HAL 解耦)
 */
#ifndef INA226_H
#define INA226_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 寄存器 */
#define INA226_REG_CONFIG       0x00U
#define INA226_REG_SHUNT        0x01U
#define INA226_REG_BUS          0x02U
#define INA226_REG_POWER        0x03U
#define INA226_REG_CURRENT      0x04U
#define INA226_REG_CALIBRATION  0x05U
#define INA226_REG_MASK         0x06U
#define INA226_REG_ALERT        0x07U
#define INA226_REG_MANUF_ID     0xFEU
#define INA226_REG_DIE_ID       0xFFU

#define INA226_MANUF_ID         0x5449U
#define INA226_DIE_ID           0x2260U

/* 0x4527: AVG=16, VBUSCT=1.1ms, VSHCT=1.1ms, 连续测量分流+总线 */
#define INA226_CONFIG_VALUE     0x4527U

typedef bool (*ina226_read_fn)(void *ctx, uint8_t addr, uint8_t reg,
                               uint8_t *buf, uint16_t len);
typedef bool (*ina226_write_fn)(void *ctx, uint8_t addr, uint8_t reg,
                                const uint8_t *buf, uint16_t len);

typedef struct {
    ina226_read_fn read;
    ina226_write_fn write;
    void *ctx;
    uint8_t addr;
    uint32_t shunt_uohm;
    uint32_t current_lsb_ua;
    uint16_t calibration;
    int8_t current_sign;    /* +1: 充电为正; -1: 反接时取反 */
    bool initialized;
} ina226_t;

typedef struct {
    uint32_t bus_mv;
    int32_t current_ma;     /* + 充电, - 放电 */
    int32_t power_mw;       /* 电池侧功率, 充电为正 */
    int32_t shunt_uv;
    bool valid;
} ina226_sample_t;

/**
 * @brief 初始化 INA226: 校验参数、写配置寄存器、写校准寄存器。
 * @param max_current_ma 期望量程 (<= 32767 * current_lsb)
 * @return false: 参数非法、校准值溢出或 I2C 写失败。
 */
bool ina226_init(ina226_t *dev,
                 ina226_read_fn read,
                 ina226_write_fn write,
                 void *ctx,
                 uint8_t addr,
                 uint32_t shunt_uohm,
                 uint32_t max_current_ma,
                 int8_t current_sign);

/**
 * @brief 可选: 读取厂商 ID / Die ID, 用于确认焊接与供电。
 */
bool ina226_check_id(ina226_t *dev);

/**
 * @brief 一次读取分流、总线、功率、电流寄存器并换算。
 *
 * 数据手册要求校准后等待至少一个转换周期 (16 次平均约 35 ms)。
 */
bool ina226_read(ina226_t *dev, ina226_sample_t *out);

/**
 * @brief 仅读取总线电压, 用于快速检查。
 */
bool ina226_read_bus_mv(ina226_t *dev, uint32_t *bus_mv);

#ifdef __cplusplus
}
#endif

#endif /* INA226_H */
