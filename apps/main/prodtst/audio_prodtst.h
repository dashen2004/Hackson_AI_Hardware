#ifndef _AUDIO_PRODTST_H_
#define _AUDIO_PRODTST_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "qm_audio_processor.h"

#define FLASH_SECTOR_KEY                                     "fac_flg"

#define WIFI_SCAN_TIMEOUT_MS                                    (6000)

int audio_prodtst_init(void);
int prodtst_config_get(void);
qm_err_t prodtst_rec_engine_cb(audio_rec_evt_t *event, void *user_data);
#ifdef __cplusplus
}
#endif

#endif
