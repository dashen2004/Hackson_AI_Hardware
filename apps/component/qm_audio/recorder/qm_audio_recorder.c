/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_err.h"

#include "audio_error.h"
#include "audio_mem.h"
#include "audio_sys.h"
#include "audio_thread.h"

#include "qm.h"

#include "qm_audio_recorder.h"
#include "qm_audio_sr.h"
#include "qm_audio_vc.h"
#include "qm_audio_encoder.h"
#include "qm_audio_sr_iface.h"
#include "qm_audio_vc_iface.h"
#include "qm_audio_subproc_iface.h"
#include "qm_audio_board_def.h"

#define DEFAULT_READ_BUFFER_SIZE (2048)
#define DEFAULT_CMD_Q_LEN        (3)

#define RECORDER_SR_FLAG         (BIT1)
#define RECORDER_DESTROYED       (BIT0)


typedef enum __audio_recorder_state {
    RECORDER_ST_IDLE,
    RECORDER_ST_WAKEUP,
    RECORDER_ST_WAIT_FOR_SPEECH,
    RECORDER_ST_SPEECHING,
    RECORDER_ST_WAIT_FOR_SILENCE,
    RECORDER_ST_WAIT_FOR_SLEEP,
} audio_recorder_state_t;

typedef enum __audio_recorder_event {
    RECORDER_EVENT_NOISE_DECT,
    RECORDER_EVENT_SPEECH_DECT,
    RECORDER_EVENT_WWE_DECT,
    RECORDER_EVENT_WAKEUP_TIMER_EXPIRED,
    RECORDER_EVENT_VAD_TIMER_EXPIRED
} audio_recorder_event_t;

typedef struct __recorder_data_pack {
    uint8_t *data; /*!< Data pointer*/
    int      len;  /*!< Data length */
} recorder_data_pack_t;

typedef struct __recorder_message {
    enum {
        RECORDER_CMD_UPDATE_STATE,
        RECORDER_CMD_TRIGGER_START,
        RECORDER_CMD_TRIGGER_STOP,
        RECORDER_CMD_VAD_CTRL,
        RECORDER_CMD_WWE_CTRL,
        RECORDER_CMD_MN_CTRL,
        RECORDER_CMD_MN_DECT,
        RECORDER_CMD_DESTROY,
    } id;
    void *data;
    int  data_len;
} recorder_msg_t;

/**
 * @brief Container of recorder
 */
typedef struct __audio_recorder {
    rec_event_cb_t            event_cb;
    void                      *user_data;
    qm_audio_data_read_t      read;
    
    void                      *vc_handle;      /*!< SR handle */
    qm_audio_vc_iface_t       *vc_iface;       /*!< SR interface */

    void                      *sr_handle;
    qm_audio_sr_iface_t       *sr_iface;

    void                      *encoder_handle;
    qm_audio_encoder_iface_t  *encoder_iface;

    audio_thread_t            task_handle;
    audio_thread_t            feed_handle;

    int                       wakeup_time;
    esp_timer_handle_t        wakeup_timer;

    EventGroupHandle_t        sync_evt;
    QueueHandle_t             cmd_queue;
    bool                      vad_check;

    audio_recorder_state_t    state;
    qm_audio_sr_wakeup_result_t wakeup_result;

} audio_recorder_t;

// #define DEBUG_FLAG 1

#if DEBUG_FLAG

#include "qm_utils_timer.h"

static int play_size = 0;
static int16_t *play_buff = NULL;
static qm_utils_time_t debug_timer = {0};
extern void qm_audio_ws_data_cb(void *data, int len, void *user_data);
#endif

static void audio_recorder_reset(audio_recorder_t *recorder);

static const char *TAG = "AUDIO_RECORDER";

static esp_err_t audio_recorder_send_msg(audio_recorder_t *recorder, int msg_id, void *data, int len)
{
    AUDIO_NULL_CHECK(TAG, recorder, return ESP_FAIL);

    recorder_msg_t msg = { 0 };
    msg.id = msg_id;
    msg.data_len = len;
    if (msg.data_len && data) {
        msg.data = audio_calloc(1, msg.data_len);
        AUDIO_NULL_CHECK(TAG, msg.data, return ESP_ERR_NO_MEM);
        memcpy(msg.data, data, msg.data_len);
    } else {
        msg.data = data;
    }
    if (xQueueSend(recorder->cmd_queue, &msg, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Recorder event send failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t audio_recorder_notify_events(audio_recorder_t *recorder, int event)
{
    AUDIO_NULL_CHECK(TAG, recorder, return ESP_FAIL);

    return audio_recorder_send_msg(recorder, RECORDER_CMD_UPDATE_STATE, (void *)event, 0);
}

static inline esp_err_t audio_recorder_update_state_2_user(audio_recorder_t *recorder, int type, void *event_data, size_t len)
{
    AUDIO_NULL_CHECK(TAG, recorder, return ESP_FAIL);
    audio_rec_evt_t event = {
        .type = type,
        .event_data = event_data,
        .data_len = len,
    };

    return recorder->event_cb(&event, recorder->user_data);
}

static inline void audio_recorder_set_state(audio_recorder_t *recorder, int state)
{
    recorder->state = state;
}

static esp_err_t audio_recorder_wakeup_timer_start(audio_recorder_t *recorder)
{
    int interval = recorder->wakeup_time;

    esp_timer_stop(recorder->wakeup_timer);
    if (interval > 0) {
        return esp_timer_start_once(recorder->wakeup_timer, interval * 1000);
    } else {
        return ESP_FAIL;
    }
}

static qm_err_t audio_recorder_afe_monitor(qm_audio_sr_result_t *result, void *user_ctx)
{
    AUDIO_NULL_CHECK(TAG, user_ctx, return ESP_FAIL);
    audio_recorder_t *recorder = (audio_recorder_t *)user_ctx;
    
    switch (result->type) {
        case SR_RESULT_VERIFIED:
            break;
        case SR_RESULT_WAKEUP:
            xEventGroupClearBits(recorder->sync_evt, RECORDER_SR_FLAG);
            audio_recorder_update_state_2_user(recorder, AUDIO_REC_WAKEUP_START, &result->info.wakeup_info, sizeof(qm_audio_sr_wakeup_result_t));
            break;
        case SR_RESULT_COMMAND:
            break;
        default:
            break;
    }
    return ESP_OK;
}

static qm_err_t audio_recorder_vc_afe_monitor(qm_audio_vc_result_t *result, void *user_ctx)
{
    AUDIO_NULL_CHECK(TAG, user_ctx, return ESP_FAIL);
    audio_recorder_t *recorder = (audio_recorder_t *)user_ctx;

    static int type = VC_RESULT_NOISE;

    if(result->type != type){

        switch (result->type) {
            case VC_RESULT_NOISE:
                audio_recorder_update_state_2_user(recorder, AUDIO_REC_VAD_END, NULL, 0);
                break;
            case VC_RESULT_SPEECH:
                audio_recorder_update_state_2_user(recorder, AUDIO_REC_VAD_START, NULL, 0);
                break;
            default:
                break;
        }

        type = result->type;
    }

    return ESP_OK;
}

static qm_err_t audio_recorder_mn_monitor(qm_audio_sr_result_t *result, void *user_ctx)
{
    int command_num = 0;
    qm_audio_sr_cmd_table_t *command_str = NULL;
    audio_recorder_t *recorder_sr = (audio_recorder_t *)user_ctx;
    if (result->type >= 0) {    

#if AUDIO_DIY_WAKENET
    qm_audio_sr_get_speech_cmd(recorder_sr->sr_handle, &command_str, &command_num);
    QM_LOGD("1", "command_str:%p,command_num:%d", command_str, command_num);
    for (int i = 0; i < command_num; i++)
    {
        if(command_str[i].id == result->type){
            QM_LOGD("1", "type:%d,cmd:%s,prob:%f", command_str[i].type, result->info.mn_info.str, result->info.mn_info.prob);
            if(result->info.mn_info.prob < command_str[i].prob){
                QM_LOGW("1","except prob:%f, now prob:%f", result->info.mn_info.prob, command_str[i].prob);
                break;
            }

            switch (command_str[i].type)
            {
                case QM_AUDIO_SR_WAKEUP_CMD:
                    xEventGroupClearBits(recorder_sr->sync_evt, RECORDER_SR_FLAG);
                    audio_recorder_update_state_2_user((audio_recorder_t *)user_ctx, AUDIO_REC_WAKEUP_START, &result->info.wakeup_info, sizeof(qm_audio_sr_wakeup_result_t));
                    break;
                
                case QM_AUDIO_SR_COMM_CMD:
                    audio_recorder_update_state_2_user((audio_recorder_t *)user_ctx, result->type, &result->info.mn_info, sizeof(qm_audio_sr_mn_result_t));
                    break;

                default:
                    break;
            }
            break;
        }
    }
    return QM_EOK;
#else
        return audio_recorder_update_state_2_user((audio_recorder_t *)user_ctx, result->type, &result->info.mn_info, sizeof(qm_audio_sr_mn_result_t));

#endif
    }

    return ESP_FAIL;
}

static esp_err_t audio_recorder_encoder_enable(audio_recorder_t *recorder, bool enable)
{
    if (recorder->encoder_handle) {
        return recorder->encoder_iface->base.enable(recorder->encoder_handle, enable);
    }
    return ESP_OK;
}


static void audio_recorder_reset(audio_recorder_t *recorder)
{
    esp_timer_stop(recorder->wakeup_timer);
    audio_recorder_encoder_enable(recorder, false);
    recorder->state = RECORDER_ST_IDLE;
}

static int get_feed_max_size(audio_recorder_t *recorder_sr)
{
    int vc_size = 0;
    int sr_size = 0;
    int feed_size = 0;

    if(recorder_sr->sr_handle){
        sr_size = AUDIO_ADC_INPUT_CH_NUM * recorder_sr->sr_iface->get_feed_chunksize(recorder_sr->sr_handle);
    }

    if(recorder_sr->vc_handle){
        vc_size = AUDIO_ADC_INPUT_CH_NUM * recorder_sr->vc_iface->get_feed_chunksize(recorder_sr->vc_handle);
    }

    if(vc_size > sr_size){
        feed_size = vc_size;
    }else{
        feed_size = sr_size;

    }

    return feed_size;
}

static void feed_task(void *parameters)
{
    audio_recorder_t *recorder_sr = (audio_recorder_t *)parameters;

    int buf_size = get_feed_max_size(recorder_sr) * sizeof(int16_t);

    int16_t *i_buf = audio_calloc(1, buf_size);
    assert(i_buf);

    int fill_cnt = 0;

    while (1) {
      
        EventBits_t bits = xEventGroupWaitBits(recorder_sr->sync_evt, RECORDER_SR_FLAG, false, true, 0);
        if (bits & RECORDER_SR_FLAG) {
            if(recorder_sr->sr_handle){
                buf_size = sizeof(int16_t) * AUDIO_ADC_INPUT_CH_NUM * recorder_sr->sr_iface->get_feed_chunksize(recorder_sr->sr_handle);
            }
        }else{
            if(recorder_sr->vc_handle){
                buf_size = sizeof(int16_t) * AUDIO_ADC_INPUT_CH_NUM * recorder_sr->vc_iface->get_feed_chunksize(recorder_sr->vc_handle);
            }
        }

        fill_cnt += recorder_sr->read((char *)i_buf + fill_cnt, buf_size - fill_cnt, recorder_sr->user_data, portMAX_DELAY);
        if(fill_cnt == buf_size) {

        #if DEBUG_FLAG        
        
            for(int i = 0; i < (buf_size / sizeof(int16_t) / AUDIO_ADC_INPUT_CH_NUM); i++)
            {
                play_buff[i + play_size] = i_buf[i * AUDIO_ADC_INPUT_CH_NUM + 1];
            }
            play_size+= (buf_size / sizeof(int16_t) / AUDIO_ADC_INPUT_CH_NUM);

            if(!qm_utils_time_left(&debug_timer)){
                ESP_LOGI(TAG, "Audio communication task fetch size: %d", play_size);
                qm_audio_ws_data_cb(play_buff, play_size, NULL);
                play_size = 0;
                qm_utils_time_countdown_ms(&debug_timer, 5 * 1000);
            }
        #endif      

            if (bits & RECORDER_SR_FLAG) {

                if(recorder_sr->sr_handle){
                    short_ich_to_och(i_buf, 
                            recorder_sr->sr_iface->get_feed_chunksize(recorder_sr->sr_handle),
                            AUDIO_ADC_INPUT_CH_NUM, recorder_sr->sr_iface->get_feed_channel_num(recorder_sr->sr_handle));
                    recorder_sr->sr_iface->base.feed(recorder_sr->sr_handle, i_buf, buf_size);
                }
                 
            }else{
                if(recorder_sr->vc_handle){
                    short_ich_to_och(i_buf,
                                recorder_sr->sr_iface->get_feed_chunksize(recorder_sr->sr_handle),
                                AUDIO_ADC_INPUT_CH_NUM, recorder_sr->vc_iface->get_feed_channel_num(recorder_sr->vc_handle));
                    recorder_sr->vc_iface->base.feed(recorder_sr->vc_handle, i_buf, buf_size);
                }
            }

            fill_cnt -= buf_size;
        } else if (fill_cnt > buf_size) {
            ESP_LOGE(TAG, "fill cnt > buffer_size, there may be memory out of range");
        }
    }
}

static int audio_recorder_read_from_sr(void *buffer, int buf_sz, void *user_ctx, TickType_t ticks)
{
    AUDIO_NULL_CHECK(TAG, user_ctx, return ESP_FAIL);
    audio_recorder_t *recorder = (audio_recorder_t *)user_ctx;
    int ret = 0;
    if (recorder->vc_handle) {
        qm_audio_subproc_iface_t *subproc = &recorder->vc_iface->base;
        ret = subproc->fetch(recorder->vc_handle, buffer, buf_sz, ticks);
    }else if(recorder->sr_handle){
        qm_audio_subproc_iface_t *subproc = &recorder->sr_iface->base;
        ret = subproc->fetch(recorder->sr_handle, buffer, buf_sz, ticks);
    }

    return ret;
}

static void audio_recorder_free(audio_recorder_t *recorder)
{
    AUDIO_NULL_CHECK(TAG, recorder, return;);

    esp_timer_stop(recorder->wakeup_timer);
    esp_timer_delete(recorder->wakeup_timer);

    if (recorder->cmd_queue) {
        vQueueDelete(recorder->cmd_queue);
    }
    if (recorder->sync_evt) {
        vEventGroupDelete(recorder->sync_evt);
    }
    free(recorder);
}

audio_rec_handle_t qm_audio_recorder_create(audio_rec_cfg_t *config)
{
    AUDIO_NULL_CHECK(TAG, config, return NULL);
    AUDIO_NULL_CHECK(TAG, config->event_cb, return NULL);
    AUDIO_NULL_CHECK(TAG, config->read, return NULL);

    audio_recorder_t *recorder = audio_calloc(1, sizeof(audio_recorder_t));
    AUDIO_NULL_CHECK(TAG, recorder, return NULL);
    recorder->state = RECORDER_ST_IDLE;

    recorder->event_cb       = config->event_cb;
    recorder->user_data      = config->user_data;
    recorder->read           = config->read;
    recorder->vc_handle      = config->vc_handle;
    recorder->vc_iface       = config->vc_iface;

    recorder->sr_handle      = config->sr_handle;
    recorder->sr_iface       = config->sr_iface;
    recorder->wakeup_time    = config->wakeup_time;
    recorder->encoder_handle = config->encoder_handle;
    recorder->encoder_iface  = config->encoder_iface;

    recorder->vad_check = true;
    recorder->sync_evt = xEventGroupCreate();

#if DEBUG_FLAG
    play_buff = (int16_t *)qm_malloc(256 * 1024 * sizeof(int16_t));
    if(play_buff == NULL){
        return NULL;
    }
    memset(play_buff, 0,  256 * 1024 * sizeof(int16_t));
    
    qm_utils_time_countdown_ms(&debug_timer, 5 * 1000);
#endif
    if (recorder->sr_handle) {
        qm_audio_sr_iface_t *sr_iface = recorder->sr_iface;
        AUDIO_NULL_CHECK(TAG, sr_iface, goto _failed);

        sr_iface->base.set_read_cb(recorder->sr_handle, recorder->read, NULL);
        sr_iface->set_mn_monitor(recorder->sr_handle, audio_recorder_mn_monitor, recorder);
    #if !AUDIO_DIY_WAKENET
        sr_iface->set_afe_monitor(recorder->sr_handle, audio_recorder_afe_monitor, recorder);
    #endif
        sr_iface->base.enable(recorder->sr_handle, true);

        xEventGroupSetBits(recorder->sync_evt, RECORDER_SR_FLAG);
    }

    if (recorder->vc_handle) {
        qm_audio_vc_iface_t *vc_iface = recorder->vc_iface;
        AUDIO_NULL_CHECK(TAG, vc_iface, goto _failed);

        vc_iface->set_afe_monitor(recorder->vc_handle, audio_recorder_vc_afe_monitor, recorder);
        vc_iface->base.set_read_cb(recorder->vc_handle, recorder->read, NULL);
        vc_iface->base.enable(recorder->vc_handle, true);
    }


    if (recorder->encoder_handle) {
        qm_audio_encoder_iface_t *encoder_iface = recorder->encoder_iface;

        if (recorder->vc_handle || recorder->sr_handle) {
            encoder_iface->base.set_read_cb(recorder->encoder_handle, audio_recorder_read_from_sr, recorder);
        } else {
            encoder_iface->base.set_read_cb(recorder->encoder_handle, recorder->read, NULL);
        }
        recorder->encoder_iface->base.enable(recorder->encoder_handle, true);
    }

    if (audio_thread_create(&recorder->feed_handle, "feed_task", feed_task, (void *)recorder, AUDIO_REC_FEED_TASK_SZ, AUDIO_REC_FEED_TASK_PRIO, true, AUDIO_REC_FEED_TASK_CORE) != ESP_OK) {
        ESP_LOGE(TAG, "Fail to create recorder_task");
        goto _failed;
    }

    return recorder;

_failed:
    audio_recorder_free(recorder);
    recorder = NULL;
    return NULL;
}

esp_err_t audio_recorder_destroy(audio_rec_handle_t handle)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    audio_recorder_reset(recorder);

    recorder_msg_t msg = {
        .id = RECORDER_CMD_DESTROY,
        .data = NULL,
        .data_len = 0
    };
    if (xQueueSend(recorder->cmd_queue, &msg, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "The recorder destroy failed");
        return ESP_FAIL;
    }
    EventBits_t bits = xEventGroupWaitBits(recorder->sync_evt, RECORDER_DESTROYED, true, true, portMAX_DELAY);
    if (bits & RECORDER_DESTROYED) {
        ESP_LOGI(TAG, "Recorder task destroyed");
    }
    audio_recorder_free(recorder);
    ESP_LOGI(TAG, "Recorder destroyed");
    return ESP_OK;
}

esp_err_t audio_recorder_trigger_start(audio_rec_handle_t handle)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    recorder_msg_t msg = {
        .id = RECORDER_CMD_TRIGGER_START,
        .data = NULL,
        .data_len = 0
    };
    if (xQueueSend(recorder->cmd_queue, &msg, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "The recorder start failed");
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

qm_err_t qm_audio_recorder_waketimer_reset(audio_rec_handle_t handle)
{
    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    return audio_recorder_wakeup_timer_start(recorder);
}

qm_err_t qm_audio_recorder_data_reset(audio_rec_handle_t handle)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG);
    int ret = 0;
    qm_audio_subproc_iface_t *subproc = NULL;
    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    if (recorder->encoder_handle) {
        subproc = &recorder->encoder_iface->base;
        subproc->reset(recorder->encoder_handle);
    } 
    
    if (recorder->vc_handle) {
        subproc = &recorder->vc_iface->base;
        subproc->reset(recorder->vc_handle);
    } 
    
    if (recorder->sr_handle) {
        subproc = &recorder->sr_iface->base;
        subproc->reset(recorder->sr_handle);
    }

    return ret;
}

int qm_audio_recorder_data_read(audio_rec_handle_t handle, void *buffer, int length, TickType_t ticks)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG);
    audio_recorder_t *recorder = (audio_recorder_t *)handle;
    qm_audio_subproc_iface_t *subproc = NULL;
    int ret = 0;
    if (recorder->encoder_handle) {
        subproc = &recorder->encoder_iface->base;
        ret = subproc->fetch(recorder->encoder_handle, buffer, length, ticks);
    } else if (recorder->vc_handle) {
        subproc = &recorder->vc_iface->base;
        ret = subproc->fetch(recorder->vc_handle, buffer, length, ticks);
    } else if (recorder->sr_handle) {
        subproc = &recorder->sr_iface->base;
        ret = subproc->fetch(recorder->sr_handle, buffer, length, ticks);
    } else {
        ret = recorder->read(buffer, length, recorder, ticks);
    }

    return ret;
}