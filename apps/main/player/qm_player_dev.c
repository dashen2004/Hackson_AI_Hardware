#include "qm.h"

#include "audio_player.h"

#include "qm_player.h"
#include "qm_player_dev.h"

#include "bsp_audio.h"
#include "qm_audio_stream.h"

#define TAG  "player_hal"


#include "es8311.h"

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    bool_t mute_flag = bsp_audio_get_voice_mute();
    if(mute_flag){
        return QM_EOK;
    }

    if(setting == AUDIO_PLAYER_MUTE){
        es8311_set_voice_mute(true);
    }else{
        es8311_set_voice_mute(false);
    }

    return ESP_OK;
}

static esp_err_t audio_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    // ret = bsp_codec_set_fs(rate, bits_cfg, ch);

    // bsp_codec_mute_set(QM_TRUE);
    // bsp_codec_mute_set(QM_FALSE);
    // bsp_codec_volume_set(100, NULL);
    // vTaskDelay(pdMS_TO_TICKS(50));

    return ESP_OK;
}

static void audio_player_cb(audio_player_cb_ctx_t *ctx)
{
   qm_player_dev_t *player = (qm_player_dev_t *)ctx->user_ctx;

    switch (ctx->audio_event) {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
        player->event_handler(player->handle, QM_PLAYER_EVENT_IDLE, player->arg);
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
        player->event_handler(player->handle, QM_PLAYER_EVENT_COMPLETED_PLAYING_NEXT, player->arg);
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        player->event_handler(player->handle, QM_PLAYER_EVENT_PLAYING, player->arg);
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
        player->event_handler(player->handle, QM_PLAYER_EVENT_PAUSE, player->arg);
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN:
        player->event_handler(player->handle, QM_PLAYER_EVENT_SHUTDOWN, player->arg);
        break;
    default:
        break;
    }
}

static qm_err_t audio_player_dev_play_mute(qm_player_dev_t *player)
{

    return QM_EOK;
}

static qm_err_t audio_player_dev_play_unmute(qm_player_dev_t *player)
{

    vTaskDelay(pdMS_TO_TICKS(50));
    return QM_EOK;
}

extern int player_pipeline_write(char *buffer, int buf_size);

static esp_err_t bsp_i2s_write(void * audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    *bytes_written = len;
    return player_pipeline_write(audio_buffer, len);
}

static qm_err_t audio_player_dev_init(qm_player_dev_t *player)
{
    audio_player_config_t config = { .mute_fn = audio_mute_function,
                                     .write_fn = bsp_i2s_write,
                                     .clk_set_fn = audio_codec_set_fs,
                                     .priority = 12,
                                     .coreID = 1,
                                   };

    ESP_ERROR_CHECK(audio_player_new(config));
    audio_player_callback_register(audio_player_cb, (void *)player);


    vTaskDelay(pdMS_TO_TICKS(50));

    return QM_EOK;
}

static qm_err_t audio_player_dev_stop(qm_player_dev_t *player)
{
    return audio_player_stop();
}

static qm_err_t audio_player_dev_pause(qm_player_dev_t *player)
{
    return audio_player_pause();
}

static qm_err_t audio_player_dev_resume(qm_player_dev_t *player)
{
    return audio_player_resume();
}
 
static qm_err_t audio_player_dev_play_file(qm_player_dev_t *player, char *file_path)
{
    FILE *fp = NULL;
    esp_err_t status = ESP_FAIL;
     
    fp = fopen(file_path, "r");
    if (fp) {
        status = audio_player_play(fp);
    }
    
    if (status != ESP_OK) {
        QM_LOGE(TAG, "Error creating  request[%s]: %s\n", file_path, esp_err_to_name(status));
        return -QM_ERROR;
    }
    
    return  QM_EOK;
}

static qm_err_t audio_player_dev_play_bin(qm_player_dev_t *player, uint8_t *audio_buffer, int buffer_len)
{
    FILE *fp = NULL;
    esp_err_t status = ESP_FAIL;

    fp = fmemopen((void *)(audio_buffer), buffer_len, "rb");
    if (fp == NULL) {
        QM_LOGE(TAG, "Error fmemopen  request\n");
        return -QM_EIO;
    }

    status = audio_player_play(fp);
    if (status != ESP_OK) {
        QM_LOGE(TAG, "Error creating  request: %s\n", esp_err_to_name(status));
        return -QM_ERROR;
    }

    return  QM_EOK;
}

static qm_player_dev_ops_t player_ops =
{
    .init            = audio_player_dev_init,
    .stop            = audio_player_dev_stop,
    .pause           = audio_player_dev_pause,
    .resume          = audio_player_dev_resume,
    .play_bin        = audio_player_dev_play_bin,
    .play_file       = audio_player_dev_play_file,
    .unmute          = audio_player_dev_play_unmute,
    .mute            = audio_player_dev_play_mute,
#if 0
    .deinit          = ,
#endif
};

int qm_hal_audio_player_init(qm_player_t handle)
{
    return qm_player_dev_regsiter(handle, &player_ops, NULL);
}