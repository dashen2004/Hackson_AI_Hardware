#pragma once

#include <stdint.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QM_TASK_STACK_SPIRAM_CAPS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

/**
 * Create a task with stack in PSRAM when CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM.
 * Requires CONFIG_SPIRAM_XIP_FROM_PSRAM (code in PSRAM) if the task uses SPIFFS/NVS.
 * @p stack_words Same unit as xTaskCreate (FreeRTOS stack depth in words).
 */
BaseType_t qm_task_create_psram(TaskFunction_t fn,
                                const char *name,
                                uint32_t stack_words,
                                void *arg,
                                UBaseType_t priority,
                                TaskHandle_t *out_handle);

void qm_task_delete_psram(TaskHandle_t handle);

#ifdef __cplusplus
}
#endif
