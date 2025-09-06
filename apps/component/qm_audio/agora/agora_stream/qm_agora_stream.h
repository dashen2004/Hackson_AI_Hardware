#ifndef __QM_IOT_AGORA_STREAM_H__
#define __QM_IOT_AGORA_STREAM_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

typedef enum {
    QM_AGORA_STREAM_TYPE_TTS = 0,
    QM_AGORA_STREAM_TYPE_STT,
    QM_AGORA_STREAM_TYPE_AGENT_STATE,
    QM_AGORA_STREAM_TYPE_INTERRUPT,
} qm_agora_stream_recv_type_t;

typedef struct {
    qm_agora_stream_recv_type_t type;
    union {
        struct {
            const char *text;
            uint32_t text_len;
            uint32_t duration_ms;
        } tts;
        struct {
            const char *text;
        } stt;
        struct {
            const char *state;
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