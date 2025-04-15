#ifndef _QM_PLAYER_H_
#define _QM_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

typedef void* qm_player_t;

/**
 * @brief 模块事件
 */
typedef enum {
    QM_PLAYER_EVENT_IDLE, /**< Player is idle, not playing audio */
    QM_PLAYER_EVENT_COMPLETED_PLAYING_NEXT, /**< Player is playing and playing a new audio file */
    QM_PLAYER_EVENT_PLAYING, /**< Player is playing */
    QM_PLAYER_EVENT_PAUSE, /**< Player is pausing */
    QM_PLAYER_EVENT_SHUTDOWN, /**< Player is shutting down */
    QM_PLAYER_EVENT_UNKNOWN_FILE_TYPE, /**< File type is unknown */
    QM_PLAYER_EVENT_UNKNOWN /**< Unknown event */
} qm_player_event_type_t;

/**
 * @brief  通知用户的报文类型
 */
typedef void (*qm_player_event_t)(qm_player_t handle, qm_player_event_type_t event, void *arg);

typedef enum {

    /**
     * @brief 设置回调, 它在SDK收到网络报文的时候被调用, 告知用户, 数据类型为(qm_player_event_t)
     * 
     * @details
     * 
     * 数据类型: (qm_player_event_t)
     */
    QM_PLAYER_OPT_EVENT_HANDLER,

    /**
     * @brief 
     *
     * @details 
     * 
     * 数据类型: (void *)
     */
    QM_PLAYER_OPT_USERDATA,

    QM_PLAYER_OPT_MAX
} qm_player_option_t;

/**
 * @brief Initialize hardware, allocate memory, create and start audio task.
 * Call before any other 'audio' functions.
 *
 * @param params 
 * @return qm_err_t
 */
qm_player_t qm_player_init(void);

/**
 * @brief Initialize hardware, allocate memory, create and start audio task.
 * Call before any other 'audio' functions.
 *
 * @param params 
 * @return qm_err_t
 */
qm_err_t qm_player_deinit(qm_player_t handle);

/**
 * @brief 配置会话
 *
 * @param[in] handle 会话句柄
 * @param[in] option 配置选项, 更多信息请参考 @ref qm_player_option_t
 * @param[in] data   配置选项数据, 更多信息请参考 @ref qm_player_option_t
 *
 * @return qm_err_t
 * @retval <QM_EOK  参数配置失败
 * @retval >=QM_EOK 参数配置成功
 *
 */
qm_err_t qm_player_setopt(qm_player_t handle, qm_player_option_t option, void *data);

/**
 * @brief Play audio file.
 *
 * Will interrupt a present playback and start the new playback
 * as soon as possible.
 *
 * @param fp - If ESP_OK is returned, will be fclose()ed by the audio system
 *             when the playback has completed or in the event of a playback error.
 *             If not ESP_OK returned then should be fclose()d by the caller.
 * @return
 *    - ESP_OK: Success in queuing play request
 *    - Others: Fail
 */
qm_err_t qm_player_play_file(qm_player_t handle, char *file);

/**
 * @brief Play audio file.
 *
 * Will interrupt a present playback and start the new playback
 * as soon as possible.
 *
 * @param fp - If ESP_OK is returned, will be fclose()ed by the audio system
 *             when the playback has completed or in the event of a playback error.
 *             If not ESP_OK returned then should be fclose()d by the caller.
 * @return
 *    - ESP_OK: Success in queuing play request
 *    - Others: Fail
 */
qm_err_t qm_player_play_bin(qm_player_t handle, uint8_t *buffer, int buffer_len);

/**
 * @brief Pause playback
 *
 * @return
 *    - ESP_OK: Success in queuing pause request
 *    - Others: Fail
 */
qm_err_t qm_player_pause(qm_player_t handle);

/**
 * @brief Resume playback
 *
 * Has no effect if playback is not in progress
 * @return qm_err_t
 *    - ESP_OK: Success in queuing resume request
 *    - Others: Fail
 */
qm_err_t qm_player_resume(qm_player_t handle);

/**
 * @brief Stop playback
 *
 * Has no effect if playback is already stopped
 * @return qm_err_t
 *    - ESP_OK: Success in queuing resume request
 *    - Others: Fail
 */
qm_err_t qm_player_stop(qm_player_t handle);

/**
 * @brief IDLE flag
 *
 * Has no effect if playback is already stopped
 * @return qm_err_t
 *    - QM_TRUE: play stack is idle
 *    - QM_FLASE: play stack is busy
 */
bool_t qm_player_is_idle(qm_player_t handle);

/**
 * @brief 
 *
 * Has no effect if playback is already stopped
 * @return qm_err_t
 */
qm_err_t qm_player_mute(qm_player_t handle);

/**
 * @brief 
 *
 * Has no effect if playback is already stopped
 * @return qm_err_t
 */
qm_err_t qm_player_unmute(qm_player_t handle);

#ifdef __cplusplus
}
#endif

#endif
