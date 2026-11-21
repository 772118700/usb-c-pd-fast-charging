/**
 * @file    app_tasks.c
 * @brief   FreeRTOS 应用层: 采集、故障处理、显示三个任务
 *
 * 任务划分与图片需求一致:
 *   - 采集任务: ADC/INA226/NTC/SOC/PD, 100 ms
 *   - 故障处理任务: 故障锁存、LED、可选 inhibit, 事件驱动 + 200 ms 兜底
 *   - 显示任务: SSD1306 三页轮显, 500 ms
 *
 * I2C1 由采集任务和显示任务共享, 使用互斥量保护; 单次 OLED 刷新约 23 ms
 * (400 kHz, 1 KB), 因此显示周期取 500 ms, 采集等待上限 40 ms。
 */
#include "app_tasks.h"

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app_config.h"
#include "app_state.h"
#include "balance.h"
#include "battery_math.h"
#include "board_port.h"
#include "bsp_adc.h"
#include "bsp_i2c.h"
#include "display_model.h"
#include "fault_diag.h"
#include "ina226.h"
#include "ntc.h"
#include "pd_status.h"
#include "soc.h"
#include "ssd1306.h"
#include "thermal.h"

typedef struct {
    ina226_t ina;
    soc_t soc;
    thermal_guard_t thermal;
    balance_monitor_t balance;
    fault_manager_t fault;
    pd_status_t pd;
    pd_backend_t pd_backend;
    ssd1306_t oled;
    app_state_t state;

    SemaphoreHandle_t i2c_mutex;
    TaskHandle_t fault_task;

    uint32_t last_sample_ms;
    uint32_t last_ina_retry_ms;
    uint32_t last_oled_retry_ms;
    uint32_t sample_count;
    uint32_t page_epoch_ms;

    bool ina_ready;
    bool oled_ready;
    bool pd_enabled;
    bool key_down;
    bool key_reset_armed;
    bool led_on;
    uint32_t key_since_ms;
    uint32_t last_led_ms;
    volatile bool reset_request;
} app_ctx_t;

static app_ctx_t g_app;

static const voltage_divider_t k_pack_div = {
    APP_ADC_VREF_MV, APP_ADC_FULL_SCALE,
    APP_PACK_DIVIDER_TOP_OHM, APP_PACK_DIVIDER_BOTTOM_OHM
};

static const voltage_divider_t k_cell_div = {
    APP_ADC_VREF_MV, APP_ADC_FULL_SCALE,
    APP_CELL_DIVIDER_TOP_OHM, APP_CELL_DIVIDER_BOTTOM_OHM
};

static const ntc_config_t k_ntc_cfg = {
    APP_NTC_SERIES_OHM, APP_NTC_R25_OHM, APP_NTC_BETA, APP_ADC_VREF_MV
};

/* -------------------------------------------------------------------------- */
/* I2C 适配器                                                                  */
/* -------------------------------------------------------------------------- */
static bool ina_read_adapter(void *ctx, uint8_t addr, uint8_t reg,
                             uint8_t *buf, uint16_t len)
{
    (void)ctx;
    return bsp_i2c_read_reg(addr, reg, buf, len);
}

static bool ina_write_adapter(void *ctx, uint8_t addr, uint8_t reg,
                              const uint8_t *buf, uint16_t len)
{
    (void)ctx;
    return bsp_i2c_write_reg(addr, reg, buf, len);
}

static bool oled_write_adapter(void *ctx, uint8_t addr,
                               const uint8_t *data, uint16_t len)
{
    (void)ctx;
    return bsp_i2c_write_raw(addr, data, len);
}

static bool pd_read_adapter(void *ctx, uint8_t addr, uint8_t reg,
                            uint8_t *buf, uint16_t len)
{
    (void)ctx;
    return bsp_i2c_read_reg(addr, reg, buf, len);
}

/* -------------------------------------------------------------------------- */
/* 初始化                                                                      */
/* -------------------------------------------------------------------------- */
static void read_initial_voltage(battery_voltage_t *v)
{
    uint16_t raw_pack = 0U;
    uint16_t raw_mid = 0U;

    memset(v, 0, sizeof(*v));

    if (!bsp_adc_read_raw(BSP_ADC_CH_PACK, &raw_pack) ||
        !bsp_adc_read_raw(BSP_ADC_CH_MID, &raw_mid)) {
        return;
    }
    (void)battery_calc_from_raw(raw_pack, raw_mid, &k_pack_div, &k_cell_div, v);
}

void app_init(void)
{
    board_init();
    (void)bsp_i2c_init();
    (void)bsp_adc_init();

    memset(&g_app, 0, sizeof(g_app));
    g_app.i2c_mutex = xSemaphoreCreateMutex();
    configASSERT(g_app.i2c_mutex != NULL);

    soc_init(&g_app.soc, APP_BATTERY_CAPACITY_MAH);
    thermal_init(&g_app.thermal, NULL);
    balance_init(&g_app.balance, APP_BALANCE_WARN_MV, APP_BALANCE_ALERT_MV,
                 APP_BALANCE_DEBOUNCE_MS);
    fault_init(&g_app.fault, NULL);
    pd_status_init(&g_app.pd);
    app_state_init(&g_app.state);

    g_app.ina_ready = ina226_init(&g_app.ina, ina_read_adapter, ina_write_adapter,
                                  NULL, APP_INA226_I2C_ADDR,
                                  APP_INA226_SHUNT_UOHM,
                                  APP_INA226_MAX_CURRENT_MA,
                                  APP_INA226_CURRENT_SIGN);
    if (g_app.ina_ready && APP_INA226_VERIFY_ID) {
        /* 兼容片可能没有正确 ID, 只作为诊断信息, 不阻断采样。 */
        (void)ina226_check_id(&g_app.ina);
    }

#if APP_PD_STATUS_BACKEND == 1
    g_app.pd_backend.read = pd_read_adapter;
    g_app.pd_backend.ctx = NULL;
    g_app.pd_backend.addr = APP_PD_I2C_ADDR;
    g_app.pd_backend.reg_base = APP_PD_REG_BASE;
    g_app.pd_backend.enabled = true;
    g_app.pd_enabled = true;
#else
    g_app.pd_backend.enabled = false;
    g_app.pd_enabled = false;
#endif

#if APP_OLED_ENABLED
    g_app.oled_ready = ssd1306_init(&g_app.oled, oled_write_adapter, NULL,
                                    APP_OLED_I2C_ADDR);
#endif

    /* 首次上电用静置单体电压给 SOC 播种; 若正在带载, 后续安时积分接管。 */
    battery_voltage_t v;
    read_initial_voltage(&v);
    if (v.cells_valid) {
        uint16_t avg = (uint16_t)(((uint32_t)v.cell_mv[0] + v.cell_mv[1]) / 2U);
        soc_seed_from_ocv(&g_app.soc, avg);
    }

    g_app.last_sample_ms = board_millis();
    g_app.last_ina_retry_ms = g_app.last_sample_ms;
    g_app.last_oled_retry_ms = g_app.last_sample_ms;
    g_app.page_epoch_ms = g_app.last_sample_ms;
}

/* -------------------------------------------------------------------------- */
/* 采集任务                                                                    */
/* -------------------------------------------------------------------------- */
static void acquire_adc(battery_voltage_t *v, ntc_reading_t ntc[2])
{
    uint16_t raw_pack = 0U;
    uint16_t raw_mid = 0U;
    uint16_t raw_ntc0 = 0U;
    uint16_t raw_ntc1 = 0U;

    memset(v, 0, sizeof(*v));
    memset(ntc, 0, 2U * sizeof(ntc[0]));

    if (bsp_adc_read_raw(BSP_ADC_CH_PACK, &raw_pack) &&
        bsp_adc_read_raw(BSP_ADC_CH_MID, &raw_mid)) {
        (void)battery_calc_from_raw(raw_pack, raw_mid, &k_pack_div, &k_cell_div, v);
    }
    if (bsp_adc_read_raw(BSP_ADC_CH_NTC_BATTERY, &raw_ntc0)) {
        (void)ntc_raw_to_temp(raw_ntc0, &k_ntc_cfg, &ntc[0]);
    }
    if (bsp_adc_read_raw(BSP_ADC_CH_NTC_PD, &raw_ntc1)) {
        (void)ntc_raw_to_temp(raw_ntc1, &k_ntc_cfg, &ntc[1]);
    }
}

static bool acquire_current(uint32_t now_ms, ina226_sample_t *sample)
{
    bool ok = false;
    sample->valid = false;
    sample->current_ma = 0;
    sample->bus_mv = 0U;
    sample->power_mw = 0;
    sample->shunt_uv = 0;

    if (xSemaphoreTake(g_app.i2c_mutex,
                       pdMS_TO_TICKS(APP_I2C_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return false;
    }

    if (!g_app.ina_ready &&
        (uint32_t)(now_ms - g_app.last_ina_retry_ms) >= 5000U) {
        g_app.last_ina_retry_ms = now_ms;
        g_app.ina_ready = ina226_init(&g_app.ina, ina_read_adapter,
                                      ina_write_adapter, NULL,
                                      APP_INA226_I2C_ADDR,
                                      APP_INA226_SHUNT_UOHM,
                                      APP_INA226_MAX_CURRENT_MA,
                                      APP_INA226_CURRENT_SIGN);
    }

    if (g_app.ina_ready) {
        ok = ina226_read(&g_app.ina, sample);
        if (!ok) {
            g_app.ina_ready = false;   /* 下个周期尝试重初始化 */
            g_app.last_ina_retry_ms = now_ms;
        }
    }

    xSemaphoreGive(g_app.i2c_mutex);
    return ok;
}

static void poll_pd(uint32_t now_ms)
{
    if (!g_app.pd_enabled) {
        return;
    }
    if (xSemaphoreTake(g_app.i2c_mutex,
                       pdMS_TO_TICKS(APP_I2C_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return;
    }
    (void)pd_status_poll(&g_app.pd, &g_app.pd_backend, now_ms);
    xSemaphoreGive(g_app.i2c_mutex);
}

static void acquisition_once(void)
{
    uint32_t now_ms = board_millis();
    uint32_t dt_ms = (uint32_t)(now_ms - g_app.last_sample_ms);
    if (dt_ms == 0U) {
        dt_ms = APP_ACQ_PERIOD_MS;
    } else if (dt_ms > 5000U) {
        dt_ms = 5000U;
    }
    g_app.last_sample_ms = now_ms;
    ++g_app.sample_count;

    battery_voltage_t v;
    ntc_reading_t ntc[2];
    acquire_adc(&v, ntc);

    ina226_sample_t is;
    bool current_ok = acquire_current(now_ms, &is);

    int16_t current_ma = current_ok ? (int16_t)is.current_ma : 0;
    uint16_t cell_avg = 0U;
    if (v.cells_valid) {
        cell_avg = (uint16_t)(((uint32_t)v.cell_mv[0] + v.cell_mv[1]) / 2U);
    }

    if (current_ok) {
        soc_update(&g_app.soc, current_ma, dt_ms, cell_avg, v.cells_valid);
    } else {
        g_app.soc.rest_ms = 0U;
        g_app.soc.ocv_corrected = false;
    }

    thermal_status_t thermal = thermal_eval(&g_app.thermal,
                                            ntc[APP_NTC_BATTERY].temp_c_x10,
                                            ntc[APP_NTC_BATTERY].valid,
                                            ntc[APP_NTC_PD].temp_c_x10,
                                            ntc[APP_NTC_PD].valid,
                                            current_ma);

    balance_state_t balance = balance_update(&g_app.balance,
                                             v.cell_mv[0], v.cell_mv[1],
                                             v.cells_valid, now_ms);

    if ((g_app.sample_count % APP_PD_POLL_DIV) == 0U) {
        poll_pd(now_ms);
    }
    bool pd_fresh = pd_status_is_fresh(&g_app.pd, now_ms);

    fault_input_t fi;
    memset(&fi, 0, sizeof(fi));
    fi.pack_mv = v.pack_mv;
    fi.cell_mv[0] = v.cell_mv[0];
    fi.cell_mv[1] = v.cell_mv[1];
    fi.voltage_valid = v.cells_valid;
    fi.current_ma = current_ma;
    fi.current_valid = current_ok;
    fi.thermal = thermal;
    fi.balance_delta_mv = g_app.balance.delta_mv;
    fi.balance_valid = (balance != BALANCE_INVALID);
    fi.pd_backend_enabled = g_app.pd_enabled;
    fi.pd_fresh = pd_fresh;
    fi.soc_valid = g_app.soc.initialized;
    fi.soc_pct = g_app.soc.soc_pct;

    fault_snapshot_t fault = fault_update(&g_app.fault, &fi, now_ms);
    if (g_app.reset_request) {
        g_app.reset_request = false;
        if (fault_reset(&g_app.fault)) {
            fault = fault_get_snapshot(&g_app.fault);
        }
    }

    system_state_t sys;
    memset(&sys, 0, sizeof(sys));
    sys.battery.timestamp_ms = now_ms;
    sys.battery.pack_mv = v.pack_mv;
    sys.battery.cell_mv[0] = v.cell_mv[0];
    sys.battery.cell_mv[1] = v.cell_mv[1];
    sys.battery.cell_delta_mv = v.cell_delta_mv;
    sys.battery.current_ma = current_ma;
    sys.battery.power_mw = current_ok ? is.power_mw : 0;
    sys.battery.temp_c_x10[APP_NTC_BATTERY] = ntc[APP_NTC_BATTERY].temp_c_x10;
    sys.battery.temp_c_x10[APP_NTC_PD] = ntc[APP_NTC_PD].temp_c_x10;
    sys.battery.soc_pct = g_app.soc.soc_pct;
    sys.battery.pack_valid = v.pack_valid;
    sys.battery.cells_valid = v.cells_valid;
    sys.battery.current_valid = current_ok;
    sys.battery.temp_valid[APP_NTC_BATTERY] = ntc[APP_NTC_BATTERY].valid;
    sys.battery.temp_valid[APP_NTC_PD] = ntc[APP_NTC_PD].valid;
    sys.battery.balance_state = (uint8_t)balance;
    sys.fault = fault;
    sys.pd = g_app.pd;
    sys.uptime_ms = now_ms;
    sys.sample_count = g_app.sample_count;
    sys.ina_ready = g_app.ina_ready;
    sys.oled_ready = g_app.oled_ready;
    sys.pd_backend_enabled = g_app.pd_enabled;
    app_state_publish(&g_app.state, &sys);

    if (g_app.fault_task != NULL) {
        (void)xTaskNotifyGive(g_app.fault_task);
    }
}

static void acquisition_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_ACQ_PERIOD_MS));
        acquisition_once();
    }
}

/* -------------------------------------------------------------------------- */
/* 故障处理任务                                                                */
/* -------------------------------------------------------------------------- */
static void fault_task(void *arg)
{
    (void)arg;
    uint32_t now = board_millis();
    g_app.last_led_ms = now;

    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(APP_FAULT_WAIT_MS));
        now = board_millis();

        system_state_t st;
        app_state_read(&g_app.state, &st);

        /* 可选硬件 inhibit; 无该引脚时保持关闭, 软件仍显示/记录故障。 */
#if APP_LOAD_INHIBIT_ENABLED
        board_set_pd_inhibit(st.fault.inhibit);
#else
        board_set_pd_inhibit(false);
#endif

        if (st.fault.inhibit) {
            board_set_fault_led(true);
            g_app.led_on = true;
        } else if (st.fault.active != 0U) {
            if ((uint32_t)(now - g_app.last_led_ms) >= 250U) {
                g_app.last_led_ms = now;
                g_app.led_on = !g_app.led_on;
                board_set_fault_led(g_app.led_on);
            }
        } else {
            board_set_fault_led(false);
            g_app.led_on = false;
        }

        /* 可选按键: 长按 2 s 请求清除已锁存故障。 */
        bool key = board_read_user_key();
        if (key && !g_app.key_down) {
            g_app.key_down = true;
            g_app.key_since_ms = now;
            g_app.key_reset_armed = true;
        } else if (!key) {
            g_app.key_down = false;
            g_app.key_reset_armed = true;
        } else if (g_app.key_down && g_app.key_reset_armed &&
                   (uint32_t)(now - g_app.key_since_ms) >= 2000U) {
            g_app.reset_request = true;
            g_app.key_reset_armed = false;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* 显示任务                                                                    */
/* -------------------------------------------------------------------------- */
static void display_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_DISPLAY_PERIOD_MS));

        system_state_t st;
        app_state_read(&g_app.state, &st);

        uint32_t now = board_millis();
        uint8_t page = (uint8_t)(((now - g_app.page_epoch_ms) / APP_PAGE_PERIOD_MS) %
                                 DISPLAY_PAGE_COUNT);

        display_input_t in;
        memset(&in, 0, sizeof(in));
        in.pack_mv = st.battery.pack_mv;
        in.current_ma = st.battery.current_ma;
        in.power_mw = st.battery.power_mw;
        in.soc_pct = st.battery.soc_pct;
        in.cell_delta_mv = st.battery.cell_delta_mv;
        in.temp_c_x10[0] = st.battery.temp_c_x10[0];
        in.temp_c_x10[1] = st.battery.temp_c_x10[1];
        in.temp_valid[0] = st.battery.temp_valid[0];
        in.temp_valid[1] = st.battery.temp_valid[1];
        in.balance_state = st.battery.balance_state;
        in.ina_ready = st.ina_ready;
        in.oled_ready = st.oled_ready;
        in.pd_supported = st.pd.supported;
        in.pd_valid = st.pd.valid && pd_status_is_fresh(&st.pd, now);
        in.pd_protocol = (uint8_t)st.pd.protocol;
        in.pd_pdo_index = st.pd.pdo_index;
        in.pd_mv = st.pd.voltage_mv;
        in.pd_ma = st.pd.current_ma;
        in.pd_mw = st.pd.power_mw;
        in.pd_link = (uint8_t)st.pd.link;
        in.fault_active = st.fault.active;
        in.fault_latched = st.fault.latched;
        in.inhibit = st.fault.inhibit;
        in.page = page;
        in.uptime_s = st.uptime_ms / 1000U;

        char lines[DISPLAY_LINES][DISPLAY_LINE_LEN];
        display_render(&in, lines);

#if APP_OLED_ENABLED
        if (!g_app.oled_ready &&
            (uint32_t)(now - g_app.last_oled_retry_ms) >= 10000U) {
            g_app.last_oled_retry_ms = now;
            if (xSemaphoreTake(g_app.i2c_mutex,
                               pdMS_TO_TICKS(APP_I2C_MUTEX_TIMEOUT_MS)) == pdTRUE) {
                g_app.oled_ready = ssd1306_init(&g_app.oled, oled_write_adapter,
                                                NULL, APP_OLED_I2C_ADDR);
                xSemaphoreGive(g_app.i2c_mutex);
            }
        }

        if (g_app.oled_ready &&
            xSemaphoreTake(g_app.i2c_mutex,
                           pdMS_TO_TICKS(APP_I2C_MUTEX_TIMEOUT_MS)) == pdTRUE) {
            ssd1306_clear(&g_app.oled);
            for (uint8_t i = 0U; i < DISPLAY_LINES; ++i) {
                ssd1306_draw_text(&g_app.oled, 0, (int16_t)(i * 16U), lines[i]);
            }
            bool ok = ssd1306_flush(&g_app.oled);
            xSemaphoreGive(g_app.i2c_mutex);
            if (!ok) {
                g_app.oled_ready = false;
                g_app.last_oled_retry_ms = now;
            }
        }
#endif
    }
}

void app_start(void)
{
    BaseType_t ok;
    ok = xTaskCreate(fault_task, "fault", APP_TASK_FAULT_STACK, NULL,
                     APP_TASK_PRIO_FAULT, &g_app.fault_task);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(acquisition_task, "acq", APP_TASK_ACQ_STACK, NULL,
                     APP_TASK_PRIO_ACQ, NULL);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(display_task, "disp", APP_TASK_DISPLAY_STACK, NULL,
                     APP_TASK_PRIO_DISPLAY, NULL);
    configASSERT(ok == pdPASS);
}

/* -------------------------------------------------------------------------- */
/* FreeRTOS 钩子                                                               */
/* -------------------------------------------------------------------------- */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
