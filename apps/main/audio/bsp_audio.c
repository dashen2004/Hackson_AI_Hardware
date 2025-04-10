#include "qm.h"
#include "qm_wifi.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_audio_processor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"


#include "VolcEngineRTCLite.h"

#include "cJSON.h"
#include "util_httpc.h"
#include "audio_main.h"
#include "audio_thread.h"
#include "raw_stream.h"

#define PRODTST_DEVICE_ID_MAX_LEN       (64)
#define PRODTST_DEVICE_SECRET_MAX_LEN   (256)
#define RSP_PAYLOAD_MAX_STR_LEN    (10 * 1024)   

#define CONFIG_AUDIO_SUPPORT_OPUS_DECODER   1

#define STATS_TASK_PRIO          (5)
#define JOIN_EVENT_BIT           (1 << 0)
#define WAKEUP_REC_READING       (1 << 1)
#define DEFAULT_MAX_QUEUE_NUM    (30)
#define DEFAULT_QUEUE_CACHE_NUM  (10)

typedef enum {
    CREATE_RTC_HANDLE,
    JOIN_ROOM_START,
    JOIN_ROOM_SUCCESS,
    JOIN_ROOM_EXIT,
    AUDIO_DATA_MSG,
} frame_type_t;

typedef struct {
    frame_type_t frame_type;
    int frame_len;
    char *frame_ptr;
} frame_package_t;

struct volc_rtc_t {

    int rec_type;

    qm_work_t                  post_work;

    recorder_pipeline_handle_t record_pipeline;
    player_pipeline_handle_t   player_pipeline;
    void                      *recorder_engine;
    
    byte_rtc_engine_t          engine;
    EventGroupHandle_t         event;
    
    QueueHandle_t              frame_q;

#if 0
    coze_http_req_result_t    *user_info;
#else

    char app_id[PRODTST_DEVICE_ID_MAX_LEN + 1];
    char room_id[PRODTST_DEVICE_ID_MAX_LEN + 1];
    char bot_id[PRODTST_DEVICE_ID_MAX_LEN + 1];

    char uid[PRODTST_DEVICE_ID_MAX_LEN + 1];
    char token[PRODTST_DEVICE_SECRET_MAX_LEN + 1];

#endif

    bool                       byte_rtc_running;
#if defined(CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
#define DEALULT_OPUS_DATA_CHACHE_SIZE (512)
    char                    *opus_data_cache;
    int                      opus_data_cache_len;
#endif  // CONFIF_AUDIO_SUPPORT_OPUS_DECODER
};

static struct volc_rtc_t s_volc_rtc = {0};

static void byte_rtc_engine_create(void);
static void audio_pull_data_process(char *ptr, int len);

static void qm_audio_event_handler(qm_input_event_t *input_event, void *arg)
{
    frame_package_t frame = { 0 };

    switch(input_event->sub_event){

        case CONFIG_AUDIO_SUB_EVENT_CEART_RTC:
        
            QM_LOGD(TAG, "creat rtc ");

            frame.frame_type = CREATE_RTC_HANDLE;
            if (xQueueSend(s_volc_rtc.frame_q, &frame, QM_WAIT_FOREVER) != pdPASS) {
                QM_LOGW("1", "audio frame queue full");
            }

        break;

        case CONFIG_AUDIO_SUB_EVENT_MSG_ID:
        
            // QM_LOGD(TAG, "message:%s", (char *)input_event->value);

        break;

        default:
        break;
    }
}


// byte rtc lite callbacks
static void byte_rtc_on_join_room_success(byte_rtc_engine_t engine, const char* channel, int elapsed_ms, bool ms)
{
    QM_LOGI("1", "join channel success %s elapsed %d ms now %d ms\n", channel, elapsed_ms, elapsed_ms);
};

static __attribute__((unused)) void byte_rtc_on_rejoin_room_success(byte_rtc_engine_t engine, const char* channel, int elapsed_ms)
{
    QM_LOGI("1", "rejoin channel success %s\n", channel);
};

static void byte_rtc_on_user_joined(byte_rtc_engine_t engine, const char* channel, const char* user_name, int elapsed_ms)
{
    QM_LOGI("1", "remote user joined  %s:%s", channel, user_name);
    
    frame_package_t frame = { 0 };
    frame.frame_type = JOIN_ROOM_SUCCESS;
    if (xQueueSend(s_volc_rtc.frame_q, &frame, QM_WAIT_FOREVER) != pdPASS) {
        QM_LOGW("1", "audio frame queue full");
    }
};

static void byte_rtc_on_user_offline(byte_rtc_engine_t engine, const char* channel, const char* user_name, int reason)
{
    QM_LOGI("1", "remote user offline  %s:%s", channel, user_name);
};

static void byte_rtc_on_user_mute_audio(byte_rtc_engine_t engine, const char* channel, const char* user_name, int muted)
{
    QM_LOGI("1", "remote user mute audio  %s:%s %d", channel, user_name, muted);
};

static void byte_rtc_on_user_mute_video(byte_rtc_engine_t engine, const char* channel, const char* user_name, int muted)
{
    QM_LOGI("1", "remote user mute video  %s:%s %d", channel, user_name, muted);
};

static __attribute__((unused)) void byte_rtc_on_connection_lost(byte_rtc_engine_t engine, const char* channel)
{
    QM_LOGI("1", "connection Lost  %s", channel);
};

static void byte_rtc_on_room_error(byte_rtc_engine_t engine, const char* channel, int code, const char* msg)
{
    QM_LOGE("1", "error occur %s %d %s", channel, code, msg ? msg : "UnKown");
};

// remote audio
static void byte_rtc_on_audio_data(byte_rtc_engine_t engine, const char* channel, const char*  uid , uint16_t sent_ts,
                                   audio_codec_type_e codec, const void* data_ptr, size_t data_len)
{
    pipe_player_state_e state;
    player_pipeline_get_state(s_volc_rtc.player_pipeline, &state);
    if (state == PIPE_STATE_IDLE) {
        return;
    }

    frame_package_t frame = { 0 };

    frame.frame_type = AUDIO_DATA_MSG;
    frame.frame_ptr = qm_calloc(1, data_len);
    memcpy(frame.frame_ptr, data_ptr, data_len);
    frame.frame_len = data_len;

    if (xQueueSend(s_volc_rtc.frame_q, &frame, pdMS_TO_TICKS(10)) != pdPASS) {
        QM_LOGW("1", "audio frame queue full");
        qm_free(frame.frame_ptr);
        frame.frame_ptr = NULL;
    }

}

// remote video
static void byte_rtc_on_video_data(byte_rtc_engine_t engine, const char*  channel, const char* uid, uint16_t sent_ts,
                                    video_data_type_e codec, int is_key_frame,
                                    const void * data_ptr, size_t data_len){ }

// remote message
void on_message_received(byte_rtc_engine_t engine, const char*  room, const char* uid, const uint8_t* message, int size, bool binary)
{
    QM_LOGI("1", "on_message_received uid: %s, message size: %d, message: %s", uid, message, size);
    qm_audio_recorder_waketimer_reset(s_volc_rtc.recorder_engine);
    
    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_MSG_ID , (void *)message, size);
    QM_LOGI("1", "on_message_received end");
}

static void on_target_bitrate_changed(byte_rtc_engine_t engine,const char * room, uint32_t target_bps)
{
    // QM_LOGI("1", "target_bitrate - room:%s, target_bps:%d", room, (int)target_bps);
}

static void on_key_frame_gen_req(byte_rtc_engine_t engine, const char*  channel, const char*  uid) {}
// byte rtc lite callbacks end.

static void audio_pull_data_process(char *ptr, int len)
{
    char *data_ptr = ptr;
    int data_len = len;
    /* Since OPUS is in VBR mode, it needs to be packaged into a length + data format first then to decoder*/
#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)

#define frame_length_prefix (2)
    if (s_volc_rtc.opus_data_cache_len + frame_length_prefix < len) {
        s_volc_rtc.opus_data_cache = qm_realloc(s_volc_rtc.opus_data_cache, len + frame_length_prefix);
        s_volc_rtc.opus_data_cache_len = len;
    }
    s_volc_rtc.opus_data_cache[0] = (len >> 8) & 0xFF;
    s_volc_rtc.opus_data_cache[1] = len & 0xFF;
    memcpy(s_volc_rtc.opus_data_cache + frame_length_prefix, ptr, len);
    data_ptr = s_volc_rtc.opus_data_cache;
    data_len += frame_length_prefix;
#else
    data_ptr = ptr;
    data_len = len;
#endif // CONFIG_AUDIO_SUPPORT_OPUS_DECODER
    raw_stream_write(player_pipeline_get_raw_write(s_volc_rtc.player_pipeline), data_ptr, data_len);
}

static void byte_rtc_engine_create(void)
{

    httpclient_t http = {0};              /* http client */
    httpclient_data_t http_data = {0};    /* http client data */
    char *post_data = "{\"sn\":\"FC3344556680\",\"pid\":10069,\"timestamp\":\"1725270504\",\"signature\":\"dcd1b164fe580d709ec39d7cd93aafbbbc643e35\",\"nonce\":\"1107437088\"}";

    char *rsp_payload = (char *)qm_calloc(1, RSP_PAYLOAD_MAX_STR_LEN);
    if(rsp_payload == NULL){
        return ;
    }

    http_data.post_buf = post_data;
    http_data.post_buf_len = strlen(post_data);
    http_data.response_buf = rsp_payload;
    http_data.post_content_type = "application/json";
    http_data.response_buf_len = RSP_PAYLOAD_MAX_STR_LEN;

    while (1)
    {
        int ret = http_client_common(&http, "http://aiAgentPlatform.xjiangiot.com/cozeApi/rtc/getRTCLogin", 9082, NULL, HTTPCLIENT_POST, 5 * 1000, &http_data);
        if(ret == QM_EOK){
            break;
        }

        qm_msleep(500);
    }

    http_client_close(&http);

    QM_LOGD("1", "RECV_HTTP:%s", http_data.response_buf);

    cJSON *tmp = NULL;
    cJSON *object = NULL;
    cJSON *root = cJSON_ParseWithLength((char*)http_data.response_buf, http_data.response_content_len);
    if(root == NULL){
        QM_LOGE("1", "subdev list parse error");
        return ;
    }

    object = cJSON_GetObjectItem(root, "result");
    if(object == NULL){
        return ;
    }

    tmp = cJSON_GetObjectItem(object, "appID");
    if(tmp == NULL){
        return ;
    }
    memset(s_volc_rtc.app_id, 0, PRODTST_DEVICE_ID_MAX_LEN + 1);
    memcpy(s_volc_rtc.app_id, tmp->valuestring, strlen(tmp->valuestring));

    tmp = cJSON_GetObjectItem(object, "roomID");
    if(tmp == NULL){
        return ;
    }
    memset(s_volc_rtc.room_id, 0, PRODTST_DEVICE_ID_MAX_LEN + 1);
    memcpy(s_volc_rtc.room_id, tmp->valuestring, strlen(tmp->valuestring));

    tmp = cJSON_GetObjectItem(object, "userID");
    if(tmp == NULL){
        return ;
    }
    memset(s_volc_rtc.uid, 0, PRODTST_DEVICE_ID_MAX_LEN + 1);
    memcpy(s_volc_rtc.uid, tmp->valuestring, strlen(tmp->valuestring));

    tmp = cJSON_GetObjectItem(object, "token");
    if(tmp == NULL){
        return ;
    }
    memset(s_volc_rtc.token, 0, PRODTST_DEVICE_SECRET_MAX_LEN + 1);
    memcpy(s_volc_rtc.token, tmp->valuestring, strlen(tmp->valuestring));

    tmp = cJSON_GetObjectItem(object, "botID");
    if(tmp == NULL){
        return ;
    }
    memset(s_volc_rtc.bot_id, 0, PRODTST_DEVICE_ID_MAX_LEN + 1);
    memcpy(s_volc_rtc.bot_id, tmp->valuestring, strlen(tmp->valuestring));

    cJSON_Delete(root);
    qm_free(rsp_payload);
    rsp_payload = NULL;

    QM_LOGI("1", "app_id: %s, room_id: %s, uid: %s, token: %s",
            s_volc_rtc.app_id, s_volc_rtc.room_id, s_volc_rtc.uid, s_volc_rtc.token);

#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
    s_volc_rtc.opus_data_cache_len = DEALULT_OPUS_DATA_CHACHE_SIZE;
    s_volc_rtc.opus_data_cache = qm_calloc(1, s_volc_rtc.opus_data_cache_len);
#endif // CONFIG_AUDIO_SUPPORT_OPUS_DECODER

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
        .on_target_bitrate_changed  =   on_target_bitrate_changed,
    };
    s_volc_rtc.engine = byte_rtc_create(s_volc_rtc.app_id, &handler);

    byte_rtc_set_log_level(s_volc_rtc.engine, BYTE_RTC_LOG_LEVEL_ERROR);
#ifdef RTC_TEST_ENV
    byte_rtc_set_params(s_volc_rtc.engine, "{\"env\":2}"); // test env
#endif
    byte_rtc_set_params(s_volc_rtc.engine, "{\"debug\":{\"log_to_console\":1}}"); 
    byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"pinned_to_core\":1}}}");
    byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"priority\":5}}}");
    byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"stack_size\":6144}}}");

    // byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"thread\":{\"stack_size\":6144}}}");
    // byte_rtc_set_params(s_volc_rtc.engine,"{\"rtc\":{\"network\":{\"audio_jitter_buffer_target_delay_min_ms\":200}}}");

    byte_rtc_init(s_volc_rtc.engine);
    byte_rtc_set_audio_codec(s_volc_rtc.engine, AUDIO_CODEC_TYPE_OPUS);

    frame_package_t frame = { 0 };

    frame.frame_type = JOIN_ROOM_START;
    if (xQueueSend(s_volc_rtc.frame_q, &frame, QM_WAIT_FOREVER) != pdPASS) {
        QM_LOGW("1", "audio frame queue full");
    }
}

static void volc_rtc_join_room(void)
{
    QM_LOGI("1", "start join room\n");

    byte_rtc_room_options_t options = { 0 };
    options.auto_subscribe_audio = 1;
    options.auto_subscribe_video = 0;
    byte_rtc_join_room(s_volc_rtc.engine, s_volc_rtc.room_id, s_volc_rtc.uid, s_volc_rtc.token, &options);
}

static void volc_rtc_join_room_success(void)
{
    QM_LOGI("1", "join room success, update msg\n");
    
    char *post_msg = "{\"id\":\"1\",\"event_type\":\"session.update\",\"data\":{\"chat_config\":{\"parameters\":{\"pid\":10069,\"did\":27457517,\"sn\":\"FC3344556680\"}},\"turn_detection\":{\"prefix_padding_ms\":200,\"silence_duration_ms\":200}}}";

    // char *post_msg = "{\"id\":\"1\",\"event_type\":\"session.update\",\"data\":{\"longest_silence_ms\":86400000,\"chat_config\":{\"parameters\":{\"pid\":10069,\"did\":27457517,\"sn\":\"FC3344556680\"}},\"turn_detection\":{\"prefix_padding_ms\":200,\"silence_duration_ms\":200}}}";
    int64_t ret = byte_rtc_rts_send_message(
        s_volc_rtc.engine, s_volc_rtc.room_id, "7490430487925604364",
        post_msg, strlen(post_msg), 0, RTS_MESSAGE_RELIABLE);
    if(ret >= 0){
        QM_LOGI("1", "post_msg %s\n", post_msg);
    }

}

static void audio_tone_player_event_cb(audio_element_status_t evt)
{
    if (evt == AEL_STATUS_STATE_FINISHED) {

        if(s_volc_rtc.rec_type != AUDIO_REC_WAKEUP_END){
            player_pipeline_run(s_volc_rtc.player_pipeline);
        }

    }
}

static void dispatcher_audio_play(void *arg)
{
    audio_tone_play((char *)arg);
};

static qm_err_t rec_engine_cb(audio_rec_evt_t *event, void *user_data)
{
    s_volc_rtc.rec_type = event->type;

    if (AUDIO_REC_WAKEUP_START == event->type) {

        QM_LOGI("1", "rec_engine_cb - AUDIO_REC_WAKEUP_START");
        player_pipeline_stop(s_volc_rtc.player_pipeline);
        qm_post_delayed_action(&s_volc_rtc.post_work, dispatcher_audio_play, "spiffs://spiffs/dingding.wav", 0);
        xEventGroupSetBits(s_volc_rtc.event, WAKEUP_REC_READING);

    } else if (AUDIO_REC_VAD_START == event->type) {
    } else if (AUDIO_REC_VAD_END == event->type) {
    } else if (AUDIO_REC_WAKEUP_END == event->type) {
        xEventGroupClearBits(s_volc_rtc.event, WAKEUP_REC_READING);
        player_pipeline_stop(s_volc_rtc.player_pipeline);

        qm_post_delayed_action(&s_volc_rtc.post_work, dispatcher_audio_play, "spiffs://spiffs/timeout.wav", 0);

        QM_LOGI("1", "rec_engine_cb - AUDIO_REC_WAKEUP_END");

    } 

    return ESP_OK;
}

static void voice_read_task(void *args)
{
    const int voice_data_read_sz = recorder_pipeline_get_default_read_size(s_volc_rtc.record_pipeline);
    uint8_t *voice_data = qm_calloc(1, voice_data_read_sz);

#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
    audio_frame_info_t audio_frame_info = {.data_type = AUDIO_DATA_TYPE_OPUS};
#endif // CONFIG_AUDIO_SUPPORT_OPUS_DECODER

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(s_volc_rtc.event, WAKEUP_REC_READING , false, true, portMAX_DELAY);
        if (bits & (WAKEUP_REC_READING)) {
            int ret = qm_audio_recorder_data_read(s_volc_rtc.recorder_engine, voice_data, voice_data_read_sz, portMAX_DELAY);
            if (ret == 0 || ret == -1) {
                // xEventGroupClearBits(s_volc_rtc.event,  WAKEUP_REC_READING);
            } else {
                byte_rtc_send_audio_data(s_volc_rtc.engine, s_volc_rtc.room_id, voice_data, voice_data_read_sz, &audio_frame_info);
            }
        }
    }
}

static void audio_data_process_task(void *args)
{
    frame_package_t frame = { 0 };
    while (1) {
        xQueueReceive(s_volc_rtc.frame_q, &frame, portMAX_DELAY);
        QM_LOGD("1", "frame_type",frame.frame_type);
        switch (frame.frame_type)
        {
            case CREATE_RTC_HANDLE:
            {
                byte_rtc_engine_create();
            }
            break;

            case JOIN_ROOM_START:
            {
                volc_rtc_join_room();
            }
            break;

            case JOIN_ROOM_SUCCESS:
            {
                // volc_rtc_join_room_success();
            }
            break;

            case AUDIO_DATA_MSG:
                // if (frame.frame_ptr) {
                //     audio_pull_data_process(frame.frame_ptr, frame.frame_len);
                //     qm_free(frame.frame_ptr);
                // }
            break;
            
            default:
                break;
        }

    }
}

qm_err_t bsp_audio_init(qm_audio_sr_cmd_table_t *command_str, int command_num)
{
    pipe_record_engine_params_t engine_params = {0};

    audio_tone_init(audio_tone_player_event_cb);

    s_volc_rtc.event = xEventGroupCreate();
    s_volc_rtc.frame_q = xQueueCreate(50, sizeof(frame_package_t));

    s_volc_rtc.player_pipeline = player_pipeline_open();
    s_volc_rtc.record_pipeline = recorder_pipeline_open();

    engine_params.cb = rec_engine_cb;
    engine_params.command_str = command_str;
    engine_params.command_num = command_num;

    s_volc_rtc.recorder_engine = audio_record_engine_init(s_volc_rtc.record_pipeline, &engine_params);

    player_pipeline_run(s_volc_rtc.player_pipeline);
    recorder_pipeline_run(s_volc_rtc.record_pipeline);

    qm_event_register(CONFIG_AUDIO_MAIN_EVENT_ID, qm_audio_event_handler, NULL);

    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 6 * 1024, 5, true, 0);
    audio_thread_create(NULL, "audio_data_proc", audio_data_process_task, (void *)NULL, 6 * 1024, 10, true, 0);

    return QM_EOK;
}
