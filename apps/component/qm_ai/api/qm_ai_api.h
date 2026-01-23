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
    QM_AI_EVENT_VOLUME_ADJUST,
    QM_AI_EVENT_MUTE_CTRL,

    //智能体事件
    QM_AI_EVENT_AGENT_STATE = 0x100,
    QM_AI_EVENT_AGENT_ASR,
    QM_AI_EVENT_AGENT_TTS,
    QM_AI_EVENT_AGENT_ABORT,
    QM_AI_EVENT_AGENT_LLM,
    QM_AI_EVENT_AGENT_STOP,

} qm_ai_event_type_t;

typedef enum {
    QM_AI_AGENT_STATE_NONE,
    QM_AI_AGENT_STATE_SILENT,
    QM_AI_AGENT_STATE_SPEAKING,
    QM_AI_AGENT_STATE_THINKING,
    QM_AI_AGENT_STATE_MAX,
} qm_ai_agent_state_t;

typedef struct 
{
    qm_ai_event_type_t type;
    union 
    {
        struct {
            qm_ai_agent_state_t state;
            const char *text;
        } agent_state;

        struct {
            const char *text;
        } asr;

        struct {
            char *text;
            uint32_t text_len;
            uint32_t duration_ms;
        } tts;

        struct {
            int turn_id;
        } agent_abort;

        struct 
        {
            char *emotion;
        } agent_llm;
        
        struct 
        {
            
        } agent_timeout;
    } data;
} qm_ai_event_t;

typedef void (*qm_ai_event_handler_t)(qm_ai_handle_t handle, qm_ai_event_t *event_param, void *arg);

typedef struct 
{
    /**
     * Configure audio drvier api
    */
    qm_ai_driver_param_t audio_driver;
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
    int channel_num;
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
    qm_ai_driver_param_t video_driver;
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

/**
 * @brief 初始化AI句柄
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
int32_t qm_ai_set_version(qm_ai_handle_t handle, char *version);

/**
 * @brief 设置语言
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_language(qm_ai_handle_t handle, char *language);

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
 * @brief 配置服务句柄
 * 
 * @details
 * 
 */
int32_t qm_ai_set_service_handle_option(qm_ai_handle_t handle, void *service_handle);

/**
 * @brief 配置事件回调
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_event_register_option(qm_ai_handle_t handle, qm_ai_event_handler_t handler, void *arg);

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
 * @param[in]	intr_type:	1-打断 2-打断并返回安抚词
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
int32_t qm_ai_audio_set_local_volume_up(qm_ai_handle_t handle);
int32_t qm_ai_audio_set_local_volume_down(qm_ai_handle_t handle);

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