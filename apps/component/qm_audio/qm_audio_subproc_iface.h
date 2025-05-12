#ifndef __QM_AUDIO_SUBPROC_H__
#define __QM_AUDIO_SUBPROC_H__

#include "qm.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data read interface
 */
typedef int (*qm_audio_data_read_t)(void *buffer, int buf_sz, void *user_ctx, TickType_t ticks);

/**
 * @brief Recorder subprocess base interface
 */
typedef struct {
    /**
     * @brief Enable or disable the subprocess
     *
     * @param handle    The handle of recorder subprocess
     * @param enable    Switch for the subprocess
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*enable)(void *handle, bool_t enable);

    /**
     * @brief Get the working state of the subprocess
     *
     * @param handle    The handle of recorder subprocess
     * @param state     The pointer of state buffer
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*get_state)(void *handle, void *state);

    /**
     * @brief Function to set the data read callback, if not set, `feed` should be invoked by manual.
     *
     * @param handle    The handle of recorder subprocess
     * @param buf       The pointer of data buffer
     * @param len       The length of data
     *
     * @returns ESP_OK
     *          ESP_FAIL
     *          ESP_ERR_INVALID_ARG
     */
    qm_err_t (*set_read_cb)(void *handle, qm_audio_data_read_t read_cb, void *user_ctx);

    /**
     * @brief Function to feed data into recorder subprocess
     *
     * @param handle    The handle of recorder subprocess
     * @param buf       The pointer of data buffer
     * @param len       The length of data
     *
     * @returns         Feed length
     */
    void (*reset)(void *handle);

    /**
     * @brief Function to feed data into recorder subprocess
     *
     * @param handle    The handle of recorder subprocess
     * @param buf       The pointer of data buffer
     * @param len       The length of data
     *
     * @returns         Feed length
     */
    int (*feed)(void *handle, void *in_buf, int len);

    /**
     * @brief Function to fetch data from recorder subprocess
     *
     * @param handle    The handle of recorder subprocess
     * @param buf       The pointer of data buffer
     * @param len       The length of data
     * @param ticks     Timeout for reading
     *
     * @returns         Fetch length
     *                  ESP_ERR_INVALID_ARG
     */
    int (*fetch)(void *handle, void *buf, int len, TickType_t ticks);
} qm_audio_subproc_iface_t;

#ifdef __cplusplus
}
#endif

#endif
