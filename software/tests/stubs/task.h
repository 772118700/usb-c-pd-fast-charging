/**
 * @file    task.h
 * @brief   Host syntax-check stub (NOT a real FreeRTOS kernel)
 */
#ifndef TASK_H
#define TASK_H

#include "FreeRTOS.h"

typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);

BaseType_t xTaskCreate(TaskFunction_t task, const char *name, uint16_t stack,
                       void *arg, UBaseType_t prio, TaskHandle_t *handle);
void vTaskDelayUntil(TickType_t *last_wake, TickType_t period);
TickType_t xTaskGetTickCount(void);
uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, TickType_t timeout);
BaseType_t xTaskNotifyGive(TaskHandle_t task);
void vTaskStartScheduler(void);

#define taskENTER_CRITICAL()     ((void)0)
#define taskEXIT_CRITICAL()      ((void)0)
#define taskDISABLE_INTERRUPTS() ((void)0)

#endif /* TASK_H */
