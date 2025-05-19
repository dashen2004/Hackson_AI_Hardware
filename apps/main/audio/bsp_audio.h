/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BSP_AUDIO_H_
#define _BSP_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "qm_audio_sr.h"
#include "qm_audio_ws.h"

typedef struct {
    enum {
        BSP_AUDIO_WAKEUP_CMD = 1,          /*!< Wakeup start */
        BSP_AUDIO_COMM_CMD,            /*!< Wakeup stop */
    } type;                            /*!< sr type */
    int id;
    char *sr_str;
    char *note;
    char *reply_str;
    float prob;    //normal_prob,
} bsp_audio_cmd_table_t;


qm_err_t bsp_audio_set_voice_mute(bool_t enable);

bool_t bsp_audio_get_voice_mute(void);

qm_err_t bsp_audio_init(bsp_audio_cmd_table_t *command_str, int command_num);

qm_err_t bsp_audio_resgiter(qm_audio_ws_recv_event_cb_t event_cb, void *user_data);


#ifdef __cplusplus
}
#endif

#endif
