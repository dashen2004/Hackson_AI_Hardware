#include "qm.h"
#include "qm_g711_decoder.h"

#include "g711_decoder.h"

void *qm_g711_decoder_init(qm_g711_decoder_cfg_t *config)
{
    g711_decoder_cfg_t cfg = {
        .dec_mode = config->dec_mode,
        .out_rb_size = config->out_rb_size,
        .stack_in_ext = config->stack_in_ext,
        .task_core = config->task_core,
        .task_prio = config->task_prio,
        .task_stack = config->task_stack,
    };

    return (audio_element_handle_t)g711_decoder_init(&cfg);
}

