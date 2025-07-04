#ifndef _QM_AUDIO_WS_H_
#define _QM_AUDIO_WS_H_

#include "qm.h"
#include "qm_spec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define   QM_AUDIO_WS_VERSION               (1)

#ifndef CONFIG_QM_AUDIO_WS_URL_MAX_SIZE     
#define CONFIG_QM_AUDIO_WS_URL_MAX_SIZE                   (64)
#endif

#ifndef CONFIG_QM_AUDIO_WS_QUEUE_NUM     
#define CONFIG_QM_AUDIO_WS_QUEUE_NUM                      (200)
#endif

#ifndef CONFIG_QM_AUDIO_WS_TIMEOUT_MS    
#define CONFIG_QM_AUDIO_WS_TIMEOUT_MS                     (1000)
#endif

#ifndef CONFIG_QM_AUDIO_WS_QUEUE_DELAY_MS     
#define CONFIG_QM_AUDIO_WS_QUEUE_DELAY_MS                  (60)
#endif

#ifndef CONFIG_QM_AUDIO_WS_QUEUE_TIMEOUT_COUNT     
#define CONFIG_QM_AUDIO_WS_QUEUE_TIMEOUT_COUNT             (3)
#endif

#ifndef CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT     
#define CONFIG_QM_AUDIO_WS_SEND_TASK_SUPPORT               (0)
#endif

/**
* @brief Audio WS event type
*/
typedef enum {
    QM_AUDIO_WS_RECV_TYPE_HELLO_REPLY = 0,  /*!< 服务端回复客户端 hello 请求 */
    QM_AUDIO_WS_RECV_TYPE_STT,              /*!< 服务端回复客户端上行语音的识别结果 */
    QM_AUDIO_WS_RECV_TYPE_LLM,              /*!< 服务端发送大模型识别的感情信息 */
    QM_AUDIO_WS_RECV_TYPE_TTS,              /*!< 服务端端下发回复音频的状态及字幕信息 */
    QM_AUDIO_WS_RECV_TYPE_SET,              /*!< 服务端下发设备控制命令 */
    QM_AUDIO_WS_RECV_TYPE_OTA_UPGRADE,      /*!< 服务端下发设备ota升级命令*/

    QM_AUDIO_WS_RECV_TYPE_CONNECT,          /*!< 已连接服务器*/
    QM_AUDIO_WS_RECV_TYPE_DISCONNECT,       /*!< 未连接服务器*/

    QM_AUDIO_WS_RECV_TYPE_ABORT,            /*!< 平台判断在线唤醒词中止时下发，请求设备终止播放*/
} qm_audio_ws_recv_type_t;

typedef enum {
    QM_AUDIO_WS_FMT_TYPE_OPUS = 1,
    QM_AUDIO_WS_FMT_TYPE_PCM,
    QM_AUDIO_WS_FMT_TYPE_WAV,
    QM_AUDIO_WS_FMT_TYPE_MP3,
} qm_audio_ws_fmt_type_t;                           

typedef enum {
    QM_AUDIO_WS_LISTEN_STATE_START = 1,
    QM_AUDIO_WS_LISTEN_STATE_STOP,
    QM_AUDIO_WS_LISTEN_STATE_DETECT,
} qm_audio_ws_listen_state_t;        


typedef enum {
    QM_AUDIO_WS_LISTEN_MODE_AUTO = 1,
    QM_AUDIO_WS_LISTEN_MODE_MANUAL,
    QM_AUDIO_WS_LISTEN_MODE_REALTIME,
} qm_audio_ws_listen_mode_t;        

typedef enum {
    QM_AUDIO_WS_ABORT_CODE_DETECT_WAKEUP = 1,
    QM_AUDIO_WS_ABORT_CODE_DETECT_VAD,

} qm_audio_ws_abort_code_t;    

typedef enum {
    QM_AUDIO_WS_TTS_TYPE_CHAT = 0,
    QM_AUDIO_WS_TTS_TYPE_OPENING,
    QM_AUDIO_WS_TTS_TYPE_CLOSING,
} qm_audio_ws_tts_type_t;

typedef enum {
    QM_AUDIO_WS_TTS_STATE_START = 1,
    QM_AUDIO_WS_TTS_STATE_STOP,
    QM_AUDIO_WS_TTS_STATE_SENTENCE_START,
    QM_AUDIO_WS_TTS_STATE_SENTENCE_STOP,
} qm_audio_ws_tts_state_t;    

typedef enum {
    QM_AUDIO_WS_PLAYER_STATE_STOP  = 0,
    QM_AUDIO_WS_PLAYER_STATE_START = 1,
    QM_AUDIO_WS_PLAYER_STATE_DATA,
} qm_audio_ws_player_state_t; 

typedef struct{
    int cmd; //1:开始播放, 2: 暂停播放, 3: 停止播放,  4: 上一首, 5：下一首
    int mode;//1:列表循环, 2: 单曲循环, 3：随机播放
    int times;//循环次数
} qm_audio_ws_music_param_t; 

typedef struct{

    char *text;
    qm_audio_ws_listen_state_t state;
    qm_audio_ws_listen_mode_t mode;

} qm_audio_ws_listen_param_t;     

typedef struct{

    int                           have_audio_info;

    qm_audio_ws_fmt_type_t        fmt_type;          /*!< 音频格式*/
    int                           rate;              /*!< 音频采样率*/
    int                           bits;              /*!< 音频采样位数*/
    int                           channel;           /*!< 音频声道数*/
    int                           frame_duration_ms; /*!< 音频片段长度，单位ms，默认值60*/

} qm_audio_ws_recv_hello_reply_t;  

typedef struct{

    char    *text; 

} qm_audio_ws_recv_stt_t;  

typedef struct{

    char    *icon;
    char    *emotion;

} qm_audio_ws_recv_llm_t;  

typedef struct{

    qm_audio_ws_tts_type_t tts_type;
    qm_audio_ws_tts_state_t state;
    char *text;
} qm_audio_ws_recv_tts_t;  

typedef struct{
    qm_spec_property_operation_t  *property_operation;
} qm_audio_ws_recv_set_t;  

typedef struct{
    int size;
    char *uri;
    char *md5;
    char *version;
} qm_audio_ws_recv_ota_upgrade_t; 

typedef struct{
    int reason;
} qm_audio_ws_recv_abort_t; 

/**
 * @brief Recorder event
 */
typedef struct {

    qm_audio_ws_recv_type_t recv_type;
    
    union 
    {
        qm_audio_ws_recv_hello_reply_t  hello_reply;
        qm_audio_ws_recv_stt_t          stt;
        qm_audio_ws_recv_llm_t          llm;
        qm_audio_ws_recv_tts_t          tts;
        qm_audio_ws_recv_set_t          set;
        qm_audio_ws_recv_ota_upgrade_t  ota_upgrade;
        qm_audio_ws_recv_abort_t        abort;
    }data;
    
} qm_audio_ws_recv_evt_t;

/**
 * @brief Event Notification
 */
typedef void (*qm_audio_ws_recv_event_cb_t)(qm_audio_ws_recv_evt_t *event, void *user_data);

typedef void (*qm_audio_ws_data_cb_t)(void *data, int len, void *user_data);

/**
 * @brief Audio recorder configuration
 */
typedef struct {
    
    char                          *uri;

    int                            port;
    char                          *host;
    char                          *path;
    

    int                           buff_size;

    int                           pinned_core;      /*!< Audio recorder task pinned to core */
    int                           task_prio;        /*!< Audio recorder task priority */
    int                           task_size;        /*!< Audio recorder task stack size */
    
    qm_audio_ws_data_cb_t         data_cb;          
    qm_audio_ws_recv_event_cb_t   event_cb;         /*!< Event callback function, event type as audio_rec_evt_t shown above*/
    void                        *user_data;         /*!< Pointer to user data (optional) */

    char                         *sn;
    char                         *ver;              /*!< OTA: 1.0.1 */
    int                           pid;              /*!< 产品pid */
    int                           did;              /*!< 设备did 可选, -1则无效*/
    uint32_t                      connect_timeout;  /*!< 连接服务端超时时间，默认为10s, 单位为秒*/

    qm_audio_ws_fmt_type_t        fmt_type;          /*!< 音频格式*/
    uint16_t                      rate;              /*!< 音频采样率*/
    uint16_t                      bits;              /*!< 音频采样位数*/
    uint16_t                      channel;           /*!< 音频声道数*/
    uint16_t                      frame_duration_ms; /*!< 音频片段长度，单位ms，默认值60*/

    uint8_t                       flow_ctrl_enbale;        /*!< 流控使能*/

    uint8_t                       min_len;        /*!< */
    uint8_t                       max_len;        /*!< */
    uint8_t                       normal_len;     /*!< */

    uint8_t                       min_speed;        /*!< */
    uint8_t                       max_speed;        /*!< */
    
} qm_audio_ws_cfg_t;

/**
 * @brief Audio websocket handle
 */
typedef void *qm_audio_ws_handle_t;

/**
 * @brief Initialize and start up audio recorder
 *
 * @param cfg Configuration of audio recorder
 *
 * @return NULL    failed
 *         Others  audio recorder handle
 */
qm_audio_ws_handle_t qm_audio_ws_create(qm_audio_ws_cfg_t *cfg);

/**
 * @brief 
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_start(qm_audio_ws_handle_t handle);

/**
 * @brief 
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_stop(qm_audio_ws_handle_t handle);

/**
 * @brief 
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_clean(qm_audio_ws_handle_t handle);

/**
 * @brief 
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_hello(qm_audio_ws_handle_t handle, uint32_t timeout);


/**
 * @brief 
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_music(qm_audio_ws_handle_t handle, qm_audio_ws_music_param_t *params, uint32_t timeout);

/**
 * @brief 
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_listen(qm_audio_ws_handle_t handle, qm_audio_ws_listen_param_t *params, uint32_t timeout);

/**
 * @brief 
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_abort(qm_audio_ws_handle_t handle, qm_audio_ws_abort_code_t code, uint32_t timeout);

/**
 * @brief 请求播放欢开场白，平台会终止当前回复进程，播放设备当前智能体角色的“开场白”
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_opening_remarks(qm_audio_ws_handle_t handle, uint32_t timeout);

/**
 * @brief 请求播放告别词，平台会终止当前回复进程，播放设备当前智能体角色的“告别词”
 *
 * @param handle Audio websocket handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_closing_remarks(qm_audio_ws_handle_t handle, uint32_t timeout);

/**
 * @brief 
 *
 * @param handle  Audio websocket handle
 * @param buffer  Buffer to save data
 * @param length  Size of buffer
 *
 * @return Length of data actually read
 *         ESP_ERR_INVALID_ARG
 */
int qm_audio_ws_data_send(qm_audio_ws_handle_t handle, void *buffer, int length, uint32_t timeout);

/**
 * @brief Destroy audio recorder and recycle all resource
 *
 * @param handle Audio recorder handle
 *
 * @return ESP_OK
 *         ESP_FAIL
 */
qm_err_t qm_audio_ws_destroy(qm_audio_ws_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
