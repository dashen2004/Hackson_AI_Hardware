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

#include "qm_player.h"
#include "qm_spec_api.h"
#include "qm_utils_list.h"

#include "qm_audio_ws.h"

static const char* TAG = "volc_rtc";

#ifndef CONFIG_QM_AUDIO_URL_MAX_SIZE     
#define CONFIG_QM_AUDIO_URL_MAX_SIZE                   (64)
#endif

#define JOIN_EVENT_BIT           (1 << 0)

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
};

static struct bsp_audio_handle_t bsp_audio_handle = {0};


static void qm_audio_event_handler(qm_input_event_t *input_event, void *arg)
{
    bsp_audio_cmd_table_t *cmd_table = NULL;

    switch(input_event->sub_event){

        case CONFIG_AUDIO_SUB_EVENT_CEART_WS:
            qm_audio_ws_start(bsp_audio_handle.ai_handle);
        break;

        case CONFIG_AUDIO_SUB_EVENT_MSG_ID:



        break;

        case CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID: 
        case CONFIG_AUDIO_SUB_EVENT_CONTROL_ID: 

            cmd_table = (bsp_audio_cmd_table_t *)input_event->value;
            if(cmd_table){
                QM_LOGI(TAG, "AUDIO_REC:%s", cmd_table->sr_str);
                if(cmd_table->reply_str){
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
        .mode = QM_AUDIO_WS_LISTEN_MODE_REALTIME
    };

    if (AUDIO_REC_WAKEUP_START == event->type) {

        QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_START");

        if(bsp_audio_handle.ws_connect){
            if(bsp_audio_handle.rec_type != AUDIO_REC_WAKEUP_START){
                listen.state = QM_AUDIO_WS_LISTEN_STATE_START;
                qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, &bsp_audio_handle.command_str[0], 0);
            }else{
                listen.state = QM_AUDIO_WS_LISTEN_STATE_DETECT;
            }
    
            bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_START;
            qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, QM_WAIT_FOREVER);
    
        }

    } else if (AUDIO_REC_VAD_START == event->type) {
    } else if (AUDIO_REC_VAD_END == event->type) {
    } else if (AUDIO_REC_WAKEUP_END == event->type) {

        QM_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_END");

        if(bsp_audio_handle.ws_connect){
            listen.state = QM_AUDIO_WS_LISTEN_STATE_STOP;
            bsp_audio_handle.rec_type = AUDIO_REC_WAKEUP_END;
            qm_audio_ws_listen(bsp_audio_handle.ai_handle, &listen, QM_WAIT_FOREVER);
        }


    } else {

        qm_audio_sr_mn_result_t *mn_res = (qm_audio_sr_mn_result_t *)event->event_data;
        if(mn_res == NULL){
            return ESP_OK;
        }

        for (int i = 0; i < bsp_audio_handle.command_num; i++)
        {
            res = strncmp(bsp_audio_handle.command_str[i].sr_str, 
                            mn_res->str, strlen(bsp_audio_handle.command_str[i].sr_str));
            if(!res){
                qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONTROL_ID, &bsp_audio_handle.command_str[i], 0);
                break;
            }
        }
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

                xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            }
            break;
        
        default:
            break;
    }
}

static void qm_audio_ws_recv_event(qm_audio_ws_recv_evt_t *event, void *user_data)
{
    qm_audio_ws_listen_param_t listen = {0};

    switch (event->recv_type)
    {
        case QM_AUDIO_WS_RECV_TYPE_CONNECT:
        
        break;
        
        case QM_AUDIO_WS_RECV_TYPE_HELLO_REPLY:
            bsp_audio_handle.ws_connect = QM_TRUE;
            xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
        break;

        case QM_AUDIO_WS_RECV_TYPE_STT:

        break;

        case QM_AUDIO_WS_RECV_TYPE_LLM:

        break;

        case QM_AUDIO_WS_RECV_TYPE_TTS:
        #if !CONFIG_SUPPORT_AEC
            if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_START){
                xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            }else if(event->data.tts.state == QM_AUDIO_WS_TTS_STATE_STOP){
                xEventGroupSetBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
            }
        #endif
        break;

        case QM_AUDIO_WS_RECV_TYPE_DISCONNECT:
            bsp_audio_handle.ws_connect = QM_FALSE;
            xEventGroupClearBits(bsp_audio_handle.join_event, JOIN_EVENT_BIT);
        break;
        
        default:
            break;
    }
}

static void voice_read_task(void *args)
{
    const int voice_data_read_sz = recorder_pipeline_get_default_read_size(bsp_audio_handle.record_pipeline);
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

void qm_audio_ws_data_cb(void *data, int len, void *user_data)
{
    raw_stream_write(player_pipeline_get_raw_write(bsp_audio_handle.player_pipeline), data, len);
}

qm_err_t bsp_audio_init(bsp_audio_cmd_table_t *command_str, int command_num)
{
    qm_audio_sr_cmd_table_t *sr_cmd = NULL;
    pipe_record_engine_params_t engine_params = {0};


    qm_event_register(CONFIG_AUDIO_MAIN_EVENT_ID, qm_audio_event_handler, NULL);

    bsp_audio_handle.player_pipeline = player_pipeline_open();
    bsp_audio_handle.record_pipeline = recorder_pipeline_open();

    bsp_audio_handle.command_str = command_str;
    bsp_audio_handle.command_num = command_num;

    sr_cmd = (qm_audio_sr_cmd_table_t *)qm_malloc(sizeof(qm_audio_sr_cmd_table_t) * command_num);
    for (int i = 0; i < command_num; i++)
    {
        sr_cmd[i].type = command_str[i].type;
        sr_cmd[i].id = command_str[i].id;
        sr_cmd[i].str = command_str[i].sr_str;
    }
    
    engine_params.cb = rec_engine_cb;
    engine_params.command_str = sr_cmd;
    engine_params.command_num = command_num;

    bsp_audio_handle.recorder_engine = audio_record_engine_init(bsp_audio_handle.record_pipeline, &engine_params);

    player_pipeline_run(bsp_audio_handle.player_pipeline);
    recorder_pipeline_run(bsp_audio_handle.record_pipeline);

    qm_audio_ws_cfg_t params = {

        .uri = "ws://192.168.0.81:8000/xiaozhi/v1/",
        .buff_size = 5 * 1024,
        .task_prio = 10,
        .task_size = 4 * 1024,

        .pid = CONFIG_XJIANG_CLOUD_PID,
        .did = -1,
        
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
        
    }

    bsp_audio_handle.player_handle = qm_player_init();
    if(bsp_audio_handle.player_handle == NULL){
        
    }
    qm_player_setopt(bsp_audio_handle.player_handle, QM_PLAYER_OPT_USERDATA, &bsp_audio_handle);
    qm_player_setopt(bsp_audio_handle.player_handle, QM_PLAYER_OPT_EVENT_HANDLER, audio_player_dev_cb);


    bsp_audio_handle.join_event = xEventGroupCreate();
    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 4 * 1024, 5, true, 0);

    return QM_EOK;
}
