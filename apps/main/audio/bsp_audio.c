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

static const char* TAG = "volc_rtc";

#ifndef CONFIG_QM_AUDIO_URL_MAX_SIZE     
#define CONFIG_QM_AUDIO_URL_MAX_SIZE                   (64)
#endif

#define KV_TAG "AUDIO_MODE"

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
};

typedef struct 
{
    int mode;
}bsp_audio_cfg_t;

static bsp_audio_cfg_t audio_cfg = {
    .mode = QM_AUDIO_WS_LISTEN_MODE_REALTIME,
};

static struct bsp_audio_handle_t bsp_audio_handle = {0};

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

void qm_audio_ws_data_cb(void *data, int len, void *user_data)
{
    raw_stream_write(player_pipeline_get_raw_write(bsp_audio_handle.player_pipeline), data, len);
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
#if !CONFIG_SUPPORT_ONLINE_CLOSE_WAV
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

    switch (ind_type)
    {

        case CONFIG_AUDIO_SUB_EVENT_CONFIG:
            
            player_pipeline_stop(bsp_audio_handle.player_pipeline);
            qm_player_play_file(bsp_audio_handle.player_handle, NETCONFIG_WAV);

            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONFIG_IND_TIMEOUT);
        break;
        
        case CONFIG_AUDIO_SUB_EVENT_NET_ERROR:
        
            player_pipeline_stop(bsp_audio_handle.player_pipeline);
            qm_player_play_file(bsp_audio_handle.player_handle, CONN_ERROR_WAV);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONFIG_IND_TIMEOUT);
        break;

        case CONFIG_AUDIO_SUB_EVENT_CONN_WS:

            if(net_state == CONFIG_AUDIO_SUB_EVENT_NET_ERROR){

                player_pipeline_stop(bsp_audio_handle.player_pipeline);
            #ifdef CONN_CLOUD_WAV
                qm_player_play_file(bsp_audio_handle.player_handle, CONN_CLOUD_WAV);
            #endif
            }

            net_state  = CONFIG_AUDIO_SUB_EVENT_CONN_WS;

        #if CONFIG_DISABLE_SR_FUNC
            
            listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
            bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
            qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, QM_WAIT_FOREVER);

            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, &bsp_audio_handle.command_str[0], 0);
        #endif

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

        case CONFIG_AUDIO_SUB_EVENT_CEART_WS:
        {

            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_cancel_delayed_action(&bsp_audio_handle.reconn_timer);

            if(bsp_audio_handle.ai_handle == NULL){
                qm_audio_ws_cfg_t params = {
        
                    .uri = CONFIG_NETWORK_URL,
                    .buff_size = 10 * 1024,
                    .task_prio = 20,
                    .task_size = 4 * 1024,
            
                    .pid = qm_iot_pid_get(),
                    .did = qm_iot_did_get(),
                    
                    .connect_timeout = 10,
            
                    .fmt_type = QM_AUDIO_WS_FMT_TYPE_PCM,
                    .rate = 16000,
                    .bits = 16,
                    .channel = 1,
                    .frame_duration_ms = 60,
            
                    .event_cb = qm_audio_ws_recv_event,
                    .data_cb  = qm_audio_ws_data_cb,
                };
                bsp_audio_handle.ai_handle = qm_audio_ws_create(&params);
                if(bsp_audio_handle.ai_handle == NULL){
                    break;
                }
            }

            qm_audio_ws_start(bsp_audio_handle.ai_handle);
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
                qm_player_play_file(bsp_audio_handle.player_handle, WAKEUP_TIEMOUT_PLAYER_PATH);
            }
        #endif

        break;

        case CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID: 
            
            qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);

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
                    qm_player_play_file(bsp_audio_handle.player_handle, cmd_table->reply_str);
                }
            }
            xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);

        #endif

            qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, NULL, CONFIG_AUDIO_WAKEUPT_TIMEOUT);
        break;

        case CONFIG_AUDIO_SUB_EVENT_CONTROL_ID: 

            cmd_table = (bsp_audio_cmd_table_t *)input_event->value;
            if(cmd_table){
                QM_LOGI(TAG, "AUDIO_REC[%d]:%s", cmd_table->id, cmd_table->sr_str);
                if(cmd_table->reply_str){
                    player_pipeline_stop(bsp_audio_handle.player_pipeline);
                    qm_player_play_file(bsp_audio_handle.player_handle, cmd_table->reply_str);
                }
            }
            
        break;
    
        default:
        break;
    }
}

static qm_err_t rec_engine_cb(audio_rec_evt_t *event, void *user_data)
{
    int res = 0;

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
        #if !CONFIG_DISABLE_SR_FUNC
            if(bsp_audio_handle.ws_connect){

                if(bsp_audio_handle.rec_type != AUDIO_REC_WAKEUP_START){
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
                }else{
                    listen.state = QM_AUDIO_WS_LISTEN_STATE_DETECT;
                }

                bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
                qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, QM_WAIT_FOREVER);
                
                qm_audio_recorder_enable_vc_mode(bsp_audio_handle.recorder_engine, QM_TRUE);
            }
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, &bsp_audio_handle.command_str[0], 0);
        #endif
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
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){
    #if CONFIG_AUDIO_BOARD_USER_BUTTON    
        adc_ind_type = false;
    #endif
        es8311_set_voice_mute(false);
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
            
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){
    #if CONFIG_AUDIO_BOARD_USER_BUTTON
        adc_ind_type = true;
    #endif
    }
        
    qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, NULL, CONFIG_AUDIO_WAKEUPT_TIMEOUT);
}

static void qm_audio_ws_tts_opening_handler(qm_audio_ws_recv_evt_t *event)
{
#if CONFIG_SUPPORT_ONLINE_OPEN_WAV
    if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){

        xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
    #if CONFIG_AUDIO_BOARD_USER_BUTTON    
        adc_ind_type = false;
    #endif
        es8311_set_voice_mute(false);
        
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
        es8311_set_voice_mute(false);
        
        player_pipeline_run(bsp_audio_handle.player_pipeline);
                
    }else if (event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){

    #if CONFIG_AUDIO_BOARD_USER_BUTTON
        adc_ind_type = true;
    #endif

        qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID, NULL, 0);

    }
#endif
}

static void qm_audio_ws_recv_event(qm_audio_ws_recv_evt_t *event, void *user_data)
{

    uint32_t next_backoff = 0; 

    qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
    
    switch (event->recv_type)
    {
        case QM_AUDIO_WS_RECV_TYPE_CONNECT:

        break;
        case QM_AUDIO_WS_RECV_TYPE_HELLO_REPLY:

            bsp_audio_handle.ws_connect = QM_TRUE;
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONN_WS, NULL, 0);
        break;

        case QM_AUDIO_WS_RECV_TYPE_STT:
            
            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(event, bsp_audio_handle.user_data);
            }

            qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, NULL, CONFIG_AUDIO_WAKEUPT_TIMEOUT);
        break;

        case QM_AUDIO_WS_RECV_TYPE_LLM:
            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(event, bsp_audio_handle.user_data);
            }

            qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, NULL, CONFIG_AUDIO_WAKEUPT_TIMEOUT);
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

            if(bsp_audio_handle.event_cb){
                bsp_audio_handle.event_cb(event, bsp_audio_handle.user_data);
            }
        
        break;

        case QM_AUDIO_WS_RECV_TYPE_DISCONNECT:
            
            qm_cancel_delayed_action(&bsp_audio_handle.reconn_timer);

            bsp_audio_handle.ws_connect = QM_FALSE;
            xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            
            qm_srandom((uint32_t)qm_now_ms());

            next_backoff += qm_random_get(BACKOFF_BASE_TIME_MS);
            if(next_backoff > BACKOFF_BASE_TIME_MS){
                next_backoff = qm_random_get(BACKOFF_BASE_TIME_MS);
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
    uint8_t *voice_data = audio_calloc(1, voice_data_read_sz);
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
            qm_audio_ws_data_send(bsp_audio_handle.ai_handle, voice_data, read_len, QM_WAIT_FOREVER);   
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

    qm_cli_register_command(&audio_key);
    qm_kv_get(KV_TAG, &audio_cfg, &len);

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
    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 4 * 1024, 5, true, 0);

    return QM_EOK;
}
