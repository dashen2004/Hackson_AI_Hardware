#include "qm.h"
#include "qm_work.h"
#include "qm_iot_api.h"
#include "qm_iot_core.h"
#include "qm_iot_config.h"
#include "qm_wifi.h"
#include "qm_ble_gap.h"

#include "qm_iot_prov.h"
#include "qm_iot_common.h"

#include "cJSON.h"

#include "qm_utils_string.h"
#include "qm_utils_hmac.h"

#define LOG_TAG "IOT_WIFI"

#define WIFI_COMBO_TYPE_KEY  "wifi"

#define WIFI_COMBO_DYNREG_NONCE_MAX  ((1U << 31) - 1)
#define WIFI_COMBO_DYNERG_REQ_FORMAT  "{\"sn\":\"%s\",\"pid\":\"%d\",\"timestamp\":\"%d\",\"signature\":\"%s\",\"nonce\":%d}"

#define QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN            64

typedef struct 
{
    void *prov_handle;
    qm_work_t post_work;
    qm_iot_wifi_info_t wifi_info;
}qm_iot_core_wifi_combo_t;

static char *qm_iot_dev_wifi_devinfo_get(void);
static char *qm_iot_dev_wifi_reginfo_get(void);


static int wifi_combo_open(void); 
static int wifi_combo_close(void);
static int wifi_combo_start(void); 
static int wifi_combo_stop(void);
static int wifi_combo_ctrl(qm_iot_core_ctrl_t cmd, void *param);

static void wifi_combo_provision(void);
static void wifi_combo_sta_conn_router(void);
static void qm_comm_conn_handler(qm_input_event_t *input_event, void *arg);

static qm_iot_core_wifi_combo_t *wifi_combo_handle = NULL;

static qm_iot_core_net_handle_t wifi_combo_net_handle = 
{
    .net_type = QM_IOT_NET_TYPE_WIFI_COMBO,
    .open = wifi_combo_open,
    .close = wifi_combo_close,
    .start = wifi_combo_start,
    .stop = wifi_combo_stop,
    .ctrl = wifi_combo_ctrl,
};

qm_iot_core_net_handle_t *qm_iot_core_wifi_combo_get_handle(void)
{
    return &wifi_combo_net_handle;
}

static int wifi_combo_close(void)
{
    if(wifi_combo_handle == NULL){
        return -QM_EINVAL;
    }

    if(!wifi_combo_handle->prov_handle)
    {
        qm_event_unregister(QM_EVENT_WIFI, qm_comm_conn_handler, (void *)wifi_combo_handle);

        qm_wifi_stop();

    }else {
        qm_iot_prov_deinit(&wifi_combo_handle->prov_handle);
        wifi_combo_handle->prov_handle = NULL;
    }

    qm_free(wifi_combo_handle);
    wifi_combo_handle = NULL;

    return QM_EOK;
}

static int wifi_combo_open(void)
{
    char *sn = NULL;
    // todo : 根据实际参数更改
    uint8_t mac[6] = {0};
    char str_mac[12 + 1] = {0};


    wifi_combo_handle = (qm_iot_core_wifi_combo_t *)qm_malloc(sizeof(qm_iot_core_wifi_combo_t));
    if(wifi_combo_handle == NULL){
        return -QM_ENOMEM;
    }
    memset(wifi_combo_handle, 0, sizeof(qm_iot_core_wifi_combo_t));

    sn = qm_iot_sn_get();
    if(sn[0] == '\0'){
        qm_ble_gap_get_local_addr(mac);
        qm_snprintf(str_mac, 12 + 1, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        qm_iot_sn_set(str_mac, strlen(str_mac));
    }

    QM_LOGD(LOG_TAG, "wifi open");
    qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI_COMBO, QM_IOT_CORE_EVENT_OPEN);
    return QM_EOK;
}

static int wifi_combo_start(void)
{
    qm_iot_reset_info_t reset_info = {0};
    if(wifi_combo_handle == NULL){
        return -QM_EINVAL;
    }

    qm_iot_reset_info_get(&reset_info);
    
    QM_LOGD("1", "reset:%d", reset_info.is_reset);
    
    if(reset_info.is_reset == QM_IOT_NOT_RESET){
       wifi_combo_sta_conn_router();
    }

    return QM_EOK;
}

static int wifi_combo_stop(void)
{
    if(wifi_combo_handle == NULL){
        return -QM_EINVAL;
    }

    if(!wifi_combo_handle->prov_handle)
    {
        qm_wifi_stop();

    }else{
        qm_iot_prov_deinit(&wifi_combo_handle->prov_handle);
        wifi_combo_handle->prov_handle = NULL;
    }

    return QM_EOK;
}
static int wifi_combo_ctrl(qm_iot_core_ctrl_t cmd, void *param)
{
    if(wifi_combo_handle == NULL){
        return -QM_EINVAL;
    }

    switch(cmd)
    {
        case QM_IOT_CORE_CTRL_CONFIG_NET:
            wifi_combo_provision();
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
            QM_LOGD(LOG_TAG, "get dynreg params");
            if(param) {
                *((char **)param) = qm_iot_dev_wifi_reginfo_get();
            }
        break;

        default:
            
        break;
    }

    return QM_EOK;
}


static void wifi_combo_sta_conn_router(void)
{
    qm_wifi_config_t config = {0};
    qm_iot_wifi_info_t wifi_info = {0};

    if(wifi_combo_handle == NULL){
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

    qm_event_register(QM_EVENT_WIFI, qm_comm_conn_handler, (void *)wifi_combo_handle);

    qm_wifi_init();
    qm_wifi_set_mode(QM_WIFI_MODE_STA);
    qm_wifi_set_config(QM_WIFI_IF_STA, &config);
    qm_wifi_start();
}

static void qm_comm_conn_handler(qm_input_event_t *input_event, void *arg)
{
    qm_iot_core_wifi_combo_t *wifi_combo = (qm_iot_core_wifi_combo_t *)arg;
    qm_wifi_event_info_t *event_info = (qm_wifi_event_info_t*)input_event->value;

    if(wifi_combo == NULL){
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

            qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI_COMBO, QM_IOT_CORE_EVENT_CONNECTED_NET);
        break;

        case QM_WIFI_EVENT_STA_LOST_IP:

        break;

        case QM_WIFI_EVENT_STA_DISCONNECTED:

            QM_LOGD(LOG_TAG, "sta disconnected reason: %d,retry to connect to the AP", event_info->sta_disconnected.reason);

            qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI_COMBO, QM_IOT_CORE_EVENT_DISCONNECT_NET);

            qm_wifi_connect();
        break;

        default:
        break;
    }
}

static void qm_iot_prov_delay_conn_wifi(void *arg)
{
    if(wifi_combo_handle == NULL){
        return ;
    }
    wifi_combo_sta_conn_router();
    qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI_COMBO, QM_IOT_CORE_EVENT_NETCONFG_COMPLETE);
}


static void qm_iot_prov_recv_handler(void *handle, const qm_iot_prov_recv_t *packet, void *userdata)
{
    size_t ssid_len = 0;
    size_t password_len = 0;
    qm_iot_core_wifi_combo_t *wifi_combo = (qm_iot_core_wifi_combo_t *)userdata;
    if(wifi_combo == NULL || handle == NULL || packet == NULL){
        return;
    }

#if CONFIG_QM_IOT_NTP_SUPPORT
   qm_iot_ntp_timezone_set(packet->timezone);
#endif

#if CONFIG_QM_IOT_DYNREG_SUPPORT
    size_t dynreg_host_len = strlen(packet->dynreg_host);
    if(dynreg_host_len < CONFIG_QM_IOT_DYNREG_MAX_LEN) {
        qm_iot_dynreg_host_set(packet->dynreg_host, dynreg_host_len);
    }
#endif

    qm_cancel_delayed_action(&wifi_combo->post_work);

    // 添加边界检查防止缓冲区溢出
    ssid_len = strlen(packet->ssid);
    password_len = strlen(packet->password);
    if(ssid_len >= sizeof(wifi_combo->wifi_info.ssid) || password_len >= sizeof(wifi_combo->wifi_info.pwd)) {
        return;
    }

    if(ssid_len < sizeof(wifi_combo->wifi_info.ssid) && password_len < sizeof(wifi_combo->wifi_info.pwd)) {
        memcpy(wifi_combo->wifi_info.ssid, packet->ssid, ssid_len);
        memcpy(wifi_combo->wifi_info.pwd, packet->password, password_len);
    }

    qm_iot_prov_deinit(&wifi_combo->prov_handle);
    qm_iot_wifi_info_set(&wifi_combo->wifi_info);

#if CONFIG_QM_IOT_AUDIO_URI_SUPPORT
    if(packet->audio_uri){
        qm_iot_audio_uri_set(packet->audio_uri, strlen(packet->audio_uri));
    }
#endif

    qm_post_delayed_action(&wifi_combo->post_work, qm_iot_prov_delay_conn_wifi, wifi_combo, 1000);
}

static void wifi_combo_provision(void)
{
    if(wifi_combo_handle == NULL){
        return ;
    }

    wifi_combo_handle->prov_handle = qm_iot_prov_init();
    if( wifi_combo_handle->prov_handle == NULL){
        QM_LOGE(LOG_TAG, "PROVISION CREATE FAILED!!");
        return ;
    }

    qm_iot_prov_setopt(wifi_combo_handle->prov_handle, QM_IOT_PROVDOPT_RECV_HANDLER, qm_iot_prov_recv_handler);
    qm_iot_prov_setopt(wifi_combo_handle->prov_handle, QM_IOT_PROVDOPT_USERDATA, (void *)wifi_combo_handle);

    qm_iot_prov_start(wifi_combo_handle->prov_handle);

    qm_iot_core_net_handle_event_notify(QM_IOT_NET_TYPE_WIFI_COMBO, QM_IOT_CORE_EVENT_NETCONFG_START);
}

static char *qm_iot_dev_wifi_devinfo_get(void)
{
    uint8_t mac[6] = {0};
    char *payload = NULL;
    qm_ip_info_t ip_info = {0};
    qm_wifi_ap_record_t ap_info = {0};
    char temp_value[QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN] = {0};
    cJSON *root = cJSON_CreateObject();
    if(root == NULL){
        goto __exit;
    }
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_TYPE_KEY, WIFI_COMBO_TYPE_KEY);

    qm_wifi_get_mac(QM_WIFI_IF_STA, mac);
    memset(temp_value, 0, QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN, 
                "%02X:%02X:%02X:%02X:%02X:%02X", 
                mac[0], mac[1], mac[2],mac[3],mac[4],mac[5]);
    cJSON_AddStringToObject(root, QM_IOT_COMMON_PARAMS_MAC_TYPE_KEY, temp_value); 

    qm_wifi_sta_get_ap_info(&ap_info);
    memset(temp_value, 0, QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN);
    qm_snprintf(temp_value, QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN,
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

static char *qm_iot_dev_wifi_reginfo_get(void)
{
    int nonce;
    int payload_len = 0;
    char *payload = NULL;
    char  hmac_source[128] = {0};
    char signature[64 + 1] = {0};

    uint32_t now_ms = qm_now_ms();

    qm_srandom(qm_now_ms());
    nonce = qm_random_get(WIFI_COMBO_DYNREG_NONCE_MAX);

    qm_snprintf(hmac_source,
                sizeof(hmac_source) - 1,
                "pid=%d,nonce=%d,sn=%s,timestamp=%d",
                qm_iot_pid_get(), nonce, qm_iot_sn_get(), now_ms);

    QM_LOGD(LOG_TAG, "| source(%d):%s", (int)strlen(hmac_source), hmac_source);

    qm_utils_hmac_sha1(hmac_source, strlen(hmac_source),
                    signature,
                    qm_iot_prodtuct_sercet_get(),
                    strlen(qm_iot_prodtuct_sercet_get()));

    QM_LOGD(LOG_TAG, "| signature (%d):%s", (int)strlen(signature), signature);

    payload_len = strlen(signature) + strlen(WIFI_COMBO_DYNERG_REQ_FORMAT) + QM_IOT_WIFI_COMBO_PAYLOAD_MAX_LEN;

    payload = (char *)qm_malloc(payload_len + 1);
    if(payload == NULL){
        return NULL;
    }
    memset(payload, 0, payload_len + 1);

    qm_snprintf(payload, payload_len, WIFI_COMBO_DYNERG_REQ_FORMAT, 
                    qm_iot_sn_get(), qm_iot_pid_get(), now_ms, signature, nonce);

    return payload;
}