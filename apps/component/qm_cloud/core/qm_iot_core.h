#ifndef __QM_IOT_CORE_H__
#define __QM_IOT_CORE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_iot_api.h"
#include "qm_spec_core.h"

#ifndef CONFIG_QM_IOT_CORE_EVENT_ID
#define CONFIG_QM_IOT_CORE_EVENT_ID             (0xF000)
#endif

#ifndef CONFIG_QM_IOT_CORE_TASK_USER_CORE
#define CONFIG_QM_IOT_CORE_TASK_USER_CORE       (1)
#endif

#ifndef CONFIG_QM_IOT_CORE_TASK_PRO
#define CONFIG_QM_IOT_CORE_TASK_PRO             (10)
#endif

#ifndef CONFIG_QM_IOT_CORE_TASK_SIZE
#define CONFIG_QM_IOT_CORE_TASK_SIZE           (8144)
#endif

#ifndef CONFIG_QM_IOT_CORE_TASK_QUEUE_NUM
#define CONFIG_QM_IOT_CORE_TASK_QUEUE_NUM       (10)
#endif

#ifndef CONFIG_QM_IOT_CORE_DYNREG_PORT
#define CONFIG_QM_IOT_CORE_DYNREG_PORT         (17605)
#endif

#ifndef CONFIG_QM_IOT_CORE_DYNREG_RECVTIMEOUT
#define CONFIG_QM_IOT_CORE_DYNREG_RECVTIMEOUT  (10 * 1000)
#endif

#ifndef CONFIG_QM_IOT_CORE_DYNREG_RETRY_COUNT
#define CONFIG_QM_IOT_CORE_DYNREG_RETRY_COUNT   (5)
#endif

#ifndef CONFIG_QM_IOT_CORE_MQTT_KEEPLIVE_S
#define CONFIG_QM_IOT_CORE_MQTT_KEEPLIVE_S      (60)
#endif

#ifndef CONFIG_QM_IOT_CORE_MQTT_REPUB_MS
#define CONFIG_QM_IOT_CORE_MQTT_REPUB_MS        (1 * 1000)
#endif

#ifndef CONFIG_QM_IOT_CORE_MQTT_CID_LEN
#define CONFIG_QM_IOT_CORE_MQTT_CID_LEN        (128)
#endif

#ifndef CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S
#define CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S      (30 * 60)
#endif


typedef enum
{
    QM_IOT_CORE_CTRL_NONE = 0,

    QM_IOT_CORE_CTRL_CONFIG_NET,
    QM_IOT_CORE_CTRL_CONNECT_NET,

    QM_IOT_CORE_CTRL_DISCONNECT_NET,

    // char **
    QM_IOT_CORE_CTRL_GET_DEV_PARAMS,

    // char **
    QM_IOT_CORE_CTRL_GET_DYNREG_PARAMS,

}qm_iot_core_ctrl_t;

typedef enum
{
    QM_IOT_CORE_EVENT_NONE = 0,

    QM_IOT_CORE_EVENT_OPEN,
    QM_IOT_CORE_EVENT_CLOSE,

    QM_IOT_CORE_EVENT_NETCONFG_START,
    QM_IOT_CORE_EVENT_NETCONFG_COMPLETE,
    QM_IOT_CORE_EVENT_NETCONFG_STOP,

    QM_IOT_CORE_EVENT_CONNECTED_NET,
    QM_IOT_CORE_EVENT_DISCONNECT_NET,
}qm_iot_core_net_event_type_t;

typedef struct 
{
    qm_iot_net_type_t net_type;
    int (*open)(void); 
    int (*close)(void);
    int (*start)(void); 
    int (*stop)(void);
    int (*ctrl) (qm_iot_core_ctrl_t cmd, void *param);
}qm_iot_core_net_handle_t;

typedef void (*qm_iot_core_net_event_t)(qm_iot_net_type_t net_type, qm_iot_core_net_event_type_t event, void *arg);

qm_iot_core_net_handle_t *qm_iot_core_net_get_handle(qm_iot_net_type_t net_type);

int qm_iot_core_net_handle_regsiter(qm_iot_net_type_t net_type, qm_iot_core_net_handle_t *handler,
                                        qm_iot_core_net_event_t event_handler, void *arg);

int qm_iot_core_net_handle_event_notify(qm_iot_net_type_t net_type, qm_iot_core_net_event_type_t event);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_CORE_CORE_H__ */