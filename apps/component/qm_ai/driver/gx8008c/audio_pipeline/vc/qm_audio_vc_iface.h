#ifndef __QM_AUDIO_VC_IFACE_H__
#define __QM_AUDIO_VC_IFACE_H__

#include "esp_err.h"
#include "qm_audio_subproc_iface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Speech recognition result
 */
typedef struct {
    enum {
        VC_RESULT_UNKNOW    = -100,  /*!< Unknow speech recognition result */
        VC_RESULT_NOISE     = -9,    /*!< Noise detected */
        VC_RESULT_SPEECH    = -8,    /*!< Speech detected */
    } type;
    union {
        void *reserved;          // reserved for future use
    } info;
} qm_audio_vc_result_t;

/**
 * @brief States of recorder speech recognition
 */
typedef struct {
    uint8_t afe_state ; /*!< AFE state */
    bool vad_enable : 1; /*!< Voice detection state */
} qm_audio_vc_state_t;

/**
 * @brief Monitor of the recorder speech recognition
 */
typedef qm_err_t (*qm_audio_vc_monitor_t)(qm_audio_vc_result_t *result, void *user_ctx);

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
    qm_err_t (*afe_suspend)(void *handle, bool_t suspend);

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
    qm_err_t (*set_afe_monitor)(void *handle, qm_audio_vc_monitor_t monitor, void *user_ctx);

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
    
} qm_audio_vc_iface_t;

#ifdef __cplusplus
}
#endif

#endif /* __RECORDER_SR_IFACE_H__ */
