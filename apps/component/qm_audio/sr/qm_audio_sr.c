
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
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"
#endif

#include "qm_audio_sr.h"
#include "qm_audio_sr_iface.h"
#include "qm_audio_subproc_iface.h"

#define FEED_TASK_DESTROY  (BIT(0))
#define FETCH_TASK_DESTROY (BIT(1))
#define FETCH_TASK_RUNNING (BIT(2))

static const char *TAG = "AUDIO_SR";

#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
static const esp_mn_iface_t *multinet = NULL;
#endif

typedef struct __recorder_sr {
    int                   fetch_task_core;
    int                   fetch_task_prio;
    int                   fetch_task_stack;
    ringbuf_handle_t      out_rb;
    int                   rb_size;
    EventGroupHandle_t    events;
    bool                  fetch_running;
    qm_audio_data_read_t  read;
    void                  *read_ctx;
    srmodel_list_t        *models;
    esp_afe_sr_data_t     *afe_data;
    esp_afe_sr_iface_t    *afe_handle;
    qm_audio_sr_monitor_t afe_monitor;
    void                  *afe_monitor_ctx;
    bool                  wwe_enable;
    bool                  vad_enable;
    char                  *partition_label;
    bool                  aec_enable;
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
    model_iface_data_t    *mn_handle;
    qm_audio_sr_monitor_t mn_monitor;
    void                  *mn_monitor_ctx;
    bool                  mn_enable;
    char                  *mn_language;
#endif /* !CONFIG_SR_MN_CN_NONE */
    char                  *wn_wakeword;

    qm_audio_sr_cmd_table_t *cmd_table;
    int                      cmd_num;

} recorder_sr_t;

// #define DEBUG_FLAG 1

#if DEBUG_FLAG

#include "qm_utils_timer.h"

static int play_size = 0;
static int16_t *play_buff = NULL;
static qm_utils_time_t debug_timer = {0};
extern void qm_audio_ws_data_cb(void *data, int len, void *user_data);
#endif

static qm_err_t recorder_sr_output(recorder_sr_t *recorder_sr, void *buffer, int len);

static qm_audio_sr_result_t *recorder_sr_afe_result_convert(
                                                    recorder_sr_t *recorder_sr,
                                                    afe_fetch_result_t *afe_result,
                                                    qm_audio_sr_result_t *sr_result)
{
    sr_result->type = SR_RESULT_UNKNOW;
    ESP_LOGV(TAG, "wake %d, vad %d", afe_result->wakeup_state, afe_result->vad_state);
    switch (afe_result->wakeup_state) {
        case WAKENET_CHANNEL_VERIFIED:
            sr_result->type = SR_RESULT_VERIFIED;
            break;
        case WAKENET_NO_DETECT:

            break;
        case WAKENET_DETECTED:
            sr_result->type = SR_RESULT_WAKEUP;
            sr_result->info.wakeup_info.data_volume = afe_result->data_volume;
            sr_result->info.wakeup_info.wake_word_index = afe_result->wake_word_index;
            sr_result->info.wakeup_info.wakenet_model_index = afe_result->wakenet_model_index;
            break;
        default:
            break;
    }
    return sr_result;
}

#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)

static qm_err_t recorder_mn_detect(recorder_sr_t *recorder_sr, afe_fetch_result_t *afe_result)
{
    static int detect_flag = 0;
    static vad_state_t vad_state = VAD_SILENCE;

    esp_mn_state_t mn_state = 0;

    if (!recorder_sr->mn_enable) {
        if (detect_flag == 1) {
            detect_flag = 0;
        }
        return ESP_OK;
    }

    if (recorder_sr->wwe_enable) {
        if (afe_result->wakeup_state == WAKENET_DETECTED || 
            afe_result->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            detect_flag = 1;
        }
    } else {
        
        if(vad_state != afe_result->vad_state){
            vad_state = afe_result->vad_state;
            QM_LOGD(TAG, "vad state:%d", afe_result->vad_state);
            
            multinet->clean(recorder_sr->mn_handle);

            if (afe_result->vad_state == VAD_SPEECH) {
                detect_flag = 1;

                if (afe_result->vad_cache_size > 0) {
                    int offset = 0;
                    int chunk_size = multinet->get_samp_chunksize(recorder_sr->mn_handle);
                                        
                    do{
                        if(afe_result->vad_cache_size >= chunk_size){
                            mn_state = multinet->detect(recorder_sr->mn_handle, afe_result->vad_cache + offset);
                            if(mn_state == ESP_MN_STATE_DETECTED){
                                goto __detect;
                            }
                            offset += (chunk_size / sizeof(int16_t));
                            afe_result->vad_cache_size -= chunk_size;
                        }else{
                            afe_result->vad_cache_size = 0;
                        }
                    }while (afe_result->vad_cache_size);
                }

            }else{
    
                detect_flag = 0;
            }
        }
    }

    if (detect_flag == 1) {
        mn_state = multinet->detect(recorder_sr->mn_handle, afe_result->data);

        if (mn_state == ESP_MN_STATE_DETECTING) {
            return ESP_OK;
        }
        if (mn_state == ESP_MN_STATE_DETECTED) {
    __detect:
            QM_LOGD("1", "ESP_MN_STATE_DETECTED");
            esp_mn_results_t *mn_result = multinet->get_results(recorder_sr->mn_handle);
            if (recorder_sr->mn_monitor) {
                qm_audio_sr_result_t sr_result = { 0 };

                sr_result.type = mn_result->command_id[0];
                sr_result.info.mn_info.phrase_id = mn_result->phrase_id[0];
                sr_result.info.mn_info.prob = mn_result->prob[0];
                memcpy(sr_result.info.mn_info.str, mn_result->string, RECORDER_SR_MN_STRING_MAX_LEN);

                QM_LOGD("1", "type:%d,phrase_id:%d,str:%s", 
                    sr_result.type, sr_result.info.mn_info.phrase_id, sr_result.info.mn_info.str);

                recorder_sr->mn_monitor(&sr_result, recorder_sr->mn_monitor_ctx);

            }
        }
    
        if (mn_state == ESP_MN_STATE_TIMEOUT) {
            if (recorder_sr->wwe_enable) {
                detect_flag = 0;
                ESP_LOGI(TAG, "MN dect quit");
                multinet->clean(recorder_sr->mn_handle);
            }
        }
    }
    return ESP_OK;
}
#endif /*!CONFIG_SR_MN_CN_NONE*/

static void fetch_task(void *parameters)
{
    recorder_sr_t *recorder_sr = (recorder_sr_t *)parameters;
    recorder_sr->fetch_running = true;

    int fetch_size = recorder_sr->afe_handle->get_fetch_chunksize(recorder_sr->afe_data);
    int feed_size = recorder_sr->afe_handle->get_feed_chunksize(recorder_sr->afe_data);
    ESP_LOGI(TAG, "Audio SR task started, feed size: %d fetch size: %d",
        feed_size, fetch_size);

    while (recorder_sr->fetch_running) {
        xEventGroupWaitBits(recorder_sr->events, FETCH_TASK_RUNNING, false, true, portMAX_DELAY);

        afe_fetch_result_t *afe_result = recorder_sr->afe_handle->fetch_with_delay(recorder_sr->afe_data, QM_WAIT_FOREVER);
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
        recorder_mn_detect(recorder_sr, afe_result);
#endif
        if (recorder_sr->afe_monitor) {
            qm_audio_sr_result_t sr_result = {0};
            recorder_sr->afe_monitor(
                    recorder_sr_afe_result_convert(recorder_sr, afe_result, &sr_result),
                    recorder_sr->afe_monitor_ctx);
        }
        recorder_sr_output(recorder_sr, afe_result->data, afe_result->data_size);

    #if DEBUG_FLAG        
        memcpy(play_buff + play_size, afe_result->data,afe_result->data_size);
        play_size += afe_result->data_size;

        if(!qm_utils_time_left(&debug_timer)){
            ESP_LOGI(TAG, "Audio communication task fetch size: %d", play_size);
            qm_audio_ws_data_cb(play_buff, play_size, NULL);
            play_size = 0;
            qm_utils_time_countdown_ms(&debug_timer, 5 * 1000);
        }
    #endif     

    }
    xEventGroupClearBits(recorder_sr->events, FETCH_TASK_RUNNING);
    xEventGroupSetBits(recorder_sr->events, FETCH_TASK_DESTROY);
    vTaskDelete(NULL);
}

static qm_err_t recorder_sr_output(recorder_sr_t *recorder_sr, void *buffer, int len)
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
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    return rb_read(recorder_sr->out_rb, buf, len, ticks);
}

static qm_err_t recorder_sr_suspend(void *handle, bool suspend)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

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
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    esp_err_t ret = ESP_OK;
    if (enable) {
        if (!recorder_sr->fetch_running && recorder_sr->fetch_task_stack > 0) {
            audio_thread_t fetch_thread = NULL;
            ret |= audio_thread_create(&fetch_thread, "sr_fetch",
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
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    qm_audio_sr_state_t *sr_state = (qm_audio_sr_state_t *)state;
    EventBits_t bits = xEventGroupWaitBits(recorder_sr->events, FETCH_TASK_RUNNING, false, true, 0);
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
    sr_state->mn_enable = recorder_sr->mn_enable;
#else
    sr_state->mn_enable = false;
#endif
    sr_state->wwe_enable = recorder_sr->wwe_enable;
    sr_state->vad_enable = recorder_sr->vad_enable;
    if (!recorder_sr->fetch_running) {
        sr_state->afe_state = DISABLED;
    } else if ((bits & ( FETCH_TASK_RUNNING)) == (FETCH_TASK_RUNNING)) {
        sr_state->afe_state = RUNNING;
    } else {
        sr_state->afe_state = SUSPENDED;
    }
    return ESP_OK;
}

static qm_err_t recorder_sr_set_read_cb(void *handle, qm_audio_data_read_t read_cb, void *user_ctx)
{
    AUDIO_CHECK(TAG, handle, return ESP_FAIL, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

    recorder_sr->read = read_cb;
    recorder_sr->read_ctx = user_ctx;

    return ESP_OK;
}

static qm_err_t recorder_sr_set_afe_monitor(void *handle, qm_audio_sr_monitor_t monitor, void *user_ctx)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

    recorder_sr->afe_monitor = monitor;
    recorder_sr->afe_monitor_ctx = user_ctx;

    return ESP_OK;
}

static qm_err_t recorder_sr_set_mn_monitor(void *handle, qm_audio_sr_monitor_t monitor, void *user_ctx)
{
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

    recorder_sr->mn_monitor = monitor;
    recorder_sr->mn_monitor_ctx = user_ctx;

    return ESP_OK;
#else
    ESP_LOGW(TAG, "Multinet is not enabled in SDKCONFIG");
    return ESP_FAIL;
#endif
}

static qm_err_t recorder_sr_wwe_enable(void *handle, bool_t enable)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    int ret = 0;

    if (enable) {
        if (!recorder_sr->wwe_enable) {
            ret = recorder_sr->afe_handle->enable_wakenet(recorder_sr->afe_data);
            recorder_sr->wwe_enable = true;
        }
    } else {
        if (recorder_sr->wwe_enable) {
            ret = recorder_sr->afe_handle->disable_wakenet(recorder_sr->afe_data);
            recorder_sr->wwe_enable = false;
        }
    }
    return ret == 1 ? ESP_OK : ESP_FAIL;
}

static qm_err_t recorder_sr_mn_enable(void *handle, bool_t enable)
{
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

    recorder_sr->mn_enable = enable;
    if (!recorder_sr->mn_enable && recorder_sr->mn_handle) {
        multinet->destroy(recorder_sr->mn_handle);
        esp_mn_commands_free();
        recorder_sr->mn_handle = NULL;
    }

    if (recorder_sr->mn_enable && !recorder_sr->mn_handle) {
        char *mn_name = esp_srmodel_filter(recorder_sr->models, ESP_MN_PREFIX, recorder_sr->mn_language);
        multinet = esp_mn_handle_from_name(mn_name);
        recorder_sr->mn_handle = multinet->create(mn_name, 5760);
        AUDIO_NULL_CHECK(TAG, recorder_sr->mn_handle, return ESP_FAIL);
    }
    return ESP_OK;
#else
    ESP_LOGW(TAG, "Multinet is not enabled in SDKCONFIG");
    return ESP_FAIL;
#endif
}

static int recorder_sr_feed(void *handle, void *buf, int len)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    return recorder_sr->afe_handle->feed(recorder_sr->afe_data, buf);
}

static int recorder_sr_get_feed_channel_num(void *handle)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    return recorder_sr->afe_handle->get_feed_channel_num(recorder_sr->afe_data);
}

static int recorder_sr_get_feed_chunksize(void *handle)
{
    AUDIO_CHECK(TAG, handle, return ESP_ERR_INVALID_ARG, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    return recorder_sr->afe_handle->get_feed_chunksize(recorder_sr->afe_data);
}

static void recorder_sr_clear(void *handle)
{
    AUDIO_CHECK(TAG, handle, return, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

    recorder_sr->afe_handle->reset_buffer(recorder_sr->afe_data);
    
    if (recorder_sr->out_rb) {
        rb_reset(recorder_sr->out_rb);
    }
}

static qm_audio_sr_iface_t recorder_sr_iface = {
    .base.enable = recorder_sr_enable,
    .base.get_state = recorder_sr_get_state,
    .base.set_read_cb = recorder_sr_set_read_cb,
    .base.feed = recorder_sr_feed,
    .base.fetch = recorder_sr_fetch,
    .base.reset = recorder_sr_clear,
    .afe_suspend = recorder_sr_suspend,
    .set_afe_monitor = recorder_sr_set_afe_monitor,
    .set_mn_monitor = recorder_sr_set_mn_monitor,
    .wwe_enable = recorder_sr_wwe_enable,
    .mn_enable = recorder_sr_mn_enable,
    .get_feed_channel_num = recorder_sr_get_feed_channel_num,
    .get_feed_chunksize = recorder_sr_get_feed_chunksize,
};


qm_audio_sr_handle_t qm_audio_sr_create(qm_audio_sr_cfg_t *cfg, qm_audio_sr_iface_t **iface)
{
    AUDIO_NULL_CHECK(TAG, cfg, return NULL);

#if DEBUG_FLAG
    play_buff = (int16_t *)qm_malloc(256 * 1024 * sizeof(int16_t));
    if(play_buff == NULL){
        return NULL;
    }
    memset(play_buff, 0,  256 * 1024 * sizeof(int16_t));
    
    qm_utils_time_countdown_ms(&debug_timer, 5 * 1000);
#endif

    recorder_sr_t *recorder_sr = audio_calloc(1, sizeof(recorder_sr_t));
    AUDIO_NULL_CHECK(TAG, recorder_sr, return NULL);
    recorder_sr->fetch_task_core  = cfg->fetch_task_core;
    recorder_sr->fetch_task_prio  = cfg->fetch_task_prio;
    recorder_sr->fetch_task_stack = cfg->fetch_task_stack;
    recorder_sr->rb_size          = cfg->rb_size;
    recorder_sr->partition_label  = cfg->partition_label;
    recorder_sr->aec_enable       = ((afe_config_t *)cfg->afe_cfg)->aec_init;
    recorder_sr->wn_wakeword      = cfg->wn_wakeword;
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
    recorder_sr->mn_language      = cfg->mn_language;
#endif

    recorder_sr->models = esp_srmodel_init(recorder_sr->partition_label);

    char *wn_name = NULL;
    char *wn_name_2 = NULL;

    if (recorder_sr->wn_wakeword == NULL) {
        if (recorder_sr->models!=NULL) {
            for (int i = 0; i< recorder_sr->models->num; i++) {
                if (strstr(recorder_sr->models->model_name[i], ESP_WN_PREFIX) != NULL) {
                    if (wn_name == NULL) {
                        wn_name = recorder_sr->models->model_name[i];
                        ESP_LOGI(TAG, "The first wakenet model: %s\n", wn_name);
                    } else if (wn_name_2 == NULL) {
                        wn_name_2 = recorder_sr->models->model_name[i];
                        ESP_LOGI(TAG, "The second wakenet model: %s\n", wn_name_2);
                    }
                }
            }
        } else {
            ESP_LOGE(TAG, "Please enable wakenet model and select wake word by menuconfig!\n");
            goto _failed;
        }
    } else {
        wn_name = esp_srmodel_filter(recorder_sr->models, ESP_WN_PREFIX, recorder_sr->wn_wakeword);
        if (wn_name == NULL) {
            ESP_LOGE(TAG, "Please enable wakenet model and select wake word (%s) by menuconfig!", recorder_sr->wn_wakeword);
            goto _failed;
        }
    }

    ((afe_config_t *)cfg->afe_cfg)->wakenet_model_name = wn_name;
    ((afe_config_t *)cfg->afe_cfg)->wakenet_model_name_2 = wn_name_2;

    afe_config_print(cfg->afe_cfg);
    afe_config_check(cfg->afe_cfg);

    recorder_sr->afe_handle = esp_afe_handle_from_config(cfg->afe_cfg);
    recorder_sr->afe_data = recorder_sr->afe_handle->create_from_config(cfg->afe_cfg);
    afe_config_free(cfg->afe_cfg);

    AUDIO_NULL_CHECK(TAG, recorder_sr->afe_data, goto _failed);
    if (((afe_config_t *)cfg->afe_cfg)->wakenet_init == true) {
        recorder_sr->wwe_enable = true;
    }
    if (((afe_config_t *)cfg->afe_cfg)->vad_init) {
        recorder_sr->vad_enable = true;
    }
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
    if (cfg->multinet_init) {
        char *mn_name = esp_srmodel_filter(recorder_sr->models, ESP_MN_PREFIX, recorder_sr->mn_language);
        AUDIO_NULL_CHECK(TAG, mn_name, goto _failed);
        multinet = esp_mn_handle_from_name(mn_name);
        recorder_sr->mn_handle = multinet->create(mn_name, 60 * 1000);
        AUDIO_NULL_CHECK(TAG, recorder_sr->mn_handle, goto _failed);
        recorder_sr->mn_enable = true;
    }
#endif
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

qm_err_t qm_audio_sr_destroy(qm_audio_sr_handle_t handle)
{
    AUDIO_CHECK(TAG, handle, return ESP_FAIL, "Handle is NULL");
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

    recorder_sr_enable(recorder_sr, QM_FALSE);
    recorder_sr_clear(recorder_sr);
    return ESP_OK;
}

qm_err_t qm_audio_sr_get_speech_cmd(qm_audio_sr_handle_t handle, qm_audio_sr_cmd_table_t **command_str, int *command_num)
{
    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;

    *command_num = recorder_sr->cmd_num;
    *command_str = recorder_sr->cmd_table;

    return QM_EOK;
}

qm_err_t qm_audio_sr_add_speech_cmd(qm_audio_sr_handle_t handle, qm_audio_sr_cmd_table_t *command_str, int command_num)
{
#if !defined(CONFIG_SR_MN_CN_NONE) || !defined(CONFIG_SR_MN_EN_NONE)
    AUDIO_CHECK(TAG, handle, return ESP_FAIL, "Handle is NULL");
    AUDIO_CHECK(TAG, command_str, return ESP_FAIL, "command_str is NULL");

    recorder_sr_t *recorder_sr = (recorder_sr_t *)handle;
    esp_err_t err = ESP_OK;

    if(!recorder_sr->mn_enable){
        return -QM_EINVAL;
    }

    esp_mn_commands_clear();
    
    recorder_sr->cmd_num = command_num;
    recorder_sr->cmd_table = command_str;

    for (size_t i = 0; i < command_num; i++) {
        esp_mn_commands_add(command_str[i].id, (char *)command_str[i].str);
    }
    esp_mn_commands_print();

    esp_mn_error_t *mn_err = esp_mn_commands_update(multinet, recorder_sr->mn_handle);

    return mn_err == NULL ? ESP_OK : ESP_FAIL;
#else
    ESP_LOGW(TAG, "Multinet is not enabled");
    return ESP_FAIL;
#endif
}
