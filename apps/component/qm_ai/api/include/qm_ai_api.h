#ifndef __QM_AI_API_H__
#define __QM_AI_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_ai_common.h"
#include "qm_ai_driver.h"

typedef struct qm_ai_t *qm_ai_handle_t;

typedef enum
{
    QM_AI_EVENT_NONE = 0,
    
    //驱动状态
    QM_AI_EVENT_DRIVER_WAKEUP = 0x00,
    QM_AI_EVENT_DRIVER_TIMEOUT,

    //网络状态
    QM_AI_EVENT_CLOUD_CONN = 0x10,
    QM_AI_EVENT_CLOUD_DISCONN,
    QM_AI_EVENT_CLOUD_LICENSE,

    //音量控制
    QM_AI_EVENT_VOLUME_CTRL = 0x20,
    QM_AI_EVENT_VOLUME_UP,
    QM_AI_EVENT_VOLUME_DOWN,

    //智能体事件
    QM_AI_EVENT_AGENT_STATE = 0x100,
    QM_AI_EVENT_AGENT_ABORT,
    QM_AI_EVENT_AGENT_LLM,
    QM_AI_EVENT_AGENT_STT,
    QM_AI_EVENT_AGENT_TTS,
    QM_AI_EVENT_AGENT_STOP,

}qm_ai_event_type_t;

typedef struct 
{
    qm_ai_event_type_t type;
    union 
    {
        struct
        {
            
        }agent_abort;

        struct 
        {
            
        }agent_llm;

        struct 
        {
            
        }agent_stt;

        struct 
        {
            
        }agent_tts;
        
        struct 
        {
            
        }agent_timeout;
    }data;
}qm_ai_even_t;

typedef void (*qm_ai_event_handler_t)(qm_ai_handle_t handle, qm_ai_even_t *event_param, void *arg);

typedef struct 
{
    /**
     * Configure audio drvier api
    */
    qm_ai_driver_param_t param;
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

}qm_ai_audio_options_t;

typedef struct 
{
    /**
     * Configure video drvier api
    */
    qm_ai_driver_param_t param;
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

}qm_ai_video_options_t;

#if 0
typedef struct 
{
    uint32_t pid;
    char    *secret;
    char    *license;
}qm_ai_license_t;
#endif

/**
 * @brief 初始化IOT句柄
 * 
 * @details 
 * 
 */
qm_ai_handle_t qm_ai_open(qm_ai_comm_type_t comm_type, void *arg);


/**
 * @brief 设置版本号
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_version(qm_ai_handle_t handle, char *vserion);

/**
 * @brief 设置语言
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_langage(qm_ai_handle_t handle, char *langage);

/**
 * @brief 配置音频参数
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_audio_option(qm_ai_handle_t handle, qm_ai_audio_options_t *options);

/**
 * @brief 配置视频参数
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_video_option(qm_ai_handle_t handle, qm_ai_video_options_t *options);

/**
 * @brief 配置事件回调
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_event_regsiter_option(qm_ai_handle_t handle, qm_ai_event_handler_t handler, void *arg);

#if 0
/**
 * @brief 配置lincese
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_license(qm_ai_handle_t handle, qm_ai_license_t *param);
#endif

/**
 * @brief 启动AI模块
 * 
 * @details 
 * 
 */
int32_t qm_ai_start(qm_ai_handle_t handle);

/**
 * @brief 停止AI模块
 * 
 * @details 
 * 
 */
int32_t qm_ai_stop(qm_ai_handle_t handle);

/**
 * @brief		主动打断
 * @param[in]	engine: 	实例指针
 * @param[in]	intr_type:	0-打断 1-打断并返回安抚词
 * @return		函数是否执行成功，具体见参数表
 * 
 */
int32_t qm_ai_abort_request(qm_ai_handle_t handle, int32_t intr_type);


//音视频接口

/**
 * @brief 
 * 
 * @details 
 * 
 */
int32_t qm_ai_audio_set_mic_mute(qm_ai_handle_t handle, bool_t mute);

/**
 * @brief 
 * 
 * @details 
 * 
 */
bool_t qm_ai_audio_get_mic_mute_state(qm_ai_handle_t handle);

/**
 * @brief 
 * 
 * @details 
 * 
 */
int32_t qm_ai_audio_set_player_mute(qm_ai_handle_t handle, bool_t mute);

/**
 * @brief 
 * 
 * @details 
 * 
 */
bool_t qm_ai_audio_get_player_mute_state(qm_ai_handle_t handle);

/**
 * @brief 
 * 
 * @details 
 * 
 */
int32_t qm_ai_audio_set_local_volume(qm_ai_handle_t handle, uint32_t volume);

/**
 * @brief 
 * 
 * @details 
 * 
 */
uint32_t qm_ai_audio_get_local_volume(qm_ai_handle_t handle);

/**
 * @brief 
 * 
 * @details 
 * 
 */
int32_t qm_ai_audio_player_data(qm_ai_handle_t handle, uint8_t *data, uint32_t size);


/**
 * @brief 反初始化AI句柄
 * 
 * @param[in] handle AI会话句柄
 * 
 * @details 
 * 
 */
int32_t qm_ai_close(qm_ai_handle_t *handle);


#if defined(__cplusplus)
}
#endif

#endif  /* __QM_AI_CLOUD_H__ */