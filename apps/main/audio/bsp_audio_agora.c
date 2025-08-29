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

#include "bsp_pcm.h"
#include "bsp_audio.h"
#include "audio_main.h"
#include "audio_cloud.h"

#include "qm_player.h"
#include "qm_spec_api.h"
#include "qm_utils_list.h"
#include "qm_iot_config.h"

#include "qm_audio_ws.h"
#include "qm_cli.h"
#include "qm_kv.h"

#include "qm_gpio.h"
#include "es8311.h"

#include "agora_rtc_api.h"

#define CONFIG_ENABLE_AUDIO_MIXING 1

/* Application ID */
#define CONFIG_AGORA_APP_ID "9942a7948c3f4c4f8970d54ef6adfb24" // Please replace with your own APP ID
/* Product ID */
#define PID "01978C415CF2738A924FC53579E9BB66" //Please replace with your own PID
/* 获取CUSTOMER_KEY和CUSTOMER_SECRET的方式可参考如下链接 */
/* 链接：https://docs.agora.io/cn/video-legacy/faq/restful_authentication */
#define CUSTOMER_KEY "ea4cfcfc85044ca8be38917b2bd1bb42"
#define CUSTOMER_SECRET "661c55665871462283beb4525ff43baa"
/* 设备唯一ID，不可与其他设备重复 */
#define DEVICE_ID "1234567890"

#define DEFAULT_TOKEN "007eJxTYKi78X5nsfyxryo/2nVYb6nm+8XY1bk9uu6juOWs+amTeZMUGCwtTYwSzS1NLJKN00ySTdIsLM0NUkxNUtPMElPSkoxMWMTiMhoCGRneNIUyMTIwMrAAMYjPBCaZwSQLmORgKEktLjEwNDJmYAAAYzsjIw=="
#define DEFAULT_CHANNEL_NAME "test0123"
#define DEFAULT_USER_ID 0 // sdk will automatically assign a random user id

#define CONFIG_USE_G711U_CODEC

#include "xjiang_cloud.h"
#include "bsp_bat.h"
#include "bsp_audio_opus.h"


#define KV_TAG "AUDIO_MODE"

static const char* TAG = "volc_rtc";

#ifndef CONFIG_QM_AUDIO_URL_MAX_SIZE     
#define CONFIG_QM_AUDIO_URL_MAX_SIZE                   (64)
#endif

#define JOIN_EVENT_BIT           (1 << 0)

#define BACKOFF_BASE_TIME_MS    (10 * 1000)

struct bsp_audio_handle_t {
    
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

    char audio_uri[CONFIG_QM_AUDIO_URL_MAX_SIZE + 1];

    int mode;
    int switch_mode_on;

    void *opus_handle;
    
    int tts_start;

    qm_task_t                     binary_task;
    qm_sem_t                      binary_sem;
    qm_list_t                     binary_list;
    qm_mutex_t                    binary_lock;

};

typedef struct 
{
    int mode;
}bsp_audio_cfg_t;

static bsp_audio_cfg_t audio_cfg = {
    .mode = QM_AUDIO_WS_LISTEN_MODE_REALTIME,
};


static connection_id_t g_conn_id;
static connection_info_t g_conn_info;

static struct bsp_audio_handle_t bsp_audio_handle = {0};

#if CONFIG_AUDIO_BOARD_USER_STATUS_LED
static qm_gpio_dev_t blue_io = {
    .port = 11,
    .config = {
        .mode = QM_GPIO_MODE_OUTPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    }
};

static qm_gpio_dev_t red_io = {
    .port = 12,
    .config = {
        .mode = QM_GPIO_MODE_OUTPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    }
};

#endif

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

static void bsp_audio_write(uint8_t *buffer, int buf_size)
{
    int pcm_size = 0;
    static char pcm_data[1280] = {0};
    pcm_size = bsp_audio_opus_decode_data(bsp_audio_handle.opus_handle, buffer, buf_size, pcm_data);
    raw_stream_write(player_pipeline_get_raw_write(bsp_audio_handle.player_pipeline), pcm_data, pcm_size);
    qm_msleep(20);
}

static void bsp_audio_event(bsp_pcm_id_t id, bsp_pcm_event_id_t event_id, void *user_data)
{

    if(event_id == BSP_PCM_EVENT_ID_PLAY_START){
        bsp_audio_handle.tts_start = 0;
    }else if(event_id == BSP_PCM_EVENT_ID_PLAY_STOP){

        if(id != BSP_PCM_ID_POWER_OFF){
            bsp_audio_handle.tts_start = 1;
        }

        if(bsp_audio_handle.ws_connect && 
            bsp_audio_handle.rec_type == AUDIO_REC_WAKEUP_START)
        {
            es8311_set_voice_mute(false);
            player_pipeline_run(bsp_audio_handle.player_pipeline);
            if(audio_cfg.mode == QM_AUDIO_WS_LISTEN_MODE_AUTO){
                xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            }
        }
    }

}

static void bsp_audio_set_mute(int mute)
{
    if(mute){
        es8311_set_voice_mute(true);
        player_pipeline_stop(bsp_audio_handle.player_pipeline);
    }else{
        es8311_set_voice_mute(false);
        player_pipeline_run(bsp_audio_handle.player_pipeline);
    }
}

static void wakeup_timeout_handle(void *arg)
{
    qm_audio_ws_listen_param_t listen = {
        .mode = audio_cfg.mode
    };

    if(bsp_audio_handle.ws_connect){
        listen.state = QM_AUDIO_WS_LISTEN_STATE_STOP;
        bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_END;
        qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, QM_WAIT_FOREVER);
    }

    qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_FALSE);
    qm_audio_recorder_enable_sr_mode(bsp_audio_handle.recorder_engine, QM_TRUE);

    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID, NULL, 0);
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

        case CONFIG_AUDIO_SUB_EVENT_BAT_LEVEL_LOW:

            player_pipeline_stop(bsp_audio_handle.player_pipeline);
            bsp_pcm_player(BAT_LOW_WAV);

        break;

        case CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID:
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, &bsp_audio_handle.command_str[0], 0);
        break;

        case CONFIG_AUDIO_SUB_EVENT_CONFIG:
            
            bsp_pcm_player( NETCONFIG_WAV);

            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONFIG_IND_TIMEOUT);
        break;
        
        case CONFIG_AUDIO_SUB_EVENT_NET_ERROR:
        
            player_pipeline_stop(bsp_audio_handle.player_pipeline);
            bsp_pcm_player( CONN_ERROR_WAV);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONFIG_IND_TIMEOUT);
        break;

        case CONFIG_AUDIO_SUB_EVENT_CONN_WS:

            if(net_state == CONFIG_AUDIO_SUB_EVENT_NET_ERROR){

                player_pipeline_stop(bsp_audio_handle.player_pipeline);
                bsp_pcm_player( CONN_CLOUD_WAV);
            }

        #if CONFIG_DISABLE_SR_FUNC
            
            listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
            bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
            qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, QM_WAIT_FOREVER);

            xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
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
    int event_type = input_event->sub_event;
    bsp_audio_cmd_table_t *cmd_table = NULL;

#if CONFIG_SUPPORT_ONLINE_OPEN_WAV
    static int open_or_close = QM_FALSE;
#endif

    switch(input_event->sub_event){
        
        case CONFIG_AUDIO_SUB_EVENT_BAT_LEVEL_LOW:
        case CONFIG_AUDIO_SUB_EVENT_CONFIG:
        case CONFIG_AUDIO_SUB_EVENT_CONN_WS:
        case CONFIG_AUDIO_SUB_EVENT_SWITCH_MODE_ID:
        case CONFIG_AUDIO_SUB_EVENT_SWITCH_COMPLETE_ID:
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)event_type, 0);
        break;

        case CONFIG_AUDIO_SUB_EVENT_NET_ERROR:

            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)event_type, CONFIG_AUDIO_CONN_ERROR_TIMEOUT);
        
        break;

        case CONFIG_AUDIO_SUB_EVENT_POWER_OFF_BUTTON_ID:

            qm_gpio_set_level(&red_io, QM_TRUE);
            qm_gpio_set_level(&blue_io, QM_TRUE);

            player_pipeline_stop(bsp_audio_handle.player_pipeline);
            
            bsp_pcm_player( POWER_OFF_WAV);

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

        #if CONFIG_SUPPORT_ONLINE_CLOSE_WAV
            if(!open_or_close){
                break;
            }

            if(bsp_audio_handle.ws_connect){
                open_or_close = QM_FALSE;
                qm_audio_ws_closing_remarks(bsp_audio_handle.ai_handle, QM_WAIT_FOREVER);    
            }
        #else
            if(!bsp_audio_handle.ws_connect){
                bsp_pcm_player( WAKEUP_TIEMOUT_PLAYER_PATH);
            }
        #endif
            bsp_bat_enter_standby(CONFIG_AUDIO_READY_SLEEP_TIMEOUT);

        break;

        case CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID: 
            
        #if CONFIG_SUPPORT_ONLINE_OPEN_WAV
            
            if(open_or_close){
                break;
            }

            if(bsp_audio_handle.ws_connect){
                open_or_close = QM_TRUE;
                qm_audio_ws_opening_remarks(bsp_audio_handle.ai_handle, QM_WAIT_FOREVER);    
            }

        #else

            cmd_table = (bsp_audio_cmd_table_t *)input_event->value;
            if(cmd_table){
                QM_LOGI(TAG, "AUDIO_REC:%s", cmd_table->sr_str);
                if(cmd_table->reply_str){
                    player_pipeline_stop(bsp_audio_handle.player_pipeline);
                    bsp_pcm_player( cmd_table->reply_str);
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

    switch (event->type)
    {
        case AUDIO_REC_WAKEUP_START:

            if(audio_cfg.mode == QM_AUDIO_WS_LISTEN_MODE_MANUAL){
                break;
            }
            
            QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_START");
            
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);

            player_pipeline_stop(bsp_audio_handle.player_pipeline);
            bsp_pcm_player( DEAFALUT_REPLY_WAV);

        #if !CONFIG_DISABLE_SR_FUNC
            if(bsp_audio_handle.ws_connect){

                qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);

                if(bsp_audio_handle.rec_type != AUDIO_REC_WAKEUP_START){
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
                }else{
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_DETECT;
                }

                bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
                
                qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_TRUE);

                qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, NULL, CONFIG_AUDIO_WAKEUP_TIMEOUT);
            
            }
        #endif
            ind_type = CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID;
            xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_WAKEUP_FAST_NOTIFY);
           
        break;

        case AUDIO_REC_VAD_START:

        break;
        
        case AUDIO_REC_VAD_END:

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

static void qm_audio_ws_tts_chat_handler(qm_audio_ws_recv_evt_t *event)
{
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){

        es8311_set_voice_mute(false);
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
            
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){

    }

    qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, NULL, CONFIG_AUDIO_WAKEUP_TIMEOUT);
}

static void qm_audio_ws_tts_opening_handler(qm_audio_ws_recv_evt_t *event)
{
#if CONFIG_SUPPORT_ONLINE_OPEN_WAV
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){

        xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);

        es8311_set_voice_mute(false);
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
                
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){

        xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
    }
#endif
}

static void qm_audio_ws_tts_closing_handler(qm_audio_ws_recv_evt_t *event)
{
#if CONFIG_SUPPORT_ONLINE_CLOSE_WAV
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){

        xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);

        es8311_set_voice_mute(false);
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
                
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){

    }
#endif
}


typedef struct{
    uint8_t binary;
    int offset;
    int payload_len;
    uint8_t *payload;
}qm_audio_frame_t;


static void qm_audio_ws_data_cb(void *data, int len, int len_offset, int play_status, void *user_data)
{

    int pcm_size = 0;
    static char pcm_data[1280] = {0};
    
    switch (play_status)
    {
        case QM_AUDIO_WS_PLAYER_STATE_START:

        
        break;
        
        case QM_AUDIO_WS_PLAYER_STATE_DATA:

            if(bsp_audio_handle.tts_start){
                pcm_size = bsp_audio_opus_decode_data(bsp_audio_handle.opus_handle, data, len, pcm_data);
                // QM_HEX_LOGD("1", "pcm size %d",pcm_size);
                raw_stream_write(player_pipeline_get_raw_write(bsp_audio_handle.player_pipeline), pcm_data, pcm_size);
            }

            // qm_msleep(20);

        break;

        case QM_AUDIO_WS_PLAYER_STATE_STOP:

    
        break;

        default:
            break;
    }

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
        if (read_len == voice_data_read_sz && (bits & JOIN_EVENT_BIT)) {
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

qm_err_t bsp_audio_resgiter(qm_audio_ws_recv_event_cb_t event_cb, void *user_data)
{

    bsp_audio_handle.event_cb = event_cb;
    bsp_audio_handle.user_data = user_data;

    return QM_EOK;
}

qm_err_t bsp_audio_init(bsp_audio_cmd_table_t *command_str, int command_num)
{
    int len = sizeof(bsp_audio_cfg_t);
    qm_audio_sr_cmd_table_t *sr_cmd = NULL;
    pipe_record_engine_params_t engine_params = {0};

#if CONFIG_AUDIO_BOARD_USER_STATUS_LED
    qm_gpio_init(&blue_io);
    qm_gpio_init(&red_io);

    qm_gpio_set_level(&blue_io, QM_TRUE);
    qm_gpio_set_level(&red_io, QM_FALSE);
#endif

    qm_cli_register_command(&audio_key);

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

    engine_params.cb = rec_engine_cb;
    engine_params.command_str = sr_cmd;
    engine_params.command_num = command_num;

    bsp_audio_handle.recorder_engine = audio_record_engine_init(bsp_audio_handle.record_pipeline, &engine_params);

    es8311_set_voice_mute(1);
    es8311_set_voice_mute(0);

    player_pipeline_run(bsp_audio_handle.player_pipeline);
    recorder_pipeline_run(bsp_audio_handle.record_pipeline);

    bsp_audio_handle.join_event = xEventGroupCreate();

    bsp_audio_handle.opus_handle = bsp_audio_opus_encode_create(CONFIG_AUDIO_FMT_RATE, CONFIG_AUDIO_FMT_CHANNEL, CONFIG_AUDIO_FMT_DURATION_MS);

    bsp_pcm_params_t pcm_params = 
    {
        .enable_opus = 1,
        .send = bsp_audio_write,
        .event = bsp_audio_event,
        .mute = bsp_audio_set_mute,
        .sample_rate = CONFIG_AUDIO_FMT_RATE,
        .channels = CONFIG_AUDIO_FMT_CHANNEL,
        .duration_ms = CONFIG_AUDIO_FMT_DURATION_MS,
    };

    bsp_pcm_init(&pcm_params);

    bsp_pcm_player(POWER_ON_WAV);


    qm_sem_new(&bsp_audio_handle.binary_sem, 0);
    qm_mutex_new(&bsp_audio_handle.binary_lock);

    audio_thread_create(NULL, "ws_binary", audio_data_process_task, &bsp_audio_handle, 20 * 1024, 5, true, 0);
    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 30 * 1024, 5, true, 1);

    return QM_EOK;
}


static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed)
{
//   g_app.b_call_session_started = true;
  agora_rtc_get_connection_info(conn_id, &g_conn_info);
  printf("[conn-%lu] Join the channel %s successfully, uid %lu elapsed %d ms\n", conn_id, g_conn_info.channel_name, uid, elapsed);

    bsp_audio_handle.ws_connect = QM_TRUE;

#if CONFIG_AUDIO_BOARD_USER_STATUS_LED
    qm_gpio_set_level(&blue_io, QM_FALSE);
    qm_gpio_set_level(&red_io, QM_TRUE);
#endif

    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONN_WS, NULL, 0);
}

static void __on_connection_lost(connection_id_t conn_id)
{
//   g_app.b_call_session_started = false;
  printf("[conn-%lu] Lost connection from the channel\n", conn_id);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
//   g_app.b_call_session_started = true;
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

#ifdef CONFIG_ENABLE_AUDIO_MIXING
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
//   QM_LOGD("1", "[conn-%u] on_audio_data, uid %u sent_ts %u data_type %d, len %d\n", conn_id, uid, sent_ts,
//          info_ptr->data_type, len);
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

static void __on_rdt_msg(connection_id_t conn_id, uint32_t uid, rdt_stream_type_e type, const void *msg, size_t len)
{
  QM_LOGD("1", "[conn-%u] on_rdt_msg, uid %u type %u data_type %d, len %zu, msg:%s\n", conn_id, uid, type,
        len, msg);
}

static void app_init_event_handler(agora_rtc_event_handler_t *event_handler)
{
  event_handler->on_join_channel_success = __on_join_channel_success;
  event_handler->on_connection_lost = __on_connection_lost;
  event_handler->on_rejoin_channel_success = __on_rejoin_channel_success;
  event_handler->on_user_joined = __on_user_joined;
  event_handler->on_user_offline = __on_user_offline;
  event_handler->on_user_mute_audio = __on_user_mute_audio;
  event_handler->on_error = __on_error;
  event_handler->on_rdt_msg = __on_rdt_msg;

#ifdef CONFIG_ENABLE_AUDIO_MIXING
  event_handler->on_mixed_audio_data = __on_mixed_audio_data;
#else
  event_handler->on_audio_data = __on_audio_data;
#endif
}

#define DEFAULT_SDK_LOG_PATH "io.agora.rtc_sdk"

static void bsp_audio_ws_create_and_start(void)
{
   int rval;

    agora_rtc_event_handler_t event_handler = { 0 };
    app_init_event_handler(&event_handler);

    rtc_service_option_t service_opt = { 0 };
    service_opt.area_code            = AREA_CODE_GLOB;
    service_opt.log_cfg.log_disable  = false;
    service_opt.log_cfg.log_level    = RTC_LOG_INFO;
    service_opt.log_cfg.log_path     = DEFAULT_SDK_LOG_PATH;
    service_opt.license_value[0]     = '\0';
    service_opt.domain_limit         = false;

    rval = agora_rtc_init(CONFIG_AGORA_APP_ID, &event_handler, &service_opt);
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
    channel_options.enable_audio_mixer = 1;
    channel_options.enable_audio_jitter_buffer = 1;
    channel_options.audio_codec_opt.pcm_duration  = CONFIG_AUDIO_FMT_DURATION_MS;
    rval = agora_rtc_join_channel(g_conn_id, DEFAULT_CHANNEL_NAME, DEFAULT_USER_ID, DEFAULT_TOKEN, &channel_options);
    if (rval < 0) {
        printf("Failed to join channel \"%s\", reason: %s\n", DEFAULT_CHANNEL_NAME, agora_rtc_err_2_str(rval));
        return ;
    }
}
