/*
 * qm_module_interaction.h
 *
 * Copyright (c) 2024
 */

#ifndef __QM_MODULE_INTERACTION_H__
#define __QM_MODULE_INTERACTION_H__

#include <stdint.h>
#include <stdbool.h>
#include "qm.h"
#include "qm_ai_license.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QM_AI_VOICE_CONTROL_REQ_NONE = 0,
    QM_AI_VOICE_CONTROL_REQ_CORPUS,
    QM_AI_VOICE_CONTROL_REQ_ABORT,
} qm_ai_voice_control_cmd_t;

typedef enum {
    QM_AI_VOICE_CONTROL_PARAM_NONE = 0,

    QM_AI_VOICE_CONTROL_PARAM_CORPUS_OPEN = 0x10,
    QM_AI_VOICE_CONTROL_PARAM_CORPUS_CLOSE,

    QM_AI_VOICE_CONTROL_PARAM_ABORT = 0x20,
    QM_AI_VOICE_CONTROL_PARAM_ABORT_WITH_WORD,
} qm_ai_voice_control_param_t;


int32_t qm_ai_voice_control_init(qm_ai_license_config_t *config);
int32_t qm_ai_voice_control_deinit(void);

int32_t qm_ai_voice_control_start(void);
int32_t qm_ai_voice_control_stop(void);

int32_t qm_ai_voice_control_cmd(qm_ai_voice_control_cmd_t cmd, qm_ai_voice_control_param_t param);

#ifdef __cplusplus
}
#endif

#endif // __QM_MODULE_INTERACTION_H__
