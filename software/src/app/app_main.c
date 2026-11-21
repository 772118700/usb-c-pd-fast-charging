/**
 * @file    app_main.c
 * @brief   应用入口: 在 CubeMX 初始化之后调用 app_main()
 *
 * 典型 main.c:
 *   HAL_Init();
 *   SystemClock_Config();
 *   MX_GPIO_Init();
 *   MX_DMA_Init();
 *   MX_ADC1_Init();
 *   MX_I2C1_Init();
 *   MX_USART2_UART_Init();
 *   app_main();          // 不返回
 */
#include "FreeRTOS.h"
#include "task.h"

#include "app_tasks.h"

void app_main(void)
{
    app_init();
    app_start();

    vTaskStartScheduler();

    /* 堆不足时调度器不会启动; 这里停住便于调试器定位。 */
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
