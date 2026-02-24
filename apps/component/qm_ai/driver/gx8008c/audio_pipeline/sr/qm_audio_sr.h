#ifndef __QM_AUDIO_SR_H__
#define __QM_AUDIO_SR_H__

#include "qm.h"
#include "qm_audio_sr_iface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SR_FETCH_TASK_STACK_SZ      (4 * 1024)
#define SR_FETCH_TASK_PRIO          (22)
#define SR_FETCH_TASK_PINNED_CORE   (1)
#define SR_OUTPUT_RB_SIZE        (6 * 1024)

/**
 * @brief SR processor handle
 */
typedef void *qm_audio_sr_handle_t;

typedef struct {
    enum {
        QM_AUDIO_SR_WAKEUP_CMD = 1,          /*!< Wakeup start */
        QM_AUDIO_SR_COMM_CMD,            /*!< Wakeup stop */
    } type;                            /*!< sr type */
    int id;
    float prob;
    char *str;
} qm_audio_sr_cmd_table_t;

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
    bool         multinet_init;                         /*!< Enable of speech command recognition */
    int          feed_task_core;                        /*!< Core id of feed task */
    int          feed_task_prio;                        /*!< Priority of feed task*/
    int          feed_task_stack;                       /*!< Stack size of feed task */
    int          fetch_task_core;                       /*!< Core id of fetch task */
    int          fetch_task_prio;                       /*!< Priority of fetch task */
    int          fetch_task_stack;                      /*!< Stack size of fetch task */
    int          rb_size;                               /*!< Ringbuffer size of recorder sr */
    char         *partition_label;                      /*!< Partition label which stored the model data */
    char         *mn_language;                          /*!< Command language for multinet to load */
    char         *wn_wakeword;                          /*!< Wake Word for WakeNet to load. This is useful when multiple Wake Words are selected in sdkconfig. Setting this to NULL will use the first found model. */
} qm_audio_sr_cfg_t;

#define DEFAULT_QM_AUDIO_SR_CFG(fmt, partition, afe_mode) {                  \
    .afe_cfg = afe_config_init(fmt, esp_srmodel_init(partition), AFE_TYPE_SR, afe_mode),  \
    .multinet_init = true,                                                            \
    .fetch_task_core = SR_FETCH_TASK_PINNED_CORE,                                        \
    .fetch_task_prio = SR_FETCH_TASK_PRIO,                                               \
    .fetch_task_stack = SR_FETCH_TASK_STACK_SZ,                                          \
    .rb_size = SR_OUTPUT_RB_SIZE,                                                     \
    .partition_label = partition,                                                     \
    .mn_language = ESP_MN_CHINESE,                                                    \
    .wn_wakeword = NULL,                                                              \
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
qm_audio_sr_handle_t qm_audio_sr_create(qm_audio_sr_cfg_t *cfg, qm_audio_sr_iface_t **iface);

/**
 * @brief Destroy SR processor and recycle all resource
 *
 * @param handle SR processor handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_sr_destroy(qm_audio_sr_handle_t handle);

/**
 * @brief Reset the speech commands
 *
 * @param handle        SR processor handle
 * @param command_str   String of the commands. more details on `https://github.com/espressif/esp-sr/blob/release/v1.0/docs/speech_command_recognition/README.md#2reset-api-on-the-fly`
 * @param err_phrase_id error string output
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_sr_add_speech_cmd(qm_audio_sr_handle_t handle, qm_audio_sr_cmd_table_t *command_str, int command_num);


qm_err_t qm_audio_sr_get_speech_cmd(qm_audio_sr_handle_t handle, qm_audio_sr_cmd_table_t **command_str, int *command_num);

#ifdef __cplusplus
}
#endif

#endif /*__RECORDER_SR_H__*/
