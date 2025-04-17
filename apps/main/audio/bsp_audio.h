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

typedef struct {
    enum {
        BSP_AUDIO_WAKEUP_CMD,          /*!< Wakeup start */
        BSP_AUDIO_COMM_CMD,            /*!< Wakeup stop */
    } type;                            /*!< sr type */
    int id;
    char *sr_str;
    char *reply_str;
    char *note;
    float prob;
} bsp_audio_cmd_table_t;

qm_err_t bsp_audio_init(bsp_audio_cmd_table_t *command_str, int command_num);

#ifdef __cplusplus
}
#endif

#endif
