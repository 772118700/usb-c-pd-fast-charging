/**
 * @file    app_tasks.h
 * @brief   应用初始化与 FreeRTOS 任务启动
 */
#ifndef APP_TASKS_H
#define APP_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

/** 完成外设、算法与 IPC 初始化; 在 CubeMX 的 MX_*_Init() 之后调用。 */
void app_init(void);

/** 创建采集/故障/显示任务; 随后由调用者启动调度器。 */
void app_start(void);

/** 应用入口: app_init + app_start + vTaskStartScheduler, 不返回。 */
void app_main(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H */
