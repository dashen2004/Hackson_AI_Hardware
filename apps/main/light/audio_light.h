/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _AUDIO_LIGHT_H_
#define _AUDIO_LIGHT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "qm_spec_api.h"

#define AUDIO_LIGHT_DELAY_OFF_UNIT(X)   (X * 1000 * 60)

//灯源选择
typedef enum
{
    AUDIO_LIGHT_SEL_ALL  = 0,   //双光源
    AUDIO_LIGHT_SEL_DOWN = 1,   //仅下光
    AUDIO_LIGHT_SEL_UP   = 2,   //仅上光
}audio_light_sel_t;

typedef enum
{
    AUDIO_LIGHT_MODE_EXIT  = 0,   //空模式
    AUDIO_LIGHT_MODE_NIGHT = 1,   //小夜灯（上光 8%+下光 8%）
    AUDIO_LIGHT_MODE_READ  = 2,  //阅读模式（上光100%+下光100%）
    AUDIO_LIGHT_MODE_IDLE  = 3,  //休闲模式（上光100%+下光 50%）
    AUDIO_LIGHT_MODE_SLEEP = 4,  //睡觉模式（关灯）
}audio_light_mode_t;

typedef enum
{
    AUDIO_LIGHT_GRAD_UP   = 1,   //亮一点
    AUDIO_LIGHT_GRAD_DOWN = 2,   //暗一点
}audio_light_grad_t;

qm_err_t audio_light_init(void);

qm_err_t audio_light_action(qm_spec_property_t *property);

qm_err_t audio_light_report(qm_spec_property_t *property);

qm_err_t audio_light_action_and_report(qm_spec_property_t *property);

qm_err_t audio_light_snapshot_report(void);

#ifdef __cplusplus
}
#endif

#endif
