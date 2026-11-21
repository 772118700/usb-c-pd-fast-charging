/**
 * @file    FreeRTOS.h
 * @brief   Host syntax-check stub (NOT a real FreeRTOS kernel)
 */
#ifndef FREERTOS_H
#define FREERTOS_H

#include <stddef.h>
#include <stdint.h>

#include "FreeRTOSConfig.h"

typedef uint32_t TickType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#define pdFALSE 0
#define pdTRUE  1
#define pdPASS  1
#define pdFAIL  0
#define pdMS_TO_TICKS(x) ((TickType_t)(x))

#endif /* FREERTOS_H */
