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
#include "esp_peripherals.h"

#include "qm.h"
#include "qm_audio_recorder.h"


#include "audio_stream.h"
#include "audio_processor.h"

static const char *TAG = "audio processor";

#define USE_ES7210_AS_RECORD_DEVICE   (0)
#define USE_ES8311_AS_RECORD_DEVICE   (1)


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

static struct recorder_pipeline_t *s_recorder_pipeline = NULL;

static int input_cb_for_afe(int16_t *buffer, int buf_sz, void *user_ctx, TickType_t ticks)
{
    return raw_stream_read(s_recorder_pipeline->raw_reader, (char *)buffer, buf_sz);
}

qm_audio_rec_handle_t qm_audio_recorder_open(qm_audio_rec_cfg_t *rec_cfg)
{
    ESP_LOGI(TAG, "%s", __func__);
    struct recorder_pipeline_t *pipeline = audio_calloc(1, sizeof(struct recorder_pipeline_t));
    pipeline->record_state = QM_AUDIO_RECORD_STATE_IDLE;
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

    recorder_encoder_cfg_t recorder_encoder_cfg = { 0 };
    recorder_sr_cfg_t recorder_sr_cfg = get_default_audio_record_config();

    recorder_sr_cfg.table_num = rec_cfg->table_num;
    recorder_sr_cfg.cmd_table = (recorder_sr_cmd_table_t *)rec_cfg->rec_table;

    rsp_filter_cfg_t filter_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    filter_cfg.src_ch = 1;
    filter_cfg.src_rate = 16000;
    filter_cfg.dest_ch = 1;
    filter_cfg.dest_rate = 8000;
    filter_cfg.complexity = 0;
    filter_cfg.stack_in_ext = true;
    filter_cfg.max_indata_bytes = 1024;
    recorder_encoder_cfg.resample = rsp_filter_init(&filter_cfg);
    recorder_encoder_cfg.encoder = create_record_encoder_stream();

    audio_rec_cfg_t cfg = AUDIO_RECORDER_DEFAULT_CFG();
    
    
    cfg.sr_handle = recorder_sr_create(&recorder_sr_cfg, &cfg.sr_iface);

    cfg.read = (recorder_data_read_t)&input_cb_for_afe;
    cfg.event_cb = (rec_event_cb_t)rec_cfg->cb;
    cfg.user_data = rec_cfg->userdata;
    cfg.vad_start = 100;
    cfg.vad_off = 100;
    cfg.wakeup_end = 120000;
    cfg.pinned_core = 0;
    cfg.task_size = 8 * 1024;
    cfg.encoder_handle = recorder_encoder_create(&recorder_encoder_cfg, &cfg.encoder_iface);


    pipeline->recorder_engine = audio_recorder_create(&cfg);

    return (qm_audio_rec_handle_t)pipeline;
}

qm_err_t qm_audio_recorder_run(qm_audio_rec_handle_t handle)
{
    struct recorder_pipeline_t *pipeline = (struct recorder_pipeline_t *)handle;
    ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
    audio_pipeline_run(pipeline->audio_pipeline);
    pipeline->record_state = QM_AUDIO_RECORD_STATE_RUNNING;
    return ESP_OK;
};

qm_err_t qm_audio_recorder_stop(qm_audio_rec_handle_t handle)
{
    struct recorder_pipeline_t *pipeline = (struct recorder_pipeline_t *)handle;
    ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
    audio_pipeline_stop(pipeline->audio_pipeline);
    audio_pipeline_wait_for_stop(pipeline->audio_pipeline);
    pipeline->record_state = QM_AUDIO_RECORD_STATE_IDLE;
    return ESP_OK;
};

qm_err_t qm_audio_recorder_close(qm_audio_rec_handle_t handle)
{
    struct recorder_pipeline_t *pipeline = (struct recorder_pipeline_t *)handle;
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

int qm_audio_recorder_get_default_read_size(qm_audio_rec_handle_t handle)
{
    return 160; // defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
};

int qm_audio_recorder_read(qm_audio_rec_handle_t handle, char *buffer, int buf_size)
{
    struct recorder_pipeline_t *pipeline = (struct recorder_pipeline_t *)handle;
    ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
    
    return audio_recorder_data_read(pipeline->recorder_engine, buffer, buf_size, portMAX_DELAY);;
}

qm_err_t qm_audio_recorder_clean(qm_audio_rec_handle_t handle)
{
    struct recorder_pipeline_t *pipeline = (struct recorder_pipeline_t *)handle;
    ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
    return audio_recorder_multinet_clean(pipeline->recorder_engine);
}

#if 0
audio_element_handle_t recorder_pipeline_get_raw_reader(recorder_pipeline_handle_t pipeline)
{
    return pipeline->raw_reader;
}

audio_pipeline_handle_t recorder_pipeline_get_pipeline(recorder_pipeline_handle_t pipeline)
{
    return pipeline->audio_pipeline;
};
#endif

