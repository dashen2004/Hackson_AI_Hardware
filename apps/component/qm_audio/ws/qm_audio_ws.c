
#include "qm.h"
#include "qm_audio_ws.h"
#include "qm_audio_common.h"

#include "qm_ble_gap.h"
#include "qm_utils_list.h"
#include "qm_work.h"
#include "qm_event.h"

#include "cJSON.h"

#include "esp_websocket_client.h"


#ifndef CONFIG_QM_AUDIO_WS_MAX_SIZE     
#define CONFIG_QM_AUDIO_WS_MAX_SIZE                   (32)
#endif

#define LOG_TAG     "ai_audio"

#define ID_PARAMS               "id"
#define PID_PARAMS              "pid"
#define DID_PARAMS              "did"
#define SN_PARAMS               "sn"
#define VERSION_PARAMS          "version"

#define METHOD_PARAMS           "method"
#define AUDIO_INFO_PARAMS       "audioInfo"
#define FORMAT_PARAMS           "format"
#define RATE_PARAMS             "rate"
#define BIT_PARAMS              "bits"
#define CHANNEL_PARAMS          "channel"
#define FRAME_DUR_PARAMS        "frameDuration"
#define STATE_PARAMS            "state"
#define MODE_PARAMS             "mode"
#define TEXT_PARAMS             "text"
#define REASON_PARAMS            "reason"
#define VERSION_PARAMS           "version"
#define EMOTION_PARAMS           "emotion"
#define ICON_PARAMS              "icon"
#define COMMAND_PARAMS           "command"
#define PARAMS_PARAMS           "params"
#define URI_PARAMS              "uri"
#define SIZE_PARAMS             "size"
#define MD5_PARAMS              "md5"

#define HELLO_PARAMS            "hello"
#define HELLO_REPLY_PARAMS      "helloReply"
#define STT_PARAMS              "stt"
#define TTS_PARAMS              "tts"
#define LLM_PARAMS              "llm"
#define CMD_PARAMS              "cmd"
#define LISTEN_PARAMS           "listen"
#define ABORT_PARAMS            "abort"
#define OTA_VER_PARAMS           "otaVersion"
#define OTA_UPGRADE_PARAMS       "otaUpgrade"

typedef struct{
    uint8_t binary;
    int buff_len;
    int payload_len;
    uint8_t *payload;
}qm_audio_ws_frame_t;

typedef struct {
    
    // char url[CONFIG_QM_AUDIO_WS_MAX_SIZE];

    char sn[CONFIG_QM_AUDIO_WS_MAX_SIZE];
    char id[37];

    qm_task_t                     binary_task;
    qm_queue_t                    binary_queue;
#if 0
    qm_task_t                     send_task;
    qm_queue_t                    send_queue;
#endif
    qm_audio_ws_data_cb_t         data_cb; 
    qm_audio_ws_recv_event_cb_t   event_cb;
    void                        *user_data;         /*!< Pointer to user data (optional) */

    int                           pid;              /*!< 产品pid */
    int                           did;              /*!< 设备did 可选, -1则无效*/
    uint32_t                      connect_timeout;  /*!< 连接服务端超时时间，默认为10s, 单位为秒*/

    qm_audio_ws_fmt_type_t        fmt_type;          /*!< 音频格式*/
    uint16_t                      rate;              /*!< 音频采样率*/
    uint16_t                      bits;              /*!< 音频采样位数*/
    uint16_t                      channel;           /*!< 音频声道数*/
    uint16_t                      frame_duration_ms; /*!< 音频片段长度，单位ms，默认值60*/
    
    qm_mutex_t audio_lock;

    esp_websocket_client_handle_t client;

}qm_ai_audio_ctx_t;

typedef struct{
    char *method;
    void (*recv_handler)(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
}qm_ai_audio_params_table_t;

static char *cjson_str_get(cJSON *cjson, char *key);
static int cjson_int_get(cJSON *cjson, char *key, int *data);

static void hello_reply_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void stt_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void tts_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void llm_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void cmd_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void ota_upgrade_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);

static qm_ai_audio_params_table_t g_param_table[] = 
{
    {HELLO_REPLY_PARAMS,    hello_reply_recv_handler},
    {STT_PARAMS,            stt_recv_handler},
    {TTS_PARAMS,            tts_recv_handler},
    {LLM_PARAMS,            llm_recv_handler},
    {CMD_PARAMS,            cmd_recv_handler},
    {OTA_UPGRADE_PARAMS,    ota_upgrade_recv_handler},
};

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        QM_LOGE(LOG_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void cjson_recv_handler(qm_ai_audio_ctx_t *auido_ctx, uint8_t *data, int len)
{

    cJSON *root = cJSON_ParseWithLength((char*)data , (size_t)len);
    if(root == NULL){
        QM_LOGE(LOG_TAG, "CJSON parse error");
        return ;
    }

    char *method = cjson_str_get(root, METHOD_PARAMS);
    if(method == NULL){
        QM_LOGE(LOG_TAG, "method NULL!!!");
        return ;
    }
    
    for (int i = 0; i < QM_ARRAY_SIZE(g_param_table); i++)
    {
        if(!strncmp(g_param_table[i].method, method, strlen(g_param_table[i].method))){
            g_param_table[i].recv_handler(auido_ctx, root);
            break;
        }
    }

    cJSON_Delete(root);
}


static void qm_audio_event_handler(qm_input_event_t *input_event, void *arg)
{   
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_frame_t *ws_frame = NULL;
    qm_ai_audio_ctx_t *audio_dev = (qm_ai_audio_ctx_t *)arg;

    switch(input_event->sub_event){

        case QM_AUDIO_EVENT_TYPE_WS_CONNECT:
        
            QM_LOGI(LOG_TAG, "QM_AUDIO_EVENT_TYPE_WS_CONNECT");

            qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_SEND_HELLO, NULL, 0);

            recv.recv_type = QM_AUDIO_WS_RECV_TYPE_CONNECT;
            if(audio_dev->event_cb){
                audio_dev->event_cb(&recv, audio_dev->user_data);
            }

        break;

        case QM_AUDIO_EVENT_TYPE_WS_DISCONNECT:
        
            QM_LOGI(LOG_TAG, "QM_AUDIO_EVENT_TYPE_WS_DISCONNECT");

            recv.recv_type = QM_AUDIO_WS_RECV_TYPE_DISCONNECT;
            if(audio_dev->event_cb){
                audio_dev->event_cb(&recv, audio_dev->user_data);
            }

        break;

        case QM_AUDIO_EVENT_TYPE_WS_SEND_HELLO:
        
            QM_LOGI(LOG_TAG, "QM_AUDIO_EVENT_TYPE_WS_SEND_HELLO");

            qm_audio_ws_hello(audio_dev, QM_WAIT_FOREVER);
        break;
        
        case QM_AUDIO_EVENT_TYPE_WS_MSG_RECV:
        
            ws_frame = (qm_audio_ws_frame_t *)input_event->value;
            if(ws_frame->payload){
                QM_LOGE(LOG_TAG, "ws recv:%.*s", ws_frame->payload_len, ws_frame->payload);
                cjson_recv_handler(audio_dev, ws_frame->payload, ws_frame->payload_len);
                qm_free(ws_frame->payload);
            }

        break;

        default:
        break;
    }
} 
static void websocket_audio_recv(qm_ai_audio_ctx_t *audio_dev, uint8_t opcode, uint8_t *payload, int payload_len, int total_size, int offset)
{
    static qm_audio_ws_frame_t audio_recv = {0};
    if(!offset){
        audio_recv.payload = (uint8_t *)qm_calloc(1, total_size);
        if(audio_recv.payload == NULL){
            return ;
        }
        audio_recv.buff_len = 0;
        audio_recv.payload_len = total_size;
    }

    if(audio_recv.payload == NULL){
        return ;
    }

    memcpy(audio_recv.payload + offset, payload, payload_len);
    audio_recv.buff_len += payload_len;

    if(audio_recv.buff_len < audio_recv.payload_len){
        return ;
    }
    
    if(opcode == 0x02){
        qm_queue_send(&audio_dev->binary_queue, &audio_recv, sizeof(qm_audio_ws_frame_t));
    }else if(opcode == 0x01) {
        qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_MSG_RECV, &audio_recv, sizeof(qm_audio_ws_frame_t));
    }else{
        qm_free(audio_recv.payload);
    }
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    qm_ai_audio_ctx_t *audio_dev = (qm_ai_audio_ctx_t *)data->user_context;

    switch (event_id) {
    case WEBSOCKET_EVENT_BEGIN:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_BEGIN");
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_CONNECTED");
        qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_CONNECT, NULL, 0);
        break;
        
    case WEBSOCKET_EVENT_DISCONNECTED:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_DISCONNECT, NULL, 0);

        log_error_if_nonzero("HTTP status code",  data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  data->error_handle.esp_transport_sock_errno);
        }
        break;
    case WEBSOCKET_EVENT_DATA:
        if(data->op_code == 0x01 || data->op_code == 0x02){
            websocket_audio_recv(audio_dev, data->op_code ,(uint8_t *)data->data_ptr, data->data_len, data->payload_len, data->payload_offset);
        }

        break;
    case WEBSOCKET_EVENT_ERROR:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_ERROR");
        log_error_if_nonzero("HTTP status code",  data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  data->error_handle.esp_transport_sock_errno);
        }
        break;
    case WEBSOCKET_EVENT_FINISH:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_FINISH");
        break;
    }
}

static void audio_data_process_task(void *args)
{
    unsigned int size = 0; 
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)args;
    while (1) {
        qm_audio_ws_frame_t frame = { 0 };
        qm_queue_recv(&audio_ctx->binary_queue, &frame, &size, QM_WAIT_FOREVER);
        if (frame.payload == NULL) {
            continue;
        }
        
        if(audio_ctx->data_cb){
            audio_ctx->data_cb(frame.payload, frame.payload_len, audio_ctx->user_data);
        }
        qm_free(frame.payload);
    }
}

#if CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT
static void audio_send_process_task(void *args)
{
    qm_audio_ws_frame_t frame = { 0 };
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)args;
    while (1) {
        qm_queue_recv(&audio_ctx->send_queue, &frame, QM_WAIT_FOREVER);
        if (frame.payload == NULL) {
            continue;
        }
        
        
        qm_free(frame.payload);
    }
}
#endif

qm_audio_ws_handle_t qm_audio_ws_create(qm_audio_ws_cfg_t *cfg)
{
    uint8_t mac[6] = {0};
    qm_err_t ret = QM_EOK;
    char temp_data[CONFIG_QM_AUDIO_WS_MAX_SIZE] = {0};
    esp_websocket_client_config_t websocket_cfg = {0};
    
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)qm_calloc(1, sizeof(qm_ai_audio_ctx_t));
    if(audio_ctx == NULL){
        return NULL;
    }
    
    audio_ctx->data_cb = cfg->data_cb;
    audio_ctx->event_cb = cfg->event_cb;
    audio_ctx->user_data = cfg->user_data;
    audio_ctx->pid = cfg->pid;
    audio_ctx->did = cfg->did;
    audio_ctx->connect_timeout = cfg->connect_timeout;
    audio_ctx->fmt_type = cfg->fmt_type;
    audio_ctx->rate = cfg->rate;
    audio_ctx->bits = cfg->bits;
    audio_ctx->channel = cfg->channel;
    audio_ctx->frame_duration_ms = cfg->frame_duration_ms;

#if CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT
    ret = qm_queue_new(&audio_ctx->send_queue, CONFIG_QM_AUDIO_WS_QUEUE_NUM, sizeof(qm_audio_ws_frame_t));
    if(ret != QM_EOK){
        goto __err;
    }
#endif
    ret = qm_queue_new(&audio_ctx->binary_queue, CONFIG_QM_AUDIO_WS_QUEUE_NUM, sizeof(qm_audio_ws_frame_t));
    if(ret != QM_EOK){
        goto __err;
    }
    qm_ble_gap_get_local_addr(mac);
    qm_snprintf(audio_ctx->sn, CONFIG_QM_AUDIO_WS_MAX_SIZE, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    websocket_cfg.buffer_size = cfg->buff_size;
    websocket_cfg.task_prio = cfg->task_prio;
    websocket_cfg.uri = cfg->uri;
    websocket_cfg.skip_cert_common_name_check = true;
    websocket_cfg.user_context = audio_ctx;
   
    audio_ctx->client = esp_websocket_client_init(&websocket_cfg);

    esp_websocket_client_append_header(audio_ctx->client, SN_PARAMS, audio_ctx->sn);

    qm_snprintf(temp_data, CONFIG_QM_AUDIO_WS_MAX_SIZE, "%d", audio_ctx->pid);
    esp_websocket_client_append_header(audio_ctx->client, PID_PARAMS, temp_data);

    memset(temp_data, 0, CONFIG_QM_AUDIO_WS_MAX_SIZE);
    qm_snprintf(temp_data, CONFIG_QM_AUDIO_WS_MAX_SIZE, "%d", QM_AUDIO_WS_VERSION);
    esp_websocket_client_append_header(audio_ctx->client, VERSION_PARAMS, temp_data);

    if(audio_ctx->did > 0){
        memset(temp_data, 0, CONFIG_QM_AUDIO_WS_MAX_SIZE);
        qm_snprintf(temp_data, CONFIG_QM_AUDIO_WS_MAX_SIZE, "%d", audio_ctx->did);
        esp_websocket_client_append_header(audio_ctx->client, DID_PARAMS, temp_data);
    }
#if CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT   
    qm_task_new(&audio_ctx->send_task, "ws_send", audio_send_process_task, audio_ctx, cfg->task_size, cfg->task_prio);
#endif
    qm_task_new(&audio_ctx->binary_task, "ws_binary", audio_data_process_task, audio_ctx, cfg->task_size, cfg->task_prio);

    esp_websocket_register_events(audio_ctx->client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)audio_ctx->client);
    
    qm_event_register(QM_AUDIO_EVENT_ID, qm_audio_event_handler, audio_ctx);

    
    return (qm_audio_ws_handle_t)audio_ctx;

__err: 

    if(audio_ctx){
#if CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT  
        if(qm_queue_is_valid(&audio_ctx->send_queue)){
            qm_queue_free(&audio_ctx->send_queue);
        }
#endif
        if(qm_queue_is_valid(&audio_ctx->binary_queue)){
            qm_queue_free(&audio_ctx->binary_queue);
        }

        qm_free(audio_ctx);
    }

    return NULL;
}

qm_err_t qm_audio_ws_start(qm_audio_ws_handle_t handle)
{
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL){
        return -QM_EINVAL;
    }

    esp_websocket_client_start(audio_ctx->client);

    return QM_EOK;
}

qm_err_t qm_audio_ws_hello(qm_audio_ws_handle_t handle, uint32_t timeout)
{
    char *buff = NULL;
    cJSON *child = NULL;
    cJSON *parent = NULL;
    qm_err_t ret = QM_EOK;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    cJSON_AddStringToObject(parent, METHOD_PARAMS, HELLO_PARAMS);
    child = cJSON_AddObjectToObject(parent, AUDIO_INFO_PARAMS);
    if(child == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    cJSON_AddNumberToObject(child, FORMAT_PARAMS, (int32_t)audio_ctx->fmt_type);
    cJSON_AddNumberToObject(child, RATE_PARAMS, (int32_t)audio_ctx->rate);
    cJSON_AddNumberToObject(child, BIT_PARAMS, (int32_t)audio_ctx->bits);
    cJSON_AddNumberToObject(child, CHANNEL_PARAMS, (int32_t)audio_ctx->channel);
    cJSON_AddNumberToObject(child, FRAME_DUR_PARAMS, (int32_t)audio_ctx->frame_duration_ms);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    QM_LOGD(LOG_TAG, "send msg:%s",buff);
    esp_websocket_client_send_text(audio_ctx->client, buff, strlen(buff), timeout / portTICK_PERIOD_MS);
__exit:

    if(parent){
        cJSON_Delete(parent);
        parent = NULL;
    }

    if(buff){
        cJSON_free(buff);
    }

    return ret;
}

qm_err_t qm_audio_ws_listen(qm_audio_ws_handle_t handle, qm_audio_ws_listen_param_t *params, uint32_t timeout)
{
    char *buff = NULL;
    cJSON *parent = NULL;
    qm_err_t ret = QM_EOK;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL || params == NULL){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }


    cJSON_AddStringToObject(parent, METHOD_PARAMS, LISTEN_PARAMS);
    cJSON_AddStringToObject(parent, ID_PARAMS, audio_ctx->id);
    cJSON_AddNumberToObject(parent, STATE_PARAMS, (const double)params->state);
    cJSON_AddNumberToObject(parent, MODE_PARAMS, (const double)params->mode);

    if(params->text){
        cJSON_AddStringToObject(parent, TEXT_PARAMS, params->text);
    }

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        ret = -QM_ERROR;
        goto __exit;
    }   

    esp_websocket_client_send_text(audio_ctx->client, buff, strlen(buff), timeout / portTICK_PERIOD_MS);

__exit:

    if(parent){
        cJSON_Delete(parent);
        parent = NULL;
    }

    if(buff){
        cJSON_free(buff);
    }

    return ret;
}

qm_err_t qm_audio_ws_abort(qm_audio_ws_handle_t handle, qm_audio_ws_abort_code_t code, uint32_t timeout)
{
    char *buff = NULL;
    cJSON *parent = NULL;
    qm_err_t ret = QM_EOK;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        return -QM_ENOMEM;
    }

    cJSON_AddStringToObject(parent, METHOD_PARAMS, ABORT_PARAMS);
    cJSON_AddStringToObject(parent, ID_PARAMS, audio_ctx->id);
    cJSON_AddNumberToObject(parent, REASON_PARAMS, code);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        ret = -QM_ERROR;
        goto __exit;
    }   

    esp_websocket_client_send_text(audio_ctx->client, buff, strlen(buff), timeout / portTICK_PERIOD_MS);

__exit:

    if(parent){
        cJSON_Delete(parent);
        parent = NULL;
    }

    if(buff){
        cJSON_free(buff);
    }

    return QM_EOK;
}

qm_err_t qm_audio_ws_ota_version(qm_audio_ws_handle_t handle, char *version, uint32_t timeout)
{
    char *buff = NULL;
    cJSON *parent = NULL;
    qm_err_t ret = QM_EOK;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        return -QM_ENOMEM;
    }

    cJSON_AddStringToObject(parent, METHOD_PARAMS, OTA_VER_PARAMS);
    cJSON_AddStringToObject(parent, ID_PARAMS, audio_ctx->id);
    cJSON_AddStringToObject(parent, VERSION_PARAMS, version);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        ret = -QM_ERROR;
        goto __exit;
    }   

    esp_websocket_client_send_text(audio_ctx->client, buff, strlen(buff), timeout / portTICK_PERIOD_MS);
__exit:

    if(parent){
        cJSON_Delete(parent);
        parent = NULL;
    }

    if(buff){
        cJSON_free(buff);
    }


    return QM_EOK;
}

int qm_audio_ws_data_send(qm_audio_ws_handle_t handle, void *buffer, int length, uint32_t timeout)
{
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL){
        return -QM_EINVAL;
    }
    esp_websocket_client_send_bin(audio_ctx->client, buffer, length, timeout / portTICK_PERIOD_MS);
    return QM_EOK;
}

qm_err_t qm_audio_ws_destroy(qm_audio_ws_handle_t handle)
{

    return QM_EOK;
}

static char *cjson_str_get(cJSON *cjson, char *key)
{
    cJSON *tmp = NULL;
    tmp = cJSON_GetObjectItem(cjson, key);
    if(tmp == NULL){
        return NULL;
    }
    return tmp->valuestring;
}

static int cjson_int_get(cJSON *cjson, char *key, int *data)
{
    cJSON *tmp = NULL;
    tmp = cJSON_GetObjectItem(cjson, key);
    if(tmp == NULL){
        return -QM_EINVAL;
    }
    *data = tmp->valueint;
    return QM_EOK;
}


static void hello_reply_recv_handler(qm_ai_audio_ctx_t *auido_ctx, cJSON *root)
{
    cJSON *params_object = NULL;
    char *id = NULL;

    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_hello_reply_t *hello_reply = &recv.data.hello_reply;
    if(auido_ctx == NULL || root == NULL){
        return ;
    }
    
    QM_LOGD(LOG_TAG, "QM_AUDIO_WS_RECV_TYPE_HELLO_REPLY");

    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_HELLO_REPLY;

    id = cjson_str_get(root, ID_PARAMS);
    if(id == NULL){
        return ;
    }

    memcpy(&auido_ctx->id, id, strlen(id));

    params_object = cJSON_GetObjectItem(root, AUDIO_INFO_PARAMS);
    if(params_object){
        cjson_int_get(params_object, FORMAT_PARAMS, (int *)&hello_reply->fmt_type);
        cjson_int_get(params_object, RATE_PARAMS, &hello_reply->rate);
        cjson_int_get(params_object, BIT_PARAMS, &hello_reply->bits);
        cjson_int_get(params_object, CHANNEL_PARAMS, &hello_reply->channel);
        cjson_int_get(params_object, FRAME_DUR_PARAMS, &hello_reply->frame_duration_ms);

    }

    if(auido_ctx->event_cb){
        auido_ctx->event_cb(&recv, auido_ctx->user_data);
    }
}

static void stt_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{

    qm_err_t ret = QM_EOK;
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_stt_t *stt = &recv.data.stt;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_STT;

    stt->text = cjson_str_get(root, TEXT_PARAMS);
    if(stt->text){
        QM_LOGD(LOG_TAG, "stt:%s", stt->text);
    }
    
    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}

static void tts_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_tts_t *tts = &recv.data.tts;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_TTS;

    cjson_int_get(root, STATE_PARAMS, (int *)&tts->state);
    tts->text = cjson_str_get(root, TEXT_PARAMS);
    if(tts->text){
        QM_LOGD(LOG_TAG, "tts:%s", tts->text);
    }
    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}

static void llm_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_llm_t *llm = &recv.data.llm;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_LLM;

    llm->emotion = cjson_str_get(root,  EMOTION_PARAMS);
    if(llm->emotion){
        QM_LOGD(LOG_TAG, "emotion:%s", llm->emotion);
    }
    
    llm->icon = cjson_str_get(root,  ICON_PARAMS);
    if(llm->icon){
        QM_LOGD(LOG_TAG, "icon:%s", llm->icon);
    }

    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}

static void cmd_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_cmd_t *cmd = &recv.data.cmd;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_CMD;

    cjson_int_get(root, COMMAND_PARAMS, &cmd->command);
  
    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}

static void ota_upgrade_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{
    cJSON *params_object = NULL;
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_ota_upgrade_t *ota_upgrade = &recv.data.ota_upgrade;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_OTA_UPGRADE;

    params_object = cJSON_GetObjectItem(root, PARAMS_PARAMS);
    if(params_object == NULL){
        return ;
    }

    cjson_int_get(params_object, SIZE_PARAMS, &ota_upgrade->size);
    ota_upgrade->version = cjson_str_get(params_object,  VERSION_PARAMS);
    ota_upgrade->uri = cjson_str_get(params_object,  URI_PARAMS);
    ota_upgrade->md5 = cjson_str_get(params_object,  MD5_PARAMS);

    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}

