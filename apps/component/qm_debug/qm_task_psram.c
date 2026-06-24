#include "qm_task_psram.h"

#include "esp_heap_caps.h"
#include "freertos/idf_additions.h"

BaseType_t qm_task_create_psram(TaskFunction_t fn,
                                const char *name,
                                uint32_t stack_words,
                                void *arg,
                                UBaseType_t priority,
                                TaskHandle_t *out_handle)
{
#if CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
    const configSTACK_DEPTH_TYPE stack_bytes = stack_words * sizeof(StackType_t);
    return xTaskCreateWithCaps(fn, name, stack_bytes, arg, priority, out_handle, QM_TASK_STACK_SPIRAM_CAPS);
#else
    (void)QM_TASK_STACK_SPIRAM_CAPS;
    return xTaskCreate(fn, name, stack_words, arg, priority, out_handle);
#endif
}

BaseType_t qm_task_create_psram_to_core(TaskFunction_t fn,
                                        const char *name,
                                        uint32_t stack_words,
                                        void *arg,
                                        UBaseType_t priority,
                                        BaseType_t core_id,
                                        TaskHandle_t *out_handle)
{
#if CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
    const configSTACK_DEPTH_TYPE stack_bytes = stack_words * sizeof(StackType_t);
    return xTaskCreatePinnedToCoreWithCaps(fn, name, stack_bytes, arg, priority, out_handle, core_id,
                                           QM_TASK_STACK_SPIRAM_CAPS);
#else
    (void)QM_TASK_STACK_SPIRAM_CAPS;
    return xTaskCreatePinnedToCore(fn, name, stack_words, arg, priority, out_handle, core_id);
#endif
}

void qm_task_delete_psram(TaskHandle_t handle)
{
    if (handle == NULL) {
        return;
    }
#if CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
    vTaskDeleteWithCaps(handle);
#else
    vTaskDelete(handle);
#endif
}
