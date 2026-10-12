#ifndef _VB701N_PLAYER_H_
#define _VB701N_PLAYER_H_

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 绑定板级串口，创建 qm_audio，芯片回版本后播放 Opus。 */
qm_err_t vb701n_player_start(void);

#ifdef __cplusplus
}
#endif

#endif
