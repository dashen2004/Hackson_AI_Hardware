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


#pragma once

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumeration of player pipeline states
 */
typedef enum {
    QM_AUDIO_PLAYER_STATE_IDLE,     /**< The pipeline is idle and not processing any audio */
    QM_AUDIO_PLAYER_STATE_RUNNING,  /**< The pipeline is actively processing and playing audio */
} qm_audio_player_state_t;

/**
 * @brief Enumeration of player pipeline states
 */
typedef enum {
    QM_AUDIO_PLAYER_SINGLE_MIC = 1,    //单麦
    QM_AUDIO_PLAYER_DUAL_MIC,          //双麦
} qm_audio_player_mic_num_t;

/**
 * @brief  A handle to the player pipeline structure
 */
typedef void* qm_audio_player_handle_t;

/**
 * @brief Opens and initializes the player pipeline
 *
 * @return A handle to the initialized player pipeline, or NULL if the operation fails
 */
qm_audio_player_handle_t qm_audio_player_open(qm_audio_player_mic_num_t mic_num);

/**
 * @brief  Starts the player pipeline
 *
 * @param[in]  handle  The handle to the player pipeline
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_player_run(qm_audio_player_handle_t handle);

/**
 * @brief Closes the player pipeline and releases resources
 *
 * @param[in]  handle  The handle to the player pipeline
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_player_close(qm_audio_player_handle_t handle);

/**
 * @brief  Stop the player pipeline
 *
 * @param[in]  handle   handle to the player pipeline instance
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_player_stop(qm_audio_player_handle_t handle);

/**
 * @brief  Get the current state of the player pipeline
 *
 * @param[in]   handle  The handle to the player pipeline instance
 * @param[out]  state            A pointer to a variable where the pipeline state will be stored
 *                               The state will be set to one of the values defined in the 
 *                               `pipe_player_state_e` enumeration
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_player_get_state(qm_audio_player_handle_t handle, qm_audio_player_state_t *state);

/**
 * @brief Writes audio data to the player pipeline
 *
 * This function sends audio data to the pipeline for playback
 *
 * @param[in]  handle  The handle to the player pipeline
 * @param[in]  buffer           The buffer containing the audio data to be written
 * @param[in]  buf_size         The size of the audio data in bytes
 *
 * @return 
 *        - >= 0  The number of bytes successfully written
 *        - < 0   If an error occurs
 */
int qm_audio_player_write(qm_audio_player_handle_t handle, char *buffer, int buf_size);

/**
 * @brief  Play audio tone from a specified URI
 *
 * @param[in]  uri  The URI of the audio source to play. This can be a file path 
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_player_tone_play(qm_audio_player_handle_t handle, const char *uri);

/**
 * @brief Stop audio tone playback
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_player_tone_stop(qm_audio_player_handle_t handle);

/**
 * @brief Gets the raw write element handle from the player pipeline
 *
 * @param[in]  handle  The handle to the player pipeline
 *
 * @return 
        - The raw write element handle
        -  NULL, if the operation fails
 */
#if 0
audio_element_handle_t player_pipeline_get_raw_write(player_pipeline_handle_t player_pipeline);


/**
 * @brief Initialize the audio tone.
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t audio_tone_init(void);

/**
 * @brief  Play audio tone from a specified URI
 *
 * @param[in]  uri  The URI of the audio source to play. This can be a file path 
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t audio_tone_play(const char *uri);

/**
 * @brief Stop audio tone playback
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t audio_tone_stop(void);
#endif
#ifdef __cplusplus
}
#endif
