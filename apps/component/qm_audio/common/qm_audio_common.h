#ifndef __QM_AUDIO_COMMON_H__
#define __QM_AUDIO_COMMON_H__

#if defined(__cplusplus)
extern "C" {
#endif

#define QM_AUDIO_EVENT_ID            (0xF000)

#define QM_AUDIO_PROV_INFO_KV_TAG        "audio/prov"

typedef enum {
    QM_AUDIO_CODE_SUCCESS,
    QM_AUDIO_CODE_ERROR,
}qm_audio_code_t;

typedef enum {
    QM_AUDIO_EVENT_TYPE_UNKNOWN                                = - 1,
    QM_AUDIO_EVENT_TYPE_CONNECTIVITY_BASE                      = 0x0,
    QM_AUDIO_EVENT_TYPE_WIFI_DISCONNECT                        = 0x1,
    QM_AUDIO_EVENT_TYPE_WIFI_CONNECT                           = 0x2,
    QM_AUDIO_EVENT_TYPE_WIFI_SETTING_START                     = 0x3,
    QM_AUDIO_EVENT_TYPE_WIFI_SETTING_STOP                      = 0x4,
    QM_AUDIO_EVENT_TYPE_CONNECTIVITY_MAX                       = 0xFFF,

    QM_AUDIO_EVENT_TYPE_AUDIO_BASE                             = 0x1000,
    QM_AUDIO_EVENT_TYPE_AUDIO_PLAY                             = 0x1001,
    QM_AUDIO_EVENT_TYPE_AUDIO_PAUSE                            = 0x1002,
    QM_AUDIO_EVENT_TYPE_AUDIO_RESUME                           = 0x1003,
    QM_AUDIO_EVENT_TYPE_AUDIO_STOP                             = 0x1004,
    QM_AUDIO_EVENT_TYPE_AUDIO_GET_PROGRESS_BYTE                = 0x1005,
    QM_AUDIO_EVENT_TYPE_AUDIO_GET_PROGRESS_TIME                = 0x1006,
    QM_AUDIO_EVENT_TYPE_AUDIO_VOLUME_SET                       = 0x1007,
    QM_AUDIO_EVENT_TYPE_AUDIO_VOLUME_GET                       = 0x1008,
    QM_AUDIO_EVENT_TYPE_AUDIO_VOLUME_UP                        = 0x1009,
    QM_AUDIO_EVENT_TYPE_AUDIO_VOLUME_DOWN                      = 0x100A,
    QM_AUDIO_EVENT_TYPE_AUDIO_MUTE_ON                          = 0x100B,
    QM_AUDIO_EVENT_TYPE_AUDIO_MUTE_OFF                         = 0x100C,
    QM_AUDIO_EVENT_TYPE_AUDIO_MAX                              = 0x1FFF,

    QM_AUDIO_EVENT_TYPE_SR_BASE                                = 0x2000,
    QM_AUDIO_EVENT_TYPE_SR_WAKEUP                              = 0x2001,
    QM_AUDIO_EVENT_TYPE_SR_VAD_START                           = 0x2002,
    QM_AUDIO_EVENT_TYPE_SR_VAD_END                             = 0x2003,
    QM_AUDIO_EVENT_TYPE_SR_TIMEOUT                             = 0x2004,
    QM_AUDIO_EVENT_TYPE_SR_COMMAND                             = 0x2004,
    QM_AUDIO_EVENT_TYPE_SR_MAX                                 = 0x2FFF,

    QM_AUDIO_EVENT_TYPE_DISPLAY_BASE                           = 0x3000,
    QM_AUDIO_EVENT_TYPE_DISPLAY_TURN_ON                        = 0x3001,
    QM_AUDIO_EVENT_TYPE_DISPLAY_TURN_OFF                       = 0x3002,
    QM_AUDIO_EVENT_TYPE_DISPLAY_WIFI_SETTING                   = 0x3003,
    QM_AUDIO_EVENT_TYPE_DISPLAY_WIFI_DISCONNECTED              = 0x3004,
    QM_AUDIO_EVENT_TYPE_DISPLAY_WIFI_CONNECTED                 = 0x3005,
    QM_AUDIO_EVENT_TYPE_DISPLAY_SETTING_TIMEOUT                = 0x3006,
    QM_AUDIO_EVENT_TYPE_DISPLAY_BATTERY_CHARGING               = 0x3007,
    QM_AUDIO_EVENT_TYPE_DISPLAY_BATTERY_FULL                   = 0x3008,
    QM_AUDIO_EVENT_TYPE_DISPLAY_BATTERY_DISCHARGING            = 0x3009,
    QM_AUDIO_EVENT_TYPE_DISPLAY_MAX                            = 0x3FFF,

    QM_AUDIO_EVENT_TYPE_WS_BASE                                = 0x4000,
    QM_AUDIO_EVENT_TYPE_WS_DISCONNECT,
    QM_AUDIO_EVENT_TYPE_WS_CONNECT,
    QM_AUDIO_EVENT_TYPE_WS_SEND_HELLO,
    QM_AUDIO_EVENT_TYPE_WS_RECV_HELLO_REPLY,
    QM_AUDIO_EVENT_TYPE_WS_FLOW_CTRL,


    QM_AUDIO_EVENT_TYPE_WS_MSG_RECV,

    QM_AUDIO_EVENT_TYPE_CUSTOMER_BASE                          = 0x80000,
} qm_audio_event_type_t;

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_COMMON_H__ */

