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

int qm_task_new_psram(qm_task_t *task, const char *name, void (*fn)(void *), void *arg,
                      int stack_size, int prio)
{
    return qm_task_new_psram_to_core(task, name, fn, arg, stack_size, prio, tskNO_AFFINITY);
}

int qm_task_new_psram_to_core(qm_task_t *task, const char *name, void (*fn)(void *), void *arg,
                              int stack_size, int prio, int core_id)
{
    TaskHandle_t handle = NULL;
    const uint32_t stack_words = qm_task_stack_bytes_to_words((uint32_t)stack_size);
    BaseType_t ret;

    if (task == NULL) {
        return -1;
    }

#if CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
    const configSTACK_DEPTH_TYPE stack_bytes = stack_words * sizeof(StackType_t);
    if (core_id == tskNO_AFFINITY) {
        ret = xTaskCreateWithCaps(fn, name, stack_bytes, arg, prio, &handle, QM_TASK_STACK_SPIRAM_CAPS);
    } else {
        ret = xTaskCreatePinnedToCoreWithCaps(fn, name, stack_bytes, arg, prio, &handle, core_id,
                                            QM_TASK_STACK_SPIRAM_CAPS);
    }
#else
    if (core_id == tskNO_AFFINITY) {
        ret = xTaskCreate(fn, name, stack_words, arg, prio, &handle);
    } else {
        ret = xTaskCreatePinnedToCore(fn, name, stack_words, arg, prio, &handle, core_id);
    }
#endif

    if (ret != pdPASS) {
        task->hdl = NULL;
        return -1;
    }

    task->hdl = handle;
    return 0;
}

int qm_task_exit_psram(qm_task_t *task)
{
    if (task == NULL || task->hdl == NULL) {
#if CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
        vTaskDeleteWithCaps(NULL);
#else
        vTaskDelete(NULL);
#endif
        return 0;
    }

#if CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
    vTaskDeleteWithCaps(task->hdl);
#else
    vTaskDelete(task->hdl);
#endif
    task->hdl = NULL;
    return 0;
}
