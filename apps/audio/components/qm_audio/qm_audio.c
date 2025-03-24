#include "qm.h"
#include "qm_audio.h"

#include "qm_kv.h"
#include "qm_utils_timer.h"
#include "qm_ringbuf.h"
#include "qm_utils_list.h"

#include "qm_wifi.h"
#include "qm_work.h"
#include "qm_event.h"

#include "qm_audio_prov.h"
#include "qm_audio_player.h"
#include "qm_audio_recorder.h"
#include "qm_audio_common.h"

#include "volc_rtc.h"

#include "qm_spec_core.h"

#include "recorder_sr.h"

#define TAG   "AUDIO"

#define DEBUG_FLAG  0
#define PLAYER_LIST_TEST    1

#define POST_REPLAY_FAST_DELAY_MS    (200)
#define POST_REPLAY_DELAY_MS         (500)

#define JOIN_EVENT_BIT         (1 << 0)
#define WAIT_DESTORY_EVENT_BIT (1 << 1)
#define WAKEUP_REC_READING     (1 << 2)

typedef struct {
    int frame_len;
    char frame_ptr[1];
} frame_package_t;

typedef struct 
{
    qm_sem_t sem;
    qm_mutex_t lock;
    qm_list_t list;
}volc_rtc_audio_t;

typedef struct {
    int is_use;
    char ssid[CONFIG_QM_AUDIO_SSID_MAX_LEN + 1];
    char pwd[CONFIG_QM_AUDIO_PWD_MAX_LEN + 1];
}qm_audio_wifi_info_t;

typedef struct 
{
    qm_audio_cfg_t cfg;

    int sr_table_num;
    qm_audio_rec_table_t    *sr_table;

    qm_audio_prov_handle_t      prov_handle;
    qm_audio_player_handle_t    player_handle;
    qm_audio_rec_handle_t       rec_handle;
    volc_rtc_engine_t           volc_rtc_handle;

    qm_task_t                   read_task;
    qm_task_t                   process_task;

    qm_event_t                  event;

    qm_audio_wifi_info_t        wifi_info;
}qm_audio_handle_t;

static volc_rtc_audio_t g_rtc_audio = {0};
static qm_audio_handle_t *g_audio_handle = NULL;

// byte rtc lite callbacks
static void byte_rtc_on_join_room_success(volc_rtc_engine_t engine, const char* channel, int elapsed_ms, bool ms)
{
    QM_LOGI(TAG, "join channel success %s elapsed %d ms now %d ms\n", channel, elapsed_ms, elapsed_ms);
    qm_audio_cfg_t *cfg = NULL;
    
    if(g_audio_handle == NULL){
        return ;
    }
    
    cfg = &g_audio_handle->cfg;

    qm_event_set(&g_audio_handle->event, JOIN_EVENT_BIT);
    if(cfg->recv_fn){
        
    }
};

static __attribute__((unused)) void byte_rtc_on_rejoin_room_success(volc_rtc_engine_t engine, const char* channel, int elapsed_ms)
{
    QM_LOGI(TAG, "rejoin channel success %s\n", channel);
};

static void byte_rtc_on_user_joined(volc_rtc_engine_t engine, const char* channel, const char* user_name, int elapsed_ms)
{
    QM_LOGI(TAG, "remote user joined  %s:%s", channel, user_name);
};

static void byte_rtc_on_user_offline(volc_rtc_engine_t engine, const char* channel, const char* user_name, int reason)
{
    QM_LOGI(TAG, "remote user offline  %s:%s", channel, user_name);
};

static void byte_rtc_on_user_mute_audio(volc_rtc_engine_t engine, const char* channel, const char* user_name, int muted)
{
    QM_LOGI(TAG, "remote user mute audio  %s:%s %d", channel, user_name, muted);
};

static void byte_rtc_on_user_mute_video(volc_rtc_engine_t engine, const char* channel, const char* user_name, int muted)
{
    QM_LOGI(TAG, "remote user mute video  %s:%s %d", channel, user_name, muted);
};

static __attribute__((unused)) void byte_rtc_on_connection_lost(volc_rtc_engine_t engine, const char* channel)
{
    QM_LOGI(TAG, "connection Lost  %s", channel);
};

static void byte_rtc_on_room_error(volc_rtc_engine_t engine, const char* channel, int code, const char* msg)
{
    QM_LOGI(TAG, "error occur %s %d %s", channel, code, msg ? msg : "UnKown");
};

// remote audio
static void byte_rtc_on_audio_data(volc_rtc_engine_t engine, const char* channel, const char*  uid , uint16_t sent_ts,
                                volc_rtc_audio_codec_type_t codec, const void* data_ptr, size_t data_len)
{
    // QM_LOGD(TAG, "remote audio data %s %s %d %d %p %zu", channel, uid, sent_ts, codec, data_ptr, data_len);

    qm_list_node_t *node = NULL;
    frame_package_t *frame = NULL;
    qm_audio_player_state_t state;

    if(g_audio_handle == NULL){
        return ;
    }

    qm_audio_player_get_state(g_audio_handle->player_handle, &state);
    if (state == QM_AUDIO_PLAYER_STATE_IDLE) {
        return;
    }

    node = qm_list_node_extra_new(data_len + sizeof(frame_package_t) - 1);   
    if(node == NULL){
        QM_LOGW(TAG, "audio frame no memory");
        return;
    }
    frame = (frame_package_t *)qm_list_node_val_get(node);
    frame->frame_len = data_len;
    memcpy(frame->frame_ptr, data_ptr, data_len);
    
    qm_mutex_lock(&g_rtc_audio.lock, QM_WAIT_FOREVER);
    qm_list_rpush(&g_rtc_audio.list, node);
    qm_mutex_unlock(&g_rtc_audio.lock);
    
    qm_sem_signal(&g_rtc_audio.sem);

}

// remote message
void on_message_received(volc_rtc_engine_t engine, const char*  room, const char* uid, const uint8_t* message, int size, bool binary)
{
    // QM_LOGD(TAG, "on_message_received uid: %s, message: %s, message size: %d", uid, message, size);
}

static void on_key_frame_gen_req(volc_rtc_engine_t engine, const char*  channel, const char*  uid) {}
// byte rtc lite callbacks end.

static qm_err_t connect_volc_rtc(qm_audio_handle_t *audio_handle)
{
    qm_audio_cfg_t *cfg = NULL;
    volc_rtc_cfg_t volc_rtc_cfg = {0};
    if(audio_handle == NULL){
        return -QM_EINVAL;
    }
    cfg = &audio_handle->cfg;

    if(audio_handle->volc_rtc_handle == NULL){
        volc_rtc_cfg.appid = cfg->appid;
        volc_rtc_cfg.event_handler.on_join_room_success  =   byte_rtc_on_join_room_success;
        volc_rtc_cfg.event_handler.on_room_error         =   byte_rtc_on_room_error;
        volc_rtc_cfg.event_handler.on_user_joined        =   byte_rtc_on_user_joined;
        volc_rtc_cfg.event_handler.on_user_offline       =   byte_rtc_on_user_offline;
        volc_rtc_cfg.event_handler.on_user_mute_audio    =   byte_rtc_on_user_mute_audio;
        volc_rtc_cfg.event_handler.on_user_mute_video    =   byte_rtc_on_user_mute_video;
        volc_rtc_cfg.event_handler.on_audio_data         =   byte_rtc_on_audio_data;
        volc_rtc_cfg.event_handler.on_key_frame_gen_req  =   on_key_frame_gen_req;
        volc_rtc_cfg.event_handler.on_message_received   =   on_message_received;
        
        audio_handle->volc_rtc_handle = volc_rtc_open(&volc_rtc_cfg);
        if(audio_handle->volc_rtc_handle == NULL){

        }    
    }

    volc_rtc_join_room(audio_handle->volc_rtc_handle, cfg->room, cfg->uid, cfg->token);

    return ESP_OK;
};

static qm_err_t connect_wifi(qm_audio_handle_t *audio_handle)
{
    qm_wifi_config_t config = {0};

    if(audio_handle == NULL){
        return -QM_EINVAL;
    }
 
    qm_wifi_init();
    qm_wifi_set_mode(QM_WIFI_MODE_STA);
 
    config.sta.ssid_len = strlen(audio_handle->wifi_info.ssid);
    memcpy(config.sta.ssid, audio_handle->wifi_info.ssid, config.sta.ssid_len);
    config.sta.password_len = strlen(audio_handle->wifi_info.pwd);
    memcpy(config.sta.password, audio_handle->wifi_info.pwd, config.sta.password_len);

    qm_wifi_set_config(QM_WIFI_IF_STA, &config);
    qm_wifi_start();

    return ESP_OK;
};

static void qm_event_proccess_handler(qm_input_event_t *input_event, void *arg)
{
    if(g_audio_handle == NULL){
        return ;
    }

    switch(input_event->sub_event){

        case QM_AUDIO_EVENT_TYPE_WIFI_SETTING_START:
            connect_wifi(g_audio_handle);
        break;

        case QM_AUDIO_EVENT_TYPE_WIFI_CONNECT:
            
            connect_volc_rtc(g_audio_handle);

        break;

        case QM_AUDIO_EVENT_TYPE_SR_WAKEUP:
            
            qm_audio_player_tone_play(g_audio_handle->player_handle, "spiffs://spiffs/dingding.wav");
        
        break;

        default:
        break;
    }
}

static void prov_recv_handler(void *handle, const qm_audio_prov_recv_t *packet, void *userdata)
{

    qm_audio_handle_t *audio_handle = (qm_audio_handle_t *)userdata;
    if(audio_handle == NULL || handle == NULL || packet == NULL){
        return;
    }

    audio_handle->wifi_info.is_use = QM_TRUE;
    memcpy(audio_handle->wifi_info.ssid, packet->ssid, strlen(packet->ssid));
    memcpy(audio_handle->wifi_info.pwd, packet->password, strlen(packet->password));

    qm_audio_prov_close(audio_handle->prov_handle);
    audio_handle->prov_handle = NULL;

    QM_LOGD(TAG, "CONN WIFI [%s]:[%s]", packet->ssid, packet->password);

    qm_kv_set(QM_AUDIO_PROV_INFO_KV_TAG, &audio_handle->wifi_info, sizeof(qm_audio_wifi_info_t), 1);

    qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_WIFI_SETTING_START, NULL, 0);
}

static void qm_event_handler(qm_input_event_t *input_event, void *arg)
{
    qm_audio_cfg_t *cfg = NULL;
    if(g_audio_handle == NULL){
        return ;
    }

    cfg = &g_audio_handle->cfg;

    switch(input_event->sub_event){

        case QM_WIFI_EVENT_STA_START:
            if(cfg->enable_prov){
                qm_wifi_connect();
                QM_LOGD(TAG, "connect to the AP");
            }
        break;

        case QM_WIFI_EVENT_STA_DISCONNECTED:
            if(cfg->enable_prov){
                qm_wifi_connect();
                QM_LOGD(TAG, "sta disconnected reason,retry to connect to the AP");
            }
        break;

        case QM_WIFI_EVENT_STA_GOT_IP:

            qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_WIFI_CONNECT, NULL, 0);
            
        break;

        case QM_WIFI_EVENT_STA_LOST_IP:
            

        break;

        default:
        break;
    }
}

static qm_err_t qm_audio_rec_event_cb(qm_audio_rec_evt_t *event, void *user_data)
{
    static int last_type = 0;

    switch (event->type)
    {

        case QM_AUDIO_REC_WAKEUP_START:
            QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_START");

            last_type = QM_AUDIO_REC_WAKEUP_START;
            qm_audio_player_stop(g_audio_handle->player_handle);
            qm_event_set(&g_audio_handle->event, WAKEUP_REC_READING);
            qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_SR_WAKEUP, NULL, 0);
        break;

        case QM_AUDIO_REC_WAKEUP_END:
            QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_END");
            qm_event_clear(&g_audio_handle->event, WAKEUP_REC_READING);
            qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_SR_TIMEOUT, NULL, 0);
        break;
            
        case QM_AUDIO_REC_VAD_START:
            QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_VAD_START");
            if(last_type == QM_AUDIO_REC_WAKEUP_START){
                qm_audio_player_stop(g_audio_handle->player_handle);
            }
            qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_SR_VAD_START, NULL, 0);
        break;

        case QM_AUDIO_REC_VAD_END:
            QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_VAD_END");

            if(last_type == QM_AUDIO_REC_WAKEUP_START){
                last_type = QM_AUDIO_REC_VAD_END;
                qm_audio_player_run(g_audio_handle->player_handle);
            }
            qm_audio_recorder_clean(g_audio_handle->rec_handle);

            qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_SR_VAD_END, NULL, 0);
        break;

        default:

            if (event->type > 0 )
            {   
                recorder_sr_mn_result_t *result = (recorder_sr_mn_result_t *)event->event_data;
                QM_LOGI(TAG, "AUDIO_REC_COMMAND_DECT:[%d][%s]:%f", result->phrase_id, result->str, result->prob);
                qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_SR_COMMAND, result, sizeof(recorder_sr_mn_result_t));
            }
    
            break;
    }
    return QM_EOK;
}

static void audio_data_process_task(void *args)
{
    qm_list_node_t *node = NULL;
    frame_package_t *frame = NULL;
    while (1) {
        qm_sem_wait(&g_rtc_audio.sem, QM_WAIT_FOREVER);

        qm_mutex_lock(&g_rtc_audio.lock, QM_WAIT_FOREVER);
        node = qm_list_lpop(&g_rtc_audio.list);
        qm_mutex_unlock(&g_rtc_audio.lock);

        if(node == NULL){
            continue;
        }
        
        frame = (frame_package_t *)qm_list_node_val_get(node);

        if(g_audio_handle && g_audio_handle->player_handle){
            qm_audio_player_write(g_audio_handle->player_handle, frame->frame_ptr, frame->frame_len);
        }

        qm_list_node_destroy(node);
    }
}

static void voice_read_task(void *args)
{
    qm_audio_cfg_t *cfg = NULL;
    unsigned int bits = 0;
    qm_audio_handle_t *audio_handle = (qm_audio_handle_t *)args;

    if(audio_handle == NULL){
        qm_task_exit(NULL);
        return ;
    }

    cfg = &audio_handle->cfg;

    const int voice_data_read_sz = qm_audio_recorder_get_default_read_size(audio_handle->rec_handle);
    uint8_t *voice_data = qm_calloc(1, voice_data_read_sz);

    while (1) {

        qm_event_get(&audio_handle->event, JOIN_EVENT_BIT | WAKEUP_REC_READING, QM_EVENT_AND, &bits, QM_WAIT_FOREVER);
        if(!(bits & JOIN_EVENT_BIT)){
            continue;
        }

        if (bits & WAKEUP_REC_READING) {
            int ret = qm_audio_recorder_read(audio_handle->rec_handle, (char *)voice_data, voice_data_read_sz);
            if (ret == 0 || ret == -1) {
                
            } else {
                volc_rtc_send_audio_data(audio_handle->volc_rtc_handle, cfg->room, voice_data, voice_data_read_sz);
            }
        }
    }

    qm_free(voice_data);
    qm_task_exit(NULL);
}

qm_audio_t qm_audio_open(qm_audio_cfg_t *cfg)
{
    int malloc_len = 0;
    qm_audio_rec_cfg_t rec_cfg = {0};
    qm_audio_prov_config_t prov_cfg = {0};
    int kv_len = sizeof(qm_audio_wifi_info_t);
    
    qm_audio_handle_t *audio_handle = (qm_audio_handle_t *)qm_calloc(1, sizeof(qm_audio_handle_t));
    if(audio_handle == NULL){
        return NULL;
    }
    memset(audio_handle, 0, sizeof(qm_audio_handle_t));

    memcpy(&audio_handle->cfg, cfg, sizeof(qm_audio_cfg_t));
    
    if(cfg->enable_prov){

        qm_kv_get(QM_AUDIO_PROV_INFO_KV_TAG, &audio_handle->wifi_info, &kv_len);

        if(!audio_handle->wifi_info.is_use){
            prov_cfg.pid = cfg->pid;
            prov_cfg.product_secret = cfg->product_secret;
            prov_cfg.userdata = audio_handle;
            prov_cfg.recv_fn = prov_recv_handler;
            
            audio_handle->prov_handle = qm_audio_prov_open(&prov_cfg);
            if(audio_handle->prov_handle == NULL){
                
            }
        }

    }

    audio_handle->player_handle = qm_audio_player_open(cfg->mic_num);
    if(audio_handle->player_handle == NULL){
        
    }

    malloc_len = sizeof(qm_audio_rec_table_t) * cfg->table_num;
    audio_handle->sr_table = (qm_audio_rec_table_t *)qm_calloc(1, malloc_len);
    if(audio_handle->sr_table == NULL){

    }

    for (int i = 0; i < cfg->table_num; i++)
    {
        if(cfg->table[i].rec_str == NULL){
            continue;
        }

        audio_handle->sr_table_num++;

        if(cfg->table[i].rec_type == QM_AUDIO_REC_CMD_WAKEUP){
            audio_handle->sr_table[i].type = QM_AUDIO_SR_WAKEUP_CMD;
        }else{
            audio_handle->sr_table[i].type = QM_AUDIO_SR_COMM_CMD;
        }
        
        audio_handle->sr_table[i].id = cfg->table[i].rec_id;
        audio_handle->sr_table[i].str= cfg->table[i].rec_str;
    }
    
    rec_cfg.userdata = audio_handle;
    rec_cfg.cb = qm_audio_rec_event_cb;
    rec_cfg.rec_table = audio_handle->sr_table;
    rec_cfg.table_num = audio_handle->sr_table_num;

    audio_handle->rec_handle = qm_audio_recorder_open(&rec_cfg);
    if(audio_handle->rec_handle == NULL){
        
    }

    return (qm_audio_t)audio_handle;
}

qm_err_t qm_audio_start(qm_audio_t handle)
{
    qm_audio_cfg_t *cfg = NULL;
    qm_audio_handle_t *audio_handle = (qm_audio_handle_t *)handle;

    if(audio_handle == NULL){
        return -QM_EINVAL;
    }

    cfg = &audio_handle->cfg;
    g_audio_handle = audio_handle;

    qm_event_register(QM_EVENT_WIFI, qm_event_handler, NULL);
    qm_event_register(QM_AUDIO_MAIN_EVENT_ID, qm_event_proccess_handler, NULL);

    if(!qm_mutex_is_valid(&g_rtc_audio.lock)){
        qm_mutex_new(&g_rtc_audio.lock);
    }

    if(!qm_sem_is_valid(&g_rtc_audio.sem)){
        qm_sem_new(&g_rtc_audio.sem, 0);
    }

    qm_audio_player_run(audio_handle->player_handle);
    qm_audio_recorder_run(audio_handle->rec_handle);

    qm_event_new(&audio_handle->event);

    if(cfg->enable_prov){
        if(!audio_handle->wifi_info.is_use){
            qm_audio_prov_start(audio_handle->prov_handle);
        }else{
            qm_event_post(QM_AUDIO_MAIN_EVENT_ID, QM_AUDIO_EVENT_TYPE_WIFI_SETTING_START, NULL, 0);
        }
    }

    if(cfg->enable_welcome){
        // audio_player_reply_type(audio_handle, audio_handle->role_id, QM_RECORD_TYPE_WELCOME);
    }

    qm_task_new_to_core(&audio_handle->read_task, "voice_read", voice_read_task, (void *)audio_handle, 4 * 1024, 5, 1);
    qm_task_new_to_core(&audio_handle->read_task, "audio_data_proc", audio_data_process_task, (void *)audio_handle, 4 * 1024, 10, 0);
    return QM_EOK;
}

#if 0
/**
 * @brief 模块连接AI平台
 *
 * @param[in] params 初始化结构体
 *
 * @return qm_sr_handle_t
 */
qm_err_t qm_audio_connect_ai(qm_audio_t handle)
{
    qm_audio_handle_t *audio_handle = (qm_audio_handle_t *)handle;
    if(audio_handle == NULL){
        return -QM_EINVAL;
    }

    return qm_ai_audio_start(audio_handle->ai_handle);
}

/**
 * @brief 模块断连AI平台
 *
 * @param[in] params 初始化结构体
 *
 * @return qm_sr_handle_t
 */
qm_err_t qm_audio_disconnect_ai(qm_audio_t handle)
{
    qm_audio_handle_t *audio_handle = (qm_audio_handle_t *)handle;
    if(audio_handle == NULL){
        return -QM_EINVAL;
    }

    return qm_ai_audio_stop(audio_handle->ai_handle);
}

qm_err_t qm_audio_player(qm_audio_t handle, int voice_type, int voice_id)
{


    return QM_EOK;
}
#endif