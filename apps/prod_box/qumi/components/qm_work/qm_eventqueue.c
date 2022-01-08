#include "qm_eventqueue.h"
#include "qm_kernel.h"
#include "qm_utils_list.h"
#include "qm_errno.h"
#include "qm_work.h"

extern int32_t qm_queue_info_send(uint8_t *queue_type, void *data, uint32_t len);

typedef struct{
    uint8_t used;
    uint16_t type; 
    qm_event_cb cb;
    void *arg;
}qm_event_node_t;

typedef struct{
    qm_mutex_t lock;
    qm_event_node_t event_node[CONFIG_QM_EVENT_NUM];
}qm_eventqueue_t;

static qm_eventqueue_t g_eventqueue = {0};

int32_t qm_eventqueue_init(void)
{

    qm_err_t ret = QM_EOK;
    memset(&g_eventqueue, 0, sizeof(qm_eventqueue_t));
    ret = qm_mutex_new(&g_eventqueue.lock);
    return ret;
}

int32_t qm_event_handle(qm_input_event_t *event)
{
    uint16_t i = 0;
    qm_mutex_lock(&g_eventqueue.lock, QM_WAIT_FOREVER);
    for(i = 0; i < CONFIG_QM_EVENT_NUM; i++){
        if(g_eventqueue.event_node[i].used == 1){
            if(g_eventqueue.event_node[i].type == event->type){
                qm_mutex_unlock(&g_eventqueue.lock);
                g_eventqueue.event_node[i].cb(event, g_eventqueue.event_node[i].arg);
                return QM_EOK;
            }
        }
    }
    qm_mutex_unlock(&g_eventqueue.lock);
    return -QM_EINVAL;
}

int32_t qm_event_register(uint16_t type, qm_event_cb cb, void *arg)
{
    uint16_t i = 0;
    if(cb == NULL){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&g_eventqueue.lock, QM_WAIT_FOREVER);
    for(i = 0; i < CONFIG_QM_EVENT_NUM; i++){
        if(g_eventqueue.event_node[i].used == 0){

            g_eventqueue.event_node[i].used = 1;
            g_eventqueue.event_node[i].arg = arg;
            g_eventqueue.event_node[i].cb = cb;
            g_eventqueue.event_node[i].type = type;

            qm_mutex_unlock(&g_eventqueue.lock);
            return QM_EOK;
        }
    }
    qm_mutex_unlock(&g_eventqueue.lock);

    return -QM_EFULL;
}

int32_t qm_event_unregister(uint16_t type, qm_event_cb cb, void *arg)
{
    uint16_t i = 0;
    if(cb == NULL){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&g_eventqueue.lock, QM_WAIT_FOREVER);
    for(i = 0; i < CONFIG_QM_EVENT_NUM; i++){
        if(g_eventqueue.event_node[i].used == 1){

            if((g_eventqueue.event_node[i].type == type) && 
            (g_eventqueue.event_node[i].cb == cb ) && 
            (g_eventqueue.event_node[i].arg == arg )){
                g_eventqueue.event_node[i].used = 0;
                qm_mutex_unlock(&g_eventqueue.lock);
                return QM_EOK;
            }
        }
    }
    qm_mutex_unlock(&g_eventqueue.lock);

    return QM_EOK;
}

int32_t qm_event_post(uint16_t type, uint16_t code, uint32_t value)
{

    qm_input_event_t event = {0};

    event.time = qm_now_ms();
    event.type = type;
    event.code = code;
    event.value = value;

    return qm_queue_info_send(QM_QUEUE_EVENT_TYPE, (void*)&event, sizeof(qm_input_event_t));
}



