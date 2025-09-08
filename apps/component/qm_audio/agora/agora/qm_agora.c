#include "qm.h"
#include "qm_work.h"
#include "qm_event.h"

#include "qm_spec_api.h"
#include "qm_utils_list.h"

#include "qm_kv.h"
#include "qm_utils_base64.h"

#include "audio_thread.h"
#include "agora_rtc_api.h"
#include "qm_agora.h"
#include "qm_agora_gen.h"
#include "qm_agora_stream.h"
#include "qm_utils_string.h"

#define LOG_TAG "AGORA"

#define CONFIG_ENABLE_AUDIO_MIXING 0

// #define CONFIG_STATIC_AGORA_DATA

/* Application ID */
#define CONFIG_AGORA_APP_ID "9942a7948c3f4c4f8970d54ef6adfb24" // Please replace with your own APP ID

#define DEFAULT_TOKEN "007eJxTYFj8WE3tPXNY8i63FX3PTdoSJcQvujv+ETe60v42Jp71jJYCg1FqaqqJhalFmnmKpYmpoUWiiaGFaap5kqVFkrmxaZK5qfW+jIZARoblBwOZGBkgEMRnYzAzNDAyMmZgAAAeGB1w"
// #define DEFAULT_TOKEN NULL
#define DEFAULT_CHANNEL_NAME "610223"
#define DEFAULT_USER_ID 0 // sdk will automatically assign a random user id

#define DEFAULT_SDK_LOG_PATH "io.agora.rtc_sdk"

typedef struct{
    uint32_t uid;
    int payload_len;
    uint8_t *payload;
}qm_agora_frame_t;

typedef struct {
    bool                          audio_enable;
    bool                          video_enable;
    bool                          domain_limit;
    bool                          connect;
    bool                          joined;
    uint8_t                       agora_init_flag;

    qm_task_t                     binary_task;
    qm_sem_t                      binary_sem;
    qm_list_t                     binary_list;
    qm_mutex_t                    binary_lock;

    qm_agora_gen_handle_t         *agora_gen_handle;
    connection_id_t               conn_id;
    connection_info_t             conn_info;

    int                           duration;

    qm_agora_audio_event_cb_t     audio_cb;
    qm_agora_event_cb_t           event_cb;
    void                          *user_data;

} qm_agora_handle_t;

static qm_agora_handle_t qm_agora_handle = {0};

int qm_agora_send_data(const void *data, size_t len)
{
    audio_frame_info_t info = { 0 };
    if(qm_agora_handle.joined == false) {
        return QM_ERROR;
    }

    info.data_type = AUDIO_DATA_TYPE_OPUS;

    int rval = agora_rtc_send_audio_data(qm_agora_handle.conn_id, data, len, &info);
    if (rval < 0) {
        QM_LOGD(TAG, "Failed to send audio data, reason: %s\n", agora_rtc_err_2_str(rval));
    }
    return rval;
}

static void qm_agora_audio_process_task(void *args)
{
    int list_len = 0;
    qm_list_node_t *node = NULL;
    qm_agora_frame_t *agora_node = NULL;
    qm_agora_handle_t *audio_ctx = (qm_agora_handle_t *)args;
    while (1) {
        qm_sem_wait(&audio_ctx->binary_sem, QM_WAIT_FOREVER);

        qm_mutex_lock(&audio_ctx->binary_lock, QM_WAIT_FOREVER);
        node = qm_list_lpop(&audio_ctx->binary_list);
        qm_mutex_unlock(&audio_ctx->binary_lock);

        if(node == NULL) {
            continue;
        }

        agora_node = (qm_agora_frame_t *)qm_list_node_val_get(node);
        if (agora_node == NULL || agora_node->payload == NULL) {
            continue;
        }

        if(audio_ctx->audio_cb){
            audio_ctx->audio_cb(agora_node->payload, agora_node->payload_len, audio_ctx->user_data);
        }
    
        qm_free(agora_node->payload);
        qm_list_node_destroy(node);

        qm_mutex_lock(&audio_ctx->binary_lock, QM_WAIT_FOREVER);
        list_len = qm_list_len_get(&audio_ctx->binary_list);
        qm_mutex_unlock(&audio_ctx->binary_lock);

        if(list_len){
            qm_sem_signal(&audio_ctx->binary_sem);
        }
    }
}

static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed)
{
    agora_rtc_get_connection_info(conn_id, &qm_agora_handle.conn_info);
    QM_LOGD(LOG_TAG, "[conn-%lu] Join the channel %s successfully, uid %lu elapsed %d ms\n", conn_id, qm_agora_handle.conn_info.channel_name, uid, elapsed);

    qm_agora_handle.joined = QM_TRUE;
    if(qm_agora_handle.event_cb) {
        qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_JOIN_CHANNEL, qm_agora_handle.user_data);
    }
}

static void __on_connection_lost(connection_id_t conn_id)
{
    QM_LOGD(LOG_TAG, "[conn-%lu] Lost connection from the channel\n", conn_id);
    qm_agora_handle.connect = QM_FALSE;
    qm_agora_handle.joined = QM_FALSE;
    qm_agora_gen_free(qm_agora_handle.agora_gen_handle);
    if(qm_agora_handle.event_cb) {
        qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_CONNECT_LOST, qm_agora_handle.user_data);
    }
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    QM_LOGD(LOG_TAG, "[conn-%lu] Rejoin the channel successfully, uid %lu elapsed %d ms\n", conn_id, uid, elapsed_ms);
    qm_agora_handle.joined = QM_TRUE;
    if(qm_agora_handle.event_cb) {
        qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_JOIN_CHANNEL, qm_agora_handle.user_data);
    }
}

static void __on_user_joined(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    QM_LOGD(LOG_TAG, "[conn-%lu] Remote user \"%lu\" has joined the channel, elapsed %d ms\n", conn_id, uid, elapsed_ms);
    if(uid != 100) {
        return;
    }
    if(qm_agora_handle.event_cb) {
        qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_AGENT_JOINED, qm_agora_handle.user_data);
    }
}

static void __on_user_offline(connection_id_t conn_id, uint32_t uid, int reason)
{
    QM_LOGD(LOG_TAG, "[conn-%lu] Remote user \"%lu\" has left the channel, reason %d\n", conn_id, uid, reason);
    if(uid != 100) {
        return;
    }
    if(qm_agora_handle.event_cb) {
        qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_AGENT_LEAVE, qm_agora_handle.user_data);
    }
}

static void __on_user_mute_audio(connection_id_t conn_id, uint32_t uid, bool muted)
{
    QM_LOGD(LOG_TAG, "[conn-%lu] audio: uid=%lu muted=%d\n", conn_id, uid, muted);
    if(uid != 100) {
        return;
    }
    if(qm_agora_handle.event_cb) {
        if(muted) {
            qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_AUDIO_MUTE, qm_agora_handle.user_data);
        } else {
            qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_AUDIO_UNMUTE, qm_agora_handle.user_data);
        }
    }
}

static void __on_error(connection_id_t conn_id, int code, const char *msg)
{
    if (code == ERR_VIDEO_SEND_OVER_BANDWIDTH_LIMIT) {
        QM_LOGE(LOG_TAG, "Not enough uplink bandwdith. Error msg \"%s\"\n", msg);
        return;
    }

    if (code == ERR_INVALID_APP_ID) {
        QM_LOGE(LOG_TAG, "Invalid App ID. Please double check. Error msg \"%s\"\n", msg);
    } else if (code == ERR_INVALID_CHANNEL_NAME) {
        QM_LOGE(LOG_TAG, "Invalid channel name. Please double check. Error msg \"%s\"\n", msg);
    } else if (code == ERR_INVALID_TOKEN || code == ERR_TOKEN_EXPIRED) {
        QM_LOGE(LOG_TAG, "Invalid token. Please double check. Error msg \"%s\"\n", msg);
    } else if (code == ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY) {
        QM_LOGE(LOG_TAG, "Dynamic token is enabled but is not provided. Error msg \"%s\"\n", msg);
    } else {
        QM_LOGE(LOG_TAG, "Error %d is captured. Error msg \"%s\"\n", code, msg);
    }

    if(qm_agora_handle.event_cb) {
        qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_ERROR, qm_agora_handle.user_data);
    }
}

#if CONFIG_ENABLE_AUDIO_MIXING
static void __on_mixed_audio_data(connection_id_t conn_id, const void *data, size_t len,
                                  const audio_frame_info_t *info_ptr)
{
    QM_LOGD("1", "[conn-%u] on_mixed_audio_data, data_type %d, len %zu\n", conn_id, info_ptr->data_type, len);

    qm_agora_frame_t *agora_node = NULL;
    qm_list_node_t *node = qm_list_node_extra_new(sizeof(qm_agora_frame_t));
    if(node == NULL){
        return ;
    }
    agora_node = (qm_agora_frame_t *)qm_list_node_val_get(node);

    agora_node->payload_len = len;
    agora_node->payload = (uint8_t *)qm_calloc(1, agora_node->payload_len);
    if(agora_node->payload == NULL){
        qm_list_node_destroy(node);
        return;
    }
    memset(agora_node->payload, 0, agora_node->payload_len);
    memcpy(agora_node->payload, data, agora_node->payload_len);
    
    qm_mutex_lock(&bsp_audio_handle.binary_lock, QM_WAIT_FOREVER);
    qm_list_rpush(&bsp_audio_handle.binary_list, node);
    qm_mutex_unlock(&bsp_audio_handle.binary_lock);

    qm_sem_signal(&bsp_audio_handle.binary_sem);


}
#else
static void __on_audio_data(connection_id_t conn_id, const uint32_t uid, uint16_t sent_ts, const void *data, size_t len,
                            const audio_frame_info_t *info_ptr)
{
    if(len == 0 || data == NULL) {
        return;
    }
    if(uid != 100) {
        return;
    }
    // QM_LOGD("1", "[conn-%u] on_audio_data, uid %u sent_ts %u data_type %d, len %d\n", conn_id, uid, sent_ts,
    //      info_ptr->data_type, len);
    qm_agora_frame_t *agora_node = NULL;
    qm_list_node_t *node = qm_list_node_extra_new(sizeof(qm_agora_frame_t));
    if(node == NULL){
        return ;
    }
    agora_node = (qm_agora_frame_t *)qm_list_node_val_get(node);

    agora_node->payload_len = len;
    agora_node->uid = uid;
    agora_node->payload = (uint8_t *)qm_calloc(1, agora_node->payload_len);
    if(agora_node->payload == NULL){
        qm_list_node_destroy(node);
        return;
    }
    memset(agora_node->payload, 0, agora_node->payload_len);
    memcpy(agora_node->payload, data, agora_node->payload_len);

    qm_mutex_lock(&qm_agora_handle.binary_lock, QM_WAIT_FOREVER);
    qm_list_rpush(&qm_agora_handle.binary_list, node);
    qm_mutex_unlock(&qm_agora_handle.binary_lock);

    qm_sem_signal(&qm_agora_handle.binary_sem);

}
#endif //#ifdef CONFIG_ENABLE_AUDIO_MIXING

static void app_init_event_handler(agora_rtc_event_handler_t *event_handler)
{
  event_handler->on_join_channel_success = __on_join_channel_success;
  event_handler->on_connection_lost = __on_connection_lost;
  event_handler->on_rejoin_channel_success = __on_rejoin_channel_success;
  event_handler->on_user_joined = __on_user_joined;
  event_handler->on_user_offline = __on_user_offline;
  event_handler->on_user_mute_audio = __on_user_mute_audio;
  event_handler->on_stream_message = __on_stream_message;
  event_handler->on_error = __on_error;

#if CONFIG_ENABLE_AUDIO_MIXING
  event_handler->on_mixed_audio_data = __on_mixed_audio_data;
#else
  event_handler->on_audio_data = __on_audio_data;
#endif
}

int qm_agora_join(void)
{
    uint32_t uid = 0;
    rtc_channel_options_t channel_options = { 0 };

    if(qm_agora_handle.agora_init_flag == QM_FALSE) {
        return QM_ERROR;
    }

    if(qm_agora_handle.connect == QM_FALSE) {
        return QM_EOK;
    }

    channel_options.auto_subscribe_audio = qm_agora_handle.audio_enable;
    channel_options.auto_subscribe_video = qm_agora_handle.video_enable;

#if CONFIG_ENABLE_AUDIO_MIXING
    channel_options.enable_audio_mixer = 1;
    channel_options.enable_audio_jitter_buffer = 1;
#endif
    channel_options.audio_codec_opt.pcm_duration  = qm_agora_handle.duration;

    uid = int_str_to_num(qm_agora_handle.agora_gen_handle->uid, qm_agora_handle.agora_gen_handle->uid_len);

#ifdef CONFIG_STATIC_AGORA_DATA
    return agora_rtc_join_channel(qm_agora_handle.conn_id, DEFAULT_CHANNEL_NAME, uid, DEFAULT_TOKEN, &channel_options);
#else    
    return agora_rtc_join_channel(qm_agora_handle.conn_id, 
                                    qm_agora_handle.agora_gen_handle->channelName, 
                                    uid, 
                                    qm_agora_handle.agora_gen_handle->token, 
                                    &channel_options);
#endif
}

int qm_agora_leave(void)
{
    int rval;
    if(qm_agora_handle.agora_init_flag == QM_FALSE) {
        return QM_ERROR;
    }

    if(qm_agora_handle.connect == QM_FALSE) {
        return QM_EOK;
    }
    
    rval =  agora_rtc_leave_channel(qm_agora_handle.conn_id);
    if (rval < 0) {
        QM_LOGE(LOG_TAG, "Failed to leave the channel, reason: %s\n", agora_rtc_err_2_str(rval));
    } else {
        QM_LOGD(LOG_TAG, "Leave the channel successfully.\n");
        qm_agora_handle.joined = QM_FALSE;
        if(qm_agora_handle.event_cb) {
            qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_LEAVE_CHANNEL, qm_agora_handle.user_data);
        }
    }
    return rval;
}

int qm_agora_connect(void)
{
    int rval;
    if(qm_agora_handle.agora_init_flag == QM_FALSE) {
        rval = qm_agora_gen_start(qm_agora_handle.agora_gen_handle);
        if (rval != QM_EOK) {
            return rval;
        }

        agora_rtc_event_handler_t event_handler = { 0 };
        app_init_event_handler(&event_handler);

        rtc_service_option_t service_opt = { 0 };
        service_opt.area_code            = AREA_CODE_GLOB;
        service_opt.log_cfg.log_disable  = false;
        service_opt.log_cfg.log_level    = RTC_LOG_WARNING;
        service_opt.log_cfg.log_path     = DEFAULT_SDK_LOG_PATH;
        service_opt.license_value[0]     = '\0';
        service_opt.domain_limit         = qm_agora_handle.domain_limit;

#ifdef CONFIG_STATIC_AGORA_DATA
        rval = agora_rtc_init(CONFIG_AGORA_APP_ID, &event_handler, &service_opt);
#else
        rval = agora_rtc_init(qm_agora_handle.agora_gen_handle->appId, &event_handler, &service_opt);
#endif
        if (rval < 0) {
            QM_LOGE(LOG_TAG, "Failed to initialize Agora sdk, license %p, reason: %s\n", service_opt.license_value, agora_rtc_err_2_str(rval));
            return QM_ERROR;
        }
        QM_LOGD(LOG_TAG, "~~~~~agora_rtc_init success~~~~\r\n");
        qm_agora_handle.agora_init_flag = QM_TRUE;
        if(qm_agora_handle.event_cb) {
            qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_INIT_SUCCESS, qm_agora_handle.user_data);
        }
    }
    if(qm_agora_handle.connect == QM_TRUE) {
        QM_LOGW(LOG_TAG, "agora_rtc is connected!");
        return QM_EOK;
    }

    rval = agora_rtc_create_connection(&qm_agora_handle.conn_id);
    if (rval < 0) {
        qm_agora_handle.connect = QM_FALSE;
        QM_LOGE(LOG_TAG, "Failed to create connection, reason: %s\n", agora_rtc_err_2_str(rval));
    } else {
        QM_LOGD(LOG_TAG, "~~~~~agora_rtc_create_connection~~~~\r\n");
        qm_agora_handle.connect = QM_TRUE;
        if(qm_agora_handle.event_cb) {
            qm_agora_handle.event_cb(QM_AGORA_EVENT_TYPE_CONNECTED, qm_agora_handle.user_data);
        }
    }
    return rval;
}

int qm_agora_disconnect(void)
{
    int rval;
    if(qm_agora_handle.agora_init_flag == QM_FALSE) {
        return QM_ERROR;
    }
    if(qm_agora_handle.connect == QM_FALSE) {
        return QM_EOK;
    }
    if(qm_agora_handle.joined) {
        qm_agora_leave();
    }
    rval = agora_rtc_destroy_connection(qm_agora_handle.conn_id);
    if(rval < 0) {
        QM_LOGE(LOG_TAG, "Failed to destroy connection, reason: %s\n", agora_rtc_err_2_str(rval));
    } else {
        QM_LOGD(LOG_TAG, "Destroy connection successfully.\n");
        qm_agora_handle.connect = QM_FALSE;
    }
    return rval;
}

int qm_agora_init(qm_agora_cfg_t cfg)
{
    qm_sem_new(&qm_agora_handle.binary_sem, 0);
    qm_mutex_new(&qm_agora_handle.binary_lock);
    
    qm_agora_handle.duration = cfg.duration;
    qm_agora_handle.audio_enable = cfg.audio_enable;
    qm_agora_handle.video_enable = cfg.video_enable;
    qm_agora_handle.domain_limit = cfg.domain_limit;
    qm_agora_handle.audio_cb = cfg.audio_cb;
    qm_agora_handle.event_cb = cfg.event_cb;
    qm_agora_handle.user_data = cfg.user_data;

    qm_agora_stream_init();
    qm_agora_stream_register(cfg.stream_cb, cfg.user_data);

    qm_agora_handle.agora_gen_handle = qm_agora_gen_init();
    if(qm_agora_handle.agora_gen_handle == NULL) {
        QM_LOGE(LOG_TAG, "agora init fail");
        return QM_ENOMEM;
    }
    audio_thread_create(NULL, "qm_agora_audio_process_task", qm_agora_audio_process_task, &qm_agora_handle, 20 * 1024, 5, true, 0);

    return QM_EOK;
}

