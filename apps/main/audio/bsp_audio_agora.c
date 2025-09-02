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

#include "qm_cli.h"
#include "qm_kv.h"
#include "qm_utils_base64.h"

#include "qm_gpio.h"
#include "bsp_light.h"
#include "qm_utils_timer.h"
#include "es8311.h"
#include "audio_prodtst.h"
#include "xjiang_cloud.h"
#include "ui.h"
#include "agora_rtc_api.h"
#include "qm_agora_gen.h"
#include "qm_utils_string.h"

#define KV_TAG "AUDIO_MODE"

static const char* TAG = "volc_rtc";


#define CONFIG_ENABLE_AUDIO_MIXING 0

/* Application ID */
#define CONFIG_AGORA_APP_ID "9942a7948c3f4c4f8970d54ef6adfb24" // Please replace with your own APP ID

#define DEFAULT_TOKEN "007eJxTYLg7zUw2zP8Qrxt37s8Kv1KpjMPfMj0NX/gcvXi08OFKnqUKDEapqakmFqYWaeYpliamhhaJJoYWpqnmSZYWSebGpknmX4K2ZjQEMjKolEgxMEIhiM/GYGZoYGRkzMAAAFuAHjk="
// #define DEFAULT_TOKEN NULL
#define DEFAULT_CHANNEL_NAME "610223"
#define DEFAULT_USER_ID 0 // sdk will automatically assign a random user id



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

    qm_task_t                     binary_task;
    qm_sem_t                      binary_sem;
    qm_list_t                     binary_list;
    qm_mutex_t                    binary_lock;

    qm_task_t                     stream_task;
    qm_sem_t                      stream_sem;
    qm_list_t                     stream_list;
    qm_mutex_t                    stream_lock;
    
    qm_iot_agora_gen_handle_t   *agora_handle;
    uint8_t agora_init_flag;
};

typedef struct{
    uint8_t binary;
    int offset;
    int payload_len;
    uint8_t *payload;
}qm_audio_frame_t;

typedef struct{
    int index;
    int total;
    char msg_id[32];
    uint32_t payload_length;
    uint8_t *payload;
}qm_audio_stream_t;

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

static connection_id_t g_conn_id;
static connection_info_t g_conn_info;


static struct bsp_audio_handle_t bsp_audio_handle = {0};

static void bsp_audio_ws_create_and_start(void);
static void ind_timeout_handle(void *arg);
static void wakeup_timeout_handle(void *arg);
static void handle_audio_info(char *pwbuf, int blen, int argc, char **argv);

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

void bsp_audio_llm_emotion_set(char *emotion)
{
    qm_audio_ws_recv_evt_t recv = {0};

    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_LLM;
    recv.data.llm.emotion = emotion;
    if(bsp_audio_handle.event_cb) {
        bsp_audio_handle.event_cb(&recv, bsp_audio_handle.user_data);
    }
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
    static char pcm_data[1920] = {0};

    switch (play_status)
    {
        case QM_AUDIO_WS_PLAYER_STATE_START:
            // bsp_wakeup_time_refresh();
        break;
        
        case QM_AUDIO_WS_PLAYER_STATE_DATA:
            // if(bsp_audio_handle.tts_start){
            
                pcm_size = bsp_audio_opus_decode_data(bsp_audio_handle.opus_handle, data, len, pcm_data);
                raw_stream_write(player_pipeline_get_raw_write(bsp_audio_handle.player_pipeline), pcm_data, pcm_size);
            // }
        break;

        case QM_AUDIO_WS_PLAYER_STATE_STOP:
           if(bsp_audio_handle.open_or_close){
                // bsp_wakeup_time_refresh();
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
    voice_corpus_request(2);
    // qm_audio_ws_closing_remarks(bsp_audio_handle.ai_handle, CONFIG_AUDIO_WS_TIMEOUT);
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
                // qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, CONFIG_AUDIO_WS_TIMEOUT);
                
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
                bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_END;
                listen.state = QM_AUDIO_WS_LISTEN_STATE_STOP;
                // qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, CONFIG_AUDIO_WS_TIMEOUT);
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
                // qm_audio_ws_opening_remarks(bsp_audio_handle.ai_handle, CONFIG_AUDIO_WS_TIMEOUT);    
                QM_LOGI(TAG, "qm_audio_ws_opening_remarks");
                voice_corpus_request(1);
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
                // qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, CONFIG_AUDIO_WS_TIMEOUT);
                
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

static void audio_data_process_task(void *args)
{
    #if 0
        unsigned int size = 0; 
    #endif

    int list_len = 0;
    int len_offset = 0;
    int play_status = 0;
    int speed = 0;
    int timeout_count = 0;
    uint32_t timeout = QM_WAIT_FOREVER;
    qm_list_node_t *node = NULL;
    qm_audio_frame_t *ws_node = NULL;
    struct bsp_audio_handle_t  *audio_ctx = (struct bsp_audio_handle_t  *)args;
    while (1) {
    #if 0
        qm_audio_ws_frame_t frame = { 0 };
        qm_queue_recv(&audio_ctx->binary_queue, &frame, &size, timeout);
    #endif

        qm_sem_wait(&audio_ctx->binary_sem, timeout);

        qm_mutex_lock(&audio_ctx->binary_lock, QM_WAIT_FOREVER);
        node = qm_list_lpop(&audio_ctx->binary_list);
        qm_mutex_unlock(&audio_ctx->binary_lock);

        if(node == NULL){

            len_offset = 0;
            timeout = QM_WAIT_FOREVER;

            if(play_status){
                play_status = QM_AUDIO_WS_PLAYER_STATE_STOP;
                qm_audio_ws_data_cb(NULL, 0, 0, QM_AUDIO_WS_PLAYER_STATE_STOP,audio_ctx->user_data);
            }
   
            continue;
        }

        ws_node = (qm_audio_frame_t *)qm_list_node_val_get(node);

        if (ws_node->payload == NULL) {
            if(timeout_count >= CONFIG_QM_AUDIO_WS_QUEUE_TIMEOUT_COUNT){
                timeout_count = 0;
                timeout = QM_WAIT_FOREVER;
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

        len_offset++;

        if(play_status == QM_AUDIO_WS_PLAYER_STATE_STOP){
            play_status = QM_AUDIO_WS_PLAYER_STATE_START;
            qm_audio_ws_data_cb(NULL, 0, 0, QM_AUDIO_WS_PLAYER_STATE_START,audio_ctx->user_data);
        }
        
        qm_audio_ws_data_cb(ws_node->payload, ws_node->payload_len, len_offset, QM_AUDIO_WS_PLAYER_STATE_DATA, audio_ctx->user_data);
    
        qm_free(ws_node->payload);
        qm_list_node_destroy(node);

        qm_mutex_lock(&audio_ctx->binary_lock, QM_WAIT_FOREVER);
        list_len = qm_list_len_get(&audio_ctx->binary_list);
        qm_mutex_unlock(&audio_ctx->binary_lock);


        if(list_len){
            qm_sem_signal(&audio_ctx->binary_sem);
        }

#if 0
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
        }
#endif
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
                audio_frame_info_t info = { 0 };
                info.data_type = AUDIO_DATA_TYPE_OPUS;

                int rval = agora_rtc_send_audio_data(g_conn_id, encoder_data, send_size, &info);
                if (rval < 0) {
                    printf("Failed to send audio data, reason: %s\n", agora_rtc_err_2_str(rval));
                    return ;
                }
            }
        }

    }
}

typedef struct {
    int total;
    qm_audio_stream_t **packet_data;
} stream_message_cache_t;

static stream_message_cache_t current_msg_cache = {0};

static void clear_message_cache(void)
{
    if (current_msg_cache.packet_data) {
        for (int i = 0; i < current_msg_cache.total; i++) {
            if (current_msg_cache.packet_data[i]) {
                if(current_msg_cache.packet_data[i]->payload) {
                    qm_free(current_msg_cache.packet_data[i]->payload);
                    current_msg_cache.packet_data[i]->payload = NULL;
                }
                qm_free(current_msg_cache.packet_data[i]);
                current_msg_cache.packet_data[i] = NULL;
            }
        }
        qm_free(current_msg_cache.packet_data);
        current_msg_cache.packet_data = NULL;
    }

    current_msg_cache.total = 0;
}

static void stream_msg_base64_decode(uint8_t *data, size_t len) 
{
    uint8_t *decoded_msg = NULL;
    uint32_t decoded_len = 0;
    decoded_msg = (uint8_t *)qm_malloc(len);
    if (decoded_msg == NULL) {
        return;
    }
    memset(decoded_msg, 0, len);
    qm_utils_base64decode((const uint8_t*)data, len, len, decoded_msg, &decoded_len);
    if(decoded_len == 0) {
        goto __FREE;
    }
    QM_LOGD("1", "stream message: %.*s", decoded_len, decoded_msg);
    cJSON *json = cJSON_Parse((char*)decoded_msg);
    if (json == NULL) {
        goto __FREE;
    }
    cJSON *object = cJSON_GetObjectItem(json, "object");
    if (object == NULL || cJSON_IsString(object) == false) {
        goto __FREE;
    }
    const char *obj_type = cJSON_GetStringValue(object);
    if (strcmp(obj_type, "assistant.transcription") != 0) {
        goto __FREE;
    }
    cJSON *text = cJSON_GetObjectItem(json, "text");
    if (text && cJSON_IsString(text)) {
        QM_LOGI("1", "Assistant says: %s", cJSON_GetStringValue(text));
    }
    cJSON_Delete(json);

__FREE:
    qm_free(decoded_msg);
    decoded_msg = NULL;
}

static void stream_msg_task(void *args)
{
    int list_len = 0;
    qm_list_node_t *node = NULL;
    qm_audio_stream_t *msg_node = NULL;
    struct bsp_audio_handle_t *audio_ctx = (struct bsp_audio_handle_t  *)args;
    while (1) {
        qm_sem_wait(&audio_ctx->stream_sem, QM_WAIT_FOREVER);

        qm_mutex_lock(&audio_ctx->stream_lock, QM_WAIT_FOREVER);
        node = qm_list_lpop(&audio_ctx->stream_list);
        qm_mutex_unlock(&audio_ctx->stream_lock);

        if(node == NULL){
            continue;
        }

        msg_node = (qm_audio_stream_t *)qm_list_node_val_get(node);

        if (msg_node->payload == NULL) {
            goto __FREE;
        }
        
        if(msg_node->total == 1){
            stream_msg_base64_decode(msg_node->payload, msg_node->payload_length);
            goto __FREE;
        }
        
        if(msg_node->index == 1) {
            clear_message_cache();
            current_msg_cache.total = msg_node->total;
            current_msg_cache.packet_data = (qm_audio_stream_t **)qm_malloc(sizeof(qm_audio_stream_t *) * current_msg_cache.total);
            if(current_msg_cache.packet_data == NULL) {
                clear_message_cache();
                goto __FREE;
            }
            memset(current_msg_cache.packet_data, 0, sizeof(qm_audio_stream_t *) * current_msg_cache.total);
        }
        if(current_msg_cache.packet_data == NULL) {
            clear_message_cache();
            goto __FREE;
        }

        qm_audio_stream_t *msg_node_temp = qm_malloc(sizeof(qm_audio_stream_t));
        if(msg_node_temp == NULL) {
            goto __FREE;
        }

        msg_node_temp->index = msg_node->index;
        msg_node_temp->total = msg_node->total;
        msg_node_temp->payload_length = msg_node->payload_length;
        memcpy(msg_node_temp->msg_id, msg_node->msg_id, sizeof(msg_node_temp->msg_id));

        msg_node_temp->payload = (uint8_t *)qm_malloc(msg_node->payload_length + 1);
        if(msg_node_temp->payload == NULL) {
            qm_free(msg_node_temp);
            goto __FREE;
        }
        memcpy(msg_node_temp->payload, msg_node->payload, msg_node->payload_length);

        current_msg_cache.packet_data[msg_node_temp->index - 1] = msg_node_temp;

        if(msg_node_temp->index == msg_node_temp->total) {
            size_t total_length = 0;
            for(int i = 0; i < current_msg_cache.total; i++) {
                if(current_msg_cache.packet_data[i] != NULL) {
                    total_length += current_msg_cache.packet_data[i]->payload_length;
                } else {
                    total_length = 0;
                    break;
                }
            }
            if(total_length > 0) {
                uint8_t *assembled = (uint8_t *)qm_malloc(total_length + 1);
                if(assembled) {
                    size_t offset = 0;
                    for(int i = 0; i < current_msg_cache.total; i++) {
                        if(current_msg_cache.packet_data[i]) {
                            memcpy(assembled + offset, 
                                   current_msg_cache.packet_data[i]->payload, 
                                   current_msg_cache.packet_data[i]->payload_length);
                            offset += current_msg_cache.packet_data[i]->payload_length;
                        }
                    }
                    stream_msg_base64_decode(assembled, total_length);
                    qm_free(assembled);
                    assembled = NULL;
                }
            }

            clear_message_cache();
        }

__FREE:
        if(msg_node->payload) {
            qm_free(msg_node->payload);
            msg_node->payload = NULL;
        }
        if(node) {
            qm_list_node_destroy(node);
            node = NULL;
        }

        qm_mutex_lock(&audio_ctx->stream_lock, QM_WAIT_FOREVER);
        list_len = qm_list_len_get(&audio_ctx->stream_list);
        qm_mutex_unlock(&audio_ctx->stream_lock);

        if(list_len){
            qm_sem_signal(&audio_ctx->stream_sem);
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
    
    qm_sem_new(&bsp_audio_handle.binary_sem, 0);
    qm_mutex_new(&bsp_audio_handle.binary_lock);

    qm_sem_new(&bsp_audio_handle.stream_sem, 0);
    qm_mutex_new(&bsp_audio_handle.stream_lock);

    if(bsp_audio_handle.agora_handle == NULL) {
        bsp_audio_handle.agora_handle = (qm_iot_agora_gen_handle_t *)qm_malloc(sizeof(qm_iot_agora_gen_handle_t));
        if(bsp_audio_handle.agora_handle == NULL){
            return QM_ENOMEM;
        }
        memset(bsp_audio_handle.agora_handle, 0, sizeof(qm_iot_agora_gen_handle_t));
    }

    audio_thread_create(NULL, "ws_binary", audio_data_process_task, &bsp_audio_handle, 20 * 1024, 5, true, 0);
    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 30 * 1024, 5, true, 1);
    qm_task_new(&bsp_audio_handle.stream_task, "stream_msg_task", stream_msg_task, &bsp_audio_handle, 10*1024, 5);

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

static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed)
{
//   g_app.b_call_session_started = true;
  agora_rtc_get_connection_info(conn_id, &g_conn_info);
  printf("[conn-%lu] Join the channel %s successfully, uid %lu elapsed %d ms\n", conn_id, g_conn_info.channel_name, uid, elapsed);

    bsp_audio_handle.ws_connect = QM_TRUE;
    bsp_audio_handle.agora_init_flag = true;

    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONN_WS, NULL, 0);
}

static void __on_connection_lost(connection_id_t conn_id)
{
    bsp_audio_handle.agora_init_flag = false;
    qm_iot_agora_gen_free((void *)&bsp_audio_handle.agora_handle);
    printf("[conn-%lu] Lost connection from the channel\n", conn_id);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    bsp_audio_handle.agora_init_flag = true;
    printf("[conn-%lu] Rejoin the channel successfully, uid %lu elapsed %d ms\n", conn_id, uid, elapsed_ms);
}

static void __on_user_joined(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
  printf("[conn-%lu] Remote user \"%lu\" has joined the channel, elapsed %d ms\n", uid, conn_id, elapsed_ms);
}

static void __on_user_offline(connection_id_t conn_id, uint32_t uid, int reason)
{
  printf("[conn-%lu] Remote user \"%lu\" has left the channel, reason %d\n", conn_id, uid, reason);
}

static void __on_user_mute_audio(connection_id_t conn_id, uint32_t uid, bool muted)
{
  printf("[conn-%lu] audio: uid=%lu muted=%d\n", conn_id, uid, muted);
}

static void __on_error(connection_id_t conn_id, int code, const char *msg)
{
  if (code == ERR_VIDEO_SEND_OVER_BANDWIDTH_LIMIT) {
    printf("Not enough uplink bandwdith. Error msg \"%s\"\n", msg);
    return;
  }

  if (code == ERR_INVALID_APP_ID) {
    printf("Invalid App ID. Please double check. Error msg \"%s\"\n", msg);
  } else if (code == ERR_INVALID_CHANNEL_NAME) {
    printf("Invalid channel name. Please double check. Error msg \"%s\"\n", msg);
  } else if (code == ERR_INVALID_TOKEN || code == ERR_TOKEN_EXPIRED) {
    printf("Invalid token. Please double check. Error msg \"%s\"\n", msg);
  } else if (code == ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY) {
    printf("Dynamic token is enabled but is not provided. Error msg \"%s\"\n", msg);
  } else {
    printf("Error %d is captured. Error msg \"%s\"\n", code, msg);
  }
}

#if CONFIG_ENABLE_AUDIO_MIXING
static void __on_mixed_audio_data(connection_id_t conn_id, const void *data, size_t len,
                                  const audio_frame_info_t *info_ptr)
{
    QM_LOGD("1", "[conn-%u] on_mixed_audio_data, data_type %d, len %zu\n", conn_id, info_ptr->data_type, len);

    qm_audio_frame_t *ws_node = NULL;
    qm_list_node_t *node = qm_list_node_extra_new(sizeof(qm_audio_frame_t));
    if(node == NULL){
        return ;
    }
    ws_node = (qm_audio_frame_t *)qm_list_node_val_get(node);

    // ws_node->payload_len = len * sizeof(size_t);
    ws_node->payload_len = len;
    ws_node->payload = (uint8_t *)qm_calloc(1, ws_node->payload_len);
    if(ws_node->payload == NULL){
        qm_list_node_destroy(node);
        return;
    }
    memset(ws_node->payload, 0, ws_node->payload_len);
    memcpy(ws_node->payload, data, ws_node->payload_len);
    
    qm_mutex_lock(&bsp_audio_handle.binary_lock, QM_WAIT_FOREVER);
    qm_list_rpush(&bsp_audio_handle.binary_list, node);
    qm_mutex_unlock(&bsp_audio_handle.binary_lock);

    qm_sem_signal(&bsp_audio_handle.binary_sem);


}
#else
static void __on_audio_data(connection_id_t conn_id, const uint32_t uid, uint16_t sent_ts, const void *data, size_t len,
                            const audio_frame_info_t *info_ptr)
{
    // QM_LOGD("1", "[conn-%u] on_audio_data, uid %u sent_ts %u data_type %d, len %d\n", conn_id, uid, sent_ts,
    //      info_ptr->data_type, len);
    qm_audio_frame_t *ws_node = NULL;
    qm_list_node_t *node = qm_list_node_extra_new(sizeof(qm_audio_frame_t));
    if(node == NULL){
        return ;
    }
    ws_node = (qm_audio_frame_t *)qm_list_node_val_get(node);

    // ws_node->payload_len = len * sizeof(size_t);
    ws_node->payload_len = len;
    ws_node->payload = (uint8_t *)qm_calloc(1, ws_node->payload_len);
    if(ws_node->payload == NULL){
        qm_list_node_destroy(node);
        return;
    }
    memset(ws_node->payload, 0, ws_node->payload_len);
    memcpy(ws_node->payload, data, ws_node->payload_len);
    
    qm_mutex_lock(&bsp_audio_handle.binary_lock, QM_WAIT_FOREVER);
    qm_list_rpush(&bsp_audio_handle.binary_list, node);
    qm_mutex_unlock(&bsp_audio_handle.binary_lock);

    qm_sem_signal(&bsp_audio_handle.binary_sem);

}
#endif //#ifdef CONFIG_ENABLE_AUDIO_MIXING

static size_t get_packet_len(char *data, size_t length)
{
    uint8_t count = 0;
    for(size_t i = 0; i < length; i++){
        if(data[i] == '|'){
            count++;
            if(count == 3){
                return length - i - 1;
            }
        }
    }
    return 0;
}

static void __on_stream_message(connection_id_t conn_id, uint32_t uid, int stream_id, 
                                const char* data, size_t length, uint64_t sent_ts)
{
    qm_audio_stream_t *msg_node = NULL;

    qm_list_node_t *node = qm_list_node_extra_new(sizeof(qm_audio_stream_t));
    if(node == NULL){
        return ;
    }
    msg_node = (qm_audio_stream_t *)qm_list_node_val_get(node);

    msg_node->payload_length = get_packet_len(data, length);
    msg_node->payload = (uint8_t *)qm_calloc(1, msg_node->payload_length + 1);
    if(msg_node->payload == NULL){
        qm_list_node_destroy(node);
        return;
    }
    memset(msg_node->payload, 0, msg_node->payload_length + 1);
    char format[128];
    snprintf(format, sizeof(format), "%%31[^|]|%%d|%%d|%%%ds", (int)msg_node->payload_length);
    int ret = sscanf(data, format, msg_node->msg_id, &msg_node->index, &msg_node->total, msg_node->payload);

    if (ret != 4) {
        QM_LOGW("1", "Parse failed, only got %d fields", ret);
        qm_list_node_destroy(node);
        qm_free(msg_node->payload);
        return;
    }

    qm_mutex_lock(&bsp_audio_handle.stream_lock, QM_WAIT_FOREVER);
    qm_list_rpush(&bsp_audio_handle.stream_list, node);
    qm_mutex_unlock(&bsp_audio_handle.stream_lock);

    qm_sem_signal(&bsp_audio_handle.stream_sem);
}
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

#define DEFAULT_SDK_LOG_PATH "io.agora.rtc_sdk"

static void bsp_audio_ws_create_and_start(void)
{
    int rval;
    uint32_t uid = 0;
    if(bsp_audio_handle.agora_init_flag) {
        return;
    }
    
    if (qm_iot_agora_gen_start(bsp_audio_handle.agora_handle) != QM_EOK) {
        qm_iot_agora_gen_free((void *)&bsp_audio_handle.agora_handle);
        return;
    }

    uid = int_str_to_num(bsp_audio_handle.agora_handle->uid, bsp_audio_handle.agora_handle->uid_len);

    agora_rtc_event_handler_t event_handler = { 0 };
    app_init_event_handler(&event_handler);

    rtc_service_option_t service_opt = { 0 };
    service_opt.area_code            = AREA_CODE_GLOB;
    service_opt.log_cfg.log_disable  = false;
    service_opt.log_cfg.log_level    = RTC_LOG_WARNING;
    service_opt.log_cfg.log_path     = DEFAULT_SDK_LOG_PATH;
    service_opt.license_value[0]     = '\0';
    service_opt.domain_limit         = false;

    rval = agora_rtc_init(bsp_audio_handle.agora_handle->appId, &event_handler, &service_opt);
    if (rval < 0) {
        printf("Failed to initialize Agora sdk, license %p, reason: %s\n", service_opt.license_value, agora_rtc_err_2_str(rval));
        return ;
    }
    printf("~~~~~agora_rtc_init success~~~~\r\n");

    // 3. API: Create connection
    rval = agora_rtc_create_connection(&g_conn_id);
    if (rval < 0) {
        printf("Failed to create connection, reason: %s\n", agora_rtc_err_2_str(rval));
        return ;
    }

    printf("~~~~~agora_rtc_create_connection~~~~\r\n");


    // 4. API: join channel
    rtc_channel_options_t channel_options = { 0 };
    channel_options.auto_subscribe_audio = true;
    channel_options.auto_subscribe_video = false;

    /* If we want to send PCM data instead of encoded audio like AAC or Opus, here please enable
    * audio codec, as well as configure the PCM sample rate and number of channels
    */
#if 0
    channel_options.audio_codec_opt.audio_codec_type = AUDIO_CODEC_TYPE_OPUS;
    channel_options.audio_codec_opt.pcm_sample_rate  = CONFIG_AUDIO_FMT_RATE; 
    channel_options.audio_codec_opt.pcm_channel_num  = CONFIG_AUDIO_FMT_CHANNEL;
#endif
#if CONFIG_ENABLE_AUDIO_MIXING
    channel_options.enable_audio_mixer = 1;
    channel_options.enable_audio_jitter_buffer = 1;
#endif

    channel_options.audio_codec_opt.pcm_duration  = CONFIG_AUDIO_FMT_DURATION_MS;
    rval = agora_rtc_join_channel(g_conn_id, bsp_audio_handle.agora_handle->channelName, uid, bsp_audio_handle.agora_handle->token, &channel_options);
    if (rval < 0) {
        printf("Failed to join channel \"%s\", reason: %s\n", bsp_audio_handle.agora_handle->channelName, agora_rtc_err_2_str(rval));
        return ;
    }
}
