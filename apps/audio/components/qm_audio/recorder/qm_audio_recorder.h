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
    QM_AUDIO_RECORD_STATE_IDLE,     /**< The pipeline is idle and not processing any audio */
    QM_AUDIO_RECORD_STATE_RUNNING,  /**< The pipeline is actively processing and playing audio */
} qm_audio_record_state_t;

/**
 * @brief Recorder event
 */
typedef struct {

    /**
    * @brief Audio recorder event type
    */
    enum {
        QM_AUDIO_REC_WAKEUP_START = -100, /*!< Wakeup start */
        QM_AUDIO_REC_WAKEUP_END,          /*!< Wakeup stop */
        QM_AUDIO_REC_VAD_START,           /*!< Vad start */
        QM_AUDIO_REC_VAD_END,             /*!< Vad stop */
        QM_AUDIO_REC_COMMAND_DECT = 0     /*!< Form 0 is the id of the voice commands detected by Multinet*/
        /* DO NOT add items below this line */
    } type;                            /*!< Event type */

    void *event_data;                  /*!< Event data:
                                            For `AUDIO_REC_WAKEUP_START`, event data is `recorder_sr_wakeup_result_t`
                                            For `AUDIO_REC_COMMAND_DECT` or higher, event data is `recorder_sr_mn_result_t`
                                            For other events, event data is NULL
                                            */
    size_t data_len;                   /*!< Length of event data */

} qm_audio_rec_evt_t;

typedef struct {
    enum {
        QM_AUDIO_REC_WAKEUP_CMD,          /*!< Wakeup start */
        QM_AUDIO_REC_COMM_CMD,            /*!< Wakeup stop */
    } type;                            /*!< sr type */
    int id;
    char *str;
} qm_audio_rec_table_t;

/**
 * @brief  A handle to the recorder pipeline structure
 */
typedef void* qm_audio_rec_handle_t;

/**
 * @brief Event Notification
 */
typedef qm_err_t (*qm_audio_rec_event_cb_t)(qm_audio_rec_evt_t *event, void *user_data);

typedef struct {

    int table_num;
    qm_audio_rec_table_t *rec_table;

    void *userdata;
    qm_audio_rec_event_cb_t cb;

} qm_audio_rec_cfg_t;

/**
 * @brief  Opens and initializes the recorder pipeline
 *
 * This function sets up the necessary resources and configurations for the recorder pipeline
 *
 * @return 
 *       - A handle to the initialized recorder pipeline
 *       - NULL if the operation fails
 */
qm_audio_rec_handle_t qm_audio_recorder_open(qm_audio_rec_cfg_t *cfg);

/**
 * @brief  Starts the recorder pipeline
 *
 * This function begins recording audio using the specified pipeline. It must be called after `qm_audio_recorder_open()
 *
 * @param[in]  qm_audio_recorder  The handle to the recorder pipeline
 * 
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_recorder_run(qm_audio_rec_handle_t handle);

/**
 * @brief  Stops the recorder pipeline.
 *
 * This function halts the audio recording and ensures that all resources used by the pipeline are properly released
 *
 * @param[in]  qm_audio_recorder  The handle to the recorder pipeline
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_recorder_stop(qm_audio_rec_handle_t handle);

/**
 * @brief Closes the recorder pipeline and releases resources.
 *
 * This function cleans up all resources associated with the recorder pipeline. It should be called after `qm_audio_recorder_stop()
 *
 * @param[in]  handle  The handle to the recorder pipeline
 *
 * @return
 *      - ESP_OK   if the operation is successful
 *      - ESP_FAIL if the operation fails
 */
qm_err_t qm_audio_recorder_close(qm_audio_rec_handle_t handle);


/**
 * @brief Clean speech command recognition state
 *
 * @param handle Audio recorder handle
 * @param enable true: enable speech command recognition
 *               false: disable speech command recognition
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_recorder_clean(qm_audio_rec_handle_t handle);

/**
 * @brief  Gets the default buffer size for reading data from the recorder pipeline
 *
 * This function returns the recommended buffer size for optimal data reading from the recorder pipeline
 *
 * @param[in]  handle  The handle to the recorder pipeline
 *
 * @return
 *       - > 0                 The specific length of data being read
 *       - <= 0                Error code
 */
int qm_audio_recorder_get_default_read_size(qm_audio_rec_handle_t handle);

/**
 * @brief  Reads audio data from the recorder pipeline
 *
 * @param[in]  qm_audio_recorder  The handle to the recorder pipeline.
 * @param[in]  buffer             The buffer to store the audio data.
 * @param[in]  buf_size           The size of the buffer in bytes.
 *
 * @return
 *       - > 0                 The specific length of data being read
 *       - <= 0                Error code
 */
int qm_audio_recorder_read(qm_audio_rec_handle_t handle, char *buffer, int buf_size);

#ifdef __cplusplus
}
#endif
