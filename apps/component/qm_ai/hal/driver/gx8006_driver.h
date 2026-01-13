
#include "qm.h"
#include "qm_ai_driver.h"

/*
 * operate function of codec
 */
#define QM_AI_DRIVER_GX8006_DEFAULT_HANDLE()    \
{                                               \
    .comm_type = NULL,  \
    .driver_type = NULL,    \
    .arg = NULL,    \
    open =  NULL,   \
    close =  NULL,  \
    write =  NULL,  \
    read =  NULL,   \
    set_mute =  NULL,   \
    get_mute =  NULL,   \
    set_volume = NULL,  \
    get_volume = NULL,  \
    enable_pa = NULL,   \
}
