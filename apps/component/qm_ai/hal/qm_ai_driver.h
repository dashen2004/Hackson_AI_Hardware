#ifndef __QM_AI_DRIVER_H__
#define __QM_AI_DRIVER_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_ai_common.h"

typedef struct qm_ai_driver_t *qm_ai_driver_handle_t;

typedef enum
{
    QM_AI_DRIVER_COMM_TYPE_CUSTOM = 0,
    QM_AI_DRIVER_COMM_TYPE_AUDIO,
    QM_AI_DRIVER_COMM_TYPE_VEDIO,
}qm_ai_driver_comm_type_t;

typedef enum
{
    QM_AI_DRIVER_TYPE_CUSTOM = 0,
    QM_AI_DRIVER_TYPE_GX8006,
}qm_ai_driver_type_t;

typedef enum
{
    QM_AI_DRIVER_EVENT_TYPE_WAKEUP = 0,
    QM_AI_DRIVER_EVENT_TYPE_TIMEOUT,
    QM_AI_DRIVER_EVENT_TYPE_READ_DATA,
    QM_AI_DRIVER_EVENT_TYPE_CUSTOM,
}qm_ai_driver_event_type_t;

typedef union
{
    struct 
    {
        /**
         * Configure audio data type
        */
        qm_ai_audio_data_type_t data_type;

        /**
         * Configure audio codec
        */
        qm_ai_audio_codec_type_t codec_type;
        /**
         * sample rate.
         */
        int sample_rate;
        /**
         * channel number.
         */
        int hannel_num;
        /**
         * duration. default 20ms
         */
        int duration;
        /**
         * wakeup timeout. default 10s
         */
        int wakeup_timeout;

    }audio;
    
    struct 
    {
        /**
         * The video data type: #video_data_type_e.
         */
        qm_ai_video_data_type_t data_type;
        /**
         * The video stream type: #video_stream_type_e
         */
        qm_ai_video_stream_type_t stream_type;
        /**
         * The frame type of the encoded video frame: #video_frame_type_e.
         */
        qm_ai_video_frame_type_t frame_type;
        /**
         * The number of video frames per second, can be set 0-60.
         * -This value will be used for calculating timestamps of the encoded image.
         * - If frame_per_sec equals zero, then real timestamp will be used.
         * - Otherwise, timestamp will be adjusted to the value of frame_per_sec set.
         */
        qm_ai_video_frame_rate_t frame_rate;
        /**
         * The rotation information of the encoded video frame: #VIDEO_ORIENTATION.
         */
        qm_ai_video_orientation_t rotation;

    }vedio;

}qm_ai_driver_comm_param_t;

typedef union 
{
    struct 
    {
        int reserved;
    }wakeup;

    struct 
    {
        int reserved;
    }timeout;

    struct 
    {
        int read_size;  //need use read func
    }read;
}qm_ai_driver_event_param_t;

typedef struct {
    qm_ai_driver_comm_type_t comm_type;
    qm_ai_driver_type_t driver_type;
    void *arg;
    int (*open)(void);
    int (*close)(void);
    int (*write)(void *data, int len, void *arg);
    int (*read)(void *data, int len, uint32_t timeout, void *arg);
    int (*set_mute) (bool_t mute);
    int (*get_mute) (bool_t *mute);
    int (*set_volume)(uint32_t volume);
    int (*get_volume)(uint32_t *volume);
    int (*enable_pa) (bool_t enable);
} qm_ai_driver_param_t;

typedef int (*qm_ai_driver_event_notify_t)(qm_ai_driver_event_type_t event_type, qm_ai_driver_event_param_t *event_param,void *arg);

//用户自定义驱动时使用
extern qm_ai_driver_param_t  qm_ai_drvier_ext_custom_param;

/**
 * @brief  打开会话
 * 
 * @details 
 * 
 */
qm_ai_driver_handle_t qm_ai_driver_open(qm_ai_driver_param_t *param);

/**
 * @brief 事件注册
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_event_regsiter(qm_ai_driver_handle_t handle, qm_ai_driver_event_notify_t notify);

/**
 * @brief 写入数据流
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_write(qm_ai_driver_handle_t handle, void *data, int len);

/**
 * @brief 获取数据流
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_read(qm_ai_driver_handle_t handle, void *data, int len, uint32_t timeout);

/**
 * @brief 设置消音状态
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_set_mute(qm_ai_driver_handle_t handle, bool_t mute);

/**
 * @brief 获取消音状态
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_get_mute(qm_ai_driver_handle_t handle, bool_t *mute);

/**
 * @brief 设置音量
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_set_volume(qm_ai_driver_handle_t handle, uint32_t volume);

/**
 * @brief 获取音量
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_get_volume(qm_ai_driver_handle_t handle, uint32_t *volume);

/**
 * @brief 使能PA
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_enable_pa(qm_ai_driver_handle_t handle, bool_t enable);

/**
 * @brief 关闭会话
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_driver_close(qm_ai_driver_handle_t *handle);


#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_COMMON_H__ */

