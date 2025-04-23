#ifndef __QM_AUDIO_VC_H__
#define __QM_AUDIO_VC_H__

#include "qm.h"
#include "qm_audio_vc_iface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VC_FETCH_TASK_STACK_SZ      (5 * 1024)
#define VC_FETCH_TASK_PRIO          (22)
#define VC_FETCH_TASK_PINNED_CORE   (1)
#define VC_OUTPUT_RB_SIZE           (6 * 1024)

/**
 * @brief SR processor handle
 */
typedef void *qm_audio_vc_handle_t;

/**
 * @brief SR processor configuration
 * @note  Since the detection of command words requires a clear starting point,
 *        the moment the wake word is detected is taken as the default start of detection.
 *        Therefore, if the wake word detection is disabled,
 *        the detection will use the `vad_state` detected by `esp-sr` as the start of detection.
 *        However, due to the fluctuation of this `vad_state`,
 *        the effectiveness of command word detection will be limited.
 */
typedef struct {
    void        *afe_cfg;                              /*!< Configuration of AFE */
    int          feed_task_core;                        /*!< Core id of feed task */
    int          feed_task_prio;                        /*!< Priority of feed task*/
    int          feed_task_stack;                       /*!< Stack size of feed task */
    int          fetch_task_core;                       /*!< Core id of fetch task */
    int          fetch_task_prio;                       /*!< Priority of fetch task */
    int          fetch_task_stack;                      /*!< Stack size of fetch task */
    int          rb_size;                               /*!< Ringbuffer size of recorder sr */
    char         *partition_label;                      /*!< Partition label which stored the model data */
} qm_audio_vc_cfg_t;

#define DEFAULT_QM_AUDIO_VC_CFG(fmt, afe_mode) {                  \
    .afe_cfg = afe_config_init(fmt, NULL, AFE_TYPE_VC, afe_mode),  \
    .fetch_task_core = VC_FETCH_TASK_PINNED_CORE,                                     \
    .fetch_task_prio = VC_FETCH_TASK_PRIO,                                            \
    .fetch_task_stack = VC_FETCH_TASK_STACK_SZ,                                       \
    .rb_size = VC_OUTPUT_RB_SIZE,                                                     \
}

/**
 * @brief Initialize sr processor, and the sr is disabled as default.
 *
 * @param cfg   Configuration of sr
 * @param iface User interface provide by recorder sr
 *
 * @return NULL    failed
 *         Others  SR handle
 */
qm_audio_vc_handle_t qm_audio_vc_create(qm_audio_vc_cfg_t *cfg, qm_audio_vc_iface_t **iface);

/**
 * @brief Destroy SR processor and recycle all resource
 *
 * @param handle SR processor handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_vc_destroy(qm_audio_vc_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif /*__RECORDER_SR_H__*/
