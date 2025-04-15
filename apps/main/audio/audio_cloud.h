/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _AUDIO_CLOUD_H_
#define _AUDIO_CLOUD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define DEAFALUT_REPLY_WAV    ("/spiffs/dingding.wav")

qm_err_t audio_cloud_init(void);

#ifdef __cplusplus
}
#endif

#endif
