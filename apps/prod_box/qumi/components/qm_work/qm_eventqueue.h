#ifndef __QM_EVENT_QUEUE_H__
#define __QM_EVENT_QUEUE_H__

#include "qm_types.h"

#ifndef CONFIG_QM_EVENT_NUM 
#define CONFIG_QM_EVENT_NUM    10
#endif   


typedef struct {
    /* The time event is generated, auto filled */
    uint32_t time;
    /* Event type, value < 0x1000 are used by qmos system */
    uint16_t type;
    /* Defined according to type */
    uint16_t code;
    /* Defined according to type/code */
    uint32_t value;
} qm_input_event_t;

/* Event callback */
typedef void (*qm_event_cb)(qm_input_event_t *event, void *arg);


/**
 * Register system event filter callback.
 *
 * @param[in]  type  event type interested.
 * @param[in]  cb    system event callback.
 * @param[in]  arg   private data past to cb.
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_register(uint16_t type, qm_event_cb cb, void *arg);

/**
 * Unregister native event callback.
 *
 * @param[in]  type  event type interested.
 * @param[in]  cb    system event callback.
 * @param[in]  arg   private data past to cb.
 *
 * @return  the operation status, 0 is OK, others is error.
 */
int32_t qm_event_unregister(uint16_t type, qm_event_cb cb, void *arg);

/**
 * Post local event.
 *
 * @param[in]  type   event type.
 * @param[in]  code   event code.
 * @param[in]  value  event value.
 *
 * @return  the operation status, 0 is OK,others is error.
 */
int32_t qm_event_post(uint16_t type, uint16_t code, uint32_t value);


#endif
