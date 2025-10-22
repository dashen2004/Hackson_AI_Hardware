/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BSP_PCM_H_
#define _BSP_PCM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define CONFIG_BSP_PCM_TASK_PRIO      (5)
#define CONFIG_BSP_PCM_TASK_SIZE      (20 * 1024)

typedef struct 
{
    int file_len;
    uint8_t *file;
}bsp_pcm_file_t;

typedef enum 
{
    BSP_PCM_ID_MODE_1,
    BSP_PCM_ID_MODE_2,
    BSP_PCM_ID_MODE_3,
    BSP_PCM_ID_NET_CONN,
    BSP_PCM_ID_NET_DISCONN,
    BSP_PCM_ID_DING_DING,
    BSP_PCM_ID_POWER_ON,
    BSP_PCM_ID_RECORD,
    BSP_PCM_ID_READY_SLEEP,
    BSP_PCM_ID_SLEEP,
    BSP_PCM_ID_REASSURE,
    BSP_PCM_ID_DISCONN_REMIND,
    BSP_PCM_ID_XJIANG_TEST,
    BSP_PCM_ID_LOW_BAT_LEVEL,
    BSP_PCM_ID_LOSS_RSSI,

    BSP_PCM_ID_MAX,
}bsp_pcm_id_t;

typedef enum 
{
    BSP_PCM_EVENT_ID_PLAY_START,
    BSP_PCM_EVENT_ID_PLAY_DATA,
    BSP_PCM_EVENT_ID_PLAY_STOP,
    BSP_PCM_EVENT_ID_MAX,
}bsp_pcm_event_id_t;



typedef void (*bsp_pcm_mute_t)(int enable);
typedef void (*bsp_pcm_send_t)(uint8_t *data, int len);

typedef void (*bsp_pcm_event_t)(bsp_pcm_id_t id, bsp_pcm_event_id_t event_id, void *user_data);


typedef struct 
{
    void *user_data;

    bsp_pcm_send_t send;
    bsp_pcm_mute_t mute;
    bsp_pcm_event_t event;

    int channels;
    int sample_rate;
    int duration_ms;
    int enable_opus;
    
}bsp_pcm_params_t;

extern bsp_pcm_file_t bsp_pcm_file[BSP_PCM_ID_MAX];

qm_err_t bsp_pcm_init(bsp_pcm_params_t *params);

qm_err_t bsp_pcm_player(bsp_pcm_id_t pcm_id);

#ifdef __cplusplus
}
#endif

#endif
