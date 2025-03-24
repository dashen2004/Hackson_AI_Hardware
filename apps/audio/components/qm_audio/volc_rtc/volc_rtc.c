/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "sdkconfig.h"

#include "audio_recorder.h"
#include "recorder_sr.h"
#include "audio_pipeline.h"
#include "audio_thread.h"
#include "raw_stream.h"
#include "audio_mem.h"
#include "esp_delegate.h"
#include "esp_dispatcher.h"

#include "VolcEngineRTCLite.h"
#include "audio_processor.h"
#include "config.h"

#include "qm.h"
#include "volc_rtc.h"

#define CONFIG_LANGUAGE_WAKEUP_MODE  1

static const char* TAG = "volc_rtc";

#define STATS_TASK_PRIO        (5)
#define JOIN_EVENT_BIT         (1 << 0)
#define WAIT_DESTORY_EVENT_BIT (1 << 0)
#define WAKEUP_REC_READING     (1 << 0)

#define volc_rtc_safe_free(x, fn) do {   \
    if (x) {                             \
        fn(x);                           \
        x = NULL;                        \
    }                                    \
} while (0)

struct volc_rtc_t {
    volc_rtc_cfg_t cfg;
    byte_rtc_engine_t          engine;
};

static struct volc_rtc_t s_volc_rtc;


// static void audio_pull_data_process(char *ptr, int len);

// byte rtc lite callbacks
static void byte_rtc_on_join_room_success(byte_rtc_engine_t engine, const char* channel, int elapsed_ms, bool ms)
{
    s_volc_rtc.cfg.event_handler.on_join_room_success((volc_rtc_engine_t)engine, channel, elapsed_ms, ms);
};

static __attribute__((unused)) void byte_rtc_on_rejoin_room_success(byte_rtc_engine_t engine, const char* channel, int elapsed_ms)
{

};

static void byte_rtc_on_user_joined(byte_rtc_engine_t engine, const char* channel, const char* user_name, int elapsed_ms)
{
    s_volc_rtc.cfg.event_handler.on_user_joined((volc_rtc_engine_t)engine, channel, user_name, elapsed_ms);
};

static void byte_rtc_on_user_offline(byte_rtc_engine_t engine, const char* channel, const char* user_name, int reason)
{
    s_volc_rtc.cfg.event_handler.on_user_offline((volc_rtc_engine_t)engine, channel, user_name, reason);
};

static void byte_rtc_on_user_mute_audio(byte_rtc_engine_t engine, const char* channel, const char* user_name, int muted)
{
    s_volc_rtc.cfg.event_handler.on_user_mute_audio((volc_rtc_engine_t)engine, channel, user_name, muted);
};

static void byte_rtc_on_user_mute_video(byte_rtc_engine_t engine, const char* channel, const char* user_name, int muted)
{
    s_volc_rtc.cfg.event_handler.on_user_mute_video((volc_rtc_engine_t)engine, channel, user_name, muted);
};

static __attribute__((unused)) void byte_rtc_on_connection_lost(byte_rtc_engine_t engine, const char* channel)
{

};

static void byte_rtc_on_room_error(byte_rtc_engine_t engine, const char* channel, int code, const char* msg)
{
    s_volc_rtc.cfg.event_handler.on_room_error((volc_rtc_engine_t)engine, channel, code, msg);
};

// remote audio
static void byte_rtc_on_audio_data(byte_rtc_engine_t engine, const char* channel, const char*  uid , uint16_t sent_ts,
                                   audio_codec_type_e codec, const void* data_ptr, size_t data_len)
{
    s_volc_rtc.cfg.event_handler.on_audio_data((volc_rtc_engine_t)engine, channel, uid, sent_ts, codec, data_ptr, data_len);
}

// remote video
static void byte_rtc_on_video_data(byte_rtc_engine_t engine, const char*  channel, const char* uid, uint16_t sent_ts,
                                    video_data_type_e codec, int is_key_frame,
                                    const void * data_ptr, size_t data_len){ }

// remote message
static void on_message_received(byte_rtc_engine_t engine, const char*  room, const char* uid, const uint8_t* message, int size, bool binary)
{
    s_volc_rtc.cfg.event_handler.on_message_received((volc_rtc_engine_t)engine, room, uid, message, size, binary);
}

static void on_key_frame_gen_req(byte_rtc_engine_t engine, const char*  channel, const char*  uid) {}
// byte rtc lite callbacks end.

#if 0
static void byte_rtc_engine_destroy() 
{
    if (s_volc_rtc.engine) {
        byte_rtc_fini(s_volc_rtc.engine);
        vTaskDelay(pdMS_TO_TICKS(1000));
        byte_rtc_destory(s_volc_rtc.engine);
        s_volc_rtc.engine = NULL;
    }
}

static esp_err_t byte_rtc_engine_create()
{

    byte_rtc_event_handler_t handler = {
        .on_join_room_success       =   byte_rtc_on_join_room_success,
        .on_room_error              =   byte_rtc_on_room_error,
        .on_user_joined             =   byte_rtc_on_user_joined,
        .on_user_offline            =   byte_rtc_on_user_offline,
        .on_user_mute_audio         =   byte_rtc_on_user_mute_audio,
        .on_user_mute_video         =   byte_rtc_on_user_mute_video,
        .on_audio_data              =   byte_rtc_on_audio_data,
        .on_video_data              =   byte_rtc_on_video_data,
        .on_key_frame_gen_req       =   on_key_frame_gen_req,
        .on_message_received        =   on_message_received,
    };

    s_volc_rtc.engine = byte_rtc_create(DEFAULT_APPID, &handler);

    byte_rtc_set_log_level(s_volc_rtc.engine, BYTE_RTC_LOG_LEVEL_ERROR);
#ifdef RTC_TEST_ENV
    byte_rtc_set_params(s_volc_rtc.engine, "{\"env\":2}"); // test env
#endif
    byte_rtc_set_params(s_volc_rtc.engine, "{\"debug\":{\"log_to_console\":1}}"); 
    byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"pinned_to_core\":1}}}");
    byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"priority\":20}}}");

    byte_rtc_init(s_volc_rtc.engine);
#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
    byte_rtc_set_audio_codec(s_volc_rtc.engine, AUDIO_CODEC_TYPE_OPUS);
#elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
    byte_rtc_set_audio_codec(s_volc_rtc.engine, AUDIO_CODEC_TYPE_G711A);
#else // AACLC Encoder
    byte_rtc_set_audio_codec(s_volc_rtc.engine, AUDIO_CODEC_TYPE_AACLC);
#endif // CONFIG_AUDIO_SUPPORT_OPUS_DECODER
    open_audio_pipeline();

    ESP_LOGI(TAG, "start join room\n");
    byte_rtc_room_options_t options = { 0 };
    options.auto_subscribe_audio = 1;
    options.auto_subscribe_video = 0;
    byte_rtc_join_room(s_volc_rtc.engine, DEFAULT_ROOMID, DEFAULT_UID, DEFAULT_TOKEN, &options);

    // Default wait time is forever
    xEventGroupWaitBits(s_volc_rtc.join_event, JOIN_EVENT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "join room success\n");

    return ESP_OK;
}

static void audio_data_process_task(void *args)
{
    s_volc_rtc.data_proc_running = true;

    qm_list_node_t *node = NULL;
    frame_package_t *frame = NULL;
    while (s_volc_rtc.data_proc_running) {
        qm_sem_wait(&g_rtc_audio.sem, QM_WAIT_FOREVER);

        qm_mutex_lock(&g_rtc_audio.lock, QM_WAIT_FOREVER);
        node = qm_list_lpop(&g_rtc_audio.list);
        qm_mutex_unlock(&g_rtc_audio.lock);

        if(node == NULL){
            continue;
        }
        frame = (frame_package_t *)qm_list_node_val_get(node);
        audio_pull_data_process(frame->frame_ptr, frame->frame_len);
        qm_list_node_destroy(node);
    }

    vTaskDelete(NULL);
}

static void voice_read_task(void *args)
{
    const int voice_data_read_sz = recorder_pipeline_get_default_read_size(s_volc_rtc.record_pipeline);
    uint8_t *voice_data = audio_calloc(1, voice_data_read_sz);
    bool runing = true;

    audio_frame_info_t audio_frame_info = {.data_type = AUDIO_DATA_TYPE_PCMA};

#if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
    TickType_t wait_tm = portMAX_DELAY;
#else
    TickType_t wait_tm = 0;
#endif // CONFIG_LANGUAGE_WAKEUP_MODE
    while (runing) {
        EventBits_t bits = xEventGroupWaitBits(s_volc_rtc.wakeup_event, WAKEUP_REC_READING , false, true, wait_tm);
    #if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
        if (bits & WAKEUP_REC_READING) {
            int ret = audio_recorder_data_read(s_volc_rtc.recorder_engine, voice_data, voice_data_read_sz, portMAX_DELAY);
            if (ret == 0 || ret == -1) {
                xEventGroupClearBits(s_volc_rtc.wakeup_event,  WAKEUP_REC_READING);
            } else {
                byte_rtc_send_audio_data(s_volc_rtc.engine, DEFAULT_ROOMID, voice_data, voice_data_read_sz, &audio_frame_info);
            }
        }
    #else
        audio_recorder_data_read(s_volc_rtc.recorder_engine, voice_data, voice_data_read_sz, portMAX_DELAY);
        byte_rtc_send_audio_data(s_volc_rtc.engine, DEFAULT_ROOMID, voice_data, voice_data_read_sz, &audio_frame_info);
    #endif
    }
    xEventGroupClearBits(s_volc_rtc.wakeup_event, WAKEUP_REC_READING);
    audio_free(voice_data);
    vTaskDelete(NULL);
}

#endif

static void log_clear(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("AUDIO_THREAD", ESP_LOG_ERROR);
    esp_log_level_set("i2c_bus_v2", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_HAL", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_ERROR);
    esp_log_level_set("I2S_STREAM_IDF5.x", ESP_LOG_ERROR);
    esp_log_level_set("RSP_FILTER", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_EVT", ESP_LOG_ERROR);
}

qm_err_t volc_rtc_send_audio_data(volc_rtc_engine_t engine,const char * room,const void * data_ptr, size_t data_len)
{
    // defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
    audio_frame_info_t audio_frame_info = {.data_type = AUDIO_DATA_TYPE_PCMA};
    return byte_rtc_send_audio_data(s_volc_rtc.engine, room, data_ptr, data_len, &audio_frame_info);
}

qm_err_t volc_rtc_join_room(volc_rtc_engine_t engine,const char *room, const char * uid, const char * token)
{
    byte_rtc_room_options_t options = { 0 };
    options.auto_subscribe_audio = 1;
    options.auto_subscribe_video = 0;
    byte_rtc_join_room(s_volc_rtc.engine, room, uid, token, &options);
#if 0
    // Default wait time is forever
    xEventGroupWaitBits(s_volc_rtc.join_event, JOIN_EVENT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "join room success\n");
#endif
    return ESP_OK;
}

volc_rtc_engine_t volc_rtc_open(volc_rtc_cfg_t *cfg)
{
#if 0
    byte_rtc_engine_create();
    s_volc_rtc.recorder_engine = audio_record_engine_init(s_volc_rtc.record_pipeline, rec_engine_cb);
    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 5 * 1024, 5, true, 1);
    audio_thread_create(NULL, "audio_data_process_task", audio_data_process_task, (void *)NULL, 8 * 1024, 10, true, 1);
    s_volc_rtc.join_event = xEventGroupCreate();
    s_volc_rtc.wait_destory_event = xEventGroupCreate();
    log_clear();
#endif


    byte_rtc_event_handler_t handler = {
        .on_join_room_success       =   byte_rtc_on_join_room_success,
        .on_room_error              =   byte_rtc_on_room_error,
        .on_user_joined             =   byte_rtc_on_user_joined,
        .on_user_offline            =   byte_rtc_on_user_offline,
        .on_user_mute_audio         =   byte_rtc_on_user_mute_audio,
        .on_user_mute_video         =   byte_rtc_on_user_mute_video,
        .on_audio_data              =   byte_rtc_on_audio_data,
        .on_video_data              =   byte_rtc_on_video_data,
        .on_key_frame_gen_req       =   on_key_frame_gen_req,
        .on_message_received        =   on_message_received,
    };

    memcpy(&s_volc_rtc.cfg, cfg, sizeof(volc_rtc_cfg_t));
    s_volc_rtc.engine = byte_rtc_create(cfg->appid, &handler);
    
    byte_rtc_set_log_level(s_volc_rtc.engine, BYTE_RTC_LOG_LEVEL_ERROR);

#ifdef RTC_TEST_ENV
    byte_rtc_set_params(s_volc_rtc.engine, "{\"env\":2}"); // test env
#endif
    byte_rtc_set_params(s_volc_rtc.engine, "{\"debug\":{\"log_to_console\":1}}"); 
    byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"pinned_to_core\":1}}}");
    byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"priority\":10}}}");

    byte_rtc_init(s_volc_rtc.engine);

// defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
    byte_rtc_set_audio_codec(s_volc_rtc.engine, AUDIO_CODEC_TYPE_G711A);

    return (volc_rtc_engine_t)s_volc_rtc.engine;

#if 0
    ESP_LOGI(TAG, "start join room\n");
    byte_rtc_room_options_t options = { 0 };
    options.auto_subscribe_audio = 1;
    options.auto_subscribe_video = 0;
    byte_rtc_join_room(s_volc_rtc.engine, DEFAULT_ROOMID, DEFAULT_UID, DEFAULT_TOKEN, &options);

    // Default wait time is forever
    xEventGroupWaitBits(s_volc_rtc.join_event, JOIN_EVENT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "join room success\n");
    
    return ESP_OK;
#endif
}

qm_err_t volc_rtc_close(volc_rtc_engine_t engine)
{
#if 0
    s_volc_rtc.byte_rtc_running = false;
    s_volc_rtc.data_proc_running = false;
    xEventGroupWaitBits(s_volc_rtc.wait_destory_event, WAIT_DESTORY_EVENT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    byte_rtc_engine_destroy();
#endif
    return ESP_OK;
}
