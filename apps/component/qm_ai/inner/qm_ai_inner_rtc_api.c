#include "qm.h"
#include "qm_agora.h"
#include "qm_agora_gen.h"
#include "qm_agora_stream.h"
#include "qm_ai_inner_rtc_api.h"

#define RTC_TAG "AI_RTC"

typedef struct {
    uint8_t rtc_init_flag;
    uint8_t rtc_connect_flag;
    uint8_t rtc_join_flag;
} qm_ai_rtc_handle_t;

static qm_ai_rtc_handle_t qm_ai_rtc_handle = {0};

int qm_ai_rtc_init(qm_ai_rtc_cfg_t cfg)
{
    if (cfg.license == NULL || cfg.license[0] == '\0')
    {
        QM_LOGE(RTC_TAG, "No rtc license, please retry!");
        return QM_ERROR;
    }

    if (qm_ai_rtc_handle.rtc_init_flag == QM_TRUE) {
        QM_LOGE(RTC_TAG, "Rtc already inited!");
        return QM_ERROR;
    }

    qm_agora_cfg_t agora_cfg = {
        .audio_enable = cfg.audio_enable,
        .video_enable = cfg.video_enable,
        .stream_cb = cfg.stream_cb,
        .audio_cb = cfg.audio_cb,
        .event_cb = cfg.event_cb,
    };

    agora_cfg.license = (char *)malloc(strlen(cfg.license) + 1);
    memcpy(agora_cfg.license, cfg.license, strlen(cfg.license) + 1);

    if (qm_agora_init(agora_cfg) != QM_EOK) {
        QM_LOGE(RTC_TAG, "Fail to init rtc!");
        return QM_ERROR;
    }

    QM_LOGI(RTC_TAG, "Link init success!");
    qm_ai_rtc_handle.rtc_init_flag = QM_TRUE;

    return QM_EOK;
}

int qm_ai_rtc_connect(void)
{
    if (qm_ai_rtc_handle.rtc_init_flag != QM_TRUE) {
        QM_LOGE(RTC_TAG, "Fail to connect rtc, not inited!");
        return QM_EINIT;
    }

    int ret = qm_agora_connect();
    if (ret != QM_EOK) {
        QM_LOGE(RTC_TAG, "Fail to connect rtc!");
        return ret;
    }

    QM_LOGI(RTC_TAG, "Link connect success!");
    qm_ai_rtc_handle.rtc_connect_flag = QM_TRUE;

    return QM_EOK;
}

int qm_ai_rtc_join(void)
{
    if (qm_ai_rtc_handle.rtc_connect_flag != QM_TRUE) {
        QM_LOGE(RTC_TAG, "Fail to join rtc channel, not connected!");
        return QM_EINIT;
    }

    int ret = qm_agora_join();
    if (ret != QM_EOK) {
        QM_LOGE(RTC_TAG, "Fail to join rtc channel!");
        return ret;
    }

    QM_LOGI(RTC_TAG, "Link join success!");
    qm_ai_rtc_handle.rtc_join_flag = QM_TRUE;

    return QM_EOK;
}

int qm_ai_rtc_send_audio_data(const void *data, size_t len)
{
    if (qm_ai_rtc_handle.rtc_join_flag != QM_TRUE) {
        QM_LOGE(RTC_TAG, "Fail to send audio data, not joined rtc channel!");
        return QM_EINIT;
    }

    if (data == NULL || len == 0) {
        return QM_EINVAL;
    }

    int ret = qm_agora_send_audio_data(data, len);

    return ret;
}

int qm_ai_rtc_deinit(void)
{
    qm_ai_rtc_handle.rtc_init_flag = QM_FALSE;
    qm_ai_rtc_handle.rtc_connect_flag = QM_FALSE;
    qm_ai_rtc_handle.rtc_join_flag = QM_FALSE;

    int ret = qm_agora_deinit();

    return ret;
}

int qm_ai_rtc_disconnect(void)
{
    qm_ai_rtc_handle.rtc_connect_flag = QM_FALSE;
    qm_ai_rtc_handle.rtc_join_flag = QM_FALSE;

    int ret = qm_agora_disconnect();

    return ret;
}

int qm_ai_rtc_leave(void)
{
    qm_ai_rtc_handle.rtc_join_flag = QM_FALSE;

    int ret = qm_agora_leave();

    return ret;
}

int qm_ai_rtc_clear_buffer_data(void)
{
    qm_agora_reset_data();

    return QM_EOK;
}
