#include "agora_pcm_bridge.h"

#include <string.h>

#include "agora_opus.h"
#include "qm.h"
#include "qm_log.h"
#include "vb701n.h"

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_TAG "agora_pcm"

/* 芯片麦一包 512 字节。播放侧按 PCM 打开，驱动自己切成 320 字节/10 ms。 */
#define CHIP_PCM_READ_MAX 512
/* 16 kHz 下一包按最长 120 ms 解。 */
#define CHIP_PCM_DEC_MAX  3840
/* opus_encode 会 alloca。qm_audio_cap 只有 4 KB，编码放到这个任务里。 */
#define ENC_STACK_BYTES   (30 * 1024)
#define ENC_PKT_MAX       512
#define ENC_Q_LEN         8

static const qm_audio_format_t s_chip_pcm = {
    .codec = QM_AUDIO_CODEC_PCM,
    .data_type = QM_AUDIO_CODEC_PCM,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 10,
};

static const qm_audio_format_t s_wire_opus = {
    .codec = QM_AUDIO_CODEC_OPUS,
    .data_type = QM_AUDIO_CODEC_OPUS,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 20,
};

typedef struct {
    uint16_t len;
    uint8_t data[ENC_PKT_MAX];
} enc_pkt_t;

static qm_queue_t s_enc_q;
static StaticTask_t s_enc_tcb;
static StackType_t *s_enc_stack;
static uint8_t s_enc_ready;
static void *s_enc_ctx;

static void enc_task(void *arg)
{
    uint8_t pcm[CHIP_PCM_READ_MAX];

    (void)arg;
    QM_LOGI(LOG_TAG, "enc loop");
    for (;;) {
        enc_pkt_t pkt;
        int n;
        int pkt_len;

        n = vb701n_audio_driver.read(s_enc_ctx, pcm, sizeof(pcm), QM_WAIT_FOREVER);
        if (n <= 0) {
            qm_msleep(20);
            continue;
        }

        pkt_len = agora_opus_pcm_to_packet(pcm, (uint32_t)n, pkt.data, sizeof(pkt.data));
        if (pkt_len <= 0) {
            continue;
        }
        pkt.len = (uint16_t)pkt_len;
        if (qm_queue_send(&s_enc_q, &pkt, sizeof(pkt)) != QM_EOK) {
            QM_LOGW(LOG_TAG, "enc queue full, drop %d", pkt_len);
        }
    }
}

static int enc_start(void *driver_ctx)
{
    if (s_enc_ready) {
        return QM_EOK;
    }
    s_enc_ctx = driver_ctx;
    if (qm_queue_new(&s_enc_q, ENC_Q_LEN, sizeof(enc_pkt_t)) != QM_EOK) {
        QM_LOGE(LOG_TAG, "enc queue create failed");
        return -QM_ENOMEM;
    }
    s_enc_stack = (StackType_t *)heap_caps_malloc(ENC_STACK_BYTES,
                                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_enc_stack == NULL) {
        QM_LOGE(LOG_TAG, "enc stack alloc failed");
        (void)qm_queue_free(&s_enc_q);
        return -QM_ENOMEM;
    }
    if (xTaskCreateStatic(enc_task, "agora_enc",
                          ENC_STACK_BYTES / sizeof(StackType_t),
                          NULL, CONFIG_QM_AUDIO_CAPTURE_TASK_PRIORITY,
                          s_enc_stack, &s_enc_tcb) == NULL) {
        QM_LOGE(LOG_TAG, "enc task create failed");
        heap_caps_free(s_enc_stack);
        s_enc_stack = NULL;
        (void)qm_queue_free(&s_enc_q);
        return -QM_ENOMEM;
    }
    s_enc_ready = 1;
    QM_LOGI(LOG_TAG, "enc task stack=%u", (unsigned int)ENC_STACK_BYTES);
    return QM_EOK;
}

static int bridge_open(void **driver_ctx,
                       const qm_audio_format_t *input_format,
                       const qm_audio_format_t *output_format,
                       qm_audio_event_cb_t event_cb,
                       void *user_ctx)
{
    static uint8_t s_logged;
    const qm_audio_format_t *chip_in = NULL;
    const qm_audio_format_t *chip_out = NULL;

    if (input_format != NULL) {
        chip_in = &s_chip_pcm;
    }
    if (output_format != NULL) {
        chip_out = &s_chip_pcm;
    }
    if (!s_logged) {
        s_logged = 1;
        QM_LOGI(LOG_TAG, "chip pcm, agent opus");
    }
    {
        int ret = vb701n_audio_driver.open(driver_ctx, chip_in, chip_out, event_cb, user_ctx);

        if (ret != QM_EOK) {
            return ret;
        }
        /* 采集这一侧才编码。播放 open 不带 input_format。 */
        if (input_format != NULL) {
            ret = enc_start(*driver_ctx);
            if (ret != QM_EOK) {
                (void)vb701n_audio_driver.close(*driver_ctx);
            }
        }
        return ret;
    }
}

static int bridge_close(void *driver_ctx)
{
    return vb701n_audio_driver.close(driver_ctx);
}

static int bridge_read(void *driver_ctx, void *data, uint32_t len, uint32_t timeout_ms)
{
    enc_pkt_t pkt;
    unsigned int size = sizeof(pkt);

    (void)driver_ctx;
    if (data == NULL || len == 0) {
        return -QM_EINVAL;
    }
    if (!s_enc_ready) {
        return -QM_EINIT;
    }
    if (qm_queue_recv(&s_enc_q, &pkt, &size, timeout_ms) != QM_EOK) {
        return 0;
    }
    if (pkt.len == 0 || pkt.len > len) {
        QM_LOGE(LOG_TAG, "enc pkt len=%u cap=%u", (unsigned)pkt.len, (unsigned)len);
        return -QM_EINVAL;
    }
    memcpy(data, pkt.data, pkt.len);
    return (int)pkt.len;
}

static int bridge_write_ex(void *driver_ctx,
                           const void *data,
                           uint32_t len,
                           uint32_t timeout_ms,
                           uint32_t *written)
{
    static uint8_t pcm[CHIP_PCM_DEC_MAX];
    int pcm_len;
    int ret;

    if (written != NULL) {
        *written = 0;
    }
    if (data == NULL || len == 0) {
        return -QM_EINVAL;
    }

    pcm_len = agora_opus_packet_to_pcm((const uint8_t *)data, len, pcm, sizeof(pcm));
    if (pcm_len <= 0) {
        return pcm_len < 0 ? pcm_len : -QM_ERROR;
    }

    ret = vb701n_audio_driver.write_ex(driver_ctx, pcm, (uint32_t)pcm_len, timeout_ms, NULL);
    if (ret < 0) {
        return ret;
    }
    if (written != NULL) {
        *written = len;
    }
    return (int)len;
}

static int bridge_write(void *driver_ctx, const void *data, uint32_t len)
{
    return bridge_write_ex(driver_ctx, data, len, 0, NULL);
}

static int bridge_get_caps(void *driver_ctx, qm_audio_caps_t *caps)
{
    int ret;

    ret = vb701n_audio_driver.get_caps(driver_ctx, caps);
    if (ret != QM_EOK) {
        return ret;
    }
    caps->input_format = s_wire_opus;
    caps->output_format = s_wire_opus;
    return QM_EOK;
}

static int bridge_abort_read(void *driver_ctx)
{
    return vb701n_audio_driver.abort_read(driver_ctx);
}

static int bridge_abort_write(void *driver_ctx)
{
    return vb701n_audio_driver.abort_write(driver_ctx);
}

static int bridge_playback_begin(void *driver_ctx, qm_audio_playback_delivery_t delivery)
{
    return vb701n_audio_driver.playback_begin(driver_ctx, delivery);
}

static int bridge_playback_end(void *driver_ctx)
{
    return vb701n_audio_driver.playback_end(driver_ctx);
}

static int bridge_get_progress(void *driver_ctx, uint32_t *position_ms)
{
    return vb701n_audio_driver.get_playback_progress_ms(driver_ctx, position_ms);
}

static int bridge_control(void *driver_ctx, qm_audio_ctrl_t ctrl, void *arg)
{
    return vb701n_audio_driver.control(driver_ctx, ctrl, arg);
}

const qm_audio_driver_if_t agora_pcm_bridge_driver = {
    .name = "agora_pcm",
    .open = bridge_open,
    .close = bridge_close,
    .read = bridge_read,
    .write = bridge_write,
    .write_ex = bridge_write_ex,
    .get_caps = bridge_get_caps,
    .abort_read = bridge_abort_read,
    .abort_write = bridge_abort_write,
    .playback_begin = bridge_playback_begin,
    .playback_end = bridge_playback_end,
    .get_playback_progress_ms = bridge_get_progress,
    .control = bridge_control,
};
