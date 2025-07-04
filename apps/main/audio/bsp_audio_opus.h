/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BSP_AUDIO_OPUS_H_
#define _BSP_AUDIO_OPUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define MAX_OPUS_PACKET_SIZE 1920

void *bsp_audio_opus_encode_create(int sample_rate, int channels, int duration_ms);

int bsp_audio_opus_decode_data(void *handle, void *in_opus, int opus_size, void *out_pcm);

int bsp_audio_opus_encode_data(void *handle, void *in_pcm, void *out_opus);

qm_err_t bsp_audio_opus_reset_status(void *handle);

qm_err_t bsp_audio_opus_set_complexity(void *handle, int complexity);


#ifdef __cplusplus
}
#endif

#endif
