#include "qm.h"
#include "qm_log.h"
#include "qm_work.h"
#include "qm_audio.h"
#include "vb701n.h"
#include "voice.h"

#include <string.h>

#define LOG_TAG "voice"

/* 1: 芯片侧双向 PCM。0: 双向 Opus 40 字节/20 ms。报给 agent 的格式始终是 Opus。 */
#ifndef VB701N_APP_PCM
#define VB701N_APP_PCM  1
#endif

#if CONFIG_VOICE_BACKEND_AGORA && VB701N_APP_PCM
#include "pcm_drv.h"
#endif

static const qm_audio_format_t g_in_fmt = {
    .codec = QM_AUDIO_CODEC_OPUS,
    .data_type = QM_AUDIO_CODEC_OPUS,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 20,
};
static const qm_audio_format_t g_out_fmt = {
    .codec = QM_AUDIO_CODEC_OPUS,
    .data_type = QM_AUDIO_CODEC_OPUS,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 20,
};

static qm_audio_handle_t g_audio;
static voice_wakeup_cb_t g_wakeup_cb;
static qm_work_t g_ver_work;
static qm_work_t g_play_work;
static uint8_t g_ver_retry;
static uint8_t g_got_version;
static uint8_t g_play_started;

static void on_chip_ready(void *arg)
{
    uint32_t vol = 20;

    (void)arg;
    if (g_play_started) {
        return;
    }
    if (g_audio == NULL) {
        return;
    }
    g_play_started = 1;
    qm_audio_control(g_audio, QM_AUDIO_CTRL_SET_VOLUME, &vol);
    {
        qm_audio_capture_policy_t policy = QM_AUDIO_CAPTURE_POLICY_KEEP;

        /* KEEP 发 02 31 01 开麦。PAUSE 会发 02 01，喇叭和麦一起停。 */
        qm_audio_control(g_audio, QM_AUDIO_CTRL_SET_CAPTURE_POLICY, &policy);
        QM_LOGD(LOG_TAG, "mic record start, opus 16k/20ms");
    }
}

static void on_audio_event(const qm_audio_event_t *event, void *user)
{
    const uint8_t *data;
    uint32_t len;

    (void)user;
    if (event == NULL) {
        return;
    }
    data = (const uint8_t *)event->data;
    len = event->data_len;
    if (event->id == QM_AUDIO_EVENT_VERSION) {
        g_got_version = 1;
        if (data != NULL && len > 0) {
            char ver[64];
            uint32_t n = len;
            if (n >= sizeof(ver)) {
                n = sizeof(ver) - 1;
            }
            memcpy(ver, data, n);
            ver[n] = '\0';
            QM_LOGD(LOG_TAG, "version: %s", ver);
        }
        qm_post_delayed_action(&g_play_work, on_chip_ready, NULL, 500);
        return;
    }
    if (event->id == QM_AUDIO_EVENT_WAKEUP) {
        char text[128];
        uint32_t n = len;
        if (data == NULL || len == 0) {
            QM_LOGD(LOG_TAG, "wakeup: (empty)");
        } else {
            if (n >= sizeof(text)) {
                n = sizeof(text) - 1;
            }
            memcpy(text, data, n);
            text[n] = '\0';
            QM_LOGD(LOG_TAG, "wakeup: %s", text);
        }
        if (g_wakeup_cb != NULL) {
            g_wakeup_cb();
        }
    }
}

void voice_set_wakeup_cb(voice_wakeup_cb_t cb)
{
    g_wakeup_cb = cb;
}

static void ver_retry_handle(void *arg)
{
    (void)arg;
    if (g_got_version) {
        return;
    }
    if (g_ver_retry >= 5) {
        QM_LOGD(LOG_TAG, "version query timeout, continue anyway");
        on_chip_ready(NULL);
        return;
    }
    g_ver_retry++;
    qm_audio_control(g_audio, QM_AUDIO_CTRL_QUERY_VERSION, NULL);
    qm_post_delayed_action(&g_ver_work, ver_retry_handle, NULL, 3000);
}

qm_err_t voice_start(void)
{
    qm_audio_config_t audio_cfg;
    qm_audio_event_cb_t event_callback = on_audio_event;

    memset(&audio_cfg, 0, sizeof(audio_cfg));
#if CONFIG_VOICE_BACKEND_AGORA && VB701N_APP_PCM
    audio_cfg.capture_driver_if = &pcm_driver;
    audio_cfg.playback_driver_if = &pcm_driver;
#else
    audio_cfg.capture_driver_if = &vb701n_audio_driver;
    audio_cfg.playback_driver_if = &vb701n_audio_driver;
#endif
    audio_cfg.input_format = &g_in_fmt;
    audio_cfg.output_format = &g_out_fmt;
    g_audio = qm_audio_create(&audio_cfg);
    if (g_audio == NULL) {
        QM_LOGD(LOG_TAG, "qm_audio_create failed");
        return -QM_ENOMEM;
    }
    qm_audio_control(g_audio, QM_AUDIO_CTRL_SET_EVENT_CALLBACK, &event_callback);
    qm_post_delayed_action(&g_ver_work, ver_retry_handle, NULL, 1500);
#if CONFIG_VOICE_BACKEND_AGORA && VB701N_APP_PCM
    QM_LOGI(LOG_TAG, "qm_audio ready, chip pcm, agent opus in %ums out %ums",
            (unsigned)g_in_fmt.frame_ms, (unsigned)g_out_fmt.frame_ms);
#else
    QM_LOGI(LOG_TAG, "qm_audio ready, %s in %ums out %ums",
            VB701N_APP_PCM ? "pcm" : "opus",
            (unsigned)g_in_fmt.frame_ms, (unsigned)g_out_fmt.frame_ms);
#endif
    return QM_EOK;
}

qm_audio_handle_t voice_get_audio(void)
{
    return g_audio;
}
