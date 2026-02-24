#ifndef _QM_AUDIO_RECORDER_H_
#define _QM_AUDIO_RECORDER_H_

#include "qm.h"
#include "qm_audio_sr.h"
#include "qm_audio_vc.h"
#include "qm_audio_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_REC_DEF_TASK_SZ   (3 * 1024) /*!< Stack size of recorder task */
#define AUDIO_REC_DEF_TASK_PRIO (10)       /*!< Priority of recoder task */
#define AUDIO_REC_DEF_TASK_CORE (1)        /*!< Pinned to core */

#define AUDIO_REC_FEED_TASK_SZ   (10 * 1024) /*!< Stack size of recorder task */
#define AUDIO_REC_FEED_TASK_PRIO (5)       /*!< Priority of recoder task */
#define AUDIO_REC_FEED_TASK_CORE (0)        /*!< Pinned to core */


#define AUDIO_REC_DEF_WAKEUP_TM       (10000) /*!< Default wake up time (ms) */
#define AUDIO_REC_DEF_WAKEEND_TM      (900)   /*!< Duration after vad off (ms) */

/**
 * @brief Recorder event
 */
typedef struct {

    /**
    * @brief Audio recorder event type
    */
    enum {
        AUDIO_REC_WAKEUP_START = -100, /*!< Wakeup start */
        AUDIO_REC_WAKEUP_END,          /*!< Wakeup stop */
        AUDIO_REC_VAD_START,           /*!< Vad start */
        AUDIO_REC_VAD_END,             /*!< Vad stop */
        AUDIO_REC_COMMAND_DECT = 0     /*!< Form 0 is the id of the voice commands detected by Multinet*/
        /* DO NOT add items below this line */
    } type;                            /*!< Event type */
    void *event_data;                  /*!< Event data:
                                            For `AUDIO_REC_WAKEUP_START`, event data is `recorder_sr_wakeup_result_t`
                                            For `AUDIO_REC_COMMAND_DECT` or higher, event data is `recorder_sr_mn_result_t`
                                            For other events, event data is NULL
                                            */
    size_t data_len;                   /*!< Length of event data */
} audio_rec_evt_t;

/**
 * @brief Event Notification
 */
typedef qm_err_t (*rec_event_cb_t)(audio_rec_evt_t *event, void *user_data);

/**
 * @brief Audio recorder configuration
 */
typedef struct {
    int                       pinned_core;     /*!< Audio recorder task pinned to core */
    int                       task_prio;       /*!< Audio recorder task priority */
    int                       task_size;       /*!< Audio recorder task stack size */
    rec_event_cb_t            event_cb;        /*!< Event callback function, event type as audio_rec_evt_t shown above*/
    void                      *user_data;      /*!< Pointer to user data (optional) */
    qm_audio_data_read_t      read;            /*!< Data callback function used feed data to audio recorder */
    void                      *vc_handle;      /*!< SR handle */
    qm_audio_vc_iface_t       *vc_iface;       /*!< SR interface */
    void                      *sr_handle;      /*!< SR handle */
    qm_audio_sr_iface_t       *sr_iface;       /*!< SR interface */
    int                       wakeup_time;     /*!< Unit:ms. The duration that the wakeup state remains when VAD is not triggered */
    void                      *encoder_handle; /*!< Encoder handle */
    qm_audio_encoder_iface_t  *encoder_iface;  /*!< Encoder interface */
} audio_rec_cfg_t;

/**
 * @brief Audio recorder handle
 */
typedef struct __audio_recorder *audio_rec_handle_t;

#define AUDIO_RECORDER_DEFAULT_CFG()                     \
    {                                                    \
        .pinned_core    = AUDIO_REC_DEF_TASK_CORE,       \
        .task_prio      = AUDIO_REC_DEF_TASK_PRIO,       \
        .task_size      = AUDIO_REC_DEF_TASK_SZ,         \
        .event_cb       = NULL,                          \
        .user_data      = NULL,                          \
        .read           = NULL,                          \
        .sr_handle      = NULL,                          \
        .sr_iface       = NULL,                          \
        .wakeup_time    = AUDIO_REC_DEF_WAKEUP_TM,       \
        .encoder_handle = NULL,                          \
        .encoder_iface  = NULL,                          \
    }

/**
 * @brief Initialize and start up audio recorder
 *
 * @param cfg Configuration of audio recorder
 *
 * @return NULL    failed
 *         Others  audio recorder handle
 */
audio_rec_handle_t qm_audio_recorder_create(audio_rec_cfg_t *cfg);


/**
 * @brief 
 *
 * @param handle Audio recorder handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_recorder_waketimer_reset(audio_rec_handle_t handle);

/**
 * @brief 
 *
 * @param handle Audio recorder handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_recorder_enable_sr_mode(audio_rec_handle_t handle, uint8_t enable);

/**
 * @brief 
 *
 * @param handle Audio recorder handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_recorder_enable_vc_mode(audio_rec_handle_t handle, uint8_t enable);

/**
 * @brief Read data from audio recorder
 *
 * @param handle  Audio recorder handle
 * @param buffer  Buffer to save data
 * @param length  Size of buffer
 * @param ticks   Timeout for reading
 *
 * @return Length of data actually read
 *         ESP_ERR_INVALID_ARG
 */
int qm_audio_recorder_data_read(audio_rec_handle_t handle, void *buffer, int length, TickType_t ticks);


/**
 * @brief 
 *
 * @param handle Audio recorder handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_recorder_data_reset(audio_rec_handle_t handle);

/**
 * @brief Destroy audio recorder and recycle all resource
 *
 * @param handle Audio recorder handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_recorder_destroy(audio_rec_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
