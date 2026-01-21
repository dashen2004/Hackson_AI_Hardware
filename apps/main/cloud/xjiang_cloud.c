#include "qm.h"
#include "qm_iot_api.h"
#include "qm_ble_gap.h"
#include "qm_cli.h"
#include "qm_kv.h"
#include "qm_utils_string.h"
#include "xjiang_cloud.h"

#include "qm_work.h"
#include "qm_event.h"
#include "qm_iot_ota.h"

#include "audio_sys.h"
#include "audio_thread.h"
#include "audio_mem.h"

#include "ai_main.h"
#include "qm_wifi.h"
#include "qm_ai_license.h"
#include "qm_ai_api.h"

#include "backoff_algorithm.h"

#include "esp_netif.h"
#include "esp_netif_types.h"

#define LOG_TAG "APP"
#define WEATHER_REQ_TIMEOUT_MS      (15 * 1000)
#define BACKOFF_BASE_TIME_MS        (3 * 1000)
#define BACKOFF_RECV_TIMEOUT_MS     (30 * 1000)
#define BACKOFF_DEFAULT_TIMEOUT_MS  (5 * 60 * 1000)
#define BACKOFF_RETRY_NUM           BACKOFF_ALGORITHM_RETRY_FOREVER

static void *iot_handle = NULL;
static bool_t connect = QM_FALSE;
static qm_timer_t weather_request_timer = { 0 };
static qm_timer_t weather_timeout_timer = { 0 };
static qm_work_t weather_request_action = { 0 };
static qm_work_t set_disconnected_action = { 0 };
static qm_work_t set_connected_action = { 0 };
static qm_work_t set_blink_action = { 0 };
static uint8_t ota_status_flag = 0;
backoff_algorithm_ctx_t backoff_ctx = {0};
backoff_algorithm_status_t backoff_status = BACKOFF_ALGORITHM_SUCCESS;
uint16_t next_backoff = 0;

static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv);
static void handle_resetinfo(char *pwbuf, int blen, int argc, char **argv);
static void handle_sta_rssi(char *pwbuf, int blen, int argc, char **argv);

bool_t get_connect(void) {
    return connect;
}

uint8_t get_ota_status_flag(void) {
    return ota_status_flag;
}

static struct qm_cli_command meminfo_key = {
    .name = "meminfo",
    .help = "meminfo",
    .function = handle_meminfo
};

static struct qm_cli_command resetinfo_key = {
    .name = "reset",
    .help = "cloud reset",
    .function = handle_resetinfo
};

static struct qm_cli_command sta_rssi_key = {
    .name = "sta_rssi",
    .help = "get sta rssi",
    .function = handle_sta_rssi
};


static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv)
{
    audio_sys_get_real_time_stats();
    AUDIO_MEM_SHOW("XJ");
}

static void handle_resetinfo(char *pwbuf, int blen, int argc, char **argv)
{
    // qm_set_reset_flag();
    qm_iot_reset(iot_handle);
}

static void handle_sta_rssi(char *pwbuf, int blen, int argc, char **argv)
{
    int8_t rssi = 0;

    if(!connect){
        qm_cli_printf("WARING:device not connect wifi!!!");
        return ;
    }

    qm_wifi_sta_get_rssi(&rssi);

    qm_cli_printf("wifi rssi:%d!!!", (int)rssi);
}

static void qm_iot_event_handler(void *handle, const qm_iot_event_t *event, void *userdata)
{
    static qm_iot_event_type_t last_type;
    if(handle == NULL || event == NULL){
        return ;
    }

    switch (event->type)
    {
        case QM_IOT_EVENT_CONFIG_TIMEOUT:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONFIG_TIMEOUT");
            connect = QM_FALSE;
            // qm_post_delayed_action(&set_disconnected_action, delay_set_disconnected, NULL, 0);
            // qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONFIG_TIMEOUT, NULL, 0);    
            break;

        case QM_IOT_EVENT_CONFIG:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONFIG");
            connect = QM_FALSE;
            // qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONFIG, NULL, 0);
            break;

        case QM_IOT_EVENT_CONNING_LOCAL:    // 连接路由器
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONNING_LOCAL");
            connect = QM_FALSE;
            // qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_USB_EVENT_CONNING_LOCAL, NULL, 0);
            break;

        case QM_IOT_EVENT_CLOUD_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_CONN");
            connect = QM_TRUE;
            // qm_event_post(QM_EVENT_QM_AI, QM_AI_EVENT_CLOUD_CONN, NULL, 0);
            // qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CEART_WS, NULL, 0);
            // qm_post_delayed_action(&set_connected_action, delay_set_connected, NULL, 0);
            break;
        case QM_IOT_EVENT_BIND_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_BIND_NOTIFY");
            // qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CEART_WS, NULL, 0);
            break;
        case QM_IOT_EVENT_CLOUD_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_DISCONN");
            connect = QM_FALSE;
            // qm_post_delayed_action(&set_blink_action, delay_set_blink, NULL, 0);
            break;

        case QM_IOT_EVENT_LOCAL_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_CONN");
            connect = QM_FALSE;
            break;

        case QM_IOT_EVENT_LOCAL_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_DISCONN");
            connect = QM_FALSE;
            // qm_post_delayed_action(&set_blink_action, delay_set_blink, NULL, 0);
            // qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_NET_ERROR, NULL, 0);
            break;

        case QM_IOT_EVENT_RESET:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_RESET");
            // qm_set_reset_flag();
            connect = QM_FALSE;
            break;

        case QM_IOT_EVENT_OTA_START:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_START");
            ota_status_flag = 1;
            break;

        case QM_IOT_EVENT_OTA_PROGRESS_UPADTE:
            ota_status_flag = 2;
            break;

        case QM_IOT_EVENT_OTA_SUCCESS:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_SUCCESS");
            // qm_set_reset_flag();
            ota_status_flag = 0;
            break;

        case QM_IOT_EVENT_OTA_FAILED:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_FAILED");
            ota_status_flag = 0;
            break;

        default:
            break;
    }
}

static void qm_iot_recv_handler(void *handle, const qm_iot_recv_t *recv, void *userdata)
{
    if(handle == NULL || recv == NULL){
        return ;
    }

    switch (recv->type)
    {
        case QM_IOT_RECV_TYPE_SET:
            QM_LOGD(LOG_TAG, "app recv set siid %d piid %d!!!", recv->siid, recv->piid);
            // audio_light_action_and_report(recv->data.set.property);
        break;
        
        case QM_IOT_RECV_TYPE_PROPERTY_SET:
            QM_LOGD(LOG_TAG, "app recv prop set did %d siid %d piid %d!!!", recv->did, recv->siid, recv->piid);
            // audio_light_action_and_report(recv->data.set.property);
        break;

        case QM_IOT_RECV_TYPE_PROPERTY_GET_RSP:
            QM_LOGD(LOG_TAG, "app recv prop get did %d siid %d piid %d!!!", recv->did, recv->siid, recv->piid);
            // audio_light_action_and_report(recv->data.set.property);
        break;

        case QM_IOT_RECV_TYPE_PROPERTY_GET_REQ:
            QM_LOGD(LOG_TAG, "app recv prop get did %d siid %d piid %d!!!", recv->did, recv->siid, recv->piid);
        break;

        default:

        break;
    }
}

// qm_err_t xjiang_cloud_multi_report(qm_iot_core_msg_type_t type, qm_spec_property_operation_t *operation)
// {
//     if(connect == QM_FALSE){
//         qm_spec_property_operation_delete(operation);
//         return -QM_EIO;
//     }
//     switch (type)
//     {
//         case QM_IOT_CORE_MSG_TYPE_AWS_REPORT:
//             qm_iot_report(iot_handle, operation);
//         break;
//         case QM_IOT_CORE_MSG_TYPE_GET_REQ:
//             qm_iot_property_get_request(iot_handle, operation);
//         break;
//         case QM_IOT_CORE_MSG_TYPE_GET_RSP:
//             qm_iot_property_get_rsponse(iot_handle, operation);
//         break;
//         case QM_IOT_CORE_MSG_TYPE_REPORT:
//             qm_iot_property_report(iot_handle, operation);
//         break;
//         default:
//         break;
//     }
//     return QM_EOK;
// }

qm_err_t xjiang_cloud_report(qm_spec_property_t *property)
{
    static qm_spec_property_operation_t *property_operation = NULL;
    qm_spec_property_t *property_pre = NULL;

    if(connect == QM_FALSE){
        return -QM_EIO;
    }

    if(property_operation == NULL){
        property_operation = qm_spec_property_operation_creat();
    }

    if(property_operation == NULL){
        return -QM_ENOMEM;
    }

    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return -QM_ENOMEM;
    }

    qm_spec_property_copy(property_pre, property);

    qm_spec_property_add(property_operation, property_pre);

    if(property->next == NULL){
        qm_iot_report(iot_handle, property_operation);
        property_operation = NULL;
    }

    return QM_EOK;
}

int xjiang_cloud_reset(void)
{
    // qm_set_reset_flag();
    // bsp_light_reset();
    qm_iot_reset(iot_handle);
    // bsp_light_store_update();
    return QM_EOK;
}

int xjiang_cloud_init(void)
{   
    uint32_t pid = CONFIG_XJIANG_CLOUD_PID;
    uint32_t version = CONFIG_XJIANG_VERSION;
  
    // qm_cli_register_command(&meminfo_key);
    // qm_cli_register_command(&resetinfo_key);
    // qm_cli_register_command(&sta_rssi_key);

    iot_handle = qm_iot_init();
    if(iot_handle == NULL){
        QM_LOGE(LOG_TAG, "qm_api_init failed !!!");
        return -QM_EIO;
    }

    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_ID, &pid);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_VERSION, &version);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_SECRET, CONFIG_XJIANG_CLOUD_SECERT);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_RECV_HANDLER, qm_iot_recv_handler);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_EVENT_HANDLER, qm_iot_event_handler);

    return qm_iot_start(iot_handle);
}

void *xjiang_cloud_get_handle()
{
    return iot_handle;
}

qm_err_t iot_voice_corpus_request(uint8_t number)
{
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    qm_spec_property_t *property_pre = NULL;
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return -QM_ENOMEM;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_CORPUS_PIID);
    qm_spec_property_pack_number(property_pre, number);
    property_pre->did = qm_iot_did_get();

    qm_spec_property_add(property_operation, property_pre);
    return 0;
    // return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_REPORT, property_operation);
}
