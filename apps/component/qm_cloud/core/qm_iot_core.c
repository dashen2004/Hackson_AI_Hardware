#include "qm.h"
#include "qm_work.h"
#include "qm_iot_api.h"
#include "qm_iot_core.h"
#include "qm_iot_config.h"


#define LOG_TAG "IOT_WIFI"

typedef struct 
{
    void *arg;
    qm_iot_core_net_handle_t *handler;
    qm_iot_core_net_event_t event_handler;
}qm_iot_core_net_t;



static qm_iot_core_net_t net_handle_list[QM_IOT_NET_TYPE_MAX] = {0};

qm_iot_core_net_handle_t *qm_iot_core_net_get_handle(qm_iot_net_type_t net_type)
{
    if(net_type == QM_IOT_NET_TYPE_NONE || net_type == QM_IOT_NET_TYPE_MAX){
        return NULL;
    }
    return net_handle_list[net_type - 1].handler;
}

int qm_iot_core_net_handle_regsiter(qm_iot_net_type_t net_type, qm_iot_core_net_handle_t *handler,
                                        qm_iot_core_net_event_t event_handler, void *arg)
{
    if(handler == NULL || net_type == QM_IOT_NET_TYPE_NONE || net_type == QM_IOT_NET_TYPE_MAX){
        return QM_EINVAL;
    }
    net_handle_list[net_type - 1].handler = handler;
    net_handle_list[net_type - 1].event_handler = event_handler;
    net_handle_list[net_type - 1].arg = arg;
    return QM_EOK;
}

int qm_iot_core_net_handle_event_notify(qm_iot_net_type_t net_type, qm_iot_core_net_event_type_t event)
{
    if(net_type == QM_IOT_NET_TYPE_NONE || net_type == QM_IOT_NET_TYPE_MAX){
        return QM_EINVAL;
    }

    if(net_handle_list[net_type - 1].event_handler != NULL){
        net_handle_list[net_type - 1].event_handler(net_type, event, net_handle_list[net_type - 1].arg);
    }

    return QM_EOK;
}