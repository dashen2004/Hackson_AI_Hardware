
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "audio_error.h"
#include "audio_mem.h"
#include "audio_thread.h"

#include "ringbuf.h"

#include "esp_afe_sr_models.h"
#include "model_path.h"

#include "qm_audio_vc.h"
#include "qm_audio_vc_iface.h"
#include "qm_audio_subproc_iface.h"

#define FEED_TASK_DESTROY  (BIT(0))
#define FETCH_TASK_DESTROY (BIT(1))
#define FETCH_TASK_RUNNING (BIT(2))

static const char *TAG = "AUDIO_VC";

typedef struct __recorder_vc {
    int                   fetch_task_core;
    int                   fetch_task_prio;
    int                   fetch_task_stack;
    ringbuf_handle_t      out_rb;
    int                   rb_size;
    EventGroupHandle_t    events;
    bool                  feed_running;
    bool                  fetch_running;
    qm_audio_data_read_t  read;
    void                  *read_ctx;
    esp_afe_sr_data_t     *afe_data;
    esp_afe_sr_iface_t    *afe_handle;
    qm_audio_vc_monitor_t afe_monitor;
    void                  *afe_monitor_ctx;
    bool                  vad_enable;
    char                  *partition_label;
    bool                  aec_enable;
} recorder_vc_t;

static qm_err_t recorder_sr_output(recorder_vc_t *recorder_sr, void *buffer, int len);

static qm_audio_vc_result_t *recorder_sr_afe_result_convert(
                                                    recorder_vc_t *recorder_sr,
                                                    afe_fetch_result_t *afe_result,
                                                    qm_audio_vc_result_t *sr_result)
{
    sr_result->type = VC_RESULT_UNKNOW;

    if (recorder_sr->vad_enable) {
        if (afe_result->vad_state == VAD_SILENCE) {
            sr_result->type = VC_RESULT_NOISE;
        } else if (afe_result->vad_state == VAD_SPEECH) {
            sr_result->type = VC_RESULT_SPEECH;
        } else {
            ESP_LOGE(TAG, "vad state error");
        }
    } else {
        sr_result->type = VC_RESULT_SPEECH;
    }

    return sr_result;
}

static void fetch_task(void *parameters)
{
    recorder_vc_t *recorder_sr = (recorder_vc_t *)parameters;
    recorder_sr->fetch_running = true;

    while (recorder_sr->fetch_running) {
        xEventGroupWaitBits(recorder_sr->events, FETCH_TASK_RUNNING, false, true, portMAX_DELAY);

        afe_fetch_result_t *afe_result = recorder_sr->afe_handle->fetch(recorder_sr->afe_data);
        if (recorder_sr->afe_monitor) {
            qm_audio_vc_result_t sr_result = {0};
            recorder_sr->afe_monitor(
                    recorder_sr_afe_result_convert(recorder_sr, afe_result, &sr_result),
                    recorder_sr->afe_monitor_ctx);
        }
        recorder_sr_output(recorder_sr, afe_result->data, afe_result->data_size);
    }
    xEventGroupClearBits(recorder_sr->events, FETCH_TASK_RUNNING);
    xEventGroupSetBits(recorder_sr->events, FETCH_TASK_DESTROY);
    vTaskDelete(NULL);
}

static qm_err_t recorder_sr_output(recorder_vc_t *recorder_sr, void *buffer, int len)
{
    if (rb_bytes_available(recorder_sr->out_rb) < len) {
        rb_read(recorder_sr->out_rb, NULL, len, 0);
    }
    int ret = rb_write(recorder_sr->out_rb, buffer, len, 0);
    return ret == len ? ESP_OK : ESP_FAIL;
}

static int recorder_sr_fetch(void *handle, void *buf, int len, TickType_t ticks)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;
    return rb_read(recorder_sr->out_rb, buf, len, ticks);
}

static qm_err_t recorder_sr_suspend(void *handle, bool_t suspend)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;

    if (suspend) {
        xEventGroupClearBits(recorder_sr->events, FETCH_TASK_RUNNING);
        if (recorder_sr->out_rb) {
            rb_done_write(recorder_sr->out_rb);
        }
    } else {
        if (recorder_sr->out_rb) {
            rb_reset(recorder_sr->out_rb);
        }
        xEventGroupSetBits(recorder_sr->events, FETCH_TASK_RUNNING);
    }
    return ESP_OK;
}

static qm_err_t recorder_sr_enable(void *handle, bool_t enable)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;
    qm_err_t ret = ESP_OK;
    if (enable) {
        if (!recorder_sr->fetch_running && recorder_sr->fetch_task_stack > 0) {
            audio_thread_t fetch_thread = NULL;
            ret |= audio_thread_create(&fetch_thread, "vc_fetch",
                fetch_task,
                (void *)recorder_sr,
                recorder_sr->fetch_task_stack,
                recorder_sr->fetch_task_prio,
                true,
                recorder_sr->fetch_task_core);
        }
   
        recorder_sr_suspend(handle, false);

        if (recorder_sr->out_rb) {
            rb_reset(recorder_sr->out_rb);
        }
    } else {
        recorder_sr_suspend(handle, false);

        if (recorder_sr->fetch_running) {
            recorder_sr->fetch_running = false;
            EventBits_t bits = xEventGroupWaitBits(recorder_sr->events, FETCH_TASK_DESTROY, true, true, portMAX_DELAY);
            if (bits & FETCH_TASK_DESTROY) {
                ESP_LOGI(TAG, "Fetch task destroyed!");
            }
        }

        if (recorder_sr->out_rb) {
            rb_done_write(recorder_sr->out_rb);
        }
    }
    return ret == ESP_OK ? ESP_OK : ESP_FAIL;
}

static qm_err_t recorder_sr_get_state(void *handle, void *state)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;
    qm_audio_vc_state_t *sr_state = (qm_audio_vc_state_t *)state;
    EventBits_t bits = xEventGroupWaitBits(recorder_sr->events, FETCH_TASK_RUNNING, false, true, 0);
    sr_state->vad_enable = recorder_sr->vad_enable;
    if (!recorder_sr->fetch_running) {
        sr_state->afe_state = 0;
    } else if ((bits & (FETCH_TASK_RUNNING)) == (FETCH_TASK_RUNNING)) {
        sr_state->afe_state = 1;
    } else {
        sr_state->afe_state = 2;
    }
    return ESP_OK;
}

static qm_err_t recorder_sr_set_read_cb(void *handle, qm_audio_data_read_t read_cb, void *user_ctx)
{
    AUDIO_CHECK(TAG, handle, return ESP_FAIL, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;

    recorder_sr->read = read_cb;
    recorder_sr->read_ctx = user_ctx;

    return ESP_OK;
}

static qm_err_t recorder_sr_set_afe_monitor(void *handle, qm_audio_vc_monitor_t monitor, void *user_ctx)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;

    recorder_sr->afe_monitor = monitor;
    recorder_sr->afe_monitor_ctx = user_ctx;

    return ESP_OK;
}

static int recorder_sr_feed(void *handle, void *buf, int len)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;
    return recorder_sr->afe_handle->feed(recorder_sr->afe_data, buf);
}

static int recorder_sr_get_feed_channel_num(void *handle)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;
    return recorder_sr->afe_handle->get_feed_channel_num(recorder_sr->afe_data);
}

static int recorder_sr_get_feed_chunksize(void *handle)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;
    return recorder_sr->afe_handle->get_feed_chunksize(recorder_sr->afe_data);
}


static qm_audio_vc_iface_t recorder_sr_iface = {
    .base.enable = recorder_sr_enable,
    .base.get_state = recorder_sr_get_state,
    .base.set_read_cb = recorder_sr_set_read_cb,
    .base.feed = recorder_sr_feed,
    .base.fetch = recorder_sr_fetch,
    .afe_suspend = recorder_sr_suspend,
    .set_afe_monitor = recorder_sr_set_afe_monitor,
    .get_feed_channel_num = recorder_sr_get_feed_channel_num,
    .get_feed_chunksize = recorder_sr_get_feed_chunksize,
};

static void recorder_sr_clear(void *handle)
{
    AUDIO_CHECK(TAG, handle, return, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;

    if (recorder_sr->afe_data) {
        recorder_sr->afe_handle->destroy(recorder_sr->afe_data);
        recorder_sr->afe_data = NULL;
    }

    if (recorder_sr->out_rb) {
        rb_reset(recorder_sr->out_rb);
        rb_destroy(recorder_sr->out_rb);
    }
    if (recorder_sr->events) {
        vEventGroupDelete(recorder_sr->events);
        recorder_sr->events = NULL;
    }
    if (recorder_sr) {
        audio_free(recorder_sr);
    }
}

qm_audio_vc_handle_t qm_audio_vc_create(qm_audio_vc_cfg_t *cfg, qm_audio_vc_iface_t **iface)
{
    AUDIO_NULL_CHECK(TAG, cfg, return NULL);

    recorder_vc_t *recorder_sr = audio_calloc(1, sizeof(recorder_vc_t));
    AUDIO_NULL_CHECK(TAG, recorder_sr, return NULL);
    recorder_sr->fetch_task_core  = cfg->fetch_task_core;
    recorder_sr->fetch_task_prio  = cfg->fetch_task_prio;
    recorder_sr->fetch_task_stack = cfg->fetch_task_stack;
    recorder_sr->rb_size          = cfg->rb_size;
    recorder_sr->partition_label  = cfg->partition_label;
    recorder_sr->aec_enable       = ((afe_config_t *)cfg->afe_cfg)->aec_init;


    afe_config_print(((afe_config_t *)cfg->afe_cfg));
    afe_config_check(((afe_config_t *)cfg->afe_cfg));

    recorder_sr->afe_handle = esp_afe_handle_from_config(((afe_config_t *)cfg->afe_cfg));
    recorder_sr->afe_data = recorder_sr->afe_handle->create_from_config(((afe_config_t *)cfg->afe_cfg));
    afe_config_free(((afe_config_t *)cfg->afe_cfg));

    AUDIO_NULL_CHECK(TAG, recorder_sr->afe_data, goto _failed);

    if (((afe_config_t *)cfg->afe_cfg)->vad_init) {
        recorder_sr->vad_enable = true;
    }

    recorder_sr->events = xEventGroupCreate();
    AUDIO_NULL_CHECK(TAG, recorder_sr->events, goto _failed);
    recorder_sr->out_rb = rb_create(recorder_sr->rb_size, 1);
    AUDIO_NULL_CHECK(TAG, recorder_sr->out_rb, goto _failed);

    *iface = &recorder_sr_iface;

    return recorder_sr;

_failed:
    recorder_sr_clear(recorder_sr);
    return NULL;
}

qm_err_t qm_audio_vc_destroy(qm_audio_vc_handle_t handle)
{
    AUDIO_CHECK(TAG, handle, return ESP_FAIL, "Handle is NULL");
    recorder_vc_t *recorder_sr = (recorder_vc_t *)handle;

    recorder_sr_enable(recorder_sr, false);
    recorder_sr_clear(recorder_sr);
    return ESP_OK;
}
