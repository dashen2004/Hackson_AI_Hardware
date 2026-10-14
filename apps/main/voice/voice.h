#ifndef VOICE_H
#define VOICE_H

#include "qm.h"
#include "qm_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 创建 qm_audio。芯片回版本后开麦。 */
qm_err_t voice_start(void);

/* qm_agent 借用这份句柄，生命周期跟语音会话一样长。 */
qm_audio_handle_t voice_get_audio(void);

typedef void (*voice_wakeup_cb_t)(void);
void voice_set_wakeup_cb(voice_wakeup_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif
