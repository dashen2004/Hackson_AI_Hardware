#include "qm.h"
#include "qm_work.h"
#include "qm_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "cJSON.h"
#include "util_httpc.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "audio_thread.h"
#include "audio_mem.h"
#include "raw_stream.h"

#include "ui.h"
#include "lcd.h"
#include "bsp_audio.h"
#include "bsp_light.h"
#include "nc_player.h"
#include "audio_main.h"
#include "audio_cloud.h"
#include "xjiang_cloud.h"

#include "qm_kv.h"
#include "qm_cli.h"
#include "qm_gpio.h"
#include "qm_utils_base64.h"
#include "qm_utils_string.h"
#include "qm_utils_list.h"
#include "qm_utils_timer.h"
#include "qm_spec_api.h"
#include "qm_iot_config.h"

#include "agora_rtc_api.h"
#include "qm_agora.h"
#include "qm_agora_gen.h"
#include "qm_agora_stream.h"

#define KV_TAG "AUDIO_MODE"
#define OPENING_KV_TAG "opening"

static const char* TAG = "volc_rtc";

struct bsp_audio_handle_t {

    int mute;
    int ws_connect;
#if CONFIG_SUPPORT_ONLINE_OPEN_WAV
    int open_or_close;
#endif

    int command_num;
    bsp_audio_cmd_table_t *command_str;

    qm_audio_ws_recv_event_cb_t event_cb;
    void *user_data;

    qm_work_t  wakeup_timer;
    qm_work_t  ind_timer;
    qm_sem_t   wait_sem;

    int mode;

    uint8_t wakeup_timeout;
    uint8_t mic_stop_flag;

    int net_cfg_hint_cnt;
    int net_err_hint_cnt;
};

static int audio_reset = 0;
static int open_or_close = QM_FALSE;

void bsp_audio_tts_text_set(qm_audio_ws_recv_tts_t tts);

static struct bsp_audio_handle_t bsp_audio_handle = {0};
static qm_audio_ws_recv_tts_t tts_content = {0};

uint8_t get_wakeup_timeout_flag(void) {
    return bsp_audio_handle.wakeup_timeout;
}

void set_wakeup_timeout_flag(uint8_t flag) {
    bsp_audio_handle.wakeup_timeout = flag;
}

uint8_t get_mic_stop_flag(void) {
    return bsp_audio_handle.mic_stop_flag;
}

void set_mic_stop_flag(uint8_t flag) {
    bsp_audio_handle.mic_stop_flag = flag;
}

uint8_t bsp_audio_player_is_idle(void) {
    return nc_audio_play_is_idle();
}

// handle voice timeout
static void wakeup_timeout_handle(void *arg)
{
    int tmo_type = (int)arg;

    if (!bsp_audio_handle.open_or_close) {
        return;
    }

#if CONFIG_SUPPORT_ONLINE_CLOSE_WAV
    switch (tmo_type)
    {
        case CONFIG_TIMEOUT_TYPE_START_CLOSE:
            QM_LOGI(TAG, "qm_audio_ws_closing_remarks - START CLOSE");
            set_mic_stop_flag(QM_TRUE);
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_TIMEOUT_ID, NULL, 0);
            qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_CLOSING, CONFIG_AUDIO_MIC_TIMEOUT);
        break;

        case CONFIG_TIMEOUT_TYPE_CLOSING:
            QM_LOGI(TAG, "qm_audio_ws_closing_remarks - CLOSING");
            set_wakeup_timeout_flag(QM_TRUE);
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CLOSING_REMARKS_ID, NULL, 0);
        break;

        case CONFIG_TIMEOUT_TYPE_CLOSED:
            QM_LOGI(TAG, "qm_audio_ws_closing_remarks - CLOSED");
            qm_agora_leave();
            screen_wakeup_timeout_exit();
            bsp_audio_handle.open_or_close = QM_FALSE;
            qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);

            nc_audio_set_wakeup_state_on_off(QM_FALSE);
            set_mic_stop_flag(QM_FALSE);
            set_wakeup_timeout_flag(QM_FALSE);
        break;

        default:
            break;
    }
#endif
}

void wakeup_quit_immediately_trigger(uint8_t action) 
{
    switch (action) {
    case 1:
        break;
    case 2:
        qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
        set_mic_stop_flag(QM_TRUE);
        set_wakeup_timeout_flag(QM_TRUE);
        qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_TIMEOUT_ID, NULL, 0);
        qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_CLOSED, 10 * 1000);
        break;
    default:
        break;
    }
}

void reset_mic_state(void)
{
    if (get_mic_stop_flag() == QM_TRUE) {
        set_mic_stop_flag(QM_FALSE);

        if (bsp_audio_handle.ws_connect) {
            nc_audio_set_wakeup_state_on_off(QM_TRUE);
        }
    }
}

static void bsp_wakeup_time_refresh(void)
{
    if (get_wakeup_timeout_flag() == QM_TRUE) {
        return;
    }

    reset_mic_state();

    QM_LOGI(TAG, "bsp_wakeup_time_refresh");
    qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
    qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_START_CLOSE, CONFIG_AUDIO_WAKEUP_TIMEOUT);
}

static void bsp_wakeup_time_refresh_by_state(uint8_t state)
{
    if (get_wakeup_timeout_flag() == QM_FALSE) {
        reset_mic_state();
    }

    switch (state)
    {
        case AGENT_STATE_SILENT:
            if (get_wakeup_timeout_flag() == QM_TRUE) {
                qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
                qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_CLOSED, CONFIG_AUDIO_CLOSING_DURATION);
            } else {
                qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
                qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_START_CLOSE, CONFIG_AUDIO_WAKEUP_TIMEOUT);
            }
        break;

        case AGENT_STATE_SPEAKING:
            if (get_wakeup_timeout_flag() != QM_TRUE) {
                qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
                qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_START_CLOSE, CONFIG_AUDIO_ERR_TIMEOUT);
            }
        break;

        case AGENT_STATE_NONE:
        case AGENT_STATE_THINKING:
            if (get_wakeup_timeout_flag() != QM_TRUE) {
                qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
                qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_START_CLOSE, CONFIG_AUDIO_WAKEUP_TIMEOUT);
            }
        break;
            
        default:
            break;
    }
}

// init qm_agora
static void qm_audio_data_recv_cb(void *data, int len, void *user_data)
{
    qm_audio_data_push(QM_AUDIO_TYPE_AGENT, data, len);
}

static void qm_agora_stream_event_cb(qm_agora_stream_event_t *event, void *user_data)
{
    switch (event->type)
    {
        case QM_AGORA_STREAM_TYPE_STT:
            QM_LOGI("1", "User says: %s", event->data.stt.text);
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_FRESH_WAKETIME_ID, NULL, 0);
            break;
        case QM_AGORA_STREAM_TYPE_TTS:
            QM_LOGI("1", "Assistant says[%lu]: %.*s", event->data.tts.duration_ms, event->data.tts.text_len, event->data.tts.text);
            tts_content.duration_ms = event->data.tts.duration_ms;
            tts_content.text = event->data.tts.text;
            bsp_audio_tts_text_set(tts_content);
            break;
        case QM_AGORA_STREAM_TYPE_AGENT_STATE:
            QM_LOGI("1", "agent state: %s", event->data.agent_state.text);
            bsp_wakeup_time_refresh_by_state(event->data.agent_state.state);
            qm_agora_set_agent_state(event->data.agent_state.state, event->data.agent_state.text);
            break;
        case QM_AGORA_STREAM_TYPE_INTERRUPT:
            QM_LOGI("1", "user interrupt %d", event->data.interrupt.turn_id);
            break;
        default:
            break;
    }
}

static void qm_agora_event_cb(qm_agora_event_type_e event, void *user_data)
{
    switch (event) {
        case QM_AGORA_EVENT_TYPE_INIT_SUCCESS:
            break;
        case QM_AGORA_EVENT_TYPE_CONNECTED:
            bsp_audio_handle.ws_connect = QM_TRUE;
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONN_WS, NULL, 0);
            break;
        case QM_AGORA_EVENT_TYPE_CONNECT_LOST:
            bsp_audio_handle.ws_connect = QM_FALSE;
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_NET_ERROR, NULL, 0);
            break;
        case QM_AGORA_EVENT_TYPE_JOIN_CHANNEL:
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_JOIN_CHANNEL_ID, NULL, 0);
            break;
        case QM_AGORA_EVENT_TYPE_LEAVE_CHANNEL:
            break;
        case QM_AGORA_EVENT_TYPE_AGENT_JOINED:
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_OPENING_REMARKS_ID, NULL, 0);
            break;
        case QM_AGORA_EVENT_TYPE_AGENT_LEAVE:
            break;
        case QM_AGORA_EVENT_TYPE_AUDIO_MUTE:
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_AGORA_MUTE_ID, NULL, 0);
            break;
        case QM_AGORA_EVENT_TYPE_AUDIO_UNMUTE:
            break;
        case QM_AGORA_EVENT_TYPE_ERROR:
            break;
        default:
            break;
    }
}

// handler
static void ind_timeout_handle(void *arg)
{
    int ind_type = (int)arg;
    static int net_state  = CONFIG_AUDIO_SUB_EVENT_NET_ERROR;

    qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);

    switch (ind_type)
    {
        case CONFIG_AUDIO_SUB_EVENT_CONFIG:
            if (!audio_reset) {
                nc_audio_play_bin(BSP_PCM_ID_DISCONN_REMIND);
                bsp_audio_handle.net_cfg_hint_cnt++;
            }

            if (bsp_audio_handle.net_cfg_hint_cnt < 3) {
                qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONFIG_IND_TIMEOUT);
            }
        break;

        case CONFIG_AUDIO_SUB_EVENT_NET_ERROR:
            net_state = CONFIG_AUDIO_SUB_EVENT_NET_ERROR;

            nc_audio_play_bin(BSP_PCM_ID_NET_DISCONN);
            bsp_audio_handle.net_err_hint_cnt++;

            if (bsp_audio_handle.net_err_hint_cnt < 3) {
                qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)ind_type, CONFIG_AUDIO_CONN_ERROR_TIMEOUT);
            }
        break;

        case CONFIG_AUDIO_SUB_EVENT_CONN_WS:
            if (net_state == CONFIG_AUDIO_SUB_EVENT_NET_ERROR){
                if (!audio_reset) {
                    nc_audio_play_bin(BSP_PCM_ID_NET_CONN);
                }
            }

            net_state  = CONFIG_AUDIO_SUB_EVENT_CONN_WS;

            if (audio_reset && open_or_close){
                bsp_wakeup_time_refresh();
                bsp_audio_handle.open_or_close = QM_TRUE;
            }

            audio_reset = 0;
        break;

        default:
            break;
    }
}

static void qm_audio_event_handler(qm_input_event_t *input_event, void *arg)
{
    int event_type = input_event->sub_event;

    QM_LOGD("1", "AUDIO_EVENT: %04x", input_event->sub_event);

    switch (input_event->sub_event) 
    {
        case CONFIG_AUDIO_SUB_EVENT_FRESH_WAKETIME_ID:
            bsp_wakeup_time_refresh();
        break;

        case CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID:
            set_wakeup_timeout_flag(QM_FALSE);
            bsp_wakeup_time_refresh();

        #if CONFIG_SUPPORT_ONLINE_OPEN_WAV
            if (!bsp_audio_handle.mute) {
                nc_audio_set_mute(false);
            }

            // open mic
            nc_audio_set_mic_on_off(QM_TRUE);
            nc_audio_set_wakeup_on_off(QM_TRUE);
            nc_audio_set_wakeup_state_on_off(QM_TRUE);

            if (bsp_audio_handle.open_or_close) {
                break;
            }

            if (bsp_audio_handle.ws_connect) {
                bsp_audio_handle.open_or_close = QM_TRUE;
                qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);

                qm_agora_join();

                extern void goto_recog_page();
                void goto_recog_page(void);
            } else {
                qm_iot_reset_info_t reset_info;
                qm_iot_reset_info_get(&reset_info);
                qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
                if (reset_info.is_reset == QM_TRUE) {
                    if (bsp_audio_handle.net_cfg_hint_cnt > -1)
                        bsp_audio_handle.net_cfg_hint_cnt--;
                    qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)CONFIG_AUDIO_SUB_EVENT_CONFIG, 0);
                } else {
                    if (bsp_audio_handle.net_err_hint_cnt > -1)
                        bsp_audio_handle.net_err_hint_cnt--;
                    qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)CONFIG_AUDIO_SUB_EVENT_NET_ERROR, 0);
                }
            }
        #endif
        break;

        case CONFIG_AUDIO_SUB_EVENT_BUTTON_WAKEUP_ID:
            if (bsp_audio_handle.open_or_close) {
                qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_AGORA_MUTE_ID, NULL, 0);
                bsp_lcd_set_int_screen();
            }
            else {
                qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, NULL, 0);
            }
        break;

        case CONFIG_AUDIO_SUB_EVENT_BUTTON_MUTE_ID:
        break;

        case CONFIG_AUDIO_SUB_EVENT_BUTTON_WAKEUP_HOLD_ID:
        break;

        case CONFIG_AUDIO_SUB_EVENT_BUTTON_MUTE_HOLD_ID:
        break;

        case CONFIG_AUDIO_USB_EVENT_CONNING_LOCAL:
        case CONFIG_AUDIO_SUB_EVENT_CONFIG:
        case CONFIG_AUDIO_SUB_EVENT_CONN_WS:
        case CONFIG_AUDIO_SUB_EVENT_SWITCH_MODE_ID:
        case CONFIG_AUDIO_SUB_EVENT_SWITCH_COMPLETE_ID:
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)event_type, 0);
        break;

        case CONFIG_AUDIO_SUB_EVENT_NET_ERROR:
            qm_cancel_delayed_action(&bsp_audio_handle.wakeup_timer);
            
            bsp_audio_handle.ws_connect = QM_FALSE;
            bsp_audio_handle.open_or_close = QM_FALSE;
            qm_kv_set(OPENING_KV_TAG, &bsp_audio_handle.open_or_close, sizeof(int), QM_TRUE);

            nc_audio_set_wakeup_state_on_off(QM_FALSE);

            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            qm_post_delayed_action(&bsp_audio_handle.ind_timer, ind_timeout_handle, (void *)event_type, CONFIG_AUDIO_CONN_ERR_DELAY);

            qm_agora_disconnect();
            screen_wakeup_timeout_exit();
        break;

        case CONFIG_AUDIO_SUB_EVENT_CEART_WS:
            qm_cancel_delayed_action(&bsp_audio_handle.ind_timer);
            
            qm_agora_connect();
        break;

        case CONFIG_AUDIO_SUB_EVENT_TIMEOUT_ID:
            nc_audio_set_wakeup_state_on_off(QM_FALSE);
        break;

        case CONFIG_AUDIO_SUB_EVENT_JOIN_CHANNEL_ID:
            bsp_wakeup_time_refresh();
        break;

        case CONFIG_AUDIO_SUB_EVENT_OPENING_REMARKS_ID:
            voice_corpus_request(CONFIG_AUDIO_OPENING_REMARKS);
        break;

        case CONFIG_AUDIO_SUB_EVENT_CLOSING_REMARKS_ID:
            voice_corpus_request(CONFIG_AUDIO_CLOSING_REMARKS);

            qm_post_delayed_action(&bsp_audio_handle.wakeup_timer, wakeup_timeout_handle, (void *)CONFIG_TIMEOUT_TYPE_CLOSED, CONFIG_AUDIO_WAKEEND_TIMEOUT);
        break;

        case CONFIG_AUDIO_SUB_EVENT_AGORA_MUTE_ID:
            qm_audio_mute();
            qm_agora_stream_tts_data_reset();

            bsp_wakeup_time_refresh();
        break;

        default:
            break;
    }
}

void bsp_audio_play_event(bsp_pcm_id_t id, qm_audio_event_t event_id, void *user_data)
{
    switch (id)
    {
        case BSP_PCM_ID_NET_CONN:
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, NULL, 0);
        break;

        case BSP_PCM_ID_POWER_ON:
            qm_sem_signal(&bsp_audio_handle.wait_sem);
        break;

        default:
            break;
    }
}

static int nc_audio_event_cb(nc_audio_event_type_t event_type, nc_audio_event_param_t *p_event)
{
    nc_audio_mic_data_t *mic_data = &p_event->mic_data;

    switch (event_type)
    {
        case NC_AUDIO_EVENT_TYPE_MIC_DATA_GET:
            switch (mic_data->status)
            {
                case NC_AUDIO_VAD_START:
                    QM_LOGD(TAG, "vad start %d %d", bsp_audio_handle.open_or_close, bsp_audio_handle.ws_connect);
                    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_FRESH_WAKETIME_ID, NULL, 0);
                break;
                case NC_AUDIO_VAD_DATA:
                    if (bsp_audio_handle.open_or_close && bsp_audio_handle.ws_connect && !get_mic_stop_flag()) {
                        qm_agora_send_data(mic_data->payload, mic_data->payload_len);
                    }
                break;
                case NC_AUDIO_VAD_STOP:
                    QM_LOGD(TAG, "vad stop");
                    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_FRESH_WAKETIME_ID, NULL, 0);
                break;
                default:
                break;
            }
            break;
        case NC_AUDIO_EVENT_TYPE_WAKEUP:
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID, NULL, 0);
        break;
        default:
        break;
    }
    
    return QM_EOK;
}

qm_err_t bsp_audio_init(bsp_audio_cmd_table_t *command_str, int command_num)
{
    int len = sizeof(int);

    audio_reset = qm_is_crash_reset();
    qm_kv_get(OPENING_KV_TAG, &open_or_close, &len);

    bsp_audio_handle.ws_connect = QM_FALSE;

    qm_event_register(CONFIG_AUDIO_MAIN_EVENT_ID, qm_audio_event_handler, NULL);

    qm_sem_new(&bsp_audio_handle.wait_sem, 0);

    set_wakeup_timeout_flag(QM_FALSE);
    set_mic_stop_flag(QM_FALSE);

    nc_player_param_t player_param = {
        .event_cb = bsp_audio_play_event,
        .notify = nc_audio_event_cb,
        .user_data = &bsp_audio_handle,
    };

    nc_player_init(player_param);

    qm_agora_cfg_t agora_cfg = {
        .audio_enable = true,
        .video_enable = false,
        .domain_limit = false,
        .enable_audio_ai_qos = true,
        .duration = CONFIG_AUDIO_FMT_DURATION_MS,
        .user_data = &bsp_audio_handle,
        .audio_cb = qm_audio_data_recv_cb,
        .stream_cb = qm_agora_stream_event_cb,
        .event_cb = qm_agora_event_cb,
    };

    qm_agora_init(agora_cfg);

    nc_audio_set_mic_on_off(QM_FALSE);
    nc_audio_set_wakeup_on_off(QM_FALSE);
    nc_audio_set_wakeup_state_on_off(QM_FALSE);

    nc_audio_play_bin(BSP_PCM_ID_POWER_ON);
    qm_sem_wait(&bsp_audio_handle.wait_sem, CONFIG_AUDIO_POWER_ON_DURATION);

    nc_audio_set_spk_volume(nc_audio_get_volume());

    return QM_EOK;
}

// for external call
qm_err_t bsp_audio_resgiter(qm_audio_ws_recv_event_cb_t event_cb, void *user_data)
{
    bsp_audio_handle.event_cb = event_cb;
    bsp_audio_handle.user_data = user_data;

    return QM_EOK;
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

void bsp_audio_tts_text_set(qm_audio_ws_recv_tts_t tts)
{
    if (tts.text[0] == '\0' || tts.text == NULL) {
        return;
    }

    qm_audio_ws_recv_evt_t recv = {0};
    recv.recv_type = QM_AUDIO_WS_RECV_TYPE_TTS;

    recv.data.tts.text = tts.text;
    recv.data.tts.duration_ms = tts.duration_ms;
    if (bsp_audio_handle.event_cb) {
        bsp_audio_handle.event_cb(&recv, bsp_audio_handle.user_data);
    }
}

qm_err_t bsp_audio_ota_set_voice_mute(void)
{
    bsp_audio_set_voice_mute(true);
    return QM_EOK;
}

qm_err_t bsp_audio_ota_set_voice_unmute(void)
{
    bsp_audio_set_voice_mute(false);
    return QM_EOK;
}

qm_err_t bsp_audio_set_voice_mute(bool_t enable)
{
    bsp_audio_handle.mute = enable;
    nc_audio_set_mute(enable);

    return QM_EOK;
}

bool_t bsp_audio_get_voice_mute(void)
{
    return bsp_audio_handle.mute;
}
