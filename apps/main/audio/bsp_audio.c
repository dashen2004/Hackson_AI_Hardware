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

#include "esp_heap_caps.h"  

#include "cJSON.h"
#include "util_httpc.h"

#include "esp_log.h"
#include "audio_thread.h"
#include "audio_mem.h"
#include "raw_stream.h"

#include "bsp_audio.h"
#include "bsp_audio_opus.h"
#include "audio_main.h"
#include "audio_cloud.h"

#include "qm_player.h"
#include "qm_spec_api.h"
#include "qm_utils_list.h"
#include "qm_iot_config.h"
#include "qm_audio_common.h"

#include "qm_audio_ws.h"
#include "qm_cli.h"
#include "qm_kv.h"

#include "qm_gpio.h"
#include "qm_utils_timer.h"
#include "es8311.h"
#include "audio_prodtst.h"
#include "xjiang_cloud.h"
#include "ui.h"

static const char* TAG = "volc_rtc";

#ifndef CONFIG_QM_AUDIO_URL_MAX_SIZE     
#define CONFIG_QM_AUDIO_URL_MAX_SIZE                   (64)
#endif

#define KV_TAG "AUDIO_MODE"

#define OPENING_KV_TAG "opening"


#define JOIN_EVENT_BIT           (1 << 0)

#define BACKOFF_BASE_TIME_MS    (10 * 1000)
#define BACKOFF_MIN_TIME_MS    (2 * 1000)

struct bsp_audio_handle_t {
    
    int mute;

    int ws_connect;
    int rec_type;

    int command_num;
    bsp_audio_cmd_table_t *command_str;

    recorder_pipeline_handle_t record_pipeline;
    player_pipeline_handle_t   player_pipeline;
    void                      *recorder_engine;
    EventGroupHandle_t         join_event;
    
    qm_player_t player_handle;
    qm_audio_ws_handle_t ai_handle;

    qm_audio_ws_recv_event_cb_t event_cb;
    void *user_data;

    qm_work_t  wakeup_timer;
    
    qm_work_t  ind_timer;

    qm_work_t  reconn_timer;

#if CONFIG_SUPPORT_ONLINE_OPEN_WAV
    int open_or_close;
#endif

    char audio_uri[CONFIG_QM_AUDIO_URL_MAX_SIZE + 1];

    int mode;
    int switch_mode_on;

    void *opus_handle;
    
    int tts_start;
    qm_work_t post_hello;

    qm_list_t text_list;
    qm_mutex_t text_lock;
    uint8_t wakeup_timeout;
    uint8_t network_ind_flag;
};

typedef struct 
{
    int mode;
}bsp_audio_cfg_t;

typedef struct 
{
    int dur_count;
    char payload[1];
}bsp_audio_text_handle_t;

static int audio_reset = 0;
static int open_or_close = QM_FALSE;

static bsp_audio_cfg_t audio_cfg = {
    .mode = QM_AUDIO_WS_LISTEN_MODE_REALTIME,
};

static struct bsp_audio_handle_t bsp_audio_handle = {0};

static void bsp_audio_ws_create_and_start(void);
static void ind_timeout_handle(void *arg);
static void wakeup_timeout_handle(void *arg);
static void handle_audio_info(char *pwbuf, int blen, int argc, char **argv);
static void qm_audio_ws_recv_event(qm_audio_ws_recv_evt_t *event, void *user_data);

static struct qm_cli_command audio_key = {
    .name = "audio",
    .help = "audio set/get <mode 1:auto 2:manual 3:realtime>",
    .function = handle_audio_info
};

uint8_t get_wakeup_timeout_flag(void) {
    return bsp_audio_handle.wakeup_timeout;
}

static void bsp_post_ws_hello_handle(void *arg){
    qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_SEND_HELLO, NULL, 0);
}

static void bsp_wakeup_time_refresh(void) {
    QM_LOGI(TAG, "bsp_wakeup_time_refresh");
    bsp_audio_handle.wakeup_timeout = 0;
    qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
    qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, NULL, CONFIG_AUDIO_WAKEUPT_TIMEOUT);
}

void qm_bsp_offline_voice_play(uint8_t index) {
    player_pipeline_stop(bsp_audio_handle.player_pipeline);
    switch (index) {
    case 1:
        qm_player_play_file(bsp_audio_handle.player_handle, XJIANG_TEST_WAV);
        break;
    default:
        break;
    }
}

void wakeup_timerout_immediately_trigger(uint8_t action) 
{
    switch (action) {
    case 1:
        break;
    case 2:
        xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
        bsp_audio_handle.open_or_close = QM_FALSE;
        qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);
        qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID, NULL, 0);
        break;
    default:
        break;
    }
}

uint8_t qm_bsp_voice_is_idle(void) {
    return qm_player_is_idle(bsp_audio_handle.player_handle);
}

static void handle_audio_info(char *pwbuf, int blen, int argc, char **argv)
{
    if(argc == 2){
        qm_cli_printf("audio mode=%d.\r\n", audio_cfg.mode);
    }else if (argc == 3) {
        audio_cfg.mode = atoi(argv[2]);
        qm_kv_set(KV_TAG, &audio_cfg, sizeof(bsp_audio_cfg_t), 1);
        qm_cli_printf("OK\r\n");
    } else {
        qm_cli_printf("Error\r\n");
        return;
    }
}

void qm_audio_ws_data_cb(void *data, int len, int len_offset, int play_status, void *user_data)
{
    int pcm_size = 0;
    static int post_count = 0;
    static char pcm_data[1920] = {0};
    qm_list_node_t  *node = NULL;
    bsp_audio_text_handle_t *text_handle = NULL;
    
    qm_audio_ws_recv_evt_t recv = {0};

    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_TTS;
    recv.data.tts.state = QM_AUDIO_WS_TTS_STATE_SENTENCE_START;

    switch (play_status)
    {
        case QM_AUDIO_WS_PLAYER_STATE_START:
            QM_LOGD("1", "QM_AUDIO_WS_PLAYER_STATE_START");

            bsp_wakeup_time_refresh();

            qm_mutex_lock(&bsp_audio_handle.text_lock, QM_WAIT_FOREVER);
            node = qm_list_lpop(&bsp_audio_handle.text_list);
            qm_mutex_unlock(&bsp_audio_handle.text_lock);
            if(node == NULL){
                return ;
            }
            text_handle = (bsp_audio_text_handle_t *)qm_list_node_val_get(node);

            post_count = text_handle->dur_count;
            recv.data.tts.text = text_handle->payload;

            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(&recv, bsp_audio_handle.user_data);
            }

            qm_list_node_destroy(node);

        break;
        
        case QM_AUDIO_WS_PLAYER_STATE_DATA:
            
            post_count--;
            
            if(!post_count){
                qm_mutex_lock(&bsp_audio_handle.text_lock, QM_WAIT_FOREVER);
                node = qm_list_lpop(&bsp_audio_handle.text_list);
                qm_mutex_unlock(&bsp_audio_handle.text_lock);
                if(node == NULL){
                    return ;
                }
                text_handle = (bsp_audio_text_handle_t *)qm_list_node_val_get(node);  
                post_count = text_handle->dur_count;
                recv.data.tts.text = text_handle->payload;
                if(bsp_audio_handle.event_cb){
                    bsp_audio_handle.event_cb(&recv, bsp_audio_handle.user_data);
                }

                qm_list_node_destroy(node);
            }

        // if(bsp_audio_handle.tts_start){
        
            pcm_size = bsp_audio_opus_decode_data(bsp_audio_handle.opus_handle, data, len, pcm_data);
            raw_stream_write(player_pipeline_get_raw_write(bsp_audio_handle.player_pipeline), pcm_data, pcm_size);
        // }

            qm_msleep(40);
        break;

        case QM_AUDIO_WS_PLAYER_STATE_STOP:
            QM_LOGD("1", "QM_AUDIO_WS_PLAYER_STATE_STOP");
            if(bsp_audio_handle.open_or_close){
                bsp_wakeup_time_refresh();
            } else {
                screen_wakeup_timeout_exit();
            }
        break;

        default:
        break;
    }


}

static void wakeup_timeout_handle(void *arg)
{
#if CONFIG_SUPPORT_ONLINE_CLOSE_WAV
    if(!bsp_audio_handle.open_or_close){
        return;
    }
    QM_LOGI(TAG, "qm_audio_ws_closing_remarks");
    xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
    qm_audio_ws_closing_remarks(bsp_audio_handle.ai_handle, CONFIG_AUDIO_WS_TIMEOUT);
    bsp_audio_handle.open_or_close = QM_FALSE;
    qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);
    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID, NULL, 0);
#endif
}

static void ind_timeout_handle(void *arg)
{
    int ind_type = (int)arg;
    qm_audio_ws_listen_param_t listen = {
        .mode = audio_cfg.mode
    };
    static int net_state  = CONFIG_AUDIO_SUB_EVENT_NET_ERROR;

    qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);

    switch (ind_type)
    {

        case CONFIG_AUDIO_SUB_EVENT_CONFIG:
            if(!audio_reset){
                player_pipeline_stop(bsp_audio_handle.player_pipeline);
                qm_player_play_file(bsp_audio_handle.player_handle, NETCONFIG_WAV);
            }
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONFIG_IND_TIMEOUT);
        break;
        
        case CONFIG_AUDIO_SUB_EVENT_NET_ERROR:
            
            bsp_audio_handle.ws_connect = QM_FALSE;
            
            xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);

            player_pipeline_stop(bsp_audio_handle.player_pipeline);
            qm_player_play_file(bsp_audio_handle.player_handle, CONN_ERROR_WAV);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONN_ERROR_TIMEOUT);
        break;

        case CONFIG_AUDIO_SUB_EVENT_CONN_WS:

            if(net_state == CONFIG_AUDIO_SUB_EVENT_NET_ERROR){

                if(!audio_reset){
                    player_pipeline_stop(bsp_audio_handle.player_pipeline);
                #ifdef CONN_CLOUD_WAV
                    qm_player_play_file(bsp_audio_handle.player_handle, CONN_CLOUD_WAV);
                #endif
                }

            }

            bsp_audio_handle.network_ind_flag = 1;

            net_state  = CONFIG_AUDIO_SUB_EVENT_CONN_WS;

            if(audio_reset && open_or_close){
                bsp_wakeup_time_refresh();

                bsp_audio_handle.open_or_close = QM_TRUE;

                if(bsp_audio_handle.rec_type != AUDIO_REC_WAKEUP_START){
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
                }else{
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_DETECT;
                }

                bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
                qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, CONFIG_AUDIO_WS_TIMEOUT);
                
                qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_TRUE);
                xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            }

            audio_reset = 0;

        #if CONFIG_DISABLE_SR_FUNC
            
            listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
            bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
            qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, CONFIG_AUDIO_WS_TIMEOUT);

            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, &bsp_audio_handle.command_str[0], 0);
        #endif

            net_state  = CONFIG_AUDIO_SUB_EVENT_CONN_WS;

        break;

        default:
            break;
    }
}

static void reconn_timeout_handle(void *arg)
{
    qm_audio_ws_start(bsp_audio_handle.ai_handle); //重连ws
}

static void qm_audio_event_handler(qm_input_event_t *input_event, void *arg)
{
    uint32_t len = CONFIG_QM_AUDIO_URL_MAX_SIZE;
    int event_type = input_event->sub_event;
    bsp_audio_cmd_table_t *cmd_table = NULL;

#if CONFIG_SUPPORT_ONLINE_OPEN_WAV
    static int open_or_close = QM_FALSE;
#endif

    switch(input_event->sub_event){
        case CONFIG_AUDIO_USB_EVENT_CONNING_LOCAL:
        case CONFIG_AUDIO_SUB_EVENT_CONFIG:
        case CONFIG_AUDIO_SUB_EVENT_CONN_WS:
        case CONFIG_AUDIO_SUB_EVENT_SWITCH_MODE_ID:
        case CONFIG_AUDIO_SUB_EVENT_SWITCH_COMPLETE_ID:
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)event_type, 0);
        break;

        case CONFIG_AUDIO_SUB_EVENT_NET_ERROR:
            bsp_audio_handle.ws_connect = QM_FALSE;
            bsp_audio_handle.open_or_close = QM_FALSE;
            qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);

            qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_FALSE);
            qm_audio_recorder_enable_sr_mode(bsp_audio_handle.recorder_engine, QM_TRUE);
            
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)event_type, CONFIG_AUDIO_CONN_ERROR_TIMEOUT);
        
        break;

        case CONFIG_AUDIO_SUB_EVENT_CEART_WS:
        {

            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_cancel_delayed_action(&bsp_audio_handle.reconn_timer);
            
            bsp_audio_ws_create_and_start();
        }

        break;

        case CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID:
            xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            qm_audio_ws_listen_param_t listen = {
                .mode = audio_cfg.mode
            };
            QM_LOGD("1","CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID");
            if(bsp_audio_handle.ws_connect){
                listen.state = QM_AUDIO_WS_LISTEN_STATE_STOP;
                bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_END;
                qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, CONFIG_AUDIO_WS_TIMEOUT);
            }
            bsp_audio_handle.wakeup_timeout = 1;
            qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_FALSE);
            qm_audio_recorder_enable_sr_mode(bsp_audio_handle.recorder_engine, QM_TRUE);

        break;

        case CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID: 

            bsp_wakeup_time_refresh();

        #if CONFIG_SUPPORT_ONLINE_OPEN_WAV
            
            if(bsp_audio_handle.open_or_close){
                xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
                break;
            }

            if(bsp_audio_handle.ws_connect){
                bsp_audio_handle.open_or_close = QM_TRUE;
                qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);
                qm_audio_ws_opening_remarks(bsp_audio_handle.ai_handle, CONFIG_AUDIO_WS_TIMEOUT);    
                QM_LOGI(TAG, "qm_audio_ws_opening_remarks");
            } else {
                if(bsp_audio_handle.network_ind_flag == 0) {
                    bsp_audio_handle.network_ind_flag = 1;
                    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONFIG, NULL, 0);
                }
            }

        #else

            cmd_table = (bsp_audio_cmd_table_t *)input_event->value;
            if(cmd_table){
                QM_LOGI(TAG, "AUDIO_REC:%s", cmd_table->sr_str);
                if(cmd_table->reply_str){
                    player_pipeline_stop(bsp_audio_handle.player_pipeline);
                    qm_player_play_file(bsp_audio_handle.player_handle, cmd_table->reply_str);
                }
            }
            xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);

        #endif
        break;
    
        default:
        break;
    }
}

static qm_err_t rec_engine_cb(audio_rec_evt_t *event, void *user_data)
{
    int res = 0;
    int ind_type = 0;
    qm_audio_ws_listen_param_t listen = {
        .mode = audio_cfg.mode
    };
    if(get_ota_status_flag() != 0) {
        return ESP_OK;
    }
    switch (event->type)
    {
        case AUDIO_REC_WAKEUP_START:

            if(audio_cfg.mode == QM_AUDIO_WS_LISTEN_MODE_MANUAL){
                break;
            }
            
            QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_START");
            
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);

            player_pipeline_stop(bsp_audio_handle.player_pipeline);

        #if !CONFIG_DISABLE_SR_FUNC
            if(bsp_audio_handle.ws_connect){

                bsp_wakeup_time_refresh();
                if(bsp_audio_handle.rec_type != AUDIO_REC_WAKEUP_START){
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
                }else{
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_DETECT;
                }

                bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
                qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, CONFIG_AUDIO_WS_TIMEOUT);
                
                qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_TRUE);
            
            }
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, &bsp_audio_handle.command_str[0], 0);
        #endif
        break;

        case AUDIO_REC_VAD_START:
            QM_LOGI(TAG, "AUDIO_REC_VAD_START");
        break;
        
        case AUDIO_REC_VAD_END:
            QM_LOGI(TAG, "AUDIO_REC_VAD_END");
        break;
        
        case AUDIO_REC_WAKEUP_END:
            
        break;

        default:

            qm_audio_sr_mn_result_t *mn_res = (qm_audio_sr_mn_result_t *)event->event_data;
            if(mn_res == NULL){
                return ESP_OK;
            }

            for (int i = 0; i < bsp_audio_handle.command_num; i++)
            {
                if(mn_res->phrase_id == bsp_audio_handle.command_str[i].id){
                    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONTROL_ID, &bsp_audio_handle.command_str[i], 0);
                    break;
                }
            }

        break;
    }
    return ESP_OK;
}

static void audio_player_dev_cb(qm_player_t handle, qm_player_event_type_t event, void *arg)
{
    switch (event)
    {
        case QM_PLAYER_EVENT_IDLE:
            if(bsp_audio_handle.ws_connect && 
                bsp_audio_handle.rec_type == AUDIO_REC_WAKEUP_START)
            {
                player_pipeline_run(bsp_audio_handle.player_pipeline);
                if(audio_cfg.mode == QM_AUDIO_WS_LISTEN_MODE_AUTO){
                    xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
                }
            }
            break;
        
        default:
            break;
    }
}

static void qm_audio_ws_tts_chat_handler(qm_audio_ws_recv_evt_t *event)
{
    bsp_wakeup_time_refresh();
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){

    #if CONFIG_AUDIO_BOARD_USER_BUTTON    
        adc_ind_type = false;
    #endif
        
        if(!bsp_audio_handle.mute){
            es8311_set_voice_mute(false);
        }
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
            
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){
    #if CONFIG_AUDIO_BOARD_USER_BUTTON
        adc_ind_type = true;
    #endif
    }
}

static void qm_audio_ws_tts_opening_handler(qm_audio_ws_recv_evt_t *event)
{

#if CONFIG_SUPPORT_ONLINE_OPEN_WAV
    bsp_wakeup_time_refresh();
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){

        xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
    #if CONFIG_AUDIO_BOARD_USER_BUTTON    
        adc_ind_type = false;
    #endif
        if(!bsp_audio_handle.mute){
            es8311_set_voice_mute(false);
        }
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
                
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){
    #if CONFIG_AUDIO_BOARD_USER_BUTTON
        adc_ind_type = true;
    #endif
        xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
    }
#endif
}

static void qm_audio_ws_tts_closing_handler(qm_audio_ws_recv_evt_t *event)
{

#if CONFIG_SUPPORT_ONLINE_CLOSE_WAV
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){
        xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);

    #if CONFIG_AUDIO_BOARD_USER_BUTTON    
        adc_ind_type = false;
    #endif

        if(!bsp_audio_handle.mute){
            es8311_set_voice_mute(false);
        }
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
                
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){
    #if CONFIG_AUDIO_BOARD_USER_BUTTON
        adc_ind_type = true;
    #endif
    #if 0
        bsp_audio_handle.open_or_close = QM_FALSE;
        qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);
        qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID, NULL, 0);
    #endif
    }
#endif
}

static void qm_audio_ws_recv_event(qm_audio_ws_recv_evt_t *event, void *user_data)
{
    uint32_t next_backoff = 0;
    qm_list_node_t  *node = NULL;
    bsp_audio_text_handle_t *text_handle = NULL;

    if(get_ota_status_flag() != 0) {
        return;
    }
    
    switch (event->recv_type)
    {
        case QM_AUDIO_WS_RECV_TYPE_CONNECT:
            qm_cancel_delayed_action(&bsp_audio_handle.reconn_timer);

        break;

        case QM_AUDIO_WS_RECV_TYPE_HELLO_REPLY:
            
            bsp_wakeup_time_refresh();
            qm_cancel_delayed_action(&bsp_audio_handle.post_hello);

            bsp_audio_handle.ws_connect = QM_TRUE;
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONN_WS, NULL, 0);

        break;

        case QM_AUDIO_WS_RECV_TYPE_STT:

            bsp_wakeup_time_refresh();
        
            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(event, bsp_audio_handle.user_data);
            }

        break;

        case QM_AUDIO_WS_RECV_TYPE_LLM:
            
            bsp_wakeup_time_refresh();

            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(event, bsp_audio_handle.user_data);
            }

        break;

        case QM_AUDIO_WS_RECV_TYPE_TTS:

            switch (event->data.tts.tts_type)
            {

                case QM_AUDIO_WS_TTS_TYPE_CHAT:
                    qm_audio_ws_tts_chat_handler(event);
                break;

                case QM_AUDIO_WS_TTS_TYPE_OPENING:
                    qm_audio_ws_tts_opening_handler(event);
                    break;

                    case QM_AUDIO_WS_TTS_TYPE_CLOSING:
                    qm_audio_ws_tts_closing_handler(event);
                break;

                default:
                break;
            }


            switch (event->data.tts.state)
            {

                case QM_AUDIO_WS_TTS_STATE_START:
                    qm_mutex_lock(&bsp_audio_handle.text_lock, QM_WAIT_FOREVER);
                    while (1)
                    {
                        node = qm_list_lpop(&bsp_audio_handle.text_list);
                        if(node == NULL){
                            break;
                        }
                        qm_list_node_destroy(node);
                    }
                    qm_mutex_unlock(&bsp_audio_handle.text_lock);
                break;

                case QM_AUDIO_WS_TTS_STATE_SENTENCE_START:

                    node = qm_list_node_extra_new(sizeof(bsp_audio_text_handle_t) + strlen(event->data.tts.text));
                    if(node == NULL){
                        return ;
                    }
                    text_handle =(bsp_audio_text_handle_t *)qm_list_node_val_get(node);

                    text_handle->dur_count = event->data.tts.duration_ms / CONFIG_AUDIO_FMT_DURATION_MS;
                    memcpy(text_handle->payload, event->data.tts.text, strlen(event->data.tts.text));

                    qm_mutex_lock(&bsp_audio_handle.text_lock, QM_WAIT_FOREVER);
                    qm_list_rpush(&bsp_audio_handle.text_list, node);
                    qm_mutex_unlock(&bsp_audio_handle.text_lock); 
                break;

                default:
                break;
            }

        #if 0
            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(event, bsp_audio_handle.user_data);
            }
        #endif

        break;

        case QM_AUDIO_WS_RECV_TYPE_DISCONNECT:
            
            
            qm_cancel_delayed_action(&bsp_audio_handle.reconn_timer);

            bsp_audio_handle.ws_connect = QM_FALSE;
            xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            

            qm_audio_ws_stop(bsp_audio_handle.ai_handle);

            qm_srandom((uint32_t)qm_now_ms());

            next_backoff += qm_random_get(BACKOFF_BASE_TIME_MS);
            if(next_backoff > BACKOFF_BASE_TIME_MS){
                next_backoff = BACKOFF_BASE_TIME_MS;
            } else if(next_backoff < BACKOFF_MIN_TIME_MS){
                next_backoff = BACKOFF_MIN_TIME_MS;
            }

            QM_LOGW(TAG, "WS DISCONN - RECONNECT_NEXT:%d", next_backoff);
            qm_post_delayed_action(&bsp_audio_handle.reconn_timer, reconn_timeout_handle, NULL, next_backoff);
            
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_NET_ERROR, NULL, 0);
        break;
        
        case QM_AUDIO_WS_RECV_TYPE_SET:
            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(event, bsp_audio_handle.user_data);
            }
        break;

        default:
            break;
    }
}

static void voice_read_task(void *args)
{
    const int voice_data_read_sz = sizeof(int16_t) * recorder_pipeline_get_default_read_size(bsp_audio_handle.record_pipeline);
    int16_t *voice_data = audio_calloc(1, voice_data_read_sz);
    
    int send_size = 0;
    const int encoder_read_sz = sizeof(int16_t) * recorder_pipeline_get_default_read_size(bsp_audio_handle.record_pipeline);
    int16_t *encoder_data = audio_calloc(1, encoder_read_sz);

    EventBits_t old_bits = 0;
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT, false, true, 0);
        
        if(old_bits != bits){
            if((bits & JOIN_EVENT_BIT)){
                QM_LOGD("1", "SEND START");
            }else{
                QM_LOGD("1", "SEND END");
            }
            old_bits = bits;
            qm_audio_recorder_data_reset(bsp_audio_handle.recorder_engine);
        }

        int read_len = qm_audio_recorder_data_read(bsp_audio_handle.recorder_engine, voice_data, voice_data_read_sz, portMAX_DELAY);
        if (read_len == voice_data_read_sz && (bits & JOIN_EVENT_BIT) && bsp_audio_handle.ws_connect) {
            send_size = bsp_audio_opus_encode_data(bsp_audio_handle.opus_handle, voice_data, encoder_data);
            if(send_size){
                // qm_audio_ws_data_cb(encoder_data, send_size, voice_data); 
                // QM_LOGD("1","audio raw_len:%d, enc_len:%d", voice_data_read_sz, send_size);
                // QM_HEX_LOGD("1","audio ", encoder_data, send_size);
                qm_audio_ws_data_send(bsp_audio_handle.ai_handle, encoder_data, send_size, BACKOFF_BASE_TIME_MS);   
            }
        }

    }
}

qm_err_t bsp_audio_resgiter(qm_audio_ws_recv_event_cb_t event_cb, void *user_data)
{

    bsp_audio_handle.event_cb = event_cb;
    bsp_audio_handle.user_data = user_data;

    return QM_EOK;
}

qm_err_t bsp_audio_init(bsp_audio_cmd_table_t *command_str, int command_num)
{
    int len = sizeof(int);
    qm_audio_sr_cmd_table_t *sr_cmd = NULL;
    pipe_record_engine_params_t engine_params = {0};

    // qm_cli_register_command(&audio_key);
    audio_reset = qm_is_crash_reset();
    qm_kv_get(OPENING_KV_TAG, &open_or_close, &len);

    qm_event_register(CONFIG_AUDIO_MAIN_EVENT_ID, qm_audio_event_handler, NULL);

    bsp_audio_handle.player_pipeline = player_pipeline_open();
    bsp_audio_handle.record_pipeline = recorder_pipeline_open();

    bsp_audio_handle.command_str = command_str;
    bsp_audio_handle.command_num = command_num;

    if(command_str && command_num){
        sr_cmd = (qm_audio_sr_cmd_table_t *)qm_malloc(sizeof(qm_audio_sr_cmd_table_t) * command_num);
        for (int i = 0; i < command_num; i++)
        {
            sr_cmd[i].type = command_str[i].type;
            sr_cmd[i].id = command_str[i].id;
            sr_cmd[i].str = command_str[i].sr_str;
            sr_cmd[i].prob = command_str[i].prob;
        }
    }
    if(prodtst_config_get() == 1) {
        engine_params.cb = prodtst_rec_engine_cb;
    } else {
        engine_params.cb = rec_engine_cb;
    }
    engine_params.command_str = sr_cmd;
    engine_params.command_num = command_num;
    bsp_audio_handle.wakeup_timeout = 1;

    bsp_audio_handle.recorder_engine = audio_record_engine_init(bsp_audio_handle.record_pipeline, &engine_params);

    player_pipeline_run(bsp_audio_handle.player_pipeline);
    recorder_pipeline_run(bsp_audio_handle.record_pipeline);

    bsp_audio_handle.player_handle = qm_player_init();
    if(bsp_audio_handle.player_handle == NULL){
        
    }
    qm_player_setopt(bsp_audio_handle.player_handle, QM_PLAYER_OPT_USERDATA, &bsp_audio_handle);
    qm_player_setopt(bsp_audio_handle.player_handle, QM_PLAYER_OPT_EVENT_HANDLER, audio_player_dev_cb);

#if CONFIG_DISABLE_SR_FUNC
    qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_TRUE);    
#endif

#ifdef POWER_ON_WAV
    qm_player_play_file(bsp_audio_handle.player_handle, POWER_ON_WAV);
#endif
    bsp_audio_handle.join_event = xEventGroupCreate();

    bsp_audio_handle.opus_handle = bsp_audio_opus_encode_create(CONFIG_AUDIO_FMT_RATE, CONFIG_AUDIO_FMT_CHANNEL, CONFIG_AUDIO_FMT_DURATION_MS);

    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 30 * 1024, 5, true, 1);

    return QM_EOK;
}

qm_err_t bsp_audio_ota_set_voice_mute(void)
{
    xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
    bsp_audio_set_voice_mute(true);
    return QM_EOK;
}

qm_err_t bsp_audio_set_voice_mute(bool_t enable)
{
    bsp_audio_handle.mute = enable;
    if(enable){
        es8311_set_voice_mute(true);
    }else{
        es8311_set_voice_mute(false);
    }

    return QM_EOK;
}

bool_t bsp_audio_get_voice_mute(void)
{
    return bsp_audio_handle.mute;
}

static void bsp_audio_ws_create_and_start(void)
{
    uint32_t len = CONFIG_QM_AUDIO_URL_MAX_SIZE;
    qm_audio_ws_cfg_t params = {
        .uri = CONFIG_NETWORK_URL,
        .buff_size = 10 * 1024,
        .task_prio = 10,
        .task_size = 20 * 1024,

        .pid = qm_iot_pid_get(),
        .did = qm_iot_did_get(),
        .sn = qm_iot_sn_get(),

        .connect_timeout = 10,

        .fmt_type = QM_AUDIO_WS_FMT_TYPE_OPUS,
        .rate = CONFIG_AUDIO_FMT_RATE,
        .bits = CONFIG_AUDIO_FMT_BITS,
        .channel = CONFIG_AUDIO_FMT_CHANNEL,
        .frame_duration_ms = CONFIG_AUDIO_FMT_DURATION_MS,
        .flow_ctrl_enbale = CONFIG_AUDIO_FMT_FLOW_CTRL_ENABLE,
        .max_len = CONFIG_AUDIO_FMT_MAX_LEN,
        .min_len = CONFIG_AUDIO_FMT_MIN_LEN,
        .min_speed = CONFIG_AUDIO_FMT_MIN_SPEED,
        .max_speed = CONFIG_AUDIO_FMT_MAX_SPEED,
        .event_cb = qm_audio_ws_recv_event,
        .data_cb  = qm_audio_ws_data_cb,
    };

    if(bsp_audio_handle.ai_handle == NULL){

        qm_iot_audio_uri_get(bsp_audio_handle.audio_uri, &len);
        if(bsp_audio_handle.audio_uri[0] == '\0'){
            QM_LOGW("AUDIO", "dy audio uri NULL");
        }else{
            params.uri = bsp_audio_handle.audio_uri;
        }

        bsp_audio_handle.ai_handle = qm_audio_ws_create(&params);
        if(bsp_audio_handle.ai_handle == NULL){
            return ;
        }
        qm_audio_ws_start(bsp_audio_handle.ai_handle);
    } else {
        qm_audio_ws_stop(bsp_audio_handle.ai_handle);
        qm_audio_ws_start(bsp_audio_handle.ai_handle);
        if(bsp_audio_handle.ws_connect == QM_FALSE) {
            qm_cancel_delayed_action(&bsp_audio_handle.post_hello);
            qm_post_delayed_action(&bsp_audio_handle.post_hello, bsp_post_ws_hello_handle, NULL, 1000);
        }
    }

}
