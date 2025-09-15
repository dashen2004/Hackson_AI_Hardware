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

#define DEAFALUT_REPLY_WAV              ("/spiffs/dingding.wav")
#define NETCONFIG_WAV                   ("/spiffs/1.wav")
#define CONN_CLOUD_WAV                  ("/spiffs/2.wav")
#define CONN_ERROR_WAV                  ("/spiffs/3.wav")
#define XJIANG_TEST_WAV                 ("/spiffs/4.wav")

#if 0
#define MODE_SWITCH_WAV                 ("/spiffs/7.wav")
#define MODE_SWITCH_COMPLETE_WAV        ("/spiffs/8.wav")

#define MODE_SWITCH_AUTO_WAV            ("/spiffs/9.wav")
#define MODE_SWITCH_MANUAL_WAV          ("/spiffs/10.wav")
#define MODE_SWITCH_REALTIME_WAV        ("/spiffs/11.wav")
#endif

typedef enum {
    PCM_DINGDING_ID = 0,
    PCM_NETCONFIG_ID,
    PCM_CONN_CLOUD_ID,
    PCM_CONN_ERROR_ID,
    PCM_XJIANG_TEST_ID,
} PCM_FILE_ID_t;

#define WAKEUP_TIEMOUT_PLAYER_PATH      (DEAFALUT_REPLY_WAV)

// #define TEST_PLAYER_PATH     ("/spiffs/test.wav")

qm_err_t audio_cloud_init(void);

void audio_cloud_event_post(int audio_event);

void qm_bsp_offline_voice_play(PCM_FILE_ID_t pcm_id);
uint8_t qm_bsp_voice_is_idle(void);
#ifdef __cplusplus
}
#endif

#endif
