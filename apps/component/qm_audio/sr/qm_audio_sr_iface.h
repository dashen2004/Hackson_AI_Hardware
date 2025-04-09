#ifndef __QM_AUDIO_SR_IFACE_H__
#define __QM_AUDIO_SR_IFACE_H__

#include "esp_err.h"
#include "qm_audio_subproc_iface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECORDER_SR_MN_STRING_MAX_LEN (256)

/**
 * @brief Wake up result
 */
typedef struct {
    float data_volume;                /*!< Volume of input audio, the unit is decibel(dB) */
    int wake_word_index;              /*!< Wake word index which start from 1 */
    int wakenet_model_index;          /*!< Wakenets index which start from 1 */
} qm_audio_sr_wakeup_result_t;        /*!< Information when wakeup word detected */

/**
 * @brief Voice command result
 */
typedef struct {
    int phrase_id;                    /*!< Phrase ID */
    float prob;                       /*!< probability */
    char str[RECORDER_SR_MN_STRING_MAX_LEN]; /*!< Command string */
} qm_audio_sr_mn_result_t;            /*!< Information when voice command detected */

/**
 * @brief Speech recognition result
 */
typedef struct {
    enum {
        SR_RESULT_UNKNOW    = -100,  /*!< Unknow speech recognition result */
        SR_RESULT_VERIFIED  = -10,   /*!< Channel verified */
        SR_RESULT_WAKEUP    = -7,    /*!< Wake word detected */
        SR_RESULT_COMMAND   = 0      /*!< From 0 is used as command id detected by multinet */
    } type;
    union {
        qm_audio_sr_wakeup_result_t wakeup_info;    /*!< Information when wakeup word detected */
        qm_audio_sr_mn_result_t mn_info;            /*!< Information when voice command detected */
    } info;
} qm_audio_sr_result_t;

/**
 * @brief States of recorder speech recognition
 */
typedef struct {
    enum {
        DISABLED,
        RUNNING,
        SUSPENDED,
    } afe_state     : 4; /*!< AFE state */
    bool wwe_enable : 1; /*!< Wake word detection state */
    bool mn_enable  : 1; /*!< Speech command recognition state */
    bool vad_enable : 1; /*!< Voice detection state */
} qm_audio_sr_state_t;

/**
 * @brief Monitor of the recorder speech recognition
 */
typedef qm_err_t (*qm_audio_sr_monitor_t)(qm_audio_sr_result_t *result, void *user_ctx);

typedef struct {
    /**
     * subprocess
     */
    qm_audio_subproc_iface_t base;

    /**
     * @brief Suspend the afe tasks
     *
     * @param handle    The handle of sr handle
     * @param suspend   true to suspend the afe tasks, false to resume.
     *
     * @returns ESP_OK
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*afe_suspend)(void *handle, bool suspend);

    /**
     * @brief Function to set afe process result
     *
     * @param handle    The handle of sr handle
     * @param monitor   The monitor callback
     * @param user_ctx  User ctx
     *
     * @returns ESP_OK
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*set_afe_monitor)(void *handle, qm_audio_sr_monitor_t monitor, void *user_ctx);

    /**
     * @brief Function to set multinet process result
     *
     * @param handle    The handle of sr handle
     * @param monitor   The monitor callback
     * @param user_ctx  User ctx
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*set_mn_monitor)(void *handle, qm_audio_sr_monitor_t monitor, void *user_ctx);

    /**
     * @brief Enable or disable the wake word detection
     *
     * @param handle    The handle of sr handle
     * @param enable    Switch for the wake word detection
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*wwe_enable)(void *handle, bool_t enable);

    /**
     * @brief Enable or disable the speech command recognition
     *
     * @param handle    The handle of sr handle
     * @param enable    Switch for the speech command recognition
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*mn_enable)(void *handle, bool_t enable);

    /**
     * @brief 
     *
     * @param handle    The handle of sr handle
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    int (*get_feed_chunksize)(void *handle);

    /**
     * @brief 
     *
     * @param handle    The handle of sr handle
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    int (*get_feed_channel_num)(void *handle);

} qm_audio_sr_iface_t;

#ifdef __cplusplus
}
#endif

#endif /* __RECORDER_SR_IFACE_H__ */
