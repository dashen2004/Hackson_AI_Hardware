
#include "qm.h"
#include "qm_kv.h"

#include "esp_log.h"
#include "esp_system.h"

#include "ai_main.h"
#include "xjiang_cloud.h"
#include "qm_ai_inner_rtc_api.h"
#include "qm_ai_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static qm_ai_handle_t ai_handle;

static void qm_ai_event_cb(qm_ai_handle_t handle, qm_ai_event_t *event, void *user_data)
{
    switch (event->type)
    {
        case QM_AI_EVENT_CLOUD_LICENSE:
            QM_LOGI("1", "Cloud license success!");
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

void qm_application_start(void)
{
    QM_LOGD("1", "Ver:%s", CONFIG_QM_AI_SDK_VERSION);

    xjiang_cloud_init();

    qm_msleep(15 * 1000);

    ai_handle = qm_ai_open(QM_AI_COMM_TYPE_RTC, NULL);

    void *iot_handle = xjiang_cloud_get_handle();
    qm_ai_init(ai_handle, iot_handle);

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
    qm_ai_set_service_option(ai_handle, QM_AI_MOD_VOLUME, NULL);
    qm_ai_set_event_register_option(ai_handle, qm_ai_event_cb, NULL);
}
