// Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
// All rights reserved.

#ifndef _QM_G711_DECODER_H_
#define _QM_G711_DECODER_H_

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      G711 Decoder configurations
 */
typedef struct {
    int                     out_rb_size;    /*!< Size of output ringbuffer */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
    bool_t                  stack_in_ext;   /*!< Try to allocate stack in external memory */
    bool_t                  dec_mode;       /*!< 0: a-law  1: u-law */
} qm_g711_decoder_cfg_t;

#define QM_G711_DECODER_TASK_STACK          (3 * 1024)
#define QM_G711_DECODER_TASK_CORE           (0)
#define QM_G711_DECODER_TASK_PRIO           (5)
#define QM_G711_DECODER_RINGBUFFER_SIZE     (2 * 1024)

#define DEFAULT_QM_G711_DECODER_CONFIG() {\
    .out_rb_size        = QM_G711_DECODER_RINGBUFFER_SIZE,\
    .task_stack         = QM_G711_DECODER_TASK_STACK,\
    .task_core          = QM_G711_DECODER_TASK_CORE,\
    .task_prio          = QM_G711_DECODER_TASK_PRIO,\
    .stack_in_ext       = QM_TRUE,\
    .dec_mode           = 0,\
}

/**
 * @brief      Create a handle to an Audio Element to decode incoming data using G711 format
 *
 * @param      config  The configuration
 *
 * @return     The handle
 */
void *qm_g711_decoder_init(qm_g711_decoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif
