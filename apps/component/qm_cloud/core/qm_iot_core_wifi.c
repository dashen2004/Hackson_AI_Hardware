#include "qm.h"
#if CONFIG_QM_IOT_NETWORK_TYPE_WIFI_NORMAL
#include "qm_work.h"
#include "qm_iot_api.h"
#include "qm_iot_core.h"
#include "qm_iot_config.h"
#include "qm_iot_common.h"
#include "qm_utils_hmac.h"
#include "qm_utils_string.h"
#include "qm_wifi.h"
#include "cJSON.h"

#define LOG_TAG "IOT_WIFI"

#define WIFI_TYPE_KEY  "wifi"

#define QM_IOT_WIFI_PAYLOAD_MAX_LEN            64

typedef struct 
{
    void *prov_handle;
    qm_work_t post_work;
    qm_iot_wifi_info_t wifi_info;
}qm_iot_core_wifi_t;

static char *qm_iot_dev_wifi_devinfo_get(void);

static int wifi_open(void); 
static int wifi_close(void);
static int wifi_start(void); 
static int wifi_stop(void);
static int wifi_ctrl(qm_iot_core_ctrl_t cmd, void *param);

static void wifi_provision(void);
static void wifi_sta_conn_router(void);
static void qm_comm_conn_handler(qm_input_event_t *input_event, void *arg);

static qm_iot_core_wifi_t *wifi_handle = NULL;

static qm_iot_core_net_handle_t wifi_net_handle = 
{
    .net_type = QM_IOT_NET_TYPE_WIFI,
    .open = wifi_open,
    .close = wifi_close,
    .start = wifi_start,
    .stop = wifi_stop,
    .ctrl = wifi_ctrl,
};

qm_iot_core_net_handle_t *qm_iot_core_wifi_get_handle(void)
{
    return &wifi_net_handle;
}

static int wifi_close(void)
{
    if(wifi_handle == NULL){
        return -QM_EINVAL;
    }

    qm_event_unregister(QM_EVENT_WIFI, qm_comm_conn_handler, (void *)wifi_handle);

    qm_wifi_stop();

    qm_free(wifi_handle);
    wifi_handle = NULL;

    return QM_EOK;
}

static int wifi_open(void)
{
    // todo : 根据实际参数更改
    qm_iot_wifi_info_t wifi_info = {0};


    wifi_handle = (qm_iot_core_wifi_t *)qm_malloc(sizeof(qm_iot_core_wifi_t));
    if(wifi_handle == NULL){
        return -QM_ENOMEM;
    }
    memset(wifi_handle, 0, sizeof(qm_iot_core_wifi_t));

    qm_iot_wifi_info_get(&wifi_info);
    if(wifi_info.ssid[0] == '\0' || wifi_info.pwd[0] == '\0'){
        QM_LOGE(LOG_TAG, "wifi open failed, wifi info is empty");
        goto __error;
    }

    QM_LOGD(LOG_TAG, "wifi open");
    qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI, QM_IOT_CORE_EVENT_OPEN);
    return QM_EOK;

__error:
    qm_free(wifi_handle);
    wifi_handle = NULL;
    return -QM_EINVAL;
}

static int wifi_start(void)
{
    qm_iot_reset_info_t reset_info = {0};
    if(wifi_handle == NULL){
        return -QM_EINVAL;
    }

    qm_iot_reset_info_get(&reset_info);

    if(reset_info.is_reset == QM_IOT_NOT_RESET){
       wifi_sta_conn_router();
    }

    return QM_EOK;
}

static int wifi_stop(void)
{
    if(wifi_handle == NULL){
        return -QM_EINVAL;
    }

    qm_wifi_stop();

    return QM_EOK;
}
static int wifi_ctrl(qm_iot_core_ctrl_t cmd, void *param)
{
    if(wifi_handle == NULL){
        return -QM_EINVAL;
    }

    switch(cmd)
    {
        case QM_IOT_CORE_CTRL_CONFIG_NET:
            QM_LOGW(LOG_TAG, "can not config wifi!!");
        break;

        case QM_IOT_CORE_CTRL_CONNECT_NET:
            QM_LOGD(LOG_TAG, "connect to the AP");
            qm_wifi_connect();
        break;

        case QM_IOT_CORE_CTRL_DISCONNECT_NET:
            QM_LOGD(LOG_TAG, "disconnect AP");
            qm_wifi_disconnect();
        break;

        case QM_IOT_CORE_CTRL_GET_DEV_PARAMS:
            QM_LOGD(LOG_TAG, "get dev info");
            if(param) {
                *((char **)param) = qm_iot_dev_wifi_devinfo_get();
            }
        break;

        case QM_IOT_CORE_CTRL_GET_DYNREG_PARAMS:
            QM_LOGW(LOG_TAG, "can not get dynreg params!!");
        break;

        default:
            
        break;
    }

    return QM_EOK;
}


static void wifi_sta_conn_router(void)
{
    qm_wifi_config_t config = {0};
    qm_iot_wifi_info_t wifi_info = {0};

    if(wifi_handle == NULL){
        return ;
    }

    qm_iot_wifi_info_get(&wifi_info);

    // 添加边界检查
    size_t ssid_len = strlen(wifi_info.ssid);
    size_t password_len = strlen(wifi_info.pwd);

    if(ssid_len >= sizeof(config.sta.ssid) || password_len >= sizeof(config.sta.password)) {
        return;
    }

    config.sta.ssid_len = ssid_len;
    memcpy(config.sta.ssid, wifi_info.ssid, ssid_len);
    config.sta.password_len = password_len;
    memcpy(config.sta.password, wifi_info.pwd, password_len);

    QM_LOGD(LOG_TAG, "CONN WIFI [%s]:[%s]", config.sta.ssid, config.sta.password);

    qm_event_register(QM_EVENT_WIFI, qm_comm_conn_handler, (void *)wifi_handle);

    qm_wifi_init();
    qm_wifi_set_mode(QM_WIFI_MODE_STA);
    qm_wifi_set_config(QM_WIFI_IF_STA, &config);
    qm_wifi_start();
}

static void qm_comm_conn_handler(qm_input_event_t *input_event, void *arg)
{
    qm_iot_core_wifi_t *wifi = (qm_iot_core_wifi_t *)arg;
    qm_wifi_event_info_t *event_info = (qm_wifi_event_info_t*)input_event->value;

    if(wifi == NULL){
        return ;
    }

    switch(input_event->sub_event){

        case QM_WIFI_EVENT_STA_START:

        break;

        case QM_WIFI_EVENT_STA_STOP:

        break;

        case QM_WIFI_EVENT_STA_CONNECTED:
            QM_LOGD(LOG_TAG, "connected to the AP");
        break;

        case QM_WIFI_EVENT_STA_GOT_IP:
            QM_LOGD(LOG_TAG, "sta got ip");

            QM_LOGD(LOG_TAG, "ip:"      QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.ip));
            QM_LOGD(LOG_TAG, "netmask:" QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.netmask));
            QM_LOGD(LOG_TAG, "gw:"      QM_IPSTR, QM_IP2STR(&event_info->got_ip.ip_info.gw));

            qm_iot_wifi_ip_info_set(&event_info->got_ip.ip_info);

            qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI, QM_IOT_CORE_EVENT_CONNECTED_NET);
        break;

        case QM_WIFI_EVENT_STA_LOST_IP:

        break;

        case QM_WIFI_EVENT_STA_DISCONNECTED:

            QM_LOGD(LOG_TAG, "sta disconnected reason: %d,retry to connect to the AP", event_info->sta_disconnected.reason);

            qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI, QM_IOT_CORE_EVENT_DISCONNECT_NET);

            qm_wifi_connect();
        break;

        default:
        break;
    }
}

static char *qm_iot_dev_wifi_devinfo_get(void)
{
    uint8_t mac[6] = {0};
    char *payload = NULL;
    qm_ip_info_t ip_info = {0};
    qm_wifi_ap_record_t ap_info = {0};
    char temp_value[QM_IOT_WIFI_PAYLOAD_MAX_LEN] = {0};
    cJSON *root = cJSON_CreateObject();
    if(root == NULL){
        goto __exit;
    }
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_TYPE_KEY, WIFI_TYPE_KEY);

    qm_wifi_get_mac(QM_WIFI_IF_STA, mac);
    memset(temp_value, 0, QM_IOT_WIFI_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_WIFI_PAYLOAD_MAX_LEN, 
                "%02X:%02X:%02X:%02X:%02X:%02X", 
                mac[0], mac[1], mac[2],mac[3],mac[4],mac[5]);
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_MAC_TYPE_KEY, temp_value); 

    qm_wifi_sta_get_ap_info(&ap_info);
    memset(temp_value, 0, QM_IOT_WIFI_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_WIFI_PAYLOAD_MAX_LEN,
                "%02X:%02X:%02X:%02X:%02X:%02X", 
                ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                ap_info.bssid[3],ap_info.bssid[4],ap_info.bssid[5]);

    cJSON_AddNumberToObject(root, QM_IOT_COMMON_PARAMS_RSSI_TYPE_KEY, (double)ap_info.rssi); 
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_BSSID_TYPE_KEY, temp_value); 
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_SSID_TYPE_KEY, (char *)ap_info.ssid); 
    cJSON_AddNumberToObject(root, QM_IOT_COMMON_PARAMS_CHANNEL_TYPE_KEY, (double)ap_info.channel);

    qm_iot_wifi_ip_info_get(&ip_info);
    memset(temp_value, 0, QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN, 
                QM_IPSTR, QM_IP2STR(&ip_info.ip));    
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_IP_TYPE_KEY, temp_value); 
    
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
#endif
