#ifndef __QM_AUDIO_H__
#define __QM_AUDIO_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm_types.h"
#include "qm_errno.h"

typedef enum {
    QM_AUDIO_TYPE_NONE = 0,
    QM_AUDIO_TYPE_AGENT,
    QM_AUDIO_TYPE_MAX,
}qm_audio_type_t;

typedef enum {
    QM_AUDIO_STATE_NONE = 0,
    QM_AUDIO_STATE_SLIENT,
    QM_AUDIO_STATE_SPEAK,
    QM_AUDIO_STATE_MAX,
}qm_audio_state_t;

typedef enum{
    QM_AUDIO_EVENT_NONE = 0,
    QM_AUDIO_EVENT_PLAY_START,
    QM_AUDIO_EVENT_PLAY_STOP,
    QM_AUDIO_EVENT_PLAY_PAUSE,
    QM_AUDIO_EVENT_PLAY_MUTE,
    QM_AUDIO_EVENT_PLAY_COMPLETED,
    QM_AUDIO_EVENT_PLAY_EXIT,
    QM_AUDIO_EVENT_MAX,
}qm_audio_event_t;

typedef int (*qm_audio_play_t)(qm_audio_type_t type, uint8_t *data, int len, void *user_data);
typedef int (*qm_audio_mute_t)(void *arg);
typedef int (*qm_audio_init_t)(void *arg);
typedef int (*qm_audio_deinit_t)(void *arg);
typedef int (*qm_audio_eventcb_t)(qm_audio_event_t event, void *param);

typedef struct {
    int duration_ms;
    qm_audio_init_t audio_init;
    qm_audio_deinit_t audio_deinit;
    qm_audio_play_t audio_play;
    qm_audio_mute_t audio_mute;
    qm_audio_eventcb_t audio_eventcb;
    void *arg;
}qm_audio_param_t;

int qm_audio_init(qm_audio_param_t *audio_param);

int qm_audio_data_push(qm_audio_type_t type, uint8_t *data, int len);

int qm_audio_play(qm_audio_type_t type);

int qm_audio_mute(void);

int qm_audio_stop(void);

int qm_audio_deinit(void);


#if defined(__cplusplus)
}
#endif

#endif  /* __QM_AUDIO_H__ */
