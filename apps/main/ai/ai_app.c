#include "ai_app.h"

#include "qm_agent.h"
#include "qm_audio.h"
#include "qm_log.h"
#include "qm_string.h"
#include "vb701n_player.h"

#define LOG_TAG "ai"

static qm_agent_handle_t s_agent;
static uint8_t s_downlink_logged;

static const char *life_name(qm_agent_state_t state)
{
    switch (state) {
        case QM_AGENT_STATE_IDLE:
            return "idle";
        case QM_AGENT_STATE_READY:
            return "ready";
        case QM_AGENT_STATE_STARTING:
            return "starting";
        case QM_AGENT_STATE_RUNNING:
            return "running";
        case QM_AGENT_STATE_STOPPING:
            return "stopping";
        case QM_AGENT_STATE_ERROR:
            return "error";
        case QM_AGENT_STATE_NONE:
        default:
            return "none";
    }
}

static const char *dialog_name(qm_agent_dialog_state_t state)
{
    switch (state) {
        case QM_AGENT_DIALOG_STATE_SILENT:
            return "silent";
        case QM_AGENT_DIALOG_STATE_LISTENING:
            return "listening";
        case QM_AGENT_DIALOG_STATE_THINKING:
            return "thinking";
        case QM_AGENT_DIALOG_STATE_SPEAKING:
            return "speaking";
        case QM_AGENT_DIALOG_STATE_NONE:
        default:
            return "none";
    }
}

static void on_agent_event(qm_agent_handle_t handle,
                           const qm_agent_event_t *event,
                           void *user_ctx)
{
    (void)handle;
    (void)user_ctx;
    if (event == NULL) {
        return;
    }

    switch (event->id) {
        case QM_AGENT_EVENT_STATE_CHANGED:
            QM_LOGI(LOG_TAG, "state %s -> %s",
                    life_name(event->data.state.old_state),
                    life_name(event->data.state.new_state));
            break;
        case QM_AGENT_EVENT_CONNECTED:
            QM_LOGI(LOG_TAG, "agora connected");
            break;
        case QM_AGENT_EVENT_DISCONNECTED:
            s_downlink_logged = 0;
            QM_LOGW(LOG_TAG, "agora disconnected");
            break;
        case QM_AGENT_EVENT_ASR:
            QM_LOGI(LOG_TAG, "asr final=%d text=%s",
                    event->data.asr.final,
                    event->data.asr.text ? event->data.asr.text : "");
            break;
        case QM_AGENT_EVENT_TTS:
            QM_LOGI(LOG_TAG, "tts final=%d text=%s",
                    event->data.tts.final,
                    event->data.tts.text ? event->data.tts.text : "");
            break;
        case QM_AGENT_EVENT_AGENT_TEXT:
            QM_LOGI(LOG_TAG, "agent text final=%d text=%s",
                    event->data.agent_text.final,
                    event->data.agent_text.text ? event->data.agent_text.text : "");
            break;
        case QM_AGENT_EVENT_AGENT_STATE:
            QM_LOGI(LOG_TAG, "dialog=%s",
                    dialog_name(event->data.agent_state.state));
            break;
        case QM_AGENT_EVENT_AUDIO:
            if (!s_downlink_logged) {
                s_downlink_logged = 1;
                QM_LOGI(LOG_TAG, "downlink first frame len=%u codec=%d frame_ms=%u",
                        (unsigned)event->data.audio.len,
                        (int)event->data.audio.format.codec,
                        (unsigned)event->data.audio.format.frame_ms);
            }
            break;
        case QM_AGENT_EVENT_TTS_PLAYBACK_STARTED:
            break;
        case QM_AGENT_EVENT_ERROR:
            QM_LOGE(LOG_TAG, "agent error code=%d msg=%s",
                    event->code,
                    event->data.error.message ? event->data.error.message : "");
            break;
        default:
            QM_LOGD(LOG_TAG, "agent event id=%d", event->id);
            break;
    }
}

int ai_app_bind_cloud(void *iot_handle)
{
    qm_agent_config_t config;
    qm_audio_handle_t audio;
    qm_audio_format_t format;
    int ret;

    if (s_agent != NULL) {
        return QM_EOK;
    }
    if (iot_handle == NULL) {
        return -QM_EINVAL;
    }

    audio = vb701n_player_get_audio();
    if (audio == NULL) {
        QM_LOGE(LOG_TAG, "bind failed: vb701n audio is null");
        return -QM_EINIT;
    }

    ret = qm_audio_get_input_format(audio, &format);
    if (ret != QM_EOK) {
        QM_LOGE(LOG_TAG, "get input format failed ret=%d", ret);
        return ret;
    }
    if (format.codec != QM_AUDIO_CODEC_PCM && format.codec != QM_AUDIO_CODEC_OPUS) {
        QM_LOGE(LOG_TAG, "unsupported codec=%d frame_ms=%u",
                (int)format.codec, (unsigned)format.frame_ms);
        return -QM_EINVAL;
    }
    QM_LOGI(LOG_TAG, "voice uplink %s rate=%u ch=%u frame_ms=%u",
            format.codec == QM_AUDIO_CODEC_OPUS ? "opus" : "pcm",
            (unsigned)format.sample_rate,
            (unsigned)format.channel_num,
            (unsigned)format.frame_ms);

    qm_memset(&config, 0, sizeof(config));
    config.transport.type = QM_AGENT_TRANSPORT_AGORA;
    config.audio = audio;
    qm_agent_config_init_defaults(&config);
    /*
     * VB701N 的 PAUSE 会发 0x0201，协议上喇叭也停。
     * 对话期间用 KEEP，TTS 下行时不要把麦和喇叭一起关掉。
     */
    config.transport_capture_policy = QM_AGENT_CAPTURE_POLICY_KEEP;
    /* app_id / channel / token / license / uid 由 qm_agent 从鉴权回包填入。 */
    config.transport.data.agora.audio_enable = QM_TRUE;
    config.transport.data.agora.video_enable = QM_FALSE;
    config.iot_handle = iot_handle;
    config.event_cb = on_agent_event;

    s_agent = qm_agent_create(&config);
    if (s_agent == NULL) {
        QM_LOGE(LOG_TAG, "qm_agent_create failed");
        return -QM_EINIT;
    }
    QM_LOGI(LOG_TAG, "session created state=%s",
            life_name(qm_agent_get_state(s_agent)));
    return QM_EOK;
}

int ai_on_cloud_connected(void)
{
    int ret;

    if (s_agent == NULL) {
        QM_LOGE(LOG_TAG, "cloud connected but agent is null");
        return -QM_EINVAL;
    }

    ret = qm_agent_network_update(s_agent, QM_AGENT_NETWORK_UP);
    if (ret != QM_EOK) {
        QM_LOGW(LOG_TAG, "network up ret=%d", ret);
    }
    ret = qm_agent_start(s_agent);
    QM_LOGI(LOG_TAG, "qm_agent_start ret=%d state=%s",
            ret, life_name(qm_agent_get_state(s_agent)));
    return ret;
}

void ai_on_cloud_disconnected(void)
{
    int ret;

    if (s_agent == NULL) {
        return;
    }
    (void)qm_agent_network_update(s_agent, QM_AGENT_NETWORK_DOWN);
    ret = qm_agent_stop(s_agent);
    QM_LOGI(LOG_TAG, "qm_agent_stop ret=%d state=%s",
            ret, life_name(qm_agent_get_state(s_agent)));
}
