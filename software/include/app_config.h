/**
 * @file    app_config.h
 * @brief   USB-C PD 智能快充系统 - 编译期配置
 *
 * 目标硬件:
 *   - STM32G071CBT6 (Cortex-M0+, 64 MHz, 128 KB Flash, 36 KB RAM)
 *   - 2S1P / 2S2P 18650 + 2S BMS
 *   - IP5389 双向 PD 移动电源 PCBA (黑盒, 只读状态可选)
 *   - INA226 (10 mOhm 分流器) + SSD1306 128x64 OLED
 *
 * 修改本文件即可适配 2S2P、不同分压电阻、不同 NTC 与保护门限。
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* -------------------------------------------------------------------------- */
/* 版本                                                                        */
/* -------------------------------------------------------------------------- */
#define APP_FW_VERSION              "1.0.0"

/* -------------------------------------------------------------------------- */
/* 电池组                                                                      */
/* -------------------------------------------------------------------------- */
#define APP_CELL_COUNT              2U
#define APP_BATTERY_SERIES          2U
#define APP_BATTERY_PARALLEL        1U      /* 2S2P 时改为 2 */
#define APP_CELL_CAPACITY_MAH       2500    /* 单节约 2500 mAh (25R) */
#define APP_BATTERY_CAPACITY_MAH    (APP_CELL_CAPACITY_MAH * (int32_t)APP_BATTERY_PARALLEL)

/* -------------------------------------------------------------------------- */
/* ADC / 分压网络                                                              */
/* -------------------------------------------------------------------------- */
#define APP_ADC_VREF_MV             3300U
#define APP_ADC_FULL_SCALE          4095U
#define APP_ADC_OVERSAMPLE          8U
#define APP_ADC_MIN_VALID_RAW       8U
#define APP_ADC_MAX_VALID_RAW       4087U

/* 电池组总压与单体中点均使用 100k/47k 分压 (8.4 V -> 2.686 V) */
#define APP_PACK_DIVIDER_TOP_OHM    100000U
#define APP_PACK_DIVIDER_BOTTOM_OHM 47000U
#define APP_CELL_DIVIDER_TOP_OHM    100000U
#define APP_CELL_DIVIDER_BOTTOM_OHM 47000U

/* NTC: 10 kOhm B3950, 10 kOhm 上拉到 3.3 V, NTC 接地 */
#define APP_NTC_SERIES_OHM          10000U
#define APP_NTC_R25_OHM             10000U
#define APP_NTC_BETA                3950U
#define APP_NTC_COUNT               2U
#define APP_NTC_BATTERY             0U
#define APP_NTC_PD                  1U

/* -------------------------------------------------------------------------- */
/* INA226                                                                      */
/* -------------------------------------------------------------------------- */
#define APP_INA226_I2C_ADDR         0x40U
#define APP_INA226_SHUNT_UOHM       10000U  /* 必须把 CJMCU-226 的 R100 换成 10 mOhm */
#define APP_INA226_MAX_CURRENT_MA   8192    /* 81.92 mV / 10 mOhm */
#define APP_INA226_CURRENT_SIGN     1       /* 若充电时读数为负, 改为 -1 */
#define APP_INA226_I2C_TIMEOUT_MS   20U
#define APP_INA226_VERIFY_ID        1       /* 量产可保留; 兼容片可改 0 */

/* -------------------------------------------------------------------------- */
/* OLED / I2C                                                                  */
/* -------------------------------------------------------------------------- */
#define APP_OLED_I2C_ADDR           0x3C
#define APP_OLED_ENABLED            1
#define APP_I2C_TIMEOUT_MS          20U
#define APP_I2C_MUTEX_TIMEOUT_MS    40U

/* -------------------------------------------------------------------------- */
/* PD 状态管理                                                                 */
/* 0 = 不读取 (IP5389 黑盒默认); 1 = 通过 I2C 读取厂商开放的状态块           */
/* 注意: 这是只读监视, 不控制 PD 协商, 也不写 IP5389 任何寄存器。             */
/* -------------------------------------------------------------------------- */
#define APP_PD_STATUS_BACKEND       0
#define APP_PD_I2C_ADDR             0x2E
#define APP_PD_REG_BASE             0x00U
#define APP_PD_TIMEOUT_MS           2000U
#define APP_PD_POLL_DIV             5U      /* 每 5 个采集周期轮询一次 (500 ms) */

/* -------------------------------------------------------------------------- */
/* SOC                                                                         */
/* -------------------------------------------------------------------------- */
#define APP_SOC_REST_CURRENT_MA     50
#define APP_SOC_REST_TIME_MS        (30UL * 60UL * 1000UL)
#define APP_SOC_OCV_CORRECTION_PCT  5U      /* 单次静置修正最多 5 个百分点 */

/* -------------------------------------------------------------------------- */
/* 温度保护 (单位: 0.1 degC)                                                   */
/* -------------------------------------------------------------------------- */
#define APP_CHG_TEMP_MIN_CX10           0
#define APP_CHG_TEMP_MIN_RECOVER_CX10   30
#define APP_CHG_TEMP_MAX_CX10           450
#define APP_CHG_TEMP_MAX_RECOVER_CX10   420
#define APP_CHG_TEMP_WARN_LOW_CX10      50
#define APP_CHG_TEMP_WARN_HIGH_CX10     400

#define APP_DIS_TEMP_MIN_CX10           (-200)
#define APP_DIS_TEMP_MIN_RECOVER_CX10   (-170)
#define APP_DIS_TEMP_MAX_CX10           600
#define APP_DIS_TEMP_MAX_RECOVER_CX10   570
#define APP_DIS_TEMP_WARN_LOW_CX10      (-150)
#define APP_DIS_TEMP_WARN_HIGH_CX10     520

#define APP_PD_TEMP_WARN_CX10           700
#define APP_PD_TEMP_TRIP_CX10           850
#define APP_PD_TEMP_RECOVER_CX10        800

/* -------------------------------------------------------------------------- */
/* 故障诊断门限                                                                */
/* -------------------------------------------------------------------------- */
#define APP_CELL_OV_TRIP_MV         4250U
#define APP_CELL_OV_CLEAR_MV        4150U
#define APP_PACK_OV_TRIP_MV         8500U
#define APP_PACK_OV_CLEAR_MV        8400U

#define APP_CELL_UV_TRIP_MV         2900U
#define APP_CELL_UV_CLEAR_MV        3000U
#define APP_PACK_UV_TRIP_MV         5800U
#define APP_PACK_UV_CLEAR_MV        6000U

#define APP_OC_CHG_TRIP_MA          5000
#define APP_OC_CHG_CLEAR_MA         4500
#define APP_OC_DIS_TRIP_MA          (-7000)
#define APP_OC_DIS_CLEAR_MA         (-6500)

#define APP_IMBALANCE_TRIP_MV       100U
#define APP_IMBALANCE_CLEAR_MV      80U
#define APP_SOC_LOW_PCT             5U

#define APP_DEBOUNCE_OV_MS          500U
#define APP_DEBOUNCE_UV_MS          1000U
#define APP_DEBOUNCE_OC_MS          200U
#define APP_DEBOUNCE_TEMP_MS        1000U
#define APP_DEBOUNCE_IMBALANCE_MS   3000U
#define APP_DEBOUNCE_SENSOR_MS      1000U
#define APP_DEBOUNCE_PD_MS          1000U
#define APP_DEBOUNCE_SOC_LOW_MS     5000U

/* 压差监测告警/严重门限 (只告警, 不主动均衡; 均衡由 2S BMS 完成) */
#define APP_BALANCE_WARN_MV         50U
#define APP_BALANCE_ALERT_MV        100U
#define APP_BALANCE_DEBOUNCE_MS     3000U

/* -------------------------------------------------------------------------- */
/* RTOS 任务                                                                   */
/* -------------------------------------------------------------------------- */
#define APP_ACQ_PERIOD_MS           100U
#define APP_DISPLAY_PERIOD_MS       500U
#define APP_PAGE_PERIOD_MS          3000U
#define APP_FAULT_WAIT_MS           200U

#define APP_TASK_ACQ_STACK          512U    /* words, 2 KB */
#define APP_TASK_FAULT_STACK        384U    /* words, 1.5 KB */
#define APP_TASK_DISPLAY_STACK      512U    /* words, 2 KB */

#define APP_TASK_PRIO_FAULT         5U
#define APP_TASK_PRIO_ACQ           4U
#define APP_TASK_PRIO_DISPLAY       2U

/* 可选: 硬件具备 PD 使能/负载开关时置 1, 故障时拉高 inhibit 引脚 */
#define APP_LOAD_INHIBIT_ENABLED    0

#endif /* APP_CONFIG_H */
