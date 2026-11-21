/**
 * @file    pd_status.h
 * @brief   PD PCBA 协议状态只读管理
 *
 * IP5389 这类移动电源 PCBA 通常不向 MCU 开放 PD 协商结果。本模块默认
 * 后端为 NONE; 若模块厂商提供 I2C 状态块, 可将 APP_PD_STATUS_BACKEND
 * 设为 1 并按其寄存器映射修改 pd_status.c 的解码。
 *
 * 该模块绝不会写 PD 模块, 也不会改变 PDO/PPS。PD 协商仍由 PCBA 完成。
 */
#ifndef PD_STATUS_H
#define PD_STATUS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PD_LINK_NONE = 0,
    PD_LINK_SINK,
    PD_LINK_SOURCE,
    PD_LINK_UNKNOWN
} pd_link_t;

typedef enum {
    PD_PROTO_UNKNOWN = 0,
    PD_PROTO_PD2,
    PD_PROTO_PD3,
    PD_PROTO_PPS,
    PD_PROTO_QC2,
    PD_PROTO_QC3,
    PD_PROTO_AFC,
    PD_PROTO_FCP,
    PD_PROTO_SCP,
    PD_PROTO_APPLE,
    PD_PROTO_BC12
} pd_protocol_t;

typedef struct {
    bool supported;         /* 后端已配置 */
    bool valid;             /* 最近一次读取成功且未超时 */
    pd_link_t link;
    pd_protocol_t protocol;
    uint8_t pdo_index;
    uint16_t voltage_mv;
    uint16_t current_ma;
    uint16_t power_mw;
    uint32_t last_update_ms;
} pd_status_t;

typedef bool (*pd_read_block_fn)(void *ctx, uint8_t addr, uint8_t reg,
                                 uint8_t *buf, uint16_t len);

typedef struct {
    pd_read_block_fn read;
    void *ctx;
    uint8_t addr;
    uint8_t reg_base;
    bool enabled;
} pd_backend_t;

void pd_status_init(pd_status_t *st);

/**
 * @brief 轮询 PD 状态块。
 *
 * 状态块格式 (版本 1, 16 字节):
 *   [0..1] 'P','D'; [2] 版本=1; [3] flags
 *   bit0=attached, bit1=source, bit2=valid
 *   [4] protocol; [5] pdo_index; [6..7] mV LE; [8..9] mA LE;
 *   [10..11] mW LE; [12..13] reserved; [14] XOR checksum (0..13); [15] reserved
 *
 * @return true: 成功解析一帧; false: 后端未启用/读取失败/校验失败。
 */
bool pd_status_poll(pd_status_t *st, const pd_backend_t *backend, uint32_t now_ms);

/** 数据是否新鲜 (valid 且未超过 APP_PD_TIMEOUT_MS)。 */
bool pd_status_is_fresh(const pd_status_t *st, uint32_t now_ms);

const char *pd_protocol_name(pd_protocol_t protocol);
const char *pd_link_name(pd_link_t link);

#ifdef __cplusplus
}
#endif

#endif /* PD_STATUS_H */
