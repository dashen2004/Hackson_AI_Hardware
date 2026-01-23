
#include "qm.h"
#include "qm_kv.h"

#include "esp_log.h"
#include "esp_system.h"
#include "qm_iot_api.h"

#include "ai_main.h"
#include "qm_ai_inner_rtc_api.h"
#include "qm_ai_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_TAG "log"

#define DEAFALUT_REPLY_WAV              ("/spiffs/dingding.wav")
#define NETCONFIG_WAV                   ("/spiffs/1.wav")
#define CONN_CLOUD_WAV                  ("/spiffs/2.wav")
#define CONN_ERROR_WAV                  ("/spiffs/3.wav")
#define XJIANG_TEST_WAV                 ("/spiffs/4.wav")

static void qm_iot_event_handler(void *handle, const qm_iot_event_t *event, void *userdata);
static void qm_iot_recv_handler(void *handle, const qm_iot_recv_t *recv, void *userdata);

static int audio_driver_open(void);
static int audio_driver_close(void);
static int audio_driver_write(void *data, size_t len);
static int audio_driver_read(void *data, size_t len, uint32_t timeout);
static int audio_driver_ctrl(qm_ai_driver_ctrl_t cmd, void *param);

static void *iot_handle = NULL;
static qm_ai_handle_t ai_handle = NULL;

static void qm_example_play_data(qm_ai_handle_t handle)
{
    FILE *fd = fopen(CONN_CLOUD_WAV, "r");

    uint8_t data[256] = {0};
    int len = 0;

    while ((len = fread(data, 1, sizeof(data), fd)) > 0) {
        qm_ai_audio_player_data(handle, data, len);
    }
}

static void qm_ai_event_cb(qm_ai_handle_t handle, qm_ai_event_t *event, void *user_data)
{
    switch (event->type)
    {
        case QM_AI_EVENT_CLOUD_LICENSE:
            QM_LOGI("1", "Cloud license success!");
            qm_example_play_data(ai_handle);
            qm_ai_start(ai_handle);
        break;

        case QM_AI_EVENT_AGENT_STATE:
            QM_LOGI("1", "agent state: %s", event->data.agent_state.text);
        break;

        case QM_AI_EVENT_AGENT_ASR:
            QM_LOGI("1", "User says: %s", event->data.asr.text);
        break;

        case QM_AI_EVENT_AGENT_TTS:
            QM_LOGI("1", "Assistant says[%lu]: %.*s", event->data.tts.duration_ms, event->data.tts.text_len, event->data.tts.text);
        break;

        case QM_AI_EVENT_AGENT_ABORT:
            QM_LOGI("1", "Agent abort %d", event->data.agent_abort.turn_id);
        break;

        default:
        break;
    }
}

static void qm_example_iot_init()
{
    uint32_t pid = CONFIG_XJIANG_CLOUD_PID;
    uint32_t version = CONFIG_XJIANG_VERSION;

    iot_handle = qm_iot_init();
    if (iot_handle == NULL){
        QM_LOGE(LOG_TAG, "qm_api_init failed !!!");
        return;
    }

    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_ID, &pid);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_VERSION, &version);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_SECRET, CONFIG_XJIANG_CLOUD_SECERT);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_RECV_HANDLER, qm_iot_recv_handler);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_EVENT_HANDLER, qm_iot_event_handler);

    qm_iot_start(iot_handle);
}

static void qm_example_ai_init()
{
    ai_handle = qm_ai_open(QM_AI_COMM_TYPE_RTC, NULL);

    qm_ai_set_service_handle_option(ai_handle, iot_handle);

    qm_ai_driver_param_t driver = {0};
    /*{
        .open = audio_driver_open,
        .close = audio_driver_close,
        .ctrl = audio_driver_ctrl,
        .write = audio_driver_write,
        .read = audio_driver_read,
    };*/

    qm_ai_audio_options_t audio_op = {
        .channel_num = 1,
        .codec_type = QM_AI_AUDIO_CODEC_TYPE_G722,
        .data_type = QM_AI_AUDIO_DATA_TYPE_PCM,
        .duration = 40,
        .sample_rate = 16000,
        .audio_driver = driver,
        .wakeup_timeout = 0
    };

    qm_ai_set_audio_option(ai_handle, &audio_op);
    qm_ai_set_event_register_option(ai_handle, qm_ai_event_cb, NULL);

    // qm_ai_audio_set_player_mute(ai_handle, 1);
}

void qm_application_start(void)
{
    QM_LOGD("1", "Ver:%s", CONFIG_QM_AI_SDK_VERSION);

    qm_example_iot_init();
}

static void qm_iot_event_handler(void *handle, const qm_iot_event_t *event, void *userdata)
{
    if (handle == NULL || event == NULL){
        return ;
    }

    switch (event->type)
    {
        case QM_IOT_EVENT_CONFIG_TIMEOUT:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONFIG_TIMEOUT");
            break;

        case QM_IOT_EVENT_CONFIG:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONFIG");
            break;

        case QM_IOT_EVENT_CONNING_LOCAL:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONNING_LOCAL");
            break;

        case QM_IOT_EVENT_CLOUD_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_CONN");
            qm_example_ai_init();
            break;

        case QM_IOT_EVENT_BIND_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_BIND_NOTIFY");
            break;

        case QM_IOT_EVENT_CLOUD_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_DISCONN");
            break;

        case QM_IOT_EVENT_LOCAL_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_CONN");
            break;

        case QM_IOT_EVENT_LOCAL_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_DISCONN");
            break;

        case QM_IOT_EVENT_RESET:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_RESET");
            break;

        case QM_IOT_EVENT_OTA_START:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_START");
            break;

        case QM_IOT_EVENT_OTA_PROGRESS_UPADTE:
            break;

        case QM_IOT_EVENT_OTA_SUCCESS:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_SUCCESS");
            break;

        case QM_IOT_EVENT_OTA_FAILED:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_FAILED");
            break;

        default:
            break;
    }
}

static void qm_iot_recv_handler(void *handle, const qm_iot_recv_t *recv, void *userdata)
{
    if (handle == NULL || recv == NULL){
        return ;
    }

    switch (recv->type)
    {
        case QM_IOT_RECV_TYPE_SET:
            QM_LOGD(LOG_TAG, "app recv set siid %d piid %d!!!", recv->siid, recv->piid);
        break;
        
        case QM_IOT_RECV_TYPE_PROPERTY_SET:
            QM_LOGD(LOG_TAG, "app recv prop set did %d siid %d piid %d!!!", recv->did, recv->siid, recv->piid);
        break;

        case QM_IOT_RECV_TYPE_PROPERTY_GET_RSP:
            QM_LOGD(LOG_TAG, "app recv prop get did %d siid %d piid %d!!!", recv->did, recv->siid, recv->piid);
        break;

        case QM_IOT_RECV_TYPE_PROPERTY_GET_REQ:
            QM_LOGD(LOG_TAG, "app recv prop get did %d siid %d piid %d!!!", recv->did, recv->siid, recv->piid);
        break;

        default:

        break;
    }
}

static int audio_driver_open(void)
{
    QM_LOGI("AI_DRV", "audio driver open");

    return QM_EOK;
}

static int audio_driver_close(void)
{
    QM_LOGI("AI_DRV", "audio driver close");

    return QM_EOK;
}

static int audio_driver_write(void *data, size_t len)
{
    if (len == 0) {
        return QM_EINVAL;
    }

    QM_LOGI("AI_DRV", "audio driver write");

    return QM_EOK;
}

static int audio_driver_read(void *data, size_t len, uint32_t timeout)
{
    return QM_EOK;
}

static int audio_driver_ctrl(qm_ai_driver_ctrl_t cmd, void *param)
{
    QM_LOGI("AI_DRV", "audio driver cmd");

    switch (cmd) {
    default:
        return -QM_ERROR;
    }
}
