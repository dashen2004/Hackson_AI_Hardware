#include "app_audio_prompt.h"

#include <stdio.h>
#include <string.h>

#include "esp_spiffs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "qm_ai_api.h"
#include "qm_task_psram.h"

#define LOG_TAG                 "app_audio"
#define YBJC_START_WAV_PATH     "/spiffs/ybjc_start.wav"
#define AUDIO_PROMPT_CHUNK      512

extern qm_ai_handle_t ai_main_get_handle(void);

static bool s_spiffs_ready = false;
static bool s_playing = false;

static void app_audio_spiffs_mount(void)
{
    if (s_spiffs_ready) {
        return;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs_data",
        .max_files = 8,
        .format_if_mount_failed = false,
    };

    if (esp_vfs_spiffs_register(&conf) != ESP_OK) {
        ESP_LOGW(LOG_TAG, "spiffs_data mount failed");
        return;
    }

    s_spiffs_ready = true;
}

static uint32_t app_audio_wav_data_offset(FILE *fp)
{
    uint8_t header[12];
    uint32_t offset = 0;

    if (fread(header, 1, sizeof(header), fp) != sizeof(header)) {
        return 0;
    }

    if (memcmp(header, "RIFF", 4) != 0) {
        return 0;
    }

    offset = 12;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        return 0;
    }

    while (1) {
        char chunk_id[4];
        uint8_t size_buf[4];
        uint32_t chunk_size;

        if (fread(chunk_id, 1, 4, fp) != 4) {
            break;
        }
        if (fread(size_buf, 1, 4, fp) != 4) {
            break;
        }

        chunk_size = size_buf[0] | (size_buf[1] << 8) | (size_buf[2] << 16) | (size_buf[3] << 24);
        if (memcmp(chunk_id, "data", 4) == 0) {
            return offset + 8;
        }

        offset += 8 + chunk_size + (chunk_size & 1);
        if (fseek(fp, offset, SEEK_SET) != 0) {
            break;
        }
    }

    return 44;
}

static void app_audio_play_wav_task(void *arg)
{
    const char *path = (const char *)arg;
    qm_ai_handle_t handle = ai_main_get_handle();
    FILE *fp = NULL;
    uint8_t buf[AUDIO_PROMPT_CHUNK];
    size_t read_len;

    if (handle == NULL) {
        ESP_LOGW(LOG_TAG, "audio handle not ready");
        goto exit;
    }

    app_audio_spiffs_mount();
    fp = fopen(path, "rb");
    if (fp == NULL) {
        ESP_LOGW(LOG_TAG, "open %s failed", path);
        goto exit;
    }

    if (fseek(fp, app_audio_wav_data_offset(fp), SEEK_SET) != 0) {
        ESP_LOGW(LOG_TAG, "wav seek failed: %s", path);
        goto exit;
    }

    while ((read_len = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (qm_ai_audio_player_data(handle, buf, read_len) != QM_EOK) {
            ESP_LOGW(LOG_TAG, "audio write failed");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

exit:
    if (fp != NULL) {
        fclose(fp);
    }
    s_playing = false;
    vTaskDelete(NULL);
}

void app_audio_prompt_init(void)
{
    app_audio_spiffs_mount();
}

void app_audio_play_ybjc_start(void)
{
    if (s_playing) {
        return;
    }

    s_playing = true;
    if (qm_task_create_psram(app_audio_play_wav_task, "ybjc_audio", 4096,
                             (void *)YBJC_START_WAV_PATH, 5, NULL) != pdPASS) {
        s_playing = false;
        ESP_LOGW(LOG_TAG, "create play task failed");
    }
}
