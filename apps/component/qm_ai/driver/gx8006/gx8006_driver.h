#ifndef __GX8006_H
#define __GX8006_H

#include "qm.h"
#include "qm_ai_driver.h"
#include "qm_work.h"

typedef struct {
    uint8_t inited;
    uint8_t mic_send_en;
    qm_work_t sleep;
    qm_ai_driver_read_handler_t mic_read_cb;
    qm_ai_driver_event_handler_t event_cb;
    void *user_data;
}gx8006_info_t;

#endif
