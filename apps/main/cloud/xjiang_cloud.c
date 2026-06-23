#include "qm.h"
#include "qm_iot_api.h"
#include "qm_ble_gap.h"
#include "qm_cli.h"
#include "qm_kv.h"
#include "qm_utils_string.h"
#include "xjiang_cloud.h"

#include "ai_main.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_iot_ota.h"

#include "audio_sys.h"
#include "audio_thread.h"
#include "audio_mem.h"

#include "qm_wifi.h"
#include "vars.h"

#include "backoff_algorithm.h"

#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_log.h"

#include "qm_ai_api.h"

#include "qm_nw_switch.h"
#include "bsp_oss_upload.h"
#include "bsp_camera.h"

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
static qm_work_t wifi_disconn_action = {0};
static qm_work_t oss_upload_action = {0};
static bool oss_upload_done = false;
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
    qm_set_reset_flag();
    qm_iot_reset(iot_handle);
}

static void weather_request_action_handler(void *arg) {
    qm_timer_start(&weather_timeout_timer);

    qm_iot_set_weahter_request(iot_handle);
}

static void weather_timeout_timer_handler(qm_timer_t *timer,  void *arg) {
    qm_timer_stop(&weather_timeout_timer);

    if (backoff_ctx.attempts_num == 5) {
        QM_LOGD(LOG_TAG, "weather request retry 5 times!");
#if !APP_HEADLESS_CAMERA_OSS
        lvgl_var_weather_id_set(-1);
#endif
    }

    backoff_status = backoff_algorithm_get_next_delay(&backoff_ctx, &next_backoff);
    if (backoff_status != BACKOFF_ALGORITHM_SUCCESS) {
        next_backoff = BACKOFF_DEFAULT_TIMEOUT_MS;
    }

    qm_post_delayed_action(&weather_request_action, weather_request_action_handler, NULL, next_backoff);
}

static void weather_request_timer_handler(qm_timer_t *timer,  void *arg) {
    if (!qm_timer_is_valid(&weather_timeout_timer)) {
        qm_timer_new(&weather_timeout_timer, weather_timeout_timer_handler, NULL, WEATHER_REQ_TIMEOUT_MS, 1);
    }

    backoff_algorithm_init(&backoff_ctx, BACKOFF_BASE_TIME_MS, BACKOFF_RECV_TIMEOUT_MS, BACKOFF_RETRY_NUM);

    qm_cancel_delayed_action(&weather_request_action);
    qm_post_delayed_action(&weather_request_action, weather_request_action_handler, NULL, 0);
}

static void delay_set_disconnected(void *argv) {
    // wifi_set_disconnected();
}

static void delay_set_connected(void *argv) {
    // wifi_set_connected();
}

static void delay_set_blink(void *argv) {
    // wifi_set_blink();
}

static void switch_net_type(void *arg)
{
    qm_nw_switch_4g();
}

static void oss_upload_action_handler(void *arg)
{
    (void)arg;

    if (oss_upload_done) {
        return;
    }

    oss_upload_done = true;
    ESP_LOGI(LOG_TAG, "cloud connected, trigger camera upload");
    bsp_camera_capture_upload();
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
            qm_post_delayed_action(&set_disconnected_action, delay_set_disconnected, NULL, 0);
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONFIG_TIMEOUT, NULL, 0);    
            break;

        case QM_IOT_EVENT_CONFIG:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONFIG");
            connect = QM_FALSE;
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CONFIG, NULL, 0);
            break;

        case QM_IOT_EVENT_CONNING_LOCAL:    // 连接路由器
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CONNING_LOCAL");
            connect = QM_FALSE;
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_USB_EVENT_CONNING_LOCAL, NULL, 0);
            break;

        case QM_IOT_EVENT_CLOUD_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_CONN");
            connect = QM_TRUE;
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CEART_WS, NULL, 0);

            qm_cancel_delayed_action(&wifi_disconn_action);

            extern void qm_example_ai_init(void *handle);
            qm_example_ai_init(iot_handle);

#if !APP_HEADLESS_CAMERA_OSS
            if (!qm_timer_is_valid(&weather_timeout_timer)) {
                qm_timer_new(&weather_timeout_timer, weather_timeout_timer_handler, NULL, WEATHER_REQ_TIMEOUT_MS, 1);
            }

            backoff_algorithm_init(&backoff_ctx, BACKOFF_BASE_TIME_MS, BACKOFF_RECV_TIMEOUT_MS, BACKOFF_RETRY_NUM);

            qm_cancel_delayed_action(&weather_request_action);
            qm_post_delayed_action(&weather_request_action, weather_request_action_handler, NULL, 0);

            if (!qm_timer_is_valid(&weather_request_timer)) {
                qm_timer_new(&weather_request_timer, weather_request_timer_handler, NULL, 60 * 60 * 1000, 1); // 1小时尝试更新一次
                qm_timer_start(&weather_request_timer);
            }
#endif
            qm_post_delayed_action(&set_connected_action, delay_set_connected, NULL, 0);

#if APP_HEADLESS_CAMERA_OSS
            if (!oss_upload_done) {
                qm_cancel_delayed_action(&oss_upload_action);
                qm_post_delayed_action(&oss_upload_action, oss_upload_action_handler, NULL, 5 * 1000);
            }
#endif
            break;
        case QM_IOT_EVENT_BIND_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_BIND_NOTIFY");
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CEART_WS, NULL, 0);
            break;
        case QM_IOT_EVENT_CLOUD_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_DISCONN");
            connect = QM_FALSE;
            qm_post_delayed_action(&set_blink_action, delay_set_blink, NULL, 0);
            if(ota_status_flag) {
                ota_status_flag = 0;
                // set_var_ota_status(2, true);
            }
            if (qm_nw_get_net_type() == QM_IOT_NET_TYPE_WIFI_COMBO) {
                qm_post_delayed_action(&wifi_disconn_action, switch_net_type, NULL, 10 * 1000);
            }
            break;

        case QM_IOT_EVENT_LOCAL_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_CONN");
            esp_netif_ip_info_t ip_info;
            int ret = esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
            char s_ip[32];
            sprintf(s_ip, "%d.%d.%d.%d", IP2STR(&(ip_info.ip)));
            // set_var_ip_addr(s_ip);
            connect = QM_FALSE;
            break;

        case QM_IOT_EVENT_LOCAL_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_DISCONN");
            // set_var_ip_addr("-");
            connect = QM_FALSE;
            qm_post_delayed_action(&set_blink_action, delay_set_blink, NULL, 0);
            if(ota_status_flag) {
                ota_status_flag = 0;
                // set_var_ota_status(2, true);
            }
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_NET_ERROR, NULL, 0);
            break;

        case QM_IOT_EVENT_RESET:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_RESET");
            qm_set_reset_flag();
            connect = QM_FALSE;
            break;

        case QM_IOT_EVENT_WEATHER_NOTIFY:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_WEATHER_NOTIFY");

            QM_LOGD(LOG_TAG, "weather_type:%d", event->data.weather_notify.weather_type);
            QM_LOGD(LOG_TAG, "temp:%d", event->data.weather_notify.temp);
            QM_LOGD(LOG_TAG, "humidity:%d", event->data.weather_notify.humidity);
            QM_LOGD(LOG_TAG, "city_id:%d", event->data.weather_notify.city_id);
            QM_LOGD(LOG_TAG, "condition:%.*s", event->data.weather_notify.condition_len, event->data.weather_notify.condition);
            QM_LOGD(LOG_TAG, "country_name:%.*s", event->data.weather_notify.country_name_len, event->data.weather_notify.country_name);
            QM_LOGD(LOG_TAG, "pname:%.*s", event->data.weather_notify.pname_len, event->data.weather_notify.pname);
            QM_LOGD(LOG_TAG, "secondaryname:%.*s", event->data.weather_notify.secondaryname_len, event->data.weather_notify.secondaryname);
            QM_LOGD(LOG_TAG, "name:%.*s", event->data.weather_notify.name_len, event->data.weather_notify.name);
            QM_LOGD(LOG_TAG, "sunset:%.*s", event->data.weather_notify.sunset_len, event->data.weather_notify.sunset);
            QM_LOGD(LOG_TAG, "sunrise:%.*s", event->data.weather_notify.sunrise_len, event->data.weather_notify.sunrise);

#if !APP_HEADLESS_CAMERA_OSS
            lvgl_var_city_set(event->data.weather_notify.secondaryname, event->data.weather_notify.secondaryname_len);
            lvgl_var_weather_id_set(event->data.weather_notify.weather_type);
            lvgl_var_temperature_set(event->data.weather_notify.temp);
#endif
            qm_timer_stop(&weather_timeout_timer);
            qm_timer_free(&weather_timeout_timer);
            qm_cancel_delayed_action(&weather_request_action);
            break;

        case QM_IOT_EVENT_OTA_START:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_START");
            // set_var_ota_status(0, true);
            ota_status_flag = 1;
            break;

        case QM_IOT_EVENT_OTA_PROGRESS_UPADTE:
            // set_var_ota_status(0, true);
            // set_var_ota_percent(event->data.ota_progress_update.progress_details);
            ota_status_flag = 2;
            break;

        case QM_IOT_EVENT_OTA_SUCCESS:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_SUCCESS");
            qm_set_reset_flag();
            // set_var_ota_status(1, true);
            ota_status_flag = 0;
            break;

        case QM_IOT_EVENT_OTA_FAILED:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_OTA_FAILED");
            // set_var_ota_status(2, true);
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
            bsp_nw_cmd_action(recv->data.set.property);
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
    qm_set_reset_flag();
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
    qm_nw_switch_init(iot_handle);

    return qm_iot_start(iot_handle);
}

