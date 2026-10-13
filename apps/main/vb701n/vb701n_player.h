#ifndef _VB701N_PLAYER_H_
#define _VB701N_PLAYER_H_

#include "qm.h"
#include "qm_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 绑定板级串口，创建 qm_audio。芯片回版本后开麦。 */
qm_err_t vb701n_player_start(void);

/* qm_agent 借用这份句柄，生命周期跟播放器一样长。 */
qm_audio_handle_t vb701n_player_get_audio(void);

#ifdef __cplusplus
}
#endif

#endif
