/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include "esp_log.h"
#include "audio_hal.h"
#include "audio_mem.h"
#include "qm_audio_board.h"

static const char *TAG = "AUDIO_BOARD";

extern audio_hal_func_t AUDIO_CODEC_ES8311_DEFAULT_HANDLE;
extern audio_hal_func_t AUDIO_CODEC_ES7210_DEFAULT_HANDLE;

#define AUDIO_CODEC_DEFAULT_CONFIG(){                   \
    .adc_input  = AUDIO_HAL_ADC_INPUT_LINE1,        \
    .dac_output = AUDIO_HAL_DAC_OUTPUT_ALL,         \
    .codec_mode = AUDIO_HAL_CODEC_MODE_BOTH,        \
    .i2s_iface = {                                  \
        .mode = AUDIO_HAL_MODE_SLAVE,               \
        .fmt = AUDIO_HAL_I2S_NORMAL,                \
        .samples = AUDIO_HAL_16K_SAMPLES,           \
        .bits = AUDIO_HAL_BIT_LENGTH_16BITS,        \
    },                                              \
};


static qm_audio_board_handle_t board_handle = 0;

static audio_hal_handle_t audio_board_codec_init(void)
{
    audio_hal_codec_config_t audio_codec_cfg = AUDIO_CODEC_DEFAULT_CONFIG();
    audio_hal_handle_t codec_hal = audio_hal_init(&audio_codec_cfg, &AUDIO_CODEC_ES8311_DEFAULT_HANDLE);
    AUDIO_NULL_CHECK(TAG, codec_hal, return NULL);
    return codec_hal;
}

static audio_hal_handle_t audio_board_adc_init(void)
{
    audio_hal_codec_config_t audio_codec_cfg = AUDIO_CODEC_DEFAULT_CONFIG();
    audio_codec_cfg.codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE;
    audio_hal_handle_t adc_hal = NULL;
    adc_hal = audio_hal_init(&audio_codec_cfg, &AUDIO_CODEC_ES7210_DEFAULT_HANDLE);
    AUDIO_NULL_CHECK(TAG, adc_hal, return NULL);
    return adc_hal;
}

qm_audio_board_handle_t qm_audio_board_init(void)
{
    if (board_handle) {
        ESP_LOGW(TAG, "The board has already been initialized!");
        return board_handle;
    }
    board_handle = (qm_audio_board_handle_t) audio_calloc(1, sizeof(struct qm_audio_board_handle));
    AUDIO_MEM_CHECK(TAG, board_handle, return NULL);
    board_handle->audio_hal = audio_board_codec_init();
    board_handle->adc_hal = audio_board_adc_init();
    return board_handle;
}



qm_audio_board_handle_t qm_audio_board_get_handle(void)
{
    return board_handle;
}

qm_err_t qm_audio_board_deinit(qm_audio_board_handle_t audio_board)
{
    AUDIO_NULL_CHECK(TAG, audio_board, return ESP_FAIL);
    esp_err_t ret = ESP_OK;
    ret |= audio_hal_deinit(audio_board->audio_hal);
    ret |= audio_hal_deinit(audio_board->adc_hal);
    audio_free(audio_board);
    board_handle = NULL;
    return ret;
}
