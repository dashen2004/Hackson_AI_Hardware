#include "agent_app.h"
#include "ljyd_cloud.h"
#include "qm_config.h"

#if defined(CONFIG_LJYD_PLAYBACK_SUPPORT) && CONFIG_LJYD_PLAYBACK_SUPPORT
#include "playback.h"
#endif

#include "cJSON.h"
#include "qm_agent.h"
/* qdev has no public auth getter; keep this compatibility access app-local. */
#include "qm_agent_internal.h"
#include "qm_audio.h"
#include "qm_http_client.h"
#include "qm_iot_dynreg.h"
#include "qm_log.h"
#include "qm_string.h"
#include "qm.h"
#include "voice.h"

#define LOG_TAG "agent"

static qm_agent_handle_t s_agent;
static uint8_t s_downlink_logged;
static uint8_t s_cloud_connected;
static uint8_t s_agent_started;
static qm_sem_t s_tts_complete_sem;
static uint8_t s_tts_complete_sem_ready;
static volatile uint8_t s_waiting_tts_complete;
static volatile uint8_t s_tts_wait_aborted;
static volatile uint8_t s_tts_text_final;
static volatile uint8_t s_tts_playback_started;

static void abort_tts_wait(void);

#define AGENT_CORPUS_URL_MAX       160U
#define AGENT_CORPUS_TIMEOUT_MS    15000U

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
            if (event->data.tts.final) {
                s_tts_text_final = 1U;
            }
            break;
        case QM_AGENT_EVENT_AGENT_TEXT:
            QM_LOGI(LOG_TAG, "agent text final=%d text=%s",
                    event->data.agent_text.final,
                    event->data.agent_text.text ? event->data.agent_text.text : "");
            if (event->data.agent_text.final) {
                s_tts_text_final = 1U;
            }
            break;
        case QM_AGENT_EVENT_AGENT_STATE:
            QM_LOGI(LOG_TAG, "dialog=%s",
                    dialog_name(event->data.agent_state.state));
            /* The core may re-enable upload when it reports speaking. The
             * custom-corpus transaction owns upload until its local TTS ends. */
            if (s_waiting_tts_complete && s_agent != NULL) {
                qm_agent_set_upload_enabled(s_agent, QM_FALSE);
                if (event->data.agent_state.state == QM_AGENT_DIALOG_STATE_SILENT &&
                    s_tts_text_final && s_tts_playback_started) {
                    /* Agora has no TTS_INPUT_COMPLETE event. At silent, the
                     * final text and first local frame prove this turn can
                     * be closed; qdev will drain the speaker and emit the
                     * normal PLAYBACK_COMPLETE event. */
                    qm_agent_audio_engine_mark_tts_input_complete(s_agent);
                }
            }
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
            s_tts_playback_started = 1U;
            break;
        case QM_AGENT_EVENT_TTS_PLAYBACK_COMPLETE:
            if (s_waiting_tts_complete && s_tts_complete_sem_ready) {
                (void)qm_sem_signal(&s_tts_complete_sem);
            }
            QM_LOGI(LOG_TAG, "tts playback complete generation=%u",
                    (unsigned)event->data.tts_playback.generation);
            break;
        case QM_AGENT_EVENT_ERROR:
            QM_LOGE(LOG_TAG, "agent error code=%d msg=%s",
                    event->code,
                    event->data.error.message ? event->data.error.message : "");
            /* Agora may fail before emitting TTS_INPUT_COMPLETE. Once the
             * transport has reported an error, no more guide PCM can arrive;
             * let the local TTS queue drain and produce PLAYBACK_COMPLETE. */
            if (s_waiting_tts_complete && s_agent != NULL) {
                qm_agent_audio_engine_mark_tts_input_complete(s_agent);
            }
            break;
        default:
            QM_LOGD(LOG_TAG, "agent event id=%d", event->id);
            break;
    }
}

static int agent_app_bind_cloud(void *iot_handle)
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

    audio = voice_get_audio();
    if (audio == NULL) {
        QM_LOGE(LOG_TAG, "bind failed: voice audio is null");
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

static void agent_on_cloud_connected(void)
{
    s_cloud_connected = 1;
    if (s_agent == NULL) {
        return;
    }
    if (qm_agent_network_update(s_agent, QM_AGENT_NETWORK_UP) != QM_EOK) {
        QM_LOGW(LOG_TAG, "network up failed");
    }
}

static void agent_on_cloud_disconnected(void)
{
    int ret;

    s_cloud_connected = 0;
    s_agent_started = 0;
    abort_tts_wait();
    if (s_agent == NULL) {
        return;
    }
    (void)qm_agent_network_update(s_agent, QM_AGENT_NETWORK_DOWN);
    ret = qm_agent_stop(s_agent);
    QM_LOGI(LOG_TAG, "qm_agent_stop ret=%d state=%s",
            ret, life_name(qm_agent_get_state(s_agent)));
}

static void on_cloud_event(ljyd_cloud_event_t event, void *iot_handle, void *userdata)
{
    (void)userdata;
    (void)iot_handle;
    if (event == LJYD_CLOUD_EVENT_CONNECTED) {
        agent_on_cloud_connected();
        return;
    }
    if (event == LJYD_CLOUD_EVENT_DISCONNECTED) {
        agent_on_cloud_disconnected();
    }
}

int agent_app_leave(void)
{
    int ret;

    if (s_agent == NULL) {
        return -QM_EINVAL;
    }
    abort_tts_wait();
    ret = qm_agent_stop(s_agent);
    if (ret == QM_EOK) {
        s_agent_started = 0;
    }
    QM_LOGI(LOG_TAG, "leave stop ret=%d state=%s",
            ret, life_name(qm_agent_get_state(s_agent)));
    return ret;
}

int agent_app_is_active(void)
{
    return (s_agent_started != 0) ? 1 : 0;
}

int agent_app_play_local_audio(const void *data, uint32_t len)
{
    int ret;

    if (s_agent == NULL) {
        return -QM_EINIT;
    }
    ret = qm_agent_play_audio(s_agent, data, len, NULL, NULL);
    QM_LOGI(LOG_TAG, "local prompt len=%u ret=%d", (unsigned)len, ret);
    return ret;
}

static void drain_tts_complete_sem(void)
{
    if (!s_tts_complete_sem_ready) {
        return;
    }
    while (qm_sem_wait(&s_tts_complete_sem, QM_NO_WAIT) == QM_EOK) {
    }
}

static void abort_tts_wait(void)
{
    if (!s_waiting_tts_complete) {
        return;
    }
    s_tts_wait_aborted = 1U;
    if (s_tts_complete_sem_ready) {
        (void)qm_sem_signal(&s_tts_complete_sem);
    }
}

int agent_app_set_resource_mode(qm_bool active)
{
    qm_audio_handle_t audio = voice_get_audio();
    qm_audio_capture_policy_t policy;

    if (audio == NULL) {
        return -QM_EINIT;
    }
    policy = active ? QM_AUDIO_CAPTURE_POLICY_PAUSE :
                      QM_AUDIO_CAPTURE_POLICY_KEEP;
    if (qm_audio_control(audio, QM_AUDIO_CTRL_SET_CAPTURE_POLICY, &policy) != QM_EOK) {
        QM_LOGW(LOG_TAG, "resource capture policy failed active=%d", (int)active);
    }
    if (s_agent != NULL) {
        qm_agent_set_upload_enabled(s_agent,
                                    active ? QM_FALSE :
                                    (s_cloud_connected && s_agent_started) ? QM_TRUE : QM_FALSE);
    }
    return QM_EOK;
}

int agent_app_abort_output(void)
{
    int ret = QM_EOK;
    int step_ret;
    qm_audio_handle_t audio = voice_get_audio();

    if (s_agent != NULL) {
        step_ret = qm_agent_interrupt(s_agent);
        if (step_ret == -QM_ENOSYS) {
            /* qm_agent_interrupt() already flushed local TTS, but Agora's
             * unsupported interrupt has no ACK to reopen TTS acceptance. */
            qm_agent_audio_engine_accept_next_tts(s_agent);
            step_ret = QM_EOK;
        }
        if (step_ret != QM_EOK && ret == QM_EOK) {
            ret = step_ret;
        }
        qm_agent_set_upload_enabled(s_agent, QM_FALSE);
    }
    if (audio != NULL) {
        step_ret = qm_audio_control(audio, QM_AUDIO_CTRL_STOP_PLAYBACK, NULL);
        /* The qdev fallback may already have released the session. */
        if (step_ret != QM_EOK && step_ret != -QM_EBUSY && ret == QM_EOK) {
            ret = step_ret;
        }
    }
    abort_tts_wait();
    QM_LOGI(LOG_TAG, "abort output ret=%d", ret);
    return ret;
}

static const char *corpus_host_prefix(void)
{
    switch (qm_iot_dynreg_env_get()) {
        case QM_IOT_DYNREG_ENV_TEST:
            return "api-test";
        case QM_IOT_DYNREG_ENV_PROD:
            return "api";
        case QM_IOT_DYNREG_ENV_DEV:
        default:
            return "api-dev";
    }
}

static int get_agent_auth_info(qm_agent_auth_info_t *info)
{
    int ret;

    if (s_agent == NULL || info == NULL) {
        return -QM_EINVAL;
    }
    qm_memset(info, 0, sizeof(*info));
    if (s_agent->auth.lock_valid) {
        qm_mutex_lock(&s_agent->auth.lock, QM_WAIT_FOREVER);
    }
    if (s_agent->auth.helper == NULL || !s_agent->auth.has_auth) {
        if (s_agent->auth.lock_valid) {
            qm_mutex_unlock(&s_agent->auth.lock);
        }
        return -QM_ENOENT;
    }
    ret = qm_agent_auth_get_token(s_agent->auth.helper, info);
    if (s_agent->auth.lock_valid) {
        qm_mutex_unlock(&s_agent->auth.lock);
    }
    return ret;
}

int agent_app_user_corpus(const char *text)
{
    qm_agent_auth_info_t auth = {0};
    qm_http_client_request_t request = {0};
    qm_http_client_response_t response = {0};
    qm_http_header_t header;
    cJSON *root;
    char url[AGENT_CORPUS_URL_MAX];
    char *body;
    int ret;

    if (text == NULL || text[0] == '\0') {
        return -QM_EINVAL;
    }
    if (!s_cloud_connected || s_agent == NULL) {
        return -QM_EBUSY;
    }
    ret = get_agent_auth_info(&auth);
    if (ret != QM_EOK || auth.ljyd.token == NULL || auth.ljyd.token[0] == '\0') {
        if (ret == QM_EOK) {
            ret = -QM_ENOENT;
        }
        QM_LOGW(LOG_TAG, "user corpus auth unavailable ret=%d", ret);
        qm_agent_auth_info_deinit(&auth);
        return ret;
    }

    root = cJSON_CreateObject();
    if (root == NULL || !cJSON_AddStringToObject(root, "content", text)) {
        cJSON_Delete(root);
        qm_agent_auth_info_deinit(&auth);
        return -QM_ENOMEM;
    }
    body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (body == NULL) {
        qm_agent_auth_info_deinit(&auth);
        return -QM_ENOMEM;
    }
    ret = qm_snprintf(url, sizeof(url),
                      "https://%s.ljyd.cn/agent-device/agent/sendSpeakRequest",
                      corpus_host_prefix());
    if (ret <= 0 || ret >= (int)sizeof(url)) {
        cJSON_free(body);
        qm_agent_auth_info_deinit(&auth);
        return -QM_EFULL;
    }

    drain_tts_complete_sem();
    s_tts_wait_aborted = 0U;
    s_tts_text_final = 0U;
    s_tts_playback_started = 0U;
    s_waiting_tts_complete = 1U;
    header.name = "Authorization";
    header.value = auth.ljyd.token;
    request.method = QM_HTTP_POST;
    request.url = url;
    request.body = body;
    request.body_len = (int)qm_strlen(body);
    request.content_type = "application/json";
    request.headers = &header;
    request.header_count = 1;
    request.timeout_ms = AGENT_CORPUS_TIMEOUT_MS;
    QM_LOGI(LOG_TAG, "user corpus request url=%s text_len=%u",
            url, (unsigned)qm_strlen(text));
    ret = qm_http_client_execute(&request, &response);
    if (ret == QM_EOK && response.status_code != 200) {
        ret = -QM_EIO;
    }
    if (ret != QM_EOK) {
        s_waiting_tts_complete = 0U;
    }
    QM_LOGI(LOG_TAG, "user corpus response ret=%d status=%d",
            ret, response.status_code);
    cJSON_free(body);
    qm_agent_auth_info_deinit(&auth);
    return ret;
}

int agent_app_wait_tts_complete(uint32_t timeout_ms)
{
    int ret;
    qm_bool aborted;

    if (!s_tts_complete_sem_ready || !s_waiting_tts_complete) {
        return -QM_EINVAL;
    }
    ret = qm_sem_wait(&s_tts_complete_sem, timeout_ms);
    aborted = s_tts_wait_aborted ? QM_TRUE : QM_FALSE;
    s_waiting_tts_complete = 0U;
    s_tts_wait_aborted = 0U;
    s_tts_text_final = 0U;
    s_tts_playback_started = 0U;
    if (aborted) {
        return -QM_EINTR;
    }
    if (ret != QM_EOK) {
        QM_LOGW(LOG_TAG, "wait tts playback complete timeout=%u",
                (unsigned)timeout_ms);
        return -QM_ETIMEOUT;
    }
    return QM_EOK;
}

static void on_wakeup(void)
{
    int ret;

    /* A wake word always takes ownership back from active output. */
#if defined(CONFIG_LJYD_PLAYBACK_SUPPORT) && CONFIG_LJYD_PLAYBACK_SUPPORT
    (void)playback_stop();
#endif

    if (s_agent == NULL || !s_cloud_connected) {
        QM_LOGI(LOG_TAG, "wakeup ignored, cloud=%u agent=%p",
                (unsigned)s_cloud_connected, (void *)s_agent);
        return;
    }
    if (s_agent_started) {
        (void)agent_app_abort_output();
        return;
    }
    ret = qm_agent_start(s_agent);
    QM_LOGI(LOG_TAG, "qm_agent_start ret=%d state=%s",
            ret, life_name(qm_agent_get_state(s_agent)));
    if (ret == QM_EOK) {
        s_agent_started = 1;
    }
}

int agent_app_init(void *iot_handle)
{
    int ret;

    if (!s_tts_complete_sem_ready) {
        ret = qm_sem_new(&s_tts_complete_sem, 0);
        if (ret != QM_EOK || !qm_sem_is_valid(&s_tts_complete_sem)) {
            return -QM_ENOMEM;
        }
        s_tts_complete_sem_ready = 1U;
    }
    ret = agent_app_bind_cloud(iot_handle);
    if (ret != QM_EOK) {
        return ret;
    }
    ljyd_cloud_set_event_cb(on_cloud_event, NULL);
    voice_set_wakeup_cb(on_wakeup);
    return QM_EOK;
}
