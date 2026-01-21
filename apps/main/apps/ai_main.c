
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

static void i2s0_slave_init(void);
static void i2s_slave_read_task(void *arg);

static qm_ai_handle_t ai_handle;

static void qm_ai_event_cb(qm_ai_handle_t handle, qm_ai_event_t *event, void *user_data)
{
    switch (event->type)
    {
        case QM_AI_EVENT_CLOUD_LICENSE:
            QM_LOGI("1", "Cloud license success!");
            qm_ai_start(ai_handle);
        break;

        default:
        break;
    }
}

static void qm_ai_stream_event_cb(qm_ai_stream_event_t *event, void *user_data)
{
    switch (event->type)
    {
        case QM_AI_STREAM_TYPE_STT:
            QM_LOGI("1", "User says: %s", event->data.stt.text);
            break;
        case QM_AI_STREAM_TYPE_TTS:
            QM_LOGI("1", "Assistant says[%lu]: %.*s", event->data.tts.duration_ms, event->data.tts.text_len, event->data.tts.text);
            break;
        case QM_AI_STREAM_TYPE_AGENT_STATE:
            QM_LOGI("1", "agent state: %s", event->data.agent_state.text);
            break;
        case QM_AI_STREAM_TYPE_INTERRUPT:
            QM_LOGI("1", "user interrupt %d", event->data.interrupt.turn_id);
            break;
        default:
            break;
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

    qm_ai_register_module(ai_handle, QM_AI_MOD_VOLUME, NULL);

    qm_ai_set_event_register_option(ai_handle, qm_ai_event_cb, NULL);
    qm_ai_set_stream_register_option(ai_handle, qm_ai_stream_event_cb, NULL);

    // i2s0_slave_init();
    // xTaskCreate(i2s_slave_read_task, "draw_3d_image", 4096, NULL, 4, NULL);
}

#include "qm.h"

#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"

#define I2S0_BCLK_IO   38
#define I2S0_WS_IO     48
#define I2S0_DIN_IO    47

static i2s_chan_handle_t rx_chan = NULL;

void i2s0_slave_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0,
        I2S_ROLE_SLAVE
    );

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = false,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .bclk = I2S0_BCLK_IO,
            .ws   = I2S0_WS_IO,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S0_DIN_IO,
            .invert_flags = {
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}

#define i2s_type_t int32_t

#define FRAME_SAMPLES   (640)
#define PCM_BUF_SIZE    (FRAME_SAMPLES * 2)
static i2s_type_t pcm_buf[PCM_BUF_SIZE];
static i2s_type_t pcm_buf_left[FRAME_SAMPLES];
static int16_t pcm_buf_real[FRAME_SAMPLES];

void i2s_slave_read_task(void *arg)
{
    size_t bytes_read = 0;

    while (1) {
        esp_err_t ret = i2s_channel_read(
            rx_chan,
            pcm_buf,
            sizeof(pcm_buf),
            &bytes_read,
            0xffffffffUL
        );

        if (ret == ESP_OK && bytes_read > 0) {
            int samples = bytes_read / sizeof(i2s_type_t);

            // ESP_LOGI(TAG, "Read %d samples, first=%ld", samples, pcm_buf[0]);

            // for (int i = 0; i < 16; i++) {
            //     ESP_LOGI(TAG, "Read[%d]: %ld", i, pcm_buf[i]);
            // }

            for (int i = 0; i < samples / 2; i++) {
                pcm_buf_left[i] = pcm_buf[i * 2];
                pcm_buf_real[i] = (int16_t)(pcm_buf_left[i] >> 16);
            }

            // ESP_LOGI(TAG, "Convert %d samples, first=%d", samples / 2, pcm_buf_real[0]);

            // for (int i = 0; i < 16; i++) {
            //     ESP_LOGI(TAG, "Read[%d]: %d", i, pcm_buf_real[i]);
            // }

            qm_ai_rtc_send_audio_data(pcm_buf_real, FRAME_SAMPLES * sizeof(uint16_t));
            // qm_agora_send_data(pcm_buf, samples);
        }
    }
}
