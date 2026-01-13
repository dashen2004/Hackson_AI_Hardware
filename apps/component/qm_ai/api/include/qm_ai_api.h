#ifndef __QM_AI_API_H__
#define __QM_AI_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_spec_api.h"
#include "qm_ai_common.h"
#include "qm_ai_driver.h"

typedef struct qm_ai_t *qm_ai_handle_t;

typedef enum
{
    QM_AI_EVENT_NONE = 0,
    
    //网络状态
    QM_AI_EVENT_CONFIG,
    QM_AI_EVENT_CONFIG_TIMEOUT,
    QM_AI_EVENT_CLOUD_CONN,
    QM_AI_EVENT_CLOUD_DISCONN,
    QM_AI_EVENT_BIND_NOTIFY,
    QM_AI_EVENT_RESET,

    //OTA状态
    QM_AI_EVENT_OTA_READY,
    QM_AI_EVENT_OTA_START,
    QM_AI_EVENT_OTA_INSTALLING,
    QM_AI_EVENT_OTA_PROGRESS_UPADTE,
    QM_AI_EVENT_OTA_FAILED,
    QM_AI_EVENT_OTA_SUCCESS,
    QM_AI_EVENT_OTA_CANCEL,

    //控制指令
    QM_AI_EVENT_TYPE_SET,
    QM_AI_EVENT_TYPE_GET_RSP,
    QM_AI_EVENT_TYPE_GET_REQ,

    //识别事件
    QM_AI_EVENT_TYPE_WAKEUP,
    QM_AI_EVENT_TYPE_ABORT,
    QM_AI_EVENT_TYPE_LLM,
    QM_AI_EVENT_TYPE_STT,
    QM_AI_EVENT_TYPE_TTS,
    QM_AI_EVENT_TYPE_TIMEOUT,
}qm_ai_event_type_t;


typedef struct 
{
    qm_ai_event_type_t type;
    union 
    {
        struct 
        {
            qm_spec_property_t *property;
        }set;

        struct 
        {
            qm_spec_property_t *property;
        }get_rsp;

        struct 
        {
            qm_spec_property_t *property;
        }get_req;

        struct 
        {
            
        }wakeup;

        struct
        {
            
        }abort;

        struct 
        {
            
        }llm;

        struct 
        {
            
        }stt;

        struct 
        {
            
        }tts;


        struct 
        {
            
        }atimeout;
    }data;
}qm_ai_even_t;

typedef void (*qm_ai_event_handler_t)(void *handle, qm_ai_event_type_t event_type, qm_ai_even_t *event_param, void *arg);

typedef struct 
{
    qm_ai_driver_type_t driver_type;

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
    qm_ai_driver_type_t driver_type;
    
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

typedef struct 
{
    uint32_t pid;
    char    *sercet;
    char    *lincese_param;
}qm_ai_lincese_t;

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
 * @brief 配置lincese
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_lincese(qm_ai_handle_t handle, qm_ai_lincese_t *param);

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
 * @brief 
 * 
 * @details 
 * 
 */
int32_t qm_ai_video_set_mute(qm_ai_handle_t handle, bool_t mute);

/**
 * @brief 
 * 
 * @details 
 * 
 */
bool_t qm_ai_video_get_mute_state(qm_ai_handle_t handle);

/**
 * @brief 
 * 
 * @details 
 * 
 */
int32_t qm_ai_video_send_data(qm_ai_handle_t handle, uint8_t *data, uint32_t size);

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