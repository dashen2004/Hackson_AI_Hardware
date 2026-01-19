#include "qm.h"
#include "qm_work.h"
#include "qm_iot_api.h"
#include "qm_iot_core.h"
#include "qm_iot_config.h"
#include "qm_spec_api.h"
#include "qm_spec_core.h"
#include "qm_utils_list.h"

#if CONFIG_QM_IOT_PROV_SUPPORT
#include "qm_iot_prov.h"
#endif

#define LOG_TAG "IOT_CORE"

#define CONFIG_QM_IOT_SERVICE_SUPPORT           (1)  
#define CONFIG_QM_IOT_RESET_DELAY_MS            (200)
#define CONFIG_QM_IOT_RESET_FORCE_MS            (5000)

#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
#include "qm_iot_weather.h"
#define CONFIG_QM_IOT_WEATHER_DELAY_MS          (2000)
#endif

#if CONFIG_QM_IOT_OTA_SUPPORT
#include "qm_iot_ota.h"
#endif

#if CONFIG_QM_IOT_DEVMNT_SUPPORT
#include "qm_iot_dev.h"
#endif

#if CONFIG_QM_IOT_DYNREG_SUPPORT
#include "qm_iot_dynreg.h"
#endif

#if CONFIG_QM_IOT_NTP_SUPPORT
#include "qm_time.h"
#include "qm_iot_ntp.h"
#endif 

#if CONFIG_QM_IOT_PUB_SUPPORT
#include "qm_iot_pub.h"
#include "qm_iot_shadow.h"
#define CONFIG_QM_IOT_PUB_DELAY_MS              (100)
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT
#include "qm_iot_mqtt.h"
#define QM_IOT_CORE_MQTT_PORT   (8883)
#define QM_IOT_CORE_MQTT_CLIENT_ID_PARAMS       "%ld_%s" //pid_sn

#endif

#if CONFIG_QM_IOT_SERVICE_SUPPORT
typedef struct
{
    int    service_id;
    qm_iot_service_handler_t service_fn;
    void *userdata;
    int service_num;
    uint8_t raw_iid[1];
}qm_iot_core_service_t;
#endif

typedef struct 
{

    qm_iot_cloud_info_t cloud_info;

    char ca_crt[CONFIG_QM_IOT_CRT_MAX_LEN + 1];
    char client_crt[CONFIG_QM_IOT_CRT_MAX_LEN + 1];
    char client_key[CONFIG_QM_IOT_CRT_MAX_LEN + 1];
    char ota_signatrure[CONFIG_QM_IOT_OTA_CRT_MAX_LEN + 1];
}qm_iot_core_crt_handle_t;

typedef struct 
{
    int exit;
    int status;
    qm_work_t post_work;
    qm_iot_reset_info_t reset_info;

#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
    void *weather_handle;
#endif

#if CONFIG_QM_IOT_OTA_SUPPORT
    void *ota_handle;
#endif
    
#if CONFIG_QM_IOT_PUB_SUPPORT
    void *pub_handle;
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT
    int use_mqtt;
    void *mqtt_handle;
#endif

#if CONFIG_QM_IOT_DEVMNT_SUPPORT
    void *dev_handle;
#endif

#if CONFIG_QM_IOT_NTP_SUPPORT
    int8_t timezone;
    void *ntp_handle;
#endif

#if CONFIG_QM_IOT_DYNREG_SUPPORT
    void *dynreg_handle;
    uint8_t dynreg_enable;
    char *product_secret;
#endif
    
    qm_work_t core_req;

    qm_mutex_t lock;
    qm_task_t core_task;
    qm_iot_core_crt_handle_t crt_handle;

    qm_iot_net_type_t net_type;
    void *user_data;
    qm_iot_recv_handler_t recv_handler;
    qm_iot_event_handler_t event_handler;

#if CONFIG_QM_IOT_SERVICE_SUPPORT
    qm_list_t *service;
#endif
}qm_iot_core_handle_t;

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
extern qm_iot_core_net_handle_t *qm_iot_core_4g_get_handle(void);
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI_NORMAL
extern qm_iot_core_net_handle_t *qm_iot_core_wifi_get_handle(void);
#endif

#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
extern qm_iot_core_net_handle_t *qm_iot_core_wifi_combo_get_handle(void);
#endif

static void qm_iot_prov_timeout_notify(void *arg);
static void qm_iot_event_post(qm_iot_core_handle_t *core_handle, qm_iot_event_t *event);
static int qm_iot_net_type_set(qm_iot_core_handle_t *core_handle, qm_iot_net_type_t net_type);

#if CONFIG_QM_IOT_SERVICE_SUPPORT
static int service_map_match(void *a, void *b);
#endif

#if CONFIG_QM_IOT_PUB_SUPPORT
static void qm_iot_core_pub_process(qm_iot_core_handle_t *core_handle);
#endif
#if CONFIG_QM_IOT_OTA_SUPPORT
static void qm_iot_core_ota_process(qm_iot_core_handle_t *core_handle);
#endif
static void qm_iot_unbind_notify(qm_iot_core_handle_t *core_handle);
#if CONFIG_QM_IOT_DEVMNT_SUPPORT
static void qm_iot_core_dev_process(qm_iot_core_handle_t *core_handle);
#endif
#if CONFIG_QM_IOT_NTP_SUPPORT
static void qm_iot_core_ntp_process(qm_iot_core_handle_t *core_handle);
#endif
#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
static void qm_iot_core_weather_process(qm_iot_core_handle_t *core_handle);
#endif
#if CONFIG_QM_IOT_MQTT_SUPPORT
static void qm_iot_core_conn_destory(qm_iot_core_handle_t *core_handle);
static void qm_iot_core_conn_mqtt_process(qm_iot_core_handle_t *core_handle);
#endif
#if CONFIG_QM_IOT_DYNREG_SUPPORT
static void qm_iot_dynreg_recv_handler(void *handle, const qm_iot_dynreg_recv_t *packet, void *userdata);
static void qm_iot_core_dynreg_process(qm_iot_core_handle_t *core_handle);
#endif

static void qm_iot_core_task(void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;

    if(core_handle == NULL){
        return;
    }

    #if CONFIG_QM_IOT_DYNREG_SUPPORT
        if(core_handle->dynreg_enable){
            qm_iot_core_dynreg_process(core_handle);
        }else{
    #endif
            core_handle->status = QM_IOT_EVENT_CONNING_CLOUD;
            qm_iot_event_notify(core_handle, core_handle->status);
    #if CONFIG_QM_IOT_DYNREG_SUPPORT
        }
    #endif
    
    core_handle->use_mqtt = QM_TRUE;

#if CONFIG_QM_IOT_MQTT_SUPPORT
    while(!core_handle->exit)
    {
        if(!core_handle->use_mqtt){
            qm_msleep(CONFIG_QM_IOT_MQTT_YIELD_TIME);
            continue;
        }
        qm_iot_core_conn_mqtt_process(core_handle);
    }
#endif

    qm_task_exit(NULL);
    core_handle->core_task.hdl = NULL;
}

static void qm_iot_core_net_event_handler(qm_iot_net_type_t net_type, qm_iot_core_net_event_type_t event, void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    if(core_handle == NULL){
        return;
    }

    switch (event)
    {

        case QM_IOT_CORE_EVENT_OPEN:

        break;

        case QM_IOT_CORE_EVENT_NETCONFG_COMPLETE:
            core_handle->status = QM_IOT_EVENT_CONNING_LOCAL;
            qm_iot_event_notify(core_handle, core_handle->status);
        break;
        

        case QM_IOT_CORE_EVENT_CONNECTED_NET:
            core_handle->status = QM_IOT_EVENT_LOCAL_CONN;
            qm_iot_event_notify(core_handle, core_handle->status);
        break;
        
        case QM_IOT_CORE_EVENT_DISCONNECT_NET:

        #if CONFIG_QM_IOT_MQTT_SUPPORT
            qm_iot_core_conn_destory(core_handle);
        #endif
            core_handle->status = QM_IOT_EVENT_LOCAL_DISCONN;
            qm_iot_event_notify(core_handle, core_handle->status);
        break;
        break;

        default:

        break;
    }
}

static void qm_iot_core_event_handler(qm_input_event_t *input_event, void *arg)
{
    int ret = QM_EOK;
    qm_iot_event_t  event = {0};
    qm_iot_core_net_handle_t *net = NULL;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)input_event->arg;

    if(input_event->event != CONFIG_QM_IOT_CORE_EVENT_ID){
        return;
    }

    net = qm_iot_core_net_get_handle(core_handle->net_type);
    event.type = (qm_iot_event_type_t)input_event->sub_event;
    
    switch (input_event->sub_event)
    {
        case QM_IOT_EVENT_CONFIG_TIMEOUT:
            if(net && net->stop){
                net->stop();
            }
            qm_iot_event_post(core_handle, &event);
        break;
        
        case QM_IOT_EVENT_CONFIG:
            if(net && net->ctrl){
                net->ctrl(QM_IOT_CORE_CTRL_CONFIG_NET, NULL);
            }
            qm_iot_event_post(core_handle, &event);
        break;

        case QM_IOT_EVENT_LOCAL_DISCONN:
            core_handle->use_mqtt = QM_FALSE;
            qm_iot_event_post(core_handle, &event);
        break;

        case QM_IOT_EVENT_CONNING_LOCAL:
            if(net && net->ctrl){
                net->ctrl(QM_IOT_CORE_CTRL_CONNECT_NET, NULL);
            }
            qm_iot_event_post(core_handle, &event);
        break;

        case QM_IOT_EVENT_CLOUD_CONN:  
        case QM_IOT_EVENT_CONNING_CLOUD:
        case QM_IOT_EVENT_CLOUD_DISCONN:
            qm_iot_event_post(core_handle, &event);
        break;

        case QM_IOT_EVENT_LOCAL_CONN:
            if(!core_handle->core_task.hdl){
                ret = qm_task_new(&core_handle->core_task, "core_task", qm_iot_core_task, (void *)core_handle, CONFIG_QM_IOT_CORE_TASK_SIZE, CONFIG_QM_IOT_CORE_TASK_PRO);
                if(ret != QM_EOK){
                    QM_LOGE(LOG_TAG, "CORE TASK CREATE FAILED!!");
                    break;
                }
            }
        break;

        default:
            break;
    }
}

int32_t qm_iot_deinit(void **handle)
{
    return -QM_EINVAL;
}

void *qm_iot_init(void)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)qm_malloc(sizeof(qm_iot_core_handle_t));
    if(core_handle == NULL){
        return NULL;
    }
    memset(core_handle, 0, sizeof(qm_iot_core_handle_t));

    core_handle->status = QM_IOT_EVENT_CONNING_LOCAL;

#if CONFIG_QM_IOT_NETWORK_TYPE_4G
    qm_iot_core_net_handle_regsiter(QM_IOT_NET_TYPE_4G, 
                                        qm_iot_core_4g_get_handle(), 
                                        qm_iot_core_net_event_handler, core_handle);
#endif
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI
    qm_iot_core_net_handle_regsiter(QM_IOT_NET_TYPE_WIFI_COMBO, 
                                    qm_iot_core_wifi_combo_get_handle(),
                                        qm_iot_core_net_event_handler, core_handle);
#endif
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI_NORMAL
    qm_iot_core_net_handle_regsiter(QM_IOT_NET_TYPE_WIFI, 
                                    qm_iot_core_wifi_get_handle(),
                                        qm_iot_core_net_event_handler, core_handle);
#endif

    qm_mutex_new(&core_handle->lock);

 #if CONFIG_QM_IOT_SERVICE_SUPPORT
    core_handle->service = qm_list_new();
    if(core_handle->service == NULL){
        goto __error;
    }
    core_handle->service->match = service_map_match;
#endif

    qm_event_register(CONFIG_QM_IOT_CORE_EVENT_ID, qm_iot_core_event_handler, (void *)core_handle);

    return core_handle;

__error: 

    if(qm_mutex_is_valid(&core_handle->lock)) {
        qm_mutex_free(&core_handle->lock);
    }

    if(core_handle){
        qm_free(core_handle);
    }

    return NULL;
}

int32_t qm_iot_setopt(void *handle, qm_iot_option_t option, void *data)
{
    int ret = QM_EOK;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL || data == NULL){
        return -QM_EINVAL;
    }

    switch (option)
    {
        case QM_IOT_OPT_VERSION:
            ret = qm_iot_version_set(*((uint32_t *)data));
        break;

        case QM_IOT_OPT_PRODUCT_ID:
            ret = qm_iot_pid_set(*((uint32_t *)data));
        break;

        case QM_IOT_OPT_PRODUCT_SECRET:
            ret = qm_iot_prodtuct_sercet_set((char *)data, strlen((char *)data));
        break;

        case QM_IOT_OPT_NET_TYPE:
            core_handle->net_type = *((qm_iot_net_type_t *)data);
        break;

        case QM_IOT_OPT_RECV_HANDLER: 
            core_handle->recv_handler = (qm_iot_recv_handler_t)data;
        break;

        case QM_IOT_OPT_EVENT_HANDLER:
            core_handle->event_handler = (qm_iot_event_handler_t)data;
        break;

        case QM_IOT_OPT_USERDATA:
            core_handle->user_data = (void *)data;
        break;

        default:
            ret = -QM_EINVAL;
        break;
    }
    return ret;
}

int32_t qm_iot_start(void *handle)
{
    uint32_t did = 0;
    int ret = QM_EOK;
    uint32_t len = 0;
    qm_iot_core_net_handle_t *net = NULL;

    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }

    if(core_handle->net_type == QM_IOT_NET_TYPE_NONE){
        #if CONFIG_QM_IOT_NETWORK_TYPE_4G
            core_handle->net_type = QM_IOT_NET_TYPE_4G;
        #elif CONFIG_QM_IOT_NETWORK_TYPE_WIFI
            core_handle->net_type = QM_IOT_NET_TYPE_WIFI_COMBO;
        #else
            QM_LOGE(LOG_TAG, "NETWORK TYPE NONE!!");    
            return -QM_EINVAL;
        #endif
    }

    ret = qm_iot_net_type_set(core_handle, core_handle->net_type);
    if(ret != QM_EOK){
        QM_LOGE(LOG_TAG, "NETWORK SET FAILED!!");
        return ret;
    }

    memset(&core_handle->crt_handle, 0, sizeof(qm_iot_core_crt_handle_t));
#if CONFIG_QM_IOT_DYNREG_SUPPORT && CONFIG_QM_IOT_MQTT_SUPPORT
    core_handle->crt_handle.cloud_info.port = QM_IOT_CORE_MQTT_PORT;
#endif

    qm_iot_reset_info_get(&core_handle->reset_info);
    if(core_handle->reset_info.is_reset != QM_IOT_NOT_RESET){
        QM_LOGD(LOG_TAG, "DEVICE RESET!!");
        goto __next;
    }

#if CONFIG_QM_IOT_DYNREG_SUPPORT && CONFIG_QM_IOT_MQTT_SUPPORT
   did = qm_iot_did_get();
    if(did <= 0){
        QM_LOGW(LOG_TAG, "DID GET FAILED!!");
        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }

    ret = qm_iot_cloud_info_get(&core_handle->crt_handle.cloud_info);
    if(ret != QM_EOK){
        QM_LOGW(LOG_TAG, "CLOUD GET FAILED!!");
        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }

    len = CONFIG_QM_IOT_CRT_MAX_LEN;
    ret = qm_iot_server_crt_get(core_handle->crt_handle.ca_crt, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "ROOT CA GET FAILED!!");
        core_handle->reset_info.is_reset = QM_TRUE;

        goto __next;
    }

    len = CONFIG_QM_IOT_CRT_MAX_LEN;
    ret = qm_iot_client_crt_get(core_handle->crt_handle.client_crt, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "CLIENT CA GET FAILED!!");

        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }

    len = CONFIG_QM_IOT_CRT_MAX_LEN;
    ret = qm_iot_client_key_get(core_handle->crt_handle.client_key, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "CLIENT KEY GET FAILED!!");

        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }


    len = CONFIG_QM_IOT_OTA_CRT_MAX_LEN;
    ret = qm_iot_ota_public_key_get(core_handle->crt_handle.ota_signatrure, &len);
    if(ret != QM_EOK || len == 0){
        QM_LOGW(LOG_TAG, "PUBLIC KEY GET FAILED!!");

        core_handle->reset_info.is_reset = QM_TRUE;
        goto __next;
    }
#endif

__next:
#if CONFIG_QM_IOT_DYNREG_SUPPORT
    core_handle->product_secret = qm_iot_prodtuct_sercet_get();
#endif

    net = qm_iot_core_net_get_handle(core_handle->net_type);
    if(net && net->start){
        ret = net->start();
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "NETWORK START FAILED!!");
            return ret;
        }
    }

    if(!core_handle->reset_info.is_reset){
        core_handle->status = QM_IOT_EVENT_CONNING_LOCAL;
        qm_iot_event_notify((void *)core_handle, core_handle->status);   
    }else {
    #if CONFIG_QM_IOT_DYNREG_SUPPORT
        core_handle->dynreg_enable = QM_TRUE;
    #endif
        core_handle->status = QM_IOT_EVENT_CONFIG;
        qm_iot_event_notify((void *)core_handle, core_handle->status);
    #if CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S >= 0
        qm_post_delayed_action(&core_handle->post_work, qm_iot_prov_timeout_notify, (void *)core_handle , CONFIG_QM_IOT_CORE_PROV_TIMEOUT_S * 1000);
    #endif
    }

    return QM_EOK;
}

int32_t qm_iot_stop(void *handle)
{
    int ret = QM_EOK;
    qm_iot_core_net_handle_t *net = NULL;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
    net = qm_iot_core_net_get_handle(core_handle->net_type);
    
#if CONFIG_QM_IOT_MQTT_SUPPORT
    qm_iot_core_conn_destory(core_handle);
#endif

    if(net && net->stop){
        ret = net->stop();
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "NETWORK STOP FAILED!!");
            return ret;
        }
    }

    return QM_EOK;
}

int32_t qm_iot_report(void *handle, qm_spec_property_operation_t *property_operation)
{
#if CONFIG_QM_IOT_PUB_SUPPORT
    qm_iot_pub_msg_t pub_msg = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
    if(property_operation == NULL || core_handle->pub_handle == NULL){
        return -QM_EINVAL;
    }
    pub_msg.did = qm_iot_did_get();
    pub_msg.type =  QM_IOT_PUBMSG_SHADOW_UPDATE;
    pub_msg.data.update.property_operation = property_operation;
    return qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
#else
    return -QM_EINVAL;  
#endif
}

#if CONFIG_QM_IOT_SPEC_SUPPORT
int32_t qm_iot_property_report(void *handle, qm_spec_property_operation_t *property_operation)
{
#if CONFIG_QM_IOT_PUB_SUPPORT
    qm_iot_pub_msg_t pub_msg = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
    if(property_operation == NULL || core_handle->pub_handle == NULL){
        return -QM_EINVAL;
    }
    pub_msg.did = qm_iot_did_get();
    pub_msg.type =  QM_IOT_PUBMSG_UPDATE;
    pub_msg.data.update.property_operation = property_operation;
    return qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
#else
    return -QM_EINVAL;  
#endif
}

int32_t qm_iot_property_get_request(void *handle, qm_spec_property_operation_t *property_operation)
{
#if CONFIG_QM_IOT_PUB_SUPPORT
    qm_iot_pub_msg_t pub_msg = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
    if(property_operation == NULL || core_handle->pub_handle == NULL){
        return -QM_EINVAL;
    }
    pub_msg.did = qm_iot_did_get();
    pub_msg.type =  QM_IOT_PUBMSG_GET_REQ;
    pub_msg.data.update.property_operation = property_operation;
    return qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
#else
    return -QM_EINVAL;  
#endif
}

int32_t qm_iot_property_get_rsponse(void *handle, qm_spec_property_operation_t *property_operation)
{
#if CONFIG_QM_IOT_PUB_SUPPORT
    qm_iot_pub_msg_t pub_msg = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
    if(property_operation == NULL || core_handle->pub_handle == NULL){
        return -QM_EINVAL;
    }
    pub_msg.did = qm_iot_did_get();
    pub_msg.type =  QM_IOT_PUBMSG_GET_RSP;
    pub_msg.data.update.property_operation = property_operation;
    return qm_iot_pub_send(core_handle->pub_handle, &pub_msg, 0);
#else
    return -QM_EINVAL;  
#endif
}
#endif

int32_t qm_iot_reset(void *handle)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL){
        return -QM_EINVAL;
    }
    qm_iot_unbind_notify(core_handle);
    return QM_EOK;
}

int32_t qm_iot_set_weahter_request(void *handle)
{
#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle->weather_handle == NULL) {
        return -QM_EINVAL;
    }
    return qm_iot_weather_request(core_handle->weather_handle);
#else   
    return -QM_EINVAL;
#endif
}

int32_t qm_iot_event_notify(void *handle, qm_iot_event_type_t event)
{
    static qm_iot_event_type_t old_event = QM_IOT_EVENT_NONE;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL || old_event == event){
        return -QM_EINVAL;
    }

    old_event = event;

    qm_event_ext_post(CONFIG_QM_IOT_CORE_EVENT_ID, event, NULL, 0, core_handle);

    return QM_EOK;
}

static void qm_iot_prov_timeout_notify(void *arg)
{
    int ret = QM_EOK;
    qm_iot_core_net_handle_t *net_handle = NULL;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    if(core_handle == NULL){
        return;
    }

#if CONFIG_QM_IOT_MQTT_SUPPORT
    qm_iot_core_conn_destory(core_handle);
#endif

    net_handle = qm_iot_core_net_get_handle(core_handle->net_type);
    if(net_handle && net_handle->stop){
        ret = net_handle->stop();
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "NETWORK STOP FAILED!!");
        }
    }

    qm_iot_event_notify(core_handle, QM_IOT_EVENT_CONFIG_TIMEOUT);
}

static void qm_iot_event_post(qm_iot_core_handle_t *core_handle, qm_iot_event_t *event)
{
    if(core_handle == NULL){
        return;
    }

    if(core_handle->event_handler){
        core_handle->event_handler(core_handle, event, core_handle->user_data);
    }
}

static int qm_iot_net_type_set(qm_iot_core_handle_t *core_handle, qm_iot_net_type_t net_type)
{
    int ret = QM_EOK;
    qm_iot_core_net_handle_t *net_handle = NULL;
    static qm_iot_net_type_t last_net_type = QM_IOT_NET_TYPE_NONE;

    if(core_handle == NULL || last_net_type == net_type){
        return -QM_EINVAL;
    }

    net_handle = qm_iot_core_net_get_handle(net_type);  
    if(net_handle == NULL){
        return -QM_EINVAL;
    }

    if(last_net_type != QM_IOT_NET_TYPE_NONE)
    {
        if(net_handle->close){
            ret = net_handle->close();
            if(ret != QM_EOK){
                QM_LOGE(LOG_TAG, "NETWORK CLOSE FAILED!!");
                return ret;
            }
        }
    }

    if(net_handle->open){
        ret = net_handle->open();
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "NETWORK OPEN FAILED!!");
            return ret;
        }
    }

    last_net_type = net_type;

    return ret;
}

static void qm_iot_mqtt_reset_notify(void *arg)
{
    qm_iot_core_net_handle_t *net = NULL;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;

    if(core_handle){
        net = qm_iot_core_net_get_handle(core_handle->net_type);
        if(net && net->ctrl){
            net->ctrl(QM_IOT_CORE_CTRL_DISCONNECT_NET, NULL);
        }
    }

    qm_reboot();
}

#if CONFIG_QM_IOT_DYNREG_SUPPORT
static void qm_iot_dynreg_recv_handler(void *handle, const qm_iot_dynreg_recv_t *packet, void *userdata)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL || packet == NULL){
        return;
    }

    if(packet->type == QM_IOT_DYNREGRECV_STATUS_CODE){
        QM_LOGE(LOG_TAG, "Dynreg recv errcode:%d", packet->data.status_code.code);
        return ;
    }

#if CONFIG_QM_IOT_MQTT_SUPPORT
    memset(&core_handle->crt_handle, 0, sizeof(qm_iot_core_crt_handle_t));

    core_handle->crt_handle.cloud_info.port = QM_IOT_CORE_MQTT_PORT;

    memcpy(core_handle->crt_handle.cloud_info.host, packet->data.device_info.mqtt_host, packet->data.device_info.mqtt_host_len);
    memcpy(core_handle->crt_handle.ca_crt, packet->data.device_info.server_cert, packet->data.device_info.server_cert_len);
    memcpy(core_handle->crt_handle.client_crt, packet->data.device_info.client_crt, packet->data.device_info.client_cert_len);
    memcpy(core_handle->crt_handle.client_key, packet->data.device_info.client_private_key, packet->data.device_info.client_privkey_len);
    memcpy(core_handle->crt_handle.ota_signatrure, packet->data.device_info.public_key, packet->data.device_info.public_key_len);
    
    qm_iot_server_crt_set(core_handle->crt_handle.ca_crt, strlen(core_handle->crt_handle.ca_crt));
    qm_iot_client_crt_set(core_handle->crt_handle.client_crt, strlen(core_handle->crt_handle.client_crt));
    qm_iot_client_key_set(core_handle->crt_handle.client_key, strlen(core_handle->crt_handle.client_key));
    qm_iot_ota_public_key_set(core_handle->crt_handle.ota_signatrure, strlen(core_handle->crt_handle.ota_signatrure));

    qm_iot_cloud_info_set(&core_handle->crt_handle.cloud_info);
#endif

    qm_iot_did_set(packet->data.device_info.did);

    core_handle->use_mqtt = QM_TRUE;
    core_handle->dynreg_enable = QM_FALSE;

    core_handle->status = QM_IOT_EVENT_CONNING_CLOUD;
    qm_iot_event_notify(core_handle, QM_IOT_EVENT_CONNING_CLOUD);

    qm_iot_dynreg_deinit(&core_handle->dynreg_handle);
}

static void qm_iot_core_dynreg_process(qm_iot_core_handle_t *core_handle)
{
    char *param = NULL;
    uint16_t port = CONFIG_QM_IOT_CORE_DYNREG_PORT;
    uint32_t retery = CONFIG_QM_IOT_CORE_DYNREG_RETRY_COUNT;
    uint32_t timeout = CONFIG_QM_IOT_CORE_DYNREG_RECVTIMEOUT;
    qm_iot_core_net_handle_t *net = qm_iot_core_net_get_handle(core_handle->net_type);

    if(core_handle->dynreg_handle == NULL){
        core_handle->dynreg_handle = qm_iot_dynreg_init();
        if(core_handle->dynreg_handle == NULL){
            QM_LOGE(LOG_TAG, "DYNREG CREATE FAILED!!");
            return ;
        }
    }

    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_PORT, &port);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_MAX_RETRY_NUM, &retery);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_RECV_TIMEOUT_MS, &timeout);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_HOST, qm_iot_dynreg_host_get());

    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_RECV_HANDLER, qm_iot_dynreg_recv_handler);
    qm_iot_dynreg_setopt(core_handle->dynreg_handle, QM_IOT_DYNREGOPT_USERDATA, (void *)core_handle);

    if(net && net->ctrl){
        net->ctrl(QM_IOT_CORE_CTRL_GET_DYNREG_PARAMS, &param);
    }

    qm_iot_dynreg_start(core_handle->dynreg_handle, param);
}
#endif

#if CONFIG_QM_IOT_MQTT_SUPPORT

static void iot_req_action(void *arg)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)arg;
    if(core_handle == NULL){
        return ;
    }
    qm_cancel_delayed_action(&core_handle->core_req); 

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT
    char *param = NULL;
    qm_iot_core_net_handle_t *net = qm_iot_core_net_get_handle(core_handle->net_type);
    if(net && net->ctrl){
        net->ctrl(QM_IOT_CORE_CTRL_GET_DEV_PARAMS, &param);
    }
    qm_iot_dev_send_devinfo(core_handle->dev_handle, param);
#endif

#if CONFIG_QM_IOT_OTA_SUPPORT
    qm_iot_ota_start(core_handle->ota_handle);
#endif

#if CONFIG_QM_IOT_NTP_SUPPORT
    if(core_handle->ntp_handle){
        qm_iot_ntp_request(core_handle->ntp_handle);
    }
#endif

    qm_post_delayed_action(&core_handle->core_req, iot_req_action, (void *)core_handle, CONFIG_QM_IOT_REQ_INTERVAL * 1000);
}

static void qm_iot_core_process_start(qm_iot_core_handle_t *core_handle)
{
    qm_cancel_delayed_action(&core_handle->core_req);  

#if CONFIG_QM_IOT_MQTT_SUPPORT
    qm_iot_mqtt_pre_sub_start(core_handle->mqtt_handle);
#endif

    qm_post_delayed_action(&core_handle->core_req, iot_req_action, (void *)core_handle, 0);
}

static void qm_iot_mqtt_event_handler(void *handle, const qm_iot_mqtt_event_t *event, void *userdata)
{
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL || event == NULL){
        return;
    }
    switch (event->type)
    {
        case QM_IOT_MQTTEVT_CONNECT:
        case QM_IOT_MQTTEVT_RECONNECT:
            
        #if CONFIG_QM_IOT_DEVMNT_SUPPORT
            qm_iot_core_dev_process(core_handle);
        #endif

        #if CONFIG_QM_IOT_PUB_SUPPORT
            qm_iot_core_pub_process(core_handle);
        #endif

        #if CONFIG_QM_IOT_OTA_SUPPORT
            qm_iot_core_ota_process(core_handle);
        #endif

        #if CONFIG_QM_IOT_NTP_SUPPORT
            qm_iot_core_ntp_process(core_handle);
        #endif

        #if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT
            qm_iot_core_weather_process(core_handle);
        #endif

            qm_iot_core_process_start(core_handle);

        break;

        case QM_IOT_MQTTEVT_DISCONNECT:
            qm_cancel_delayed_action(&core_handle->core_req);  
            core_handle->status = QM_IOT_EVENT_CLOUD_DISCONN;
            qm_iot_event_notify(core_handle, QM_IOT_EVENT_CLOUD_DISCONN);
        break;

        default:
        break;
    }
}

static void qm_iot_core_conn_destory(qm_iot_core_handle_t *core_handle)
{
    if(core_handle->mqtt_handle == NULL){
        return;
    }
#if CONFIG_QM_IOT_OTA_SUPPORT
    if(core_handle->ota_handle == NULL){
        return;
    }
#endif
    qm_cancel_delayed_action(&core_handle->core_req);  
#if CONFIG_QM_IOT_OTA_SUPPORT
    qm_iot_ota_stop(core_handle->ota_handle);
#endif

    core_handle->use_mqtt = QM_FALSE;
    qm_iot_mqtt_deinit(&core_handle->mqtt_handle);
}

static void qm_iot_core_conn_mqtt_process(qm_iot_core_handle_t *core_handle)
{
    qm_iot_mqtt_network_cred_t net_cred = {0};
    uint32_t pid;   
    uint8_t clear_session = QM_TRUE;
    uint32_t repub_timeout_ms = CONFIG_QM_IOT_CORE_MQTT_REPUB_MS;

    uint16_t keeplive = CONFIG_QM_IOT_CORE_MQTT_KEEPLIVE_S;
    char str_did[CONFIG_QM_IOT_CORE_MQTT_CID_LEN + 1] = {0};

    core_handle->mqtt_handle = qm_iot_mqtt_init();
    if(core_handle->mqtt_handle == NULL){
        return;
    }

    net_cred.server_crt = core_handle->crt_handle.ca_crt;
    net_cred.client_crt = core_handle->crt_handle.client_crt;
    net_cred.client_privkey = core_handle->crt_handle.client_key;

    pid = qm_iot_pid_get();
    snprintf(str_did, CONFIG_QM_IOT_CORE_MQTT_CID_LEN, QM_IOT_CORE_MQTT_CLIENT_ID_PARAMS, pid, qm_iot_sn_get());

    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_HOST, core_handle->crt_handle.cloud_info.host);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_PORT, &core_handle->crt_handle.cloud_info.port);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_CLIENTID, str_did);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_KEEPALIVE_SEC, &keeplive);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_NETWORK_CRED, &net_cred);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_REPUB_TIMEOUT_MS, &repub_timeout_ms);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_USERDATA, (void *)core_handle);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_CLEAN_SESSION, &clear_session);
    qm_iot_mqtt_setopt(core_handle->mqtt_handle, QM_IOT_MQTTOPT_EVENT_HANDLER, qm_iot_mqtt_event_handler);

    qm_iot_mqtt_process(core_handle->mqtt_handle);

    core_handle->mqtt_handle = NULL;
}
#endif

#if CONFIG_QM_IOT_PUB_SUPPORT

#if CONFIG_QM_IOT_SERVICE_SUPPORT
static int qm_iot_pub_recv_service_handler(qm_iot_core_handle_t *core_handle, qm_iot_pub_recv_t *recv)
{
    int service_find = QM_FALSE;
    qm_list_iterator_t self = {0};
    qm_list_node_t *node = NULL;
    qm_spec_property_t *spec_prop = NULL;
    qm_iot_core_service_t *service = NULL;
    qm_iot_service_iid_t *service_iid = NULL;
    qm_spec_property_operation_t *property_operation = NULL;
    qm_iot_service_type_t service_type = QM_IOT_SERVICE_TYPE_NONE;

    if(core_handle == NULL || recv == NULL){
        return -QM_EINVAL;
    }
    
    switch (recv->type)
    {
        case QM_IOT_PUBRECV_PROPERTY_SET:
            service_type = QM_IOT_SERVICE_TYPE_SET;
            property_operation = recv->data.prop_set.operation;
        break;
        
        case QM_IOT_PUBRECV_PROPERTY_GET_REQ:
            service_type = QM_IOT_SERVICE_TYPE_GET_REQ;
            property_operation = recv->data.prop_get.operation;
        break;

        case QM_IOT_PUBRECV_PROPERTY_GET_RSP:
            service_type = QM_IOT_SERVICE_TYPE_GET_RSP;
            property_operation = recv->data.prop_get.operation;
        break;

        default:
            break;
    }

    if(service_type == QM_IOT_SERVICE_TYPE_NONE) {
        return -QM_EINVAL;
    }

    qm_mutex_lock(&core_handle->lock, QM_WAIT_FOREVER);
    self.direction = LIST_HEAD;
    self.next = core_handle->service->head;  
    node = qm_list_iterator_next(&self);
    while (node)
    {
        
        service_find = QM_TRUE;
        service = (qm_iot_core_service_t *)qm_list_node_val_get(node);
        service_iid = (qm_iot_service_iid_t *)service->raw_iid;
        
        for (int i = 0; i < service->service_num; i++)
        {
            spec_prop = qm_spec_property_find(property_operation, service_iid[i].siid, service_iid[i].piid);
            if(spec_prop == NULL) {
                service_find = QM_FALSE;
                break;
            }
        }
            
        if(service_find){
            break;
        }
    }

    if(service->service_fn){
        service->service_fn(core_handle, service->service_id, 
                            service_type, property_operation, service->userdata);
    }
    qm_mutex_unlock(&core_handle->lock);

    return (service_find ? QM_EOK : -QM_ERROR);
}
#endif

static int qm_iot_pub_recv_comm_handler(qm_iot_core_handle_t *core_handle, qm_iot_pub_recv_t *recv)
{
    qm_iot_recv_t iot_recv = {0};
    qm_spec_property_t *spec_prop = NULL;
    
    iot_recv.did = recv->did;

    switch (recv->type)
    {
        case QM_IOT_PUBRECV_SHADOW_SET:
            iot_recv.type = QM_IOT_RECV_TYPE_SET;
            iot_recv.siid = recv->data.set.spec_property->siid;
            iot_recv.piid = recv->data.set.spec_property->piid;
            iot_recv.data.set.property = recv->data.set.spec_property;
            if(core_handle->recv_handler){
                core_handle->recv_handler(core_handle, &iot_recv, core_handle->user_data);
            }
        break;
    #if CONFIG_QM_IOT_SPEC_SUPPORT
        case QM_IOT_PUBRECV_PROPERTY_SET:
            while (1)
            {
                spec_prop = qm_spec_property_next(recv->data.prop_set.operation, spec_prop);
                if(spec_prop == NULL){
                    break;
                }
                iot_recv.type = QM_IOT_RECV_TYPE_PROPERTY_SET;
                iot_recv.did = spec_prop->did;
                iot_recv.siid = spec_prop->siid;
                iot_recv.piid = spec_prop->piid;
                iot_recv.data.set.property = spec_prop;
                if(core_handle->recv_handler){
                    core_handle->recv_handler(core_handle, &iot_recv, core_handle->user_data);
                }
            }
        break;

        case QM_IOT_PUBRECV_PROPERTY_GET_RSP:
            while (1)
            {
                spec_prop = qm_spec_property_next(recv->data.prop_get.operation, spec_prop);
                if(spec_prop == NULL){
                    break;
                }
                iot_recv.type = QM_IOT_RECV_TYPE_PROPERTY_GET_RSP;
                iot_recv.did = spec_prop->did;
                iot_recv.siid = spec_prop->siid;
                iot_recv.piid = spec_prop->piid;
                iot_recv.data.get_rsp.property = spec_prop;
                if(core_handle->recv_handler){
                    core_handle->recv_handler(core_handle, &iot_recv, core_handle->user_data);
                }
            }
        break;

        case QM_IOT_PUBRECV_PROPERTY_GET_REQ:
            while (1)
            {
                spec_prop = qm_spec_property_next(recv->data.prop_get.operation, spec_prop);
                if(spec_prop == NULL){
                    break;
                }
                iot_recv.type = QM_IOT_RECV_TYPE_PROPERTY_GET_REQ;
                iot_recv.did = spec_prop->did;
                iot_recv.siid = spec_prop->siid;
                iot_recv.piid = spec_prop->piid;
                iot_recv.data.get_req.property = spec_prop;
                if(core_handle->recv_handler){
                    core_handle->recv_handler(core_handle, &iot_recv, core_handle->user_data);
                }
            }
        break;
    #endif
        default:
        break;
    }

    return QM_EOK;
}


static void qm_iot_pub_recv_handler(void *handle, qm_iot_pub_recv_t *recv, void *userdata)
{
    int ret = QM_EOK;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL || recv == NULL){
        return;
    }

#if CONFIG_QM_IOT_SERVICE_SUPPORT
    ret = qm_iot_pub_recv_service_handler(core_handle, recv);
    if(ret == QM_EOK) {
        return ;
    }
#endif

    ret = qm_iot_pub_recv_comm_handler(core_handle, recv);
    if(ret == QM_EOK) {
        return ;
    }
    QM_LOGE(LOG_TAG, "NOT DATA RECV!!");
}

static void qm_iot_core_pub_process(qm_iot_core_handle_t *core_handle)
{
    uint32_t delay_pub = CONFIG_QM_IOT_PUB_DELAY_MS;
    if(core_handle == NULL){
        return;
    }

    if(core_handle->pub_handle == NULL){
        core_handle->pub_handle = qm_iot_pub_init();
        if(core_handle->pub_handle == NULL){
            return;
        }
    }

    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_USERDATA, core_handle);
    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_DELAY_TIMEOUT, &delay_pub);
    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_pub_setopt(core_handle->pub_handle, QM_IOT_PUBOPT_RECV_HANDLER, qm_iot_pub_recv_handler);

}

#endif

#if CONFIG_QM_IOT_OTA_SUPPORT
static void qm_iot_ota_event_handler(void *handle, const qm_iot_ota_event_t *event, void *userdata)
{
    qm_iot_event_t iot_event = {0};
    qm_iot_ota_event_t *ota_event = (qm_iot_ota_event_t *)event;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(core_handle == NULL || handle == NULL){
        return;
    }

    switch (event->type)
    {
        case QM_IOT_OTA_EVENT_READY:
            iot_event.type = QM_IOT_EVENT_OTA_READY;
            qm_iot_event_post(core_handle, &iot_event);
            ota_event->data.ready.result = (qm_iot_ota_req_result_t)iot_event.data.ota_ready.is_busy;
        break;

        case QM_IOT_OTA_EVENT_START:
            iot_event.type = QM_IOT_EVENT_OTA_START;
            qm_iot_event_post(core_handle, &iot_event);
        break;
        
        case QM_IOT_OTA_EVENT_DATA_INSTALLING:
            iot_event.type = QM_IOT_EVENT_OTA_INSTALLING;
            qm_iot_event_post(core_handle, &iot_event);
        break;
        
        case QM_IOT_OTA_EVENT_PROGRESS_UPADTE:
            iot_event.type = QM_IOT_EVENT_OTA_PROGRESS_UPADTE;
            iot_event.data.ota_progress_update.progress_details = 
                            event->data.progress_update.progress_details;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        case QM_IOT_OTA_EVENT_SUCCESS:
            iot_event.type = QM_IOT_EVENT_OTA_SUCCESS;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        case QM_IOT_OTA_EVENT_FAILED:
            iot_event.type = QM_IOT_EVENT_OTA_FAILED;
            qm_iot_event_post(core_handle, &iot_event);
        break;
        
        case QM_IOT_OTA_EVENT_CANCEL:
            iot_event.type = QM_IOT_EVENT_OTA_CANCEL;
            qm_iot_event_post(core_handle, &iot_event);
        break;

        default:
            break;
    }
}

static void qm_iot_core_ota_process(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return;
    }

    if(core_handle->ota_handle == NULL){
        core_handle->ota_handle = qm_iot_ota_init();
        if(core_handle->ota_handle == NULL){
            return;
        }
    }

    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_USERDATA, core_handle);
    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_SINGATURE, core_handle->crt_handle.ota_signatrure);
    qm_iot_ota_setopt(core_handle->ota_handle, QM_IOT_OTAOPT_EVENT_HANDLER, qm_iot_ota_event_handler);

}
#endif

#if CONFIG_QM_IOT_DEVMNT_SUPPORT

static void qm_iot_dev_recv_handler(void *handle, const qm_iot_dev_recv_t *packet, void *userdata)
{
    qm_iot_event_t  event = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(packet == NULL || userdata == NULL){
        return ;
    }

    qm_cancel_delayed_action(&core_handle->post_work);
    switch (packet->type)
    {
        case QM_IOT_DEVRECV_RESET_REPLY:
            QM_LOGD(LOG_TAG, "QM_IOT_DEVRECV_RESET_REPLY");
            
            if(!core_handle->reset_info.is_reset){
                core_handle->reset_info.is_reset = QM_TRUE;
                qm_iot_reset_info_set(&core_handle->reset_info);
            }

            qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify, core_handle , 0);
        break;

        case QM_IOT_DEVRECV_UNBIND_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_DEVRECV_UNBIND_NOTIFY");

            if(!core_handle->reset_info.is_reset){
                core_handle->reset_info.is_reset = QM_TRUE;
                qm_iot_reset_info_set(&core_handle->reset_info);
            }

            event.type = QM_IOT_EVENT_RESET;
            qm_iot_event_post(core_handle, &event);

            qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify,core_handle ,CONFIG_QM_IOT_RESET_DELAY_MS);
        break;

        case QM_IOT_DEVRECV_BIND_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_DEVRECV_BIND_NOTIFY");
            if(core_handle->reset_info.is_reset){
                core_handle->reset_info.is_reset = QM_FALSE;
                qm_iot_reset_info_set(&core_handle->reset_info);
            }
            event.type = QM_IOT_EVENT_BIND_NOTIFY;
            qm_iot_event_post(core_handle, &event);
        break;

        default:
            break;
    }
}

static void qm_iot_unbind_notify(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return ;
    }

    if(!core_handle->reset_info.is_reset){
        core_handle->reset_info.is_reset = QM_TRUE;
        qm_iot_reset_info_set(&core_handle->reset_info);
    }

    qm_cancel_delayed_action(&core_handle->post_work);

    if(core_handle->mqtt_handle == NULL || core_handle->dev_handle == NULL){
        qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify,core_handle ,CONFIG_QM_IOT_RESET_DELAY_MS);
        return;
    }
    qm_iot_dev_reset_request(core_handle->dev_handle);
    qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify,core_handle ,CONFIG_QM_IOT_RESET_FORCE_MS);
}

static void qm_iot_core_dev_process(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return;
    }

    if(core_handle->dev_handle == NULL){
        core_handle->dev_handle = qm_iot_dev_init();
        if(core_handle->dev_handle == NULL){
            return;
        }
    }

    qm_iot_dev_setopt(core_handle->dev_handle, QM_IOT_DEVOPT_USERDATA, core_handle);
    qm_iot_dev_setopt(core_handle->dev_handle, QM_IOT_DEVOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_dev_setopt(core_handle->dev_handle, QM_IOT_DEVOPT_RECV_HANDLER, qm_iot_dev_recv_handler);
}
#else
static void qm_iot_unbind_notify(qm_iot_core_handle_t *core_handle)
{
    if(core_handle == NULL){
        return;
    }

    qm_cancel_delayed_action(&core_handle->post_work);
    
    if(!core_handle->reset_info.is_reset){
        core_handle->reset_info.is_reset = QM_TRUE;
        qm_iot_reset_info_set(&core_handle->reset_info);
    }

    qm_post_delayed_action(&core_handle->post_work, qm_iot_mqtt_reset_notify, core_handle ,CONFIG_QM_IOT_RESET_DELAY_MS);
}
#endif

#if CONFIG_QM_IOT_NTP_SUPPORT
static void qm_iot_ntp_recv_handler(void *handle, const qm_iot_ntp_recv_t *packet, void *userdata)
{
    qm_iot_reset_info_t reset_info = {0};

    qm_iot_event_t iot_event = {0};
    struct qm_timeval timeval = {0};

    uint32_t timestamp = (uint32_t)packet->data.local_time.timestamp;
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(packet == NULL || userdata == NULL){
        return ;
    }

    timeval.tv_sec = packet->data.local_time.timestamp;
    qm_settimeofday(&timeval, NULL);
    QM_LOGD(LOG_TAG, "timezone %u, timestamp %u", core_handle->timezone, timestamp);

    qm_iot_reset_info_get(&reset_info);
    if (reset_info.is_reset != QM_IOT_NOT_RESET){
        return ;
    }

    core_handle->status = QM_IOT_EVENT_CLOUD_CONN;
    qm_iot_event_notify(core_handle, QM_IOT_EVENT_CLOUD_CONN);
}

static void qm_iot_core_ntp_process(qm_iot_core_handle_t *core_handle)
{
    uint32_t deinit_timeout = CONFIG_QM_IOT_NTP_DEINIT_TIMEOUT;
    if(core_handle == NULL){
        return;
    }

    if(core_handle->ntp_handle == NULL){
        core_handle->ntp_handle = qm_iot_ntp_init();
        if(core_handle->ntp_handle == NULL){
            return;
        }
    }

    core_handle->timezone = CONFIG_QM_IOT_NTP_TIMEZONE;
    qm_iot_ntp_timezone_get(&core_handle->timezone);

    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_TIME_ZONE, &core_handle->timezone);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_RECV_HANDLER, qm_iot_ntp_recv_handler);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_USERDATA, core_handle);
    qm_iot_ntp_setopt(core_handle->ntp_handle, QM_IOT_NTPOPT_DEINIT_TIMEOUT_MS, &deinit_timeout);

}
#endif

#if CONFIG_QM_IOT_CORE_WEATHER_SUPPORT

static void qm_iot_weather_recv_handler(void *handle, const qm_iot_weather_recv_t *packet, void *userdata)
{
    qm_iot_event_t iot_event = {0};
    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)userdata;
    if(packet == NULL || userdata == NULL){
        return ;
    }

    iot_event.type = QM_IOT_EVENT_WEATHER_NOTIFY;

    iot_event.data.weather_notify.weather_type = packet->weather_type;
    iot_event.data.weather_notify.temp = packet->temp;
    iot_event.data.weather_notify.humidity = packet->humidity;
    iot_event.data.weather_notify.city_id = packet->city_id;

    iot_event.data.weather_notify.condition_len = packet->condition_len;
    iot_event.data.weather_notify.country_name_len = packet->country_name_len;
    iot_event.data.weather_notify.pname_len = packet->pname_len;
    iot_event.data.weather_notify.secondaryname_len = packet->secondaryname_len;
    iot_event.data.weather_notify.name_len = packet->name_len;
    iot_event.data.weather_notify.sunset_len = packet->sunset_len;
    iot_event.data.weather_notify.sunrise_len = packet->sunrise_len;

    iot_event.data.weather_notify.condition = packet->condition;
    iot_event.data.weather_notify.country_name = packet->country_name;
    iot_event.data.weather_notify.pname = packet->pname;
    iot_event.data.weather_notify.secondaryname = packet->secondaryname;
    iot_event.data.weather_notify.name = packet->name;
    iot_event.data.weather_notify.sunset = packet->sunset;
    iot_event.data.weather_notify.sunrise = packet->sunrise;

    qm_iot_event_post(core_handle, &iot_event);
}


static void qm_iot_core_weather_process(qm_iot_core_handle_t *core_handle)
{
    uint32_t delay_pub = CONFIG_QM_IOT_WEATHER_DELAY_MS;
    if(core_handle == NULL){
        return;
    }

    if(core_handle->weather_handle == NULL){
        core_handle->weather_handle = qm_iot_weather_init();
        if(core_handle->weather_handle == NULL){
            return;
        }
    }

    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_USERDATA, core_handle);
    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_DEINIT_TIMEOUT_MS, &delay_pub);
    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_MQTT_HANDLE, core_handle->mqtt_handle);
    qm_iot_weather_setopt(core_handle->weather_handle, QM_IOT_WEATHEROPT_RECV_HANDLER, qm_iot_weather_recv_handler);
}
#endif

#if CONFIG_QM_IOT_SERVICE_SUPPORT

static int service_map_match(void *a, void *b)
{
    qm_iot_core_service_t *a_handle = (qm_iot_core_service_t*)a;
    qm_iot_core_service_t *b_handle = (qm_iot_core_service_t*)b;

    if(a_handle == NULL || b_handle == NULL){
        return QM_FALSE;
    }

    if(a_handle->service_id == b_handle->service_id){
        return 1;
    }else{
        return 0;
    }
}

int qm_iot_service_register(void *handle, int service_id, qm_iot_service_iid_t *iid, int iid_num, 
                                qm_iot_service_handler_t service_fn, void *userdata)
{
    int node_len = 0;
    qm_list_node_t *node = NULL;
    qm_iot_service_iid_t *service_iid = NULL;
    qm_iot_core_service_t *service = NULL;
    qm_iot_core_service_t match_service = {0};

    qm_iot_core_handle_t *core_handle = (qm_iot_core_handle_t *)handle;
    if(core_handle == NULL || iid == NULL || iid_num == 0 || service_fn == NULL){
        return -QM_EINVAL;
    }

    node_len = (sizeof(qm_iot_core_service_t) + (sizeof(qm_iot_service_iid_t) * iid_num) - 1);

    match_service.service_id = service_id;

    qm_mutex_lock(&core_handle->lock, QM_WAIT_FOREVER);
    node = qm_list_find(core_handle->service, &match_service);
    if(node != NULL){
        QM_LOGE(LOG_TAG, "service is exist!!");
        return -QM_EINVAL;
    }

    node = qm_list_node_extra_new(node_len);
    if(node == NULL){
        qm_mutex_unlock(&core_handle->lock);
        return -QM_ENOMEM;
    }

    service = (qm_iot_core_service_t *)qm_list_node_val_get(node);

    service->service_id = service_id;
    service->userdata = userdata;
    service->service_fn = service_fn;
    service->service_num = iid_num;
    
    service_iid = (qm_iot_service_iid_t *)service->raw_iid;
    for (int i = 0; i < iid_num; i++)
    {
        service_iid[i].siid = iid[i].siid;
        service_iid[i].piid = iid[i].piid;
    }
    
    qm_list_rpush(core_handle->service, node);

    qm_mutex_unlock(&core_handle->lock);

    return QM_EOK;
}

#endif