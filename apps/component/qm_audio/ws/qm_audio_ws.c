
#include "qm.h"
#include "qm_audio_ws.h"
#include "qm_audio_ws_iface.h"
#include "qm_audio_common.h"

#include "qm_utils_list.h"
#include "qm_work.h"
#include "qm_event.h"

#include "cJSON.h"


#include "audio_mem.h"
#include "audio_thread.h"

#ifndef CONFIG_QM_AUDIO_WS_MAX_SIZE     
#define CONFIG_QM_AUDIO_WS_MAX_SIZE                   (32)
#endif

#define LOG_TAG     "ai_audio"

#define ID_PARAMS               "id"
#define PID_PARAMS              "pid"
#define DID_PARAMS              "did"
#define SN_PARAMS               "sn"
#define PROTOCOL_VERSION_PARAMS "protocolVersion"
#define OTA_VERSION_PARAMS      "otaVersion"


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
#define TYPE_PARAMS             "type"
#define REASON_PARAMS            "reason"
#define VERSION_PARAMS           "version"
#define EMOTION_PARAMS           "emotion"
#define ICON_PARAMS              "icon"
#define PARAMS_PARAMS           "params"
#define URI_PARAMS              "url"
#define SIZE_PARAMS             "size"
#define MD5_PARAMS              "md5"
#define CMD_PARAMS              "cmd"
#define TIMES_PARAMS            "times"
#define SPEED_PARAMS            "speed"
#define AUDIODURATION_PARAMS    "audioDuration"



#define HELLO_PARAMS            "hello"
#define HELLO_REPLY_PARAMS      "helloReply"
#define STT_PARAMS              "stt"
#define TTS_PARAMS              "tts"
#define LLM_PARAMS              "llm"
#define SET_PARAMS              "set"
#define LISTEN_PARAMS           "listen"
#define ABORT_PARAMS            "abort"
#define BIND_PARAMS             "bind"
#define OTA_VER_PARAMS           "otaVersion"
#define OTA_UPGRADE_PARAMS       "otaUpgrade"
#define OPENINGREMARKS_PARAMS    "openingRemarks"
#define CLOSINGREMARKS_PARAMS    "closingRemarks"
#define MUSIC_PARAMS             "music"
#define FLOW_CTRL_PARAMS         "flowControl"



#define CHAT_PARAMS         "chat"
#define OPENING_PARAMS      "opening"
#define CLOSING_PARAMS      "closing"

#define SIID_PARAMS    "sid"
#define PIID_PARAMS    "pid"
#define VALUE_PARAMS   "value"

typedef struct{
    uint8_t binary;
    int offset;
    int payload_len;
    uint8_t *payload;
}qm_audio_ws_frame_t;

typedef struct{
    int duration;
    char *payload;
}qm_audio_ws_tts_frame_t;

typedef struct {
    
    // char url[CONFIG_QM_AUDIO_WS_MAX_SIZE];

    char *sn;
    char id[37];

    qm_task_t                     binary_task;
    qm_sem_t                      binary_sem;
    qm_list_t                     binary_list;
    qm_mutex_t                    binary_lock;

    qm_list_t                     tts_list;
    qm_mutex_t                    tts_lock;

#if 0
    qm_queue_t                    binary_queue;
    qm_task_t                     send_task;
    qm_queue_t                    send_queue;
#endif
               
    int                           len_offset;
    int                           play_status;
    qm_audio_ws_data_cb_t         data_cb; 
    qm_audio_ws_recv_event_cb_t   event_cb;
    void                        *user_data;         /*!< Pointer to user data (optional) */

    char                         *ver;              /*!< OTA: 1.0.1 */
    int                           pid;              /*!< 产品pid */
    int                           did;              /*!< 设备did 可选, -1则无效*/
    uint32_t                      connect_timeout;  /*!< 连接服务端超时时间，默认为10s, 单位为秒*/

    qm_audio_ws_fmt_type_t        fmt_type;          /*!< 音频格式*/
    uint16_t                      rate;              /*!< 音频采样率*/
    uint16_t                      bits;              /*!< 音频采样位数*/
    uint16_t                      channel;           /*!< 音频声道数*/
    uint16_t                      frame_duration_ms; /*!< 音频片段长度，单位ms，默认值60*/
    
    qm_mutex_t audio_lock;

    void *client;

    int connect;

    int                           post_count;

    uint8_t                       flow_ctrl_enbale;        /*!< 流控使能*/

    uint8_t                       min_len;        /*!< */
    uint8_t                       max_len;        /*!< */

    uint8_t                       min_speed;        /*!< */
    uint8_t                       max_speed;        /*!< */

    uint8_t                       last_speed;

    qm_audio_ws_frame_t           audio_recv ;
}qm_ai_audio_ctx_t;

typedef struct{
    char *method;
    void (*recv_handler)(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
}qm_ai_audio_params_table_t;

typedef struct{
    char *str_type;
    qm_audio_ws_tts_type_t int_type;
}qm_ai_audio_tts_type_table_t;

static char *cjson_str_get(cJSON *cjson, char *key);
static int cjson_int_get(cJSON *cjson, char *key, int *data);

static void hello_reply_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void stt_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void tts_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void llm_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void set_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void ota_upgrade_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void abort_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);
static void bind_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root);


static qm_ai_audio_tts_type_table_t g_tts_type_table[] = 
{
    {CHAT_PARAMS,       QM_AUDIO_WS_TTS_TYPE_CHAT},
    {OPENING_PARAMS,    QM_AUDIO_WS_TTS_TYPE_OPENING},
    {CLOSING_PARAMS,    QM_AUDIO_WS_TTS_TYPE_CLOSING},
};

static qm_ai_audio_params_table_t g_param_table[] = 
{
    {HELLO_REPLY_PARAMS,    hello_reply_recv_handler},
    {STT_PARAMS,            stt_recv_handler},
    {TTS_PARAMS,            tts_recv_handler},
    {LLM_PARAMS,            llm_recv_handler},
    {SET_PARAMS,            set_recv_handler},
    {OTA_UPGRADE_PARAMS,    ota_upgrade_recv_handler},
    {ABORT_PARAMS,          abort_recv_handler},
    {BIND_PARAMS,           bind_recv_handler},
};

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

        case QM_AUDIO_EVENT_TYPE_WS_FLOW_CTRL:
            QM_LOGI(LOG_TAG, "QM_AUDIO_EVENT_TYPE_WS_FLOW_CTRL");
            qm_audio_ws_flow_ctrl((qm_audio_ws_handle_t)audio_dev, audio_dev->last_speed, CONFIG_QM_AUDIO_WS_TIMEOUT_MS);
        break;

        case QM_AUDIO_EVENT_TYPE_WS_CONNECT:
        
            QM_LOGI(LOG_TAG, "QM_AUDIO_EVENT_TYPE_WS_CONNECT");

            qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_SEND_HELLO, NULL, 0);

            recv.recv_type = QM_AUDIO_WS_RECV_TYPE_CONNECT;
            if(audio_dev->event_cb){
                audio_dev->event_cb(&recv, audio_dev->user_data);
            }

            audio_dev->connect = QM_TRUE;

        break;

        case QM_AUDIO_EVENT_TYPE_WS_DISCONNECT:
        
            QM_LOGI(LOG_TAG, "QM_AUDIO_EVENT_TYPE_WS_DISCONNECT");

            if(audio_dev->connect){
                recv.recv_type = QM_AUDIO_WS_RECV_TYPE_DISCONNECT;
                if(audio_dev->event_cb){
                    audio_dev->event_cb(&recv, audio_dev->user_data);
                }
            }
            audio_dev->connect = QM_FALSE;

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
    qm_list_node_t *node = NULL;
    qm_audio_ws_frame_t *ws_node = NULL;


    if(audio_dev->audio_recv.payload == NULL){
        audio_dev->audio_recv.payload = (uint8_t *)qm_calloc(1, total_size);
        if(audio_dev->audio_recv.payload == NULL){
            return ;
        }

        audio_dev->audio_recv.offset = 0;
        audio_dev->audio_recv.payload_len = total_size;
    }

    if(audio_dev->audio_recv.payload == NULL){
        return ;
    }

    memcpy(audio_dev->audio_recv.payload + audio_dev->audio_recv.offset, payload, payload_len);
    audio_dev->audio_recv.offset += payload_len;

    if(audio_dev->audio_recv.offset < audio_dev->audio_recv.payload_len){
        return ;
    }

    if(opcode == 0x02){

        node = qm_list_node_extra_new(sizeof(qm_audio_ws_frame_t));
        if(node == NULL){
            return ;
        }
        ws_node = (qm_audio_ws_frame_t *)qm_list_node_val_get(node);

        memcpy(ws_node, &audio_dev->audio_recv, sizeof(qm_audio_ws_frame_t));

        qm_mutex_lock(&audio_dev->binary_lock, QM_WAIT_FOREVER);
        qm_list_rpush(&audio_dev->binary_list, node);
        qm_mutex_unlock(&audio_dev->binary_lock);

        if(audio_dev->post_count){
            qm_sem_signal(&audio_dev->binary_sem);
        }
        
    }else if(opcode == 0x01) {

        qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_MSG_RECV, &audio_dev->audio_recv, sizeof(qm_audio_ws_frame_t));

        
    }else{
        qm_free(audio_dev->audio_recv.payload);
    }

    audio_dev->audio_recv.payload = NULL;
}

#if 1
static void websocket_event_handler(void *handle, qm_audio_ws_event_id_t event_id, qm_audio_ws_event_data_t *event_data, void *user_ctx)
{
    qm_ai_audio_ctx_t *audio_dev = (qm_ai_audio_ctx_t *)user_ctx;

    switch (event_id) {
        case QM_AUDIO_WS_EVENT_BEGIN:
            QM_LOGI(LOG_TAG, "QM_AUDIO_WS_EVENT_BEGIN");
            break;
        case QM_AUDIO_WS_EVENT_CONNECTED:
            QM_LOGI(LOG_TAG, "QM_AUDIO_WS_EVENT_CONNECTED");
            qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_CONNECT, NULL, 0);
            break;
            
        case QM_AUDIO_WS_EVENT_DISCONNECTED:
            QM_LOGI(LOG_TAG, "QM_AUDIO_WS_EVENT_DISCONNECTED");

            if(audio_dev->audio_recv.payload){
                qm_free(audio_dev->audio_recv.payload);
                audio_dev->audio_recv.payload = NULL;
            }

            audio_dev->audio_recv.offset = 0;
            audio_dev->audio_recv.payload_len = 0;

            qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_DISCONNECT, NULL, 0);
            break;
        case QM_AUDIO_WS_EVENT_DATA:


            if(event_data->op_code == 0x01 || event_data->op_code == 0x02){
                websocket_audio_recv(audio_dev, event_data->op_code ,(uint8_t *)event_data->data_ptr, event_data->data_len, event_data->payload_len, event_data->payload_offset);
            }

            break;
        case QM_AUDIO_WS_EVENT_ERROR:
            QM_LOGI(LOG_TAG, "QM_AUDIO_WS_EVENT_ERROR");
            break;

        case QM_AUDIO_WS_EVENT_CLOSED:
        case QM_AUDIO_WS_EVENT_FINISH:
            QM_LOGI(LOG_TAG, "QM_AUDIO_WS_EVENT_FINISH");
            qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_DISCONNECT, NULL, 0);
            break;

        default:
            break;
    }
}
#endif

static void audio_data_process_task(void *args)
{
    #if 0
        unsigned int size = 0; 
    #endif

    int list_len = 0;
    int speed = 0;
    int timeout_count = 0;
    qm_list_node_t *node = NULL;
    qm_audio_ws_frame_t *ws_node = NULL;
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)args;
    while (1) {
    #if 0
        qm_audio_ws_frame_t frame = { 0 };
        qm_queue_recv(&audio_ctx->binary_queue, &frame, &size, timeout);
    #endif

        qm_sem_wait(&audio_ctx->binary_sem, QM_WAIT_FOREVER);

        if(!audio_ctx->post_count){
            continue;
        }

        qm_mutex_lock(&audio_ctx->binary_lock, QM_WAIT_FOREVER);
        node = qm_list_lpop(&audio_ctx->binary_list);
        qm_mutex_unlock(&audio_ctx->binary_lock);

        if(node == NULL){

            audio_ctx->len_offset = 0;

            if(audio_ctx->play_status){
                audio_ctx->play_status = QM_AUDIO_WS_PLAYER_STATE_STOP;
                if(audio_ctx->data_cb){
                    audio_ctx->data_cb(NULL, 0, 0, QM_AUDIO_WS_PLAYER_STATE_STOP,audio_ctx->user_data);
                }
            }
   
            continue;
        }

        ws_node = (qm_audio_ws_frame_t *)qm_list_node_val_get(node);

        if (ws_node->payload == NULL) {

            audio_ctx->len_offset = 0;
            if(audio_ctx->play_status){
                audio_ctx->play_status = QM_AUDIO_WS_PLAYER_STATE_STOP;
                if(audio_ctx->data_cb){
                    audio_ctx->data_cb(NULL, 0, 0, QM_AUDIO_WS_PLAYER_STATE_STOP,audio_ctx->user_data);
                }
            }

            continue;
        }

    #if 0
        if(timeout == QM_WAIT_FOREVER){
            timeout = CONFIG_QM_AUDIO_WS_TIMEOUT_MS;
            QM_LOGW(LOG_TAG, "ws delay(%d) wait audio!!", CONFIG_QM_AUDIO_WS_QUEUE_DELAY_MS);
            qm_msleep(CONFIG_QM_AUDIO_WS_QUEUE_DELAY_MS);
        }
    #endif

    
        audio_ctx->post_count--;
        audio_ctx->len_offset++;

        if(audio_ctx->play_status == QM_AUDIO_WS_PLAYER_STATE_STOP){
            audio_ctx->play_status = QM_AUDIO_WS_PLAYER_STATE_START;
            if(audio_ctx->data_cb){
                audio_ctx->data_cb(NULL, 0, 0, QM_AUDIO_WS_PLAYER_STATE_START,audio_ctx->user_data);
            }
        }
        
        if(audio_ctx->data_cb){
            audio_ctx->data_cb(ws_node->payload, ws_node->payload_len, audio_ctx->len_offset, QM_AUDIO_WS_PLAYER_STATE_DATA, audio_ctx->user_data);
        }

        qm_free(ws_node->payload);
        qm_list_node_destroy(node);

        qm_mutex_lock(&audio_ctx->binary_lock, QM_WAIT_FOREVER);
        list_len = qm_list_len_get(&audio_ctx->binary_list);
        qm_mutex_unlock(&audio_ctx->binary_lock);


        if(list_len && audio_ctx->post_count){
            qm_sem_signal(&audio_ctx->binary_sem);
        }else{

            audio_ctx->len_offset = 0;

            if(audio_ctx->play_status){
                audio_ctx->play_status = QM_AUDIO_WS_PLAYER_STATE_STOP;
                if(audio_ctx->data_cb){
                    audio_ctx->data_cb(NULL, 0, 0, QM_AUDIO_WS_PLAYER_STATE_STOP,audio_ctx->user_data);
                }
            }
   
        }

        if(!audio_ctx->flow_ctrl_enbale){
            continue;
        }

        if(list_len < audio_ctx->min_len){
            speed = audio_ctx->min_speed;
        }
        
        if (list_len > audio_ctx->max_len){
            speed = audio_ctx->max_speed;
        }
        
        if(audio_ctx->last_speed != speed){
            QM_LOGW(LOG_TAG, "list_len(%d)!!", list_len);
            audio_ctx->last_speed = speed;
            qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_FLOW_CTRL, NULL, 0);
        }
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
    qm_err_t ret = QM_EOK;
    qm_audio_ws_iface_cfg_t websocket_cfg = {0};
    char temp_data[CONFIG_QM_AUDIO_WS_MAX_SIZE] = {0};
    
    if(!g_audio_ws_iface.init || 
        !g_audio_ws_iface.set_monitor ||
        !g_audio_ws_iface.append_header){
        return NULL;
    }

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)qm_calloc(1, sizeof(qm_ai_audio_ctx_t));
    if(audio_ctx == NULL){
        return NULL;
    }
    
    audio_ctx->sn = cfg->sn;
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

    audio_ctx->flow_ctrl_enbale = cfg->flow_ctrl_enbale;
    audio_ctx->max_len = cfg->max_len;
    audio_ctx->min_len = cfg->min_len;
    audio_ctx->min_speed = cfg->min_speed;
    audio_ctx->max_speed = cfg->max_speed;
    
    audio_ctx->last_speed = cfg->min_speed;

#if CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT
    ret = qm_queue_new(&audio_ctx->send_queue, CONFIG_QM_AUDIO_WS_QUEUE_NUM, sizeof(qm_audio_ws_frame_t));
    if(ret != QM_EOK){
        goto __err;
    }
#endif


#if 0
    ret = qm_queue_new(&audio_ctx->binary_queue, CONFIG_QM_AUDIO_WS_QUEUE_NUM, sizeof(qm_audio_ws_frame_t));
    if(ret != QM_EOK){
        goto __err;
    }
#endif

    qm_sem_new(&audio_ctx->binary_sem, 0);
    qm_mutex_new(&audio_ctx->binary_lock);

    websocket_cfg.buffer_size = cfg->buff_size;
    websocket_cfg.task_prio = cfg->task_prio;
    websocket_cfg.task_stack = cfg->task_size;
    websocket_cfg.uri = cfg->uri;
    websocket_cfg.user_context = audio_ctx;
   
    audio_ctx->client = g_audio_ws_iface.init(&websocket_cfg);

    g_audio_ws_iface.append_header(audio_ctx->client, SN_PARAMS, audio_ctx->sn);

    qm_snprintf(temp_data, CONFIG_QM_AUDIO_WS_MAX_SIZE, "%d", audio_ctx->pid);
    g_audio_ws_iface.append_header(audio_ctx->client, PID_PARAMS, temp_data);

    if(audio_ctx->did >= 0){
        memset(temp_data, 0, CONFIG_QM_AUDIO_WS_MAX_SIZE);
        qm_snprintf(temp_data, CONFIG_QM_AUDIO_WS_MAX_SIZE, "%d", audio_ctx->did);
        g_audio_ws_iface.append_header(audio_ctx->client, DID_PARAMS, temp_data);
    }

    memset(temp_data, 0, CONFIG_QM_AUDIO_WS_MAX_SIZE);
    qm_snprintf(temp_data, CONFIG_QM_AUDIO_WS_MAX_SIZE, "%d", QM_AUDIO_WS_VERSION);
    g_audio_ws_iface.append_header(audio_ctx->client, PROTOCOL_VERSION_PARAMS, temp_data);

    if(cfg->ver){
        g_audio_ws_iface.append_header(audio_ctx->client, OTA_VERSION_PARAMS, cfg->ver);
    }

#if CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT   
    qm_task_new(&audio_ctx->send_task, "ws_send", audio_send_process_task, audio_ctx, cfg->task_size, cfg->task_prio);
#endif

#if 0
    qm_task_new(&audio_ctx->binary_task, "ws_binary", audio_data_process_task, audio_ctx, cfg->task_size, cfg->task_prio);
#else
    audio_thread_create(NULL, "ws_binary",
        audio_data_process_task,
        (void *)audio_ctx,
        cfg->task_size,
        cfg->task_prio + 1,
        true,
        true);
#endif

    g_audio_ws_iface.set_monitor(audio_ctx->client, websocket_event_handler);

    qm_event_register(QM_AUDIO_EVENT_ID, qm_audio_event_handler, audio_ctx);

    
    return (qm_audio_ws_handle_t)audio_ctx;

#if 0
__err: 

    if(audio_ctx){
#if CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT  
        if(qm_queue_is_valid(&audio_ctx->send_queue)){
            qm_queue_free(&audio_ctx->send_queue);
        }
#endif

#if 0
        if(qm_queue_is_valid(&audio_ctx->binary_queue)){
            qm_queue_free(&audio_ctx->binary_queue);
        }
#endif
        qm_free(audio_ctx);
    }

    return NULL;
#endif
}

qm_err_t qm_audio_ws_clean(qm_audio_ws_handle_t handle)
{
    qm_list_node_t *node = NULL;
    qm_audio_ws_frame_t *ws_node = NULL;
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL || !g_audio_ws_iface.start){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&audio_ctx->binary_lock, QM_WAIT_FOREVER);
    audio_ctx->len_offset = 0;
    audio_ctx->post_count = 0;
    audio_ctx->play_status = QM_AUDIO_WS_PLAYER_STATE_STOP;
    while (1)
    {
        node = qm_list_lpop(&audio_ctx->binary_list);
        if(node == NULL){
            break;
        }
        ws_node = (qm_audio_ws_frame_t *)qm_list_node_val_get(node);


        qm_free(ws_node->payload);
        qm_list_node_destroy(node);
    }
    qm_mutex_unlock(&audio_ctx->binary_lock);

    return QM_EOK;
}

qm_err_t qm_audio_ws_start(qm_audio_ws_handle_t handle)
{
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL || !g_audio_ws_iface.start){
        return -QM_EINVAL;
    }

    g_audio_ws_iface.start(audio_ctx->client);

    return QM_EOK;
}

qm_err_t qm_audio_ws_stop(qm_audio_ws_handle_t handle)
{
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL || !g_audio_ws_iface.stop){
        return -QM_EINVAL;
    }

    audio_ctx->connect = QM_FALSE;
    
    g_audio_ws_iface.stop(audio_ctx->client);

    return QM_EOK;
}

qm_err_t qm_audio_ws_flow_ctrl(qm_audio_ws_handle_t handle, int speed_ms, uint32_t timeout)
{
    qm_err_t ret = QM_EOK;
    char *buff = NULL;
    cJSON *parent = NULL;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle; 
    if(audio_ctx == NULL || !g_audio_ws_iface.send_text){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    cJSON_AddStringToObject(parent, METHOD_PARAMS, FLOW_CTRL_PARAMS);

    cJSON_AddStringToObject(parent, ID_PARAMS, audio_ctx->id);
    cJSON_AddNumberToObject(parent, SPEED_PARAMS, speed_ms);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    QM_LOGD(LOG_TAG, "send msg:%s",buff);
    g_audio_ws_iface.send_text(audio_ctx->client, buff, strlen(buff), timeout);
    
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

qm_err_t qm_audio_ws_music(qm_audio_ws_handle_t handle, qm_audio_ws_music_param_t *params, uint32_t timeout)
{
    qm_err_t ret = QM_EOK;
    char *buff = NULL;
    cJSON *parent = NULL;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle; 
    if(audio_ctx == NULL || !g_audio_ws_iface.send_text){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    cJSON_AddStringToObject(parent, METHOD_PARAMS, MUSIC_PARAMS);

    cJSON_AddNumberToObject(parent, CMD_PARAMS, params->cmd);
    cJSON_AddNumberToObject(parent, MODE_PARAMS, params->mode);
    cJSON_AddNumberToObject(parent, TIMES_PARAMS, params->times);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    QM_LOGD(LOG_TAG, "send msg:%s",buff);
    g_audio_ws_iface.send_text(audio_ctx->client, buff, strlen(buff), timeout);
    
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

qm_err_t qm_audio_ws_hello(qm_audio_ws_handle_t handle, uint32_t timeout)
{
    qm_err_t ret = QM_EOK;
    char *buff = NULL;
    cJSON *child = NULL;
    cJSON *parent = NULL;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;  // NOLINT  // NOLINT
    if(audio_ctx == NULL || !g_audio_ws_iface.send_text){
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
    cJSON_AddNumberToObject(child, FLOW_CTRL_PARAMS, (int32_t)audio_ctx->flow_ctrl_enbale);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    QM_LOGD(LOG_TAG, "send msg:%s",buff);
    g_audio_ws_iface.send_text(audio_ctx->client, buff, strlen(buff), timeout);
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
    if(audio_ctx == NULL || params == NULL || !g_audio_ws_iface.send_text){
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

    QM_LOGD("1", "send:%s", buff);
    g_audio_ws_iface.send_text(audio_ctx->client, buff, strlen(buff), timeout);

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

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL || !g_audio_ws_iface.send_text){
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
        goto __exit;
    }   

    QM_LOGD("1", "send:%s", buff);
    g_audio_ws_iface.send_text(audio_ctx->client, buff, strlen(buff), timeout);

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

qm_err_t qm_audio_ws_opening_remarks(qm_audio_ws_handle_t handle, uint32_t timeout)
{
    char *buff = NULL;
    cJSON *parent = NULL;

    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL || !g_audio_ws_iface.send_text){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        return -QM_ENOMEM;
    }

    cJSON_AddStringToObject(parent, METHOD_PARAMS, OPENINGREMARKS_PARAMS);
    cJSON_AddStringToObject(parent, ID_PARAMS, audio_ctx->id);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        goto __exit;
    }   
    QM_LOGD("1", "send:%s", buff);
    g_audio_ws_iface.send_text(audio_ctx->client, buff, strlen(buff), timeout);
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

qm_err_t qm_audio_ws_closing_remarks(qm_audio_ws_handle_t handle, uint32_t timeout)
{
    char *buff = NULL;
    cJSON *parent = NULL;
    qm_ai_audio_ctx_t *audio_ctx = (qm_ai_audio_ctx_t *)handle;
    if(audio_ctx == NULL || !g_audio_ws_iface.send_text){
        return -QM_EINVAL;
    }

    parent = cJSON_CreateObject();
    if(parent == NULL){
        return -QM_ENOMEM;
    }

    cJSON_AddStringToObject(parent, METHOD_PARAMS, CLOSINGREMARKS_PARAMS);
    cJSON_AddStringToObject(parent, ID_PARAMS, audio_ctx->id);

    buff = cJSON_PrintUnformatted(parent);
    if(buff == NULL){
        goto __exit;
    }   
    QM_LOGD("1", "send:%s", buff);
    g_audio_ws_iface.send_text(audio_ctx->client, buff, strlen(buff), timeout);
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
    if(audio_ctx == NULL || !g_audio_ws_iface.send_bin){
        return -QM_EINVAL;
    }
   
    g_audio_ws_iface.send_bin(audio_ctx->client, buffer, length, timeout);
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

    auido_ctx->last_speed = auido_ctx->min_speed;
    qm_audio_ws_flow_ctrl((qm_audio_ws_handle_t)auido_ctx, auido_ctx->last_speed, CONFIG_QM_AUDIO_WS_TIMEOUT_MS);

    if(auido_ctx->event_cb){
        auido_ctx->event_cb(&recv, auido_ctx->user_data);
    }
}

static void stt_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{

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
    int state = 0;
    int dur_ms = 0;
    int last_dur_ms = 0;
    char *tts_type = NULL; 
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_tts_t *tts = &recv.data.tts;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_TTS;

    cjson_int_get(root, STATE_PARAMS, (int *)&state);
    tts->state = state;

    tts_type = cjson_str_get(root, TYPE_PARAMS);
    if(tts_type){

        QM_LOGD(LOG_TAG, "tts_type:%s", tts_type);

        for (int i = 0; i < QM_ARRAY_SIZE(g_tts_type_table); i++)
        {
            if(!strncmp(g_tts_type_table[i].str_type, tts_type, strlen(tts_type))){
                tts->tts_type = g_tts_type_table[i].int_type;
                break;
            }
        }

    }

    tts->text = cjson_str_get(root, TEXT_PARAMS);
    if(tts->text){
        QM_LOGD(LOG_TAG, "tts:%s", tts->text);
    }

    switch (tts->state)
    {

        case QM_AUDIO_WS_TTS_STATE_START:
            qm_audio_ws_clean(audio_ctx);
        break;
       
        case QM_AUDIO_WS_TTS_STATE_SENTENCE_START:
            cjson_int_get(root, AUDIODURATION_PARAMS, &last_dur_ms);
            if(last_dur_ms){
                dur_ms = last_dur_ms / audio_ctx->frame_duration_ms;
                audio_ctx->post_count += dur_ms;
                QM_LOGD(LOG_TAG, "audio duration_ms:%d->%d [%d]", last_dur_ms, dur_ms, audio_ctx->post_count);
            }
            tts->duration_ms =last_dur_ms;
        break;

        default:
            break;
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

static int qm_spec_json_data_value_pack(qm_spec_property_t *property, cJSON *object)
{

    QM_RETURN_ON_FALSE(object, -QM_EINVAL, LOG_TAG, "object inval");
    QM_RETURN_ON_FALSE(property, -QM_EINVAL, LOG_TAG, "property inval");

    switch ((object->type & 0xFF))
    {
        case cJSON_False:
            qm_spec_property_pack_bool(property, QM_FALSE);
        break;
        
        case cJSON_True:
            qm_spec_property_pack_bool(property, QM_TRUE);
        break;

        case cJSON_Number:
            qm_spec_property_pack_number(property, object->valueint);
        break;

        case cJSON_String:
            qm_spec_property_pack_string(property, object->valuestring, strlen(object->valuestring));
        break;

        default:

        break;
    }
    return QM_EOK;
}

static void set_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{
    int temp = 0;
    qm_spec_property_t *property = NULL;
    cJSON *params_object = NULL;
    cJSON *sub_object = NULL;
    cJSON *value_object = NULL;

    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_set_t *set = &recv.data.set;
    
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_SET;

    params_object = cJSON_GetObjectItem(root, PARAMS_PARAMS);
    if(params_object == NULL){
        return ;
    }

    set->property_operation = qm_spec_property_operation_creat();
    if(set->property_operation == NULL){
        return ;
    }

    for (int i = 0; i < cJSON_GetArraySize(params_object); i++)
    {
        sub_object = cJSON_GetArrayItem(params_object, i);
        if(sub_object->type != cJSON_Object){
            QM_LOGW(LOG_TAG, "set unpack type no object!!");
            break;
        }

        property = qm_spec_property_creat();
        if(property == NULL){
            QM_LOGW(LOG_TAG, "set unpack property_element no mem");
            break;  
        }

        cjson_int_get(sub_object, SIID_PARAMS, &temp);
        property->siid = temp;

        cjson_int_get(sub_object, PIID_PARAMS, &temp);
        property->piid = temp;

        value_object = cJSON_GetObjectItem(sub_object, VALUE_PARAMS);
        if(value_object == NULL){
            QM_LOGW(LOG_TAG, "set value unpack error!");
            break;
        }

        qm_spec_json_data_value_pack(property, value_object);
        qm_spec_property_add(set->property_operation, property);
    }
    
    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }

    qm_spec_property_operation_delete(set->property_operation);
    set->property_operation = NULL;
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

    qm_audio_ws_clean(audio_ctx);

    cjson_int_get(params_object, SIZE_PARAMS, &ota_upgrade->size);
    ota_upgrade->version = cjson_str_get(params_object,  VERSION_PARAMS);
    ota_upgrade->uri = cjson_str_get(params_object,  URI_PARAMS);
    ota_upgrade->md5 = cjson_str_get(params_object,  MD5_PARAMS);

    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}

static void abort_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{
    cJSON *params_object = NULL;
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_abort_t *abort = &recv.data.abort;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    qm_audio_ws_clean(audio_ctx);

    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_ABORT;

    cjson_int_get(root, REASON_PARAMS, &abort->reason);

    
    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}

static void bind_recv_handler(qm_ai_audio_ctx_t *audio_ctx, cJSON *root)
{
    cJSON *params_object = NULL;
    qm_audio_ws_recv_evt_t recv = {0};
    qm_audio_ws_recv_bind_t *bind = &recv.data.bind;
    if(audio_ctx == NULL || root == NULL){
        return ;
    }
    
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_BIND;


    cjson_int_get(root, STATE_PARAMS, &bind->state);

    if(audio_ctx->event_cb){
        audio_ctx->event_cb(&recv, audio_ctx->user_data);
    }
}