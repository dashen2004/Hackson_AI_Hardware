#ifndef __QM_IOT_AGORA_STREAM_H__
#define __QM_IOT_AGORA_STREAM_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "agora_rtc_api.h"

typedef enum {
    QM_AGORA_STREAM_TYPE_TTS = 0,
    QM_AGORA_STREAM_TYPE_STT,
    QM_AGORA_STREAM_TYPE_AGENT_STATE,
    QM_AGORA_STREAM_TYPE_INTERRUPT,
} qm_agora_stream_recv_type_t;
  
typedef enum {
    AGENT_STATE_NONE,
    AGENT_STATE_SILENT,
    AGENT_STATE_SPEAKING,
    AGENT_STATE_THINKING,
    AGENT_STATE_MAX,
} qm_agent_state_t;

typedef struct {
    qm_agora_stream_recv_type_t type;
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
            qm_agent_state_t state;
            const char *text;
        } agent_state;
        struct {
            int turn_id;
        } interrupt;
    } data;
} qm_agora_stream_event_t;

typedef void (*qm_agora_stream_event_cb_t)(qm_agora_stream_event_t *event, void *user_data);

void __on_stream_message(connection_id_t conn_id, uint32_t uid, int stream_id, 
                                const char* data, size_t length, uint64_t sent_ts);

int32_t qm_agora_stream_init(void);

void qm_agora_stream_register(qm_agora_stream_event_cb_t func, void *user_data);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_AGORA_STREAM_H__ */