/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2023 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#ifndef _QM_AUDIO_BOARD_H_
#define _QM_AUDIO_BOARD_H_

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio board handle
 */
struct qm_audio_board_handle {
    void *audio_hal; /*!< audio hardware abstract layer handle */
    void *adc_hal;   /*!< adc hardware abstract layer handle */
};

typedef struct qm_audio_board_handle *qm_audio_board_handle_t;

/**
 * @brief Initialize audio board
 *
 * @return The audio board handle
 */
qm_audio_board_handle_t qm_audio_board_init(void);

/**
 * @brief Query audio_board_handle
 *
 * @return The audio board handle
 */
qm_audio_board_handle_t qm_audio_board_get_handle(void);

/**
 * @brief Uninitialize the audio board
 *
 * @param audio_board The handle of audio board
 *
 * @return  0       success,
 *          others  fail
 */
qm_err_t qm_audio_board_deinit(qm_audio_board_handle_t audio_board);

#ifdef __cplusplus
}
#endif

#endif
