#ifndef __QM_IOT_AGORA_AGORA_H__
#define __QM_IOT_AGORA_AGORA_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_agora_stream.h"

typedef enum {
    QM_AGORA_EVENT_TYPE_INIT_SUCCESS = 0,
    QM_AGORA_EVENT_TYPE_CONNECTED,
    QM_AGORA_EVENT_TYPE_CONNECT_LOST,
    QM_AGORA_EVENT_TYPE_JOIN_CHANNEL,
    QM_AGORA_EVENT_TYPE_LEAVE_CHANNEL,
    QM_AGORA_EVENT_TYPE_AGENT_JOINED,
    QM_AGORA_EVENT_TYPE_AGENT_LEAVE,
    QM_AGORA_EVENT_TYPE_AUDIO_MUTE,
    QM_AGORA_EVENT_TYPE_AUDIO_UNMUTE,
    QM_AGORA_EVENT_TYPE_ERROR
} qm_agora_event_type_e;

typedef struct {
    qm_agora_event_type_e type;
} qm_agora_event_t;

typedef void (*qm_agora_audio_event_cb_t)(void *data, int len, void *user_data);
typedef void (*qm_agora_event_cb_t)(qm_agora_event_type_e event, void *user_data);

typedef struct {
    int duration;
    bool audio_enable;
    bool video_enable;
    bool domain_limit;
    qm_agora_stream_event_cb_t stream_cb;
    qm_agora_audio_event_cb_t audio_cb;
    qm_agora_event_cb_t event_cb;
    void *user_data;
} qm_agora_cfg_t;

int qm_agora_init(qm_agora_cfg_t cfg);

int qm_agora_connect(void);
int qm_agora_disconnect(void);

int qm_agora_join(void);
int qm_agora_leave(void);

int qm_agora_send_data(const void *data, size_t len);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_AGORA_AGORA_H__ */