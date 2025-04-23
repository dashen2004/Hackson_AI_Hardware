#include "qm.h"
#include "qm_iot_api.h"
#include "qm_ble_gap.h"
#include "qm_cli.h"
#include "qm_kv.h"
#include "qm_utils_string.h"
#include "xjiang_cloud.h"

#include "audio_main.h"
#include "audio_light.h"
#include "qm_audio.h"
#include "qm_work.h"
#include "qm_event.h"

#include "audio_sys.h"
#include "audio_thread.h"
#include "audio_mem.h"

#define LOG_TAG "APP"

static void *iot_handle = NULL;
static bool_t connect = QM_FALSE;

static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv);
static void handle_resetinfo(char *pwbuf, int blen, int argc, char **argv);

bool_t get_connect(void) {
    return connect;
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


static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv)
{
    audio_sys_get_real_time_stats();
    AUDIO_MEM_SHOW("XJ");
}

static void handle_resetinfo(char *pwbuf, int blen, int argc, char **argv)
{
    qm_iot_reset(iot_handle);
}

static void qm_iot_event_handler(void *handle, const qm_iot_event_t *event, void *userdata)
{
    static qm_iot_event_type_t last_type;
    if(handle == NULL || event == NULL){
        return ;
    }

    switch (event->type)
    {
        case QM_IOT_EVENT_CLOUD_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_CONN");
            connect = QM_TRUE;
            audio_light_snapshot_report();
            qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_CEART_WS, NULL, 0);

            qm_iot_set_weahter_request(iot_handle);
        
        break;

        case QM_IOT_EVENT_CLOUD_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_CLOUD_DISCONN");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_LOCAL_CONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_CONN");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_LOCAL_DISCONN:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_LOCAL_DISCONN");
            connect = QM_FALSE;
        break;

        case QM_IOT_EVENT_RESET:
            QM_LOGD(LOG_TAG, "QM_IOT_EVENT_RESET");
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

    QM_LOGD(LOG_TAG, "app recv type %d siid %d piid %d!!!", recv->type, recv->siid, recv->piid);

    audio_light_action_and_report(recv->data.set.property);
}

qm_err_t xjiang_cloud_multi_report(qm_spec_property_operation_t *operation)
{
    if(connect == QM_FALSE){
        qm_spec_property_operation_delete(operation);
        return -QM_EIO;
    }

    return qm_iot_report(iot_handle, operation);
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
    return qm_iot_reset(iot_handle);
}

int xjiang_cloud_init(void)
{   
    uint32_t pid = CONFIG_XJIANG_CLOUD_PID;
    uint32_t version = CONFIG_XJIANG_VERSION;
  
    qm_cli_register_command(&meminfo_key);
    qm_cli_register_command(&resetinfo_key);

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

