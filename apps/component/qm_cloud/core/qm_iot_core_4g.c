#include "qm.h"
#include "qm_work.h"
#include "qm_iot_api.h"
#include "qm_iot_core.h"
#include "qm_iot_config.h"
#include "qm_iot_common.h"
#include "qm_utils_hmac.h"
#include "qm_utils_string.h"

#include "cJSON.h"
#include "qm_modem.h"

#define LOG_TAG "IOT_MODEM_4G"

#define MODEM_4G_TYPE_KEY  "4g"


#define QM_IOT_WIFI_PAYLOAD_MAX_LEN     64
#define QM_IOT_4G_WAIT_TIMEOUT  (20 * 1000)

#define QM_IOT_4G_DEVICE_DYNREG_NONCE_MAX  ((1U << 31) - 1)

#define MODEM_4G_DYNERG_REQ_FORMAT  "{\"pid\":%d,\"sn\":\"%s\",\"authCode\":\"%s\"}"

#define QM_IOT_MODEM_4G_PAYLOAD_MAX_LEN 64

typedef struct 
{
    int get_lbs;
    qm_modem_t modem;
}qm_iot_core_4g_t;

static char *qm_iot_dev_modem_4g_devinfo_get(void);
static char *qm_iot_dev_modem_4g_reginfo_get(void);

static int modem_4g_open(void); 
static int modem_4g_close(void);
static int modem_4g_start(void); 
static int modem_4g_stop(void);
static int modem_4g_ctrl(qm_iot_core_ctrl_t cmd, void *param);

static void modem_4g_provision(void);
static void modem_4g_sta_conn_router(void);
static void qm_comm_conn_handler(qm_input_event_t *input_event, void *arg);

static qm_iot_core_4g_t *modem_4g_handle = NULL;

static qm_iot_core_net_handle_t modem_4g_net_handle = 
{
    .net_type = QM_IOT_NET_TYPE_4G,
    .open = modem_4g_open,
    .close = modem_4g_close,
    .start = modem_4g_start,
    .stop = modem_4g_stop,
    .ctrl = modem_4g_ctrl,
};

qm_iot_core_net_handle_t *qm_iot_core_4g_get_handle(void)
{
    return &modem_4g_net_handle;
}

static int modem_4g_close(void)
{
    if(modem_4g_handle == NULL){
        return -QM_EINVAL;
    }
    
    qm_modem_deinit(modem_4g_handle->modem);
    
    qm_free(modem_4g_handle);
    modem_4g_handle = NULL;

    return QM_EOK;
}

static int modem_4g_open(void)
{
    char *sn = NULL;
    qm_modem_config_t cfg = {
        .rx_buffer_size = 5*1024,
        .tx_buffer_size = 5*1024,
        .channel = QM_MODEM_CHANNLE_4G,
        .event_handler = modem_4g_event_handler,
    };


    sn = qm_iot_sn_get();
    if(sn[0] == '\0'){
        QM_LOGE(LOG_TAG, "modem 4g sn is empty");
        return -QM_EINVAL;  
    }

    modem_4g_handle = (qm_iot_core_4g_t *)qm_malloc(sizeof(qm_iot_core_4g_t));
    if(modem_4g_handle == NULL){
        return -QM_ENOMEM;
    }
    memset(modem_4g_handle, 0, sizeof(qm_iot_core_4g_t));

    cfg.arg = modem_4g_handle;
    modem_4g_handle->modem = qm_modem_init(&cfg);
    if(modem_4g_handle->modem == NULL){ 
        goto __error;
    }

    return QM_EOK;

__error:
    qm_free(modem_4g_handle);
    modem_4g_handle = NULL;
    return -QM_EINVAL;
}

static int modem_4g_start(void)
{
    qm_iot_reset_info_t reset_info = {0};
    if(modem_4g_handle == NULL){
        return -QM_EINVAL;
    }

    qm_modem_start_ppp(modem_4g_handle->modem, QM_IOT_4G_WAIT_TIMEOUT);

    return QM_EOK;
}

static int modem_4g_stop(void)
{
    if(modem_4g_handle == NULL){
        return -QM_EINVAL;
    }

    qm_modem_stop_ppp(modem_4g_handle->modem, QM_IOT_4G_WAIT_TIMEOUT);
    
    return QM_EOK;
}
static int modem_4g_ctrl(qm_iot_core_ctrl_t cmd, void *param)
{
    if(modem_4g_handle == NULL){
        return -QM_EINVAL;
    }

    switch(cmd)
    {
        case QM_IOT_CORE_CTRL_CONFIG_NET:
            QM_LOGW(LOG_TAG, "can not config modem_4g!!");
        break;

        case QM_IOT_CORE_CTRL_CONNECT_NET:
            QM_LOGD(LOG_TAG, "connect to the NET");
            
        break;

        case QM_IOT_CORE_CTRL_DISCONNECT_NET:
            QM_LOGD(LOG_TAG, "disconnect NET");
            
        break;

        case QM_IOT_CORE_CTRL_GET_DEV_PARAMS:
            QM_LOGD(LOG_TAG, "get dev info");
            if(param) {
                *((char **)param) = qm_iot_dev_modem_4g_devinfo_get();
            }
        break;

        case QM_IOT_CORE_CTRL_GET_DYNREG_PARAMS:
            QM_LOGW(LOG_TAG, "can not get dynreg params!!");
            if(param) {
                *((char **)param) = qm_iot_dev_modem_4g_reginfo_get();
            }
        break;

        default:
            
        break;
    }

    return QM_EOK;
}

static char *qm_iot_dev_modem_4g_devinfo_get(void)
{
    char imsi[64];
    char *payload = NULL;
    qm_modem_lbs_t lbs_info = {0};
    cJSON *root = cJSON_CreateObject();
    if(root == NULL){
        goto __exit;
    }
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_TYPE_KEY, MODEM_4G_TYPE_KEY);

    qm_modem_get_imsi(NULL, imsi, 64);
    if(imsi[0] == '\0'){
        goto __exit;
    }

    qm_iot_4g_lbs_info_get(&lbs_info);

    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAM_ICCID_KEY, imsi); 
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAM_LONGITUDE_KEY, lbs_info.longitude); 
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAM_LATITUDE_KEY, lbs_info.latitude); 

    payload = cJSON_PrintUnformatted(root);
    if(payload == NULL){
        goto __exit;
    }

__exit:
    if(root){
        cJSON_Delete(root);
        root = NULL;
    }

    return payload;
}

static char *qm_iot_dev_modem_4g_reginfo_get(void)
{
    int payload_len = 0;
    char *payload = NULL;

    payload_len = strlen(MODEM_4G_DYNERG_REQ_FORMAT) + QM_IOT_MODEM_4G_PAYLOAD_MAX_LEN;
    payload = (char *)qm_malloc(payload_len + 1);
    if(payload == NULL){
        return NULL;
    }
    memset(payload, 0, payload_len + 1);

    qm_snprintf(payload, payload_len, MODEM_4G_DYNERG_REQ_FORMAT, 
                    qm_iot_pid_get(), qm_iot_sn_get(), qm_iot_authcode_get());

    return payload;
}


static void modem_4g_event_handler(qm_modem_event_t event, void *data, int len, void *arg)
{
    qm_iot_core_4g_t *core_handle = (qm_iot_core_4g_t *)arg;

#if CONFIG_QM_IOT_DEV_INFO_SUPPORT
    qm_modem_lbs_t *modem_lbs = NULL;
    char imsi[CONFIG_QM_IOT_4G_IMSI_MAX_LEN] = {0};
#endif
  
    switch(event){

        case QM_MODEM_EVENT_CONN:
            QM_LOGD(LOG_TAG, "modem event conn");
            
        #if CONFIG_QM_IOT_DEV_INFO_SUPPORT
            qm_modem_get_lbs(core_handle->modem);
        #endif

        break;

        case QM_MODEM_EVENT_DISCONN:
            QM_LOGD(LOG_TAG, "modem event disconn");

            core_handle->get_lbs = QM_FALSE;;
            qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_4G, QM_IOT_CORE_EVENT_DISCONNECT_NET);
        break;

        case QM_MODEM_EVENT_LBS:

            QM_LOGD(LOG_TAG, "modem event lbs");
            modem_lbs = (qm_modem_lbs_t *)data;
        
            qm_iot_4g_lbs_info_set(modem_lbs);
            core_handle->get_lbs = QM_TRUE;

            qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_4G, QM_IOT_CORE_EVENT_CONNECTED_NET);

        break;

        case QM_MODEM_EVENT_SIMCARD_CONN:
            QM_LOGD(LOG_TAG, "modem event simcard conn");
        break;

        case QM_MODEM_EVENT_SIMCARD_DISCONN:
            QM_LOGD(LOG_TAG, "modem event simcard disconn");
        break;

        case QM_MODEM_EVENT_RESTART:
            QM_LOGD(LOG_TAG, "modem event restart");
        break;

        case QM_MODEM_EVENT_RESTART_DONE:
            QM_LOGD(LOG_TAG, "modem event restart done");
        break;

        default:
            break;
    }
}