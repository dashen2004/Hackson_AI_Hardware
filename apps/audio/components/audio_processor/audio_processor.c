/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
 
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_check.h"
#include "sdkconfig.h"

#include "audio_recorder.h"
#include "recorder_sr.h"
#include "recorder_encoder.h"
#include "audio_element.h"
#include "raw_stream.h"
#include "filter_resample.h"
#include "audio_mem.h"
#include "audio_thread.h"
#include "board.h"
#include "es7210.h"
#include "esp_peripherals.h"

// #define ENABLE_AEC_DEBUG 1
// #define ENBALE_AUDIO_STREAM_DUAL_MIC 1
#define ENABLE_AUDIO_STREAM_SINGLE_MIC 1

#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
#include "raw_opus_encoder.h"
#include "raw_opus_decoder.h"
#elif defined (CONFIG_AUDIO_SUPPORT_AAC_DECODER)
#include "aac_encoder.h"
#include "aac_decoder.h"
#elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
#include "g711_encoder.h"
#include "g711_decoder.h"
#endif
#include "audio_stream.h"
#include "audio_processor.h"

static const char *TAG = "audio processor";
#if (defined CONFIG_ESP32_S3_KORVO2_V3_BOARD || defined CONFIG_ESP32_S3_BOX_3_BOARD)
#define USE_ES7210_AS_RECORD_DEVICE   (1)
#define USE_ES8311_AS_RECORD_DEVICE   (0)
#elif defined CONFIG_ESP32_S3_KORVO2L_V1_BOARD
#define USE_ES7210_AS_RECORD_DEVICE   (0)
#define USE_ES8311_AS_RECORD_DEVICE   (1)
#else
#warning "Please according your board to config"
#endif

// aec debug
// #define ENABLE_AEC_DEBUG
#if defined (ENABLE_AEC_DEBUG)
#include <stdio.h>
#include <errno.h>
#include "esp_timer.h"

static bool    record_flag = true;
static FILE   *sfp         = NULL;
static int64_t start_tm    = 0;
#define AEC_DEBUDG_FILE_NAME "/sdcard/rec.pcm"
#define AEC_RECORD_TIME       (20)  // s
#endif  // ENABLE_AEC_DEBUG

#define audio_pipe_safe_free(x, fn) do { \
    if (x) {                             \
        fn(x);                           \
        x = NULL;                        \
    }                                    \
} while (0)

struct recorder_pipeline_t {
    audio_pipeline_handle_t audio_pipeline;
    audio_element_handle_t  i2s_stream_reader;
    audio_element_handle_t  audio_encoder;
    audio_element_handle_t  raw_reader;
    audio_rec_handle_t      recorder_engine;
    pipe_player_state_e     record_state;
};

struct player_pipeline_t {
    audio_pipeline_handle_t audio_pipeline;
    audio_element_handle_t  raw_writer;
    audio_element_handle_t  audio_decoder;
    audio_element_handle_t  i2s_stream_writer;
    audio_element_handle_t  player_rsp;
    pipe_player_state_e     player_state;
};

typedef struct {
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t  spiffs_stream;
    audio_element_handle_t  audio_decoder; // only support for wav; 16k, 16bit, double channel
    audio_element_handle_t  i2s_stream_writer;
    pipe_player_state_e     player_state;
    bool                    running;
} audio_player_t;

static audio_player_t             *s_audio_player      = NULL;
static struct recorder_pipeline_t *s_recorder_pipeline = NULL;
static struct player_pipeline_t   *s_player_pipeline   = NULL;

#if defined (ENABLE_AEC_DEBUG)
#include "qm_utils_timer.h"

#include "audio_mem.h"
#include "esp_alc.h"
#include "i2s_stream.h"

typedef struct {
    // The "RIFF" chunk descriptor
    uint8_t ChunkID[4];// Indicates the file as "RIFF" file
    int32_t ChunkSize;// The total size of the entire file, excluding the "RIFF" and the header itself, which is the file size minus 8 bytes.
    uint8_t Format[4];// File format header indicating a "WAVE" file.
    // The "fmt" sub-chunk
    uint8_t Subchunk1ID[4];// Format identifier for the "fmt" sub-chunk.
    int32_t Subchunk1Size;// The length of the fmt sub-chunk (subchunk1) excluding the Subchunk1 ID and Subchunk1 Size fields. It is typically 16, but a value greater than 16 indicates the presence of an extended area. Optional values for the length include 16, 18, 20, 40, etc.
    int16_t AudioFormat;// Audio encoding format, which represents the compression format. A value of 0x01 indicates PCM format, which is uncompressed. Please refer to table 3 for more details.
    int16_t NumChannels;// Number of audio channels
    int32_t SampleRate;// Sample rate, for example, "44100" represents a sampling rate of 44100 Hz.
    int32_t ByteRate;// Bit rate: Sample rate x bit depth x number of channels / 8. For example, the bit rate for a stereo (2 channels) audio with a sample rate of 44.1 kHz and 16-bit depth would be 176400 bits per second.
    int16_t BlockAlign;// Memory size occupied by one sample: Bit depth x number of channels / 8.
    int16_t BitsPerSample;//Sample depth, also known as bit depth.
    // The "data" sub-chunk
    uint8_t Subchunk2ID[4];// Total length of the audio data, which is the file size minus the length of the WAV file header.
    int32_t Subchunk2Size;// Length of the data section, referring to the size of the audio data excluding the header.
} qm_audio_wav_header_t;

static uint32_t play_size = 0;
static uint8_t *play_buff = NULL;
// static uint8_t play_buff[1 * 1024 * 1024] = {0};

typedef struct i2s_stream {
    audio_stream_type_t type;
    i2s_stream_cfg_t    config;
    bool                is_open;
    bool                use_alc;
    void               *volume_handle;
    int                 volume;
    bool                uninstall_drv;
    i2s_port_t          port;
    int                 buffer_length;
    struct {
        char           *buf;
        int             buffer_size;
    } expand;
} i2s_stream_t;

struct i2s_key_slot_s {
    i2s_chan_handle_t       rx_handle;
    i2s_chan_handle_t       tx_handle;
    union {
        i2s_std_config_t    rx_std_cfg;
#if SOC_I2S_SUPPORTS_PDM_RX
        i2s_pdm_rx_config_t rx_pdm_cfg;
#endif // SOC_I2S_SUPPORTS_PDM_RX
#if SOC_I2S_SUPPORTS_TDM
        i2s_tdm_config_t    rx_tdm_cfg;
#endif // SOC_I2S_SUPPORTS_TDM
    };
    union {
        i2s_std_config_t    tx_std_cfg;
#if SOC_I2S_SUPPORTS_PDM_TX
        i2s_pdm_tx_config_t tx_pdm_cfg;
#endif // SOC_I2S_SUPPORTS_PDM_TX
#if SOC_I2S_SUPPORTS_TDM
        i2s_tdm_config_t    tx_tdm_cfg;
#endif // SOC_I2S_SUPPORTS_TDM
    };
    i2s_chan_config_t       chan_cfg;
    i2s_dir_t               dir;
    int                     i2s_refcount;
};

extern struct i2s_key_slot_s i2s_key_slot[SOC_I2S_NUM];
extern void audio_pull_data_process(char *ptr, int len);
void aec_debug_data_write(char *data, int len)
{
    static qm_utils_time_t debug_timer = {0};

    if(play_buff == NULL){
        play_buff = (uint8_t *)qm_malloc(300 * 1024);
    }

    memcpy(play_buff + play_size, data, len);
    play_size += len;

    if(!qm_utils_time_left(&debug_timer)){
#if 0
        qm_audio_wav_header_t *wav_head = (qm_audio_wav_header_t *)play_buff;
        wav_head->SampleRate = 16000;
        wav_head->NumChannels = 1;
        wav_head->BitsPerSample = 16;
        wav_head->ChunkSize = play_size + sizeof(qm_audio_wav_header_t) - 8;
        wav_head->ByteRate = wav_head->SampleRate * wav_head->BitsPerSample * wav_head->NumChannels / 8;
        wav_head->Subchunk2ID[0] = 'd';
        wav_head->Subchunk2ID[1] = 'a';
        wav_head->Subchunk2ID[2] = 't';
        wav_head->Subchunk2ID[3] = 'a';
        wav_head->Subchunk2Size = play_size;
        wav_head->ChunkID[0] = 'R';
        wav_head->ChunkID[1] = 'I';
        wav_head->ChunkID[2] = 'F';
        wav_head->ChunkID[3] = 'F';
        wav_head->Format[0] = 'W';
        wav_head->Format[1] = 'A';
        wav_head->Format[2] = 'V';
        wav_head->Format[3] = 'E';
        wav_head->Subchunk1ID[0] = 'f';
        wav_head->Subchunk1ID[1] = 'm';
        wav_head->Subchunk1ID[2] = 't';
        wav_head->Subchunk1ID[3] = ' ';
#endif
        QM_LOGD("1", "RECORD[size]:%d", play_size);
        // size_t bytes_written = 0;
        // i2s_channel_write(i2s_key_slot[0].tx_handle, play_buff, play_size, &bytes_written, portMAX_DELAY);
        audio_pull_data_process((char *)play_buff, play_size);

        play_size = 0;
        qm_utils_time_countdown_ms(&debug_timer, 3000);
    }

#if 0
    if (record_flag) {
        if (sfp == NULL) {
            sfp = fopen(AEC_DEBUDG_FILE_NAME, "wb+");
            if (sfp == NULL) {
                ESP_LOGI(TAG, "Cannot open file, reason: %s\n", strerror(errno));
                return;
            }
        }
        fwrite(data, 1, len, sfp);

        if ((esp_timer_get_time() - start_tm) / 1000000 > AEC_RECORD_TIME) {
            record_flag = false;
            fclose(sfp);
            sfp = NULL;
            ESP_LOGI(TAG, "AEC debug data write done");
        }
    }
#endif
}
#endif // ENABLE_AEC_DEBUG

recorder_pipeline_handle_t recorder_pipeline_open(void)
{
    ESP_LOGI(TAG, "%s", __func__);
    recorder_pipeline_handle_t pipeline = audio_calloc(1, sizeof(struct recorder_pipeline_t));
    pipeline->record_state = PIPE_STATE_IDLE;
    s_recorder_pipeline = pipeline;
    
    ESP_LOGI(TAG, "Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline->audio_pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline->audio_pipeline);

    ESP_LOGI(TAG, "Create player audio stream");
    pipeline->i2s_stream_reader = create_record_i2s_stream();
    pipeline->raw_reader = create_record_raw_stream();

    ESP_LOGI(TAG, " Register all player elements to audio pipeline");
    audio_pipeline_register(pipeline->audio_pipeline, pipeline->i2s_stream_reader, "record_i2s");
    audio_pipeline_register(pipeline->audio_pipeline, pipeline->raw_reader, "record_raw");

    ESP_LOGI(TAG, " Link all player elements to audio pipeline");
    const char *link_tag[2] = {"record_i2s", "record_raw"};
    audio_pipeline_link(pipeline->audio_pipeline, &link_tag[0], 2);
    return pipeline;
}

esp_err_t recorder_pipeline_run(recorder_pipeline_handle_t pipeline)
{
    ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
    audio_pipeline_run(pipeline->audio_pipeline);
    pipeline->record_state = PIPE_STATE_RUNNING;
    return ESP_OK;
};

esp_err_t recorder_pipeline_stop(recorder_pipeline_handle_t pipeline)
{
    ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
    audio_pipeline_stop(pipeline->audio_pipeline);
    audio_pipeline_wait_for_stop(pipeline->audio_pipeline);
    pipeline->record_state = PIPE_STATE_IDLE;
    return ESP_OK;
};

esp_err_t recorder_pipeline_close(recorder_pipeline_handle_t pipeline)
{
    ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
    audio_pipeline_terminate(pipeline->audio_pipeline);
    audio_pipeline_unregister(pipeline->audio_pipeline, pipeline->i2s_stream_reader);
    audio_pipeline_unregister(pipeline->audio_pipeline, pipeline->raw_reader);

    /* Release all resources */
    audio_pipe_safe_free(pipeline->audio_pipeline, audio_pipeline_deinit);
    audio_pipe_safe_free(pipeline->raw_reader, audio_element_deinit);
    audio_pipe_safe_free(pipeline->i2s_stream_reader, audio_element_deinit);
    audio_pipe_safe_free(pipeline, audio_free);
    return ESP_OK;
};

int recorder_pipeline_get_default_read_size(recorder_pipeline_handle_t pipeline)
{
    #if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
        return 160;
    #elif defined (CONFIG_AUDIO_SUPPORT_AAC_DECODER)
        return 512;
    #elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
        return 160;
    #endif
    return -1;
};

audio_element_handle_t recorder_pipeline_get_raw_reader(recorder_pipeline_handle_t pipeline)
{
    return pipeline->raw_reader;
}

audio_pipeline_handle_t recorder_pipeline_get_pipeline(recorder_pipeline_handle_t pipeline)
{
    return pipeline->audio_pipeline;
};

int recorder_pipeline_read(recorder_pipeline_handle_t pipeline,char *buffer, int buf_size)
{
    return raw_stream_read(pipeline->raw_reader, buffer,buf_size);
}

static esp_err_t _player_i2s_write_cb(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{    
    return audio_element_output(s_player_pipeline->i2s_stream_writer, buffer, len);
}

static esp_err_t _player_write_nop_cb(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    return len;
}

player_pipeline_handle_t player_pipeline_open(void) 
{
    ESP_LOGI(TAG, "%s", __func__);
    player_pipeline_handle_t player_pipeline = audio_calloc(1, sizeof(struct player_pipeline_t));
    AUDIO_MEM_CHECK(TAG, player_pipeline, goto _exit_open);
    player_pipeline->player_state = PIPE_STATE_IDLE;
    s_player_pipeline = player_pipeline;

    ESP_LOGI(TAG, "Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    player_pipeline->audio_pipeline = audio_pipeline_init(&pipeline_cfg);
    AUDIO_MEM_CHECK(TAG, player_pipeline, goto _exit_open);

    ESP_LOGI(TAG, "Create playback audio stream");
    player_pipeline->raw_writer = create_player_raw_stream();
    AUDIO_MEM_CHECK(TAG, player_pipeline->raw_writer, goto _exit_open);
    player_pipeline->audio_decoder = create_player_decoder_stream();
    AUDIO_MEM_CHECK(TAG, player_pipeline->audio_decoder, goto _exit_open);
    player_pipeline->i2s_stream_writer = create_player_i2s_stream(false);
    AUDIO_MEM_CHECK(TAG, player_pipeline->i2s_stream_writer, goto _exit_open);

    ESP_LOGI(TAG, "Register all elements to playback pipeline");
    audio_pipeline_register(player_pipeline->audio_pipeline, player_pipeline->raw_writer, "player_raw");
    audio_pipeline_register(player_pipeline->audio_pipeline, player_pipeline->audio_decoder, "player_dec");

#if (defined ENBALE_AUDIO_STREAM_DUAL_MIC)
    ESP_LOGI(TAG, "ENBALE_AUDIO_STREAM_DUAL_MIC");
#if CONFIG_AUDIO_SUPPORT_G711A_DECODER
    player_pipeline->player_rsp = create_8k_ch1_to_16k_ch2_rsp_stream();
#else
    player_pipeline->player_rsp = create_ch1_to_ch2_rsp_stream();   
#endif
#elif (defined ENABLE_AUDIO_STREAM_SINGLE_MIC)
    ESP_LOGI(TAG, "ENABLE_AUDIO_STREAM_SINGLE_MIC");
#if CONFIG_AUDIO_SUPPORT_G711A_DECODER
    player_pipeline->player_rsp = create_8k_ch1_to_16k_ch2_rsp_stream();
#else
    player_pipeline->player_rsp = create_ch1_to_ch2_rsp_stream();   
#endif 
#else
    ESP_LOGE(TAG, "Invalid record device"); 
#endif // USE_ES8311_AS_RECORD_DEVICE
    AUDIO_MEM_CHECK(TAG, player_pipeline->player_rsp, goto _exit_open);

    ESP_LOGI(TAG, "Link playback element together raw-->audio_decoder-->rsp-->i2s_stream-->[codec_chip]");

    audio_element_set_write_cb(player_pipeline->player_rsp, _player_write_nop_cb, NULL);
    audio_pipeline_register(player_pipeline->audio_pipeline, player_pipeline->player_rsp, "player_rsp");
    const char *link_tag[3] = {"player_raw", "player_dec", "player_rsp"};
    audio_pipeline_link(player_pipeline->audio_pipeline, &link_tag[0], 3);

    audio_pipeline_run(player_pipeline->audio_pipeline);
    return player_pipeline;

_exit_open:
    audio_pipe_safe_free(player_pipeline->player_rsp, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->raw_writer, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->audio_decoder, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->i2s_stream_writer, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->audio_pipeline, audio_pipeline_deinit);
    audio_pipe_safe_free(player_pipeline, audio_free);
    return NULL;
}

esp_err_t player_pipeline_run(player_pipeline_handle_t player_pipeline)
{
    ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
    if (player_pipeline->player_state == PIPE_STATE_RUNNING) {
        ESP_LOGW(TAG, "player pipe is already running state");
        return ESP_OK;
    }
    ESP_LOGI(TAG, "player pipe start running");
    audio_element_set_write_cb(player_pipeline->player_rsp, _player_i2s_write_cb, NULL);
    player_pipeline->player_state = PIPE_STATE_RUNNING;
    return ESP_OK;
};

esp_err_t player_pipeline_stop(player_pipeline_handle_t player_pipeline)
{
    ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
    if (player_pipeline->player_state == PIPE_STATE_IDLE) {
        ESP_LOGW(TAG, "player pipe is idle state");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "player pipe stop running");
    audio_element_set_write_cb(player_pipeline->player_rsp, _player_write_nop_cb, NULL);
    player_pipeline->player_state = PIPE_STATE_IDLE;
    return ESP_OK;
}

esp_err_t player_pipeline_get_state(player_pipeline_handle_t player_pipeline, pipe_player_state_e *state)
{
    ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
    *state = player_pipeline->player_state;
    return ESP_OK;
}

esp_err_t player_pipeline_close(player_pipeline_handle_t player_pipeline)
{
    ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
    if (player_pipeline->player_state == PIPE_STATE_RUNNING) {
        ESP_LOGW(TAG, "Please stop player pipe first");
        return ESP_FAIL;
    }
    audio_pipeline_terminate(player_pipeline->audio_pipeline);

    audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->raw_writer);
    audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->i2s_stream_writer);
    audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->audio_decoder);
    audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->player_rsp);

    /* Release all resources */
    audio_pipe_safe_free(player_pipeline->player_rsp, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->raw_writer, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->i2s_stream_writer, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->audio_decoder, audio_element_deinit);
    audio_pipe_safe_free(player_pipeline->audio_pipeline, audio_pipeline_deinit);
    audio_pipe_safe_free(player_pipeline, audio_free);
    return ESP_OK;
};

// Just create a placeholder function
int player_pipeline_get_default_read_size(player_pipeline_handle_t player_pipeline)
{
    return 0;
};

audio_element_handle_t player_pipeline_get_raw_write(player_pipeline_handle_t player_pipeline)
{
    return player_pipeline->raw_writer; 
}

static int input_cb_for_afe(int16_t *buffer, int buf_sz, void *user_ctx, TickType_t ticks)
{
#if defined (ENABLE_AEC_DEBUG)
    aec_debug_data_write((char *)buffer, buf_sz);
#endif // ENABLE_AEC_DEBUG
    return raw_stream_read(s_recorder_pipeline->raw_reader, (char *)buffer, buf_sz);
}

void * audio_record_engine_init(recorder_pipeline_handle_t pipeline, rec_event_cb_t cb)
{
    recorder_encoder_cfg_t recorder_encoder_cfg = { 0 };
    recorder_sr_cfg_t recorder_sr_cfg = get_default_audio_record_config();
    
#if defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
    rsp_filter_cfg_t filter_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    filter_cfg.src_ch = 1;
    filter_cfg.src_rate = 16000;
    filter_cfg.dest_ch = 1;
    filter_cfg.dest_rate = 8000;
    filter_cfg.complexity = 0;
    filter_cfg.stack_in_ext = true;
    filter_cfg.max_indata_bytes = 1024;
    recorder_encoder_cfg.resample = rsp_filter_init(&filter_cfg);
#endif // CONFIG_AUDIO_SUPPORT_G711A_DECODER

    recorder_encoder_cfg.encoder = create_record_encoder_stream();

    audio_rec_cfg_t cfg = AUDIO_RECORDER_DEFAULT_CFG();
    cfg.read = (recorder_data_read_t)&input_cb_for_afe;
    cfg.sr_handle = recorder_sr_create(&recorder_sr_cfg, &cfg.sr_iface);
    cfg.event_cb = cb;
    cfg.vad_start = 300;
    cfg.vad_off = 1000;
    cfg.wakeup_end = 120000;
    cfg.pinned_core = 0;
    cfg.task_size = 8 * 1024;
    cfg.encoder_handle = recorder_encoder_create(&recorder_encoder_cfg, &cfg.encoder_iface);
    pipeline->recorder_engine = audio_recorder_create(&cfg);
    return pipeline->recorder_engine;
}

static void audio_player_state_task(void *arg)
{
    audio_event_iface_handle_t evt = (audio_event_iface_handle_t) arg;

    s_audio_player->running = true;
    while (s_audio_player->running) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) s_audio_player->audio_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(s_audio_player->audio_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from wav decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }
        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) s_audio_player->i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
            ESP_LOGI(TAG, "[ * ] Stop event received");
            audio_tone_stop();
            s_audio_player->player_state = PIPE_STATE_IDLE;
        }
    }
}

esp_err_t audio_tone_init(void)
{
    s_audio_player = (audio_player_t *)audio_calloc(1, sizeof(audio_player_t));
    AUDIO_MEM_CHECK(TAG, s_audio_player, goto _exit_open);

    ESP_LOGI(TAG, "Create audio pipeline for audio player");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    s_audio_player->pipeline = audio_pipeline_init(&pipeline_cfg);
    AUDIO_MEM_CHECK(TAG, s_audio_player->pipeline, goto _exit_open);

    ESP_LOGI(TAG, "Create audio player audio stream");
    s_audio_player->spiffs_stream = create_audio_player_spiffs_stream();
    AUDIO_MEM_CHECK(TAG, s_audio_player->spiffs_stream, goto _exit_open);
    s_audio_player->i2s_stream_writer = create_player_i2s_stream(true);
    AUDIO_MEM_CHECK(TAG, s_audio_player->i2s_stream_writer, goto _exit_open);
    s_audio_player->audio_decoder = create_player_wav_decoder_stream();
    AUDIO_MEM_CHECK(TAG, s_audio_player->audio_decoder, goto _exit_open);

    ESP_LOGI(TAG, "Register all elements to playback pipeline");
    audio_pipeline_register(s_audio_player->pipeline, s_audio_player->spiffs_stream, "audio_player_spiffs");
    audio_pipeline_register(s_audio_player->pipeline, s_audio_player->audio_decoder, "audio_player_dec");
    audio_pipeline_register(s_audio_player->pipeline, s_audio_player->i2s_stream_writer, "audio_player_i2s");

    ESP_LOGI(TAG, "Link playback element together raw-->audio_decoder-->i2s_stream-->[codec_chip]");
    const char *link_tag[3] = {"audio_player_spiffs", "audio_player_dec", "audio_player_i2s"};
    audio_pipeline_link(s_audio_player->pipeline, &link_tag[0], 3);

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    audio_pipeline_set_listener(s_audio_player->pipeline, evt);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    audio_thread_create(NULL, "audio_player_state_task", audio_player_state_task, (void *)evt, 5 * 1024, 5, true, 1);

    return ESP_OK;

_exit_open:
    audio_pipe_safe_free(s_audio_player->spiffs_stream, audio_element_deinit);
    audio_pipe_safe_free(s_audio_player->i2s_stream_writer, audio_element_deinit);
    audio_pipe_safe_free(s_audio_player->audio_decoder, audio_element_deinit);
    audio_pipe_safe_free(s_audio_player->pipeline, audio_pipeline_deinit);
    audio_pipe_safe_free(s_audio_player, audio_free);
    return ESP_FAIL;
}

esp_err_t audio_tone_play(const char *uri)
{
    ESP_RETURN_ON_FALSE(s_audio_player != NULL, ESP_FAIL, TAG, "audio tone not initialized");
    if (s_audio_player->player_state == PIPE_STATE_RUNNING) {
        return ESP_FAIL;
    }
    if (s_audio_player->player_state == PIPE_STATE_RUNNING) {
        audio_pipeline_stop(s_audio_player->pipeline);
    }
    ESP_LOGI(TAG, "audio_tone_play: %s", uri);
    
    audio_element_set_uri(s_audio_player->spiffs_stream, uri);
    audio_pipeline_run(s_audio_player->pipeline);
    s_audio_player->player_state = PIPE_STATE_RUNNING;
    return ESP_OK;
}

esp_err_t audio_tone_stop(void)
{
    ESP_RETURN_ON_FALSE(s_audio_player != NULL, ESP_FAIL, TAG, "audio tone not initialized");
    if (s_audio_player->player_state == PIPE_STATE_IDLE) {
        return ESP_FAIL;
    }
    audio_pipeline_stop(s_audio_player->pipeline);
    audio_pipeline_wait_for_stop(s_audio_player->pipeline);
    audio_pipeline_terminate(s_audio_player->pipeline);
    audio_pipeline_reset_ringbuffer(s_audio_player->pipeline);
    audio_pipeline_reset_elements(s_audio_player->pipeline);
    s_audio_player->player_state = PIPE_STATE_IDLE;
    return ESP_OK;
}
