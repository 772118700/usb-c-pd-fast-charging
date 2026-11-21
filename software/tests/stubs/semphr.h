/**
 * @file    semphr.h
 * @brief   Host syntax-check stub (NOT a real FreeRTOS kernel)
 */
#ifndef SEMPHR_H
#define SEMPHR_H

#include "FreeRTOS.h"

typedef void *SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout);
BaseType_t xSemaphoreGive(SemaphoreHandle_t sem);

#endif /* SEMPHR_H */
