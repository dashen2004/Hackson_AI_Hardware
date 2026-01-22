#ifndef __QM_AI_INNER_RTC_API_H__
#define __QM_AI_INNER_RTC_API_H__

#include "qm.h"
#include "qm_ai_api.h"

/*
    link event enum and struct
*/
typedef enum {
    QM_AI_STREAM_TYPE_TTS = 0,
    QM_AI_STREAM_TYPE_ASR,
    QM_AI_STREAM_TYPE_AGENT_STATE,
    QM_AI_STREAM_TYPE_INTERRUPT,
} qm_ai_stream_recv_type_t;

typedef struct {
    qm_ai_stream_recv_type_t type;
    union {
        struct {
            char *text;
            uint32_t text_len;
            uint32_t duration_ms;
        } tts;
        struct {
            const char *text;
        } stt;
        struct {
            qm_ai_agent_state_t state;
            const char *text;
        } agent_state;
        struct {
            int turn_id;
        } interrupt;
    } data;
} qm_ai_stream_event_t;

typedef enum {
    QM_AI_LINK_EVENT_TYPE_INIT_SUCCESS = 0,
    QM_AI_LINK_EVENT_TYPE_CONNECTED,
    QM_AI_LINK_EVENT_TYPE_CONNECT_LOST,
    QM_AI_LINK_EVENT_TYPE_JOIN_CHANNEL,
    QM_AI_LINK_EVENT_TYPE_LEAVE_CHANNEL,
    QM_AI_LINK_EVENT_TYPE_AGENT_JOINED,
    QM_AI_LINK_EVENT_TYPE_AGENT_LEAVE,
    QM_AI_LINK_EVENT_TYPE_AUDIO_MUTE,
    QM_AI_LINK_EVENT_TYPE_AUDIO_UNMUTE,
    QM_AI_LINK_EVENT_TYPE_ERROR
} qm_ai_link_event_type_e;

typedef void (*qm_ai_audio_cb_t)(void *data, int len, void *user_data);
typedef void (*qm_ai_stream_cb_t)(qm_ai_stream_event_t *event, void *user_data);
typedef void (*qm_ai_link_event_cb_t)(qm_ai_link_event_type_e event, void *user_data);


typedef struct {
    bool audio_enable;
    bool video_enable;

    char* license;

    qm_ai_audio_data_type_t data_type;
    qm_ai_audio_codec_type_t codec_type;
    int sample_rate;
    int channel_num;
    int duration;

    qm_ai_audio_cb_t audio_cb;
    qm_ai_stream_cb_t stream_cb;
    qm_ai_link_event_cb_t event_cb;

    void *user_data;
} qm_ai_rtc_cfg_t;

int qm_ai_rtc_init(qm_ai_rtc_cfg_t cfg);
int qm_ai_rtc_deinit(void);

int qm_ai_rtc_connect(void);
int qm_ai_rtc_disconnect(void);

int qm_ai_rtc_join(void);
int qm_ai_rtc_leave(void);

int qm_ai_rtc_send_audio_data(const void *data, size_t len);

int qm_ai_rtc_clear_buffer_data(void);


#endif  /* __QM_AI_INNER_RTC_API_H__ */