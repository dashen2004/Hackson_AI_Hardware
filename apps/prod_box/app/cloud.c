#include "cloud.h"
#include "qm_iot_export.h"
#include "qm_iot_import.h"
#include "qm_log.h"
#include "qmos/qm.h"
#include <qmos/qm_kernel.h>
#include "qm_kvmgr.h"
#include "sys_notify.h"

#include "esp_wifi.h"
#include "prodtst_cloud.h"
#include "qm_cli.h"
#include "qm_utils_string.h"
#include "drivers.h"


#define LOG_TAG  "cloud"

int xjiang_login = 0;
int xjiang_cloud_start = 0;
int device_info_check_ret = 0;
static int is_netconfig = 0;


#define RETRY_INTERVAL_MAX_MS  60000

static int retry_interval_ms = 500;
static int retry_interval_max_ms = RETRY_INTERVAL_MAX_MS;
static void retry_interval_ms_set(int interval_ms, int interval_max_ms);
static int retry_interval_ms_get(void);

static void cloud_thread(void *arg);
static void cloud_dynreg_thread(void *arg);

static void network_handle(void *arg);

static void handle_xjiang_key(char *pwbuf, int blen, int argc, char **argv);
static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv);

static struct qm_cli_command xjiang_key = {
    .name = "xjiang_key",
    .help = "set/get xjiang keys [<Product Id> <Device Id> <Device Secret>]",
    .function = handle_xjiang_key
};

static struct qm_cli_command meminfo_key = {
    .name = "meminfo",
    .help = "meminfo",
    .function = handle_meminfo
};

static void handle_meminfo(char *pwbuf, int blen, int argc, char **argv)
{
    qm_cli_printf("system free mem: %u", esp_get_free_heap_size());
}

static void handle_xjiang_key(char *pwbuf, int blen, int argc, char **argv)
{
    char device_id[DEVICE_ID_LEN + 1] = { 0 };
    char device_secret[DEVICE_SECRET_LEN + 1] = { 0 };
    int device_id_len = DEVICE_ID_LEN;
    int device_secret_len = DEVICE_SECRET_LEN;
    uint32_t product_id = 0;
    int len = 0;
    if(argc == 2){
        hal_get_device_id(device_id, &device_id_len);
        hal_get_device_secret(device_secret, &device_secret_len);
        hal_get_product_id(&product_id);
        qm_cli_printf("Product Id=%u.\r\n", product_id);
        qm_cli_printf("Device Id=%s.\r\n", device_id);
        qm_cli_printf("Device Secret=%s.\r\n", device_secret);
    }else if (argc == 5) {
        product_id = int_str_to_num(argv[2], strlen(argv[2]));
        hal_set_product_id(product_id);
        hal_set_device_id(argv[3], strlen(argv[3]));
        hal_set_device_secret(argv[4], strlen(argv[4]));
        qm_cli_printf("OK\r\n");
    } else {
        qm_cli_printf("Error\r\n");
        return;
    }
}

int is_netconfig_get(void)
{
    return is_netconfig;
}

int is_xjiang_login(void)
{
    return xjiang_login;
}

static void cloud_event_monitor( int event )
{
    switch(event){

        case EVENT_VALUE_LOGIN:
            xjiang_login = 1;

            retry_interval_ms_set(500, RETRY_INTERVAL_MAX_MS);

            led_on(LED_TYPE_NETWORK);

        break;

        case EVENT_VALUE_LOGOUT:
            QM_LOGI(LOG_TAG, "Device Logout");
            xjiang_login = 0;
            led_off(LED_TYPE_NETWORK);
        break;
        
        case EVENT_VALUE_TOKEN_REPORT_SUC:

            QM_LOGI(LOG_TAG, "report token success");
            
        break;

        case EVENT_VALUE_RESET: //just got reset response from cloud without any other operation
            QM_LOGI(LOG_TAG, "iot reset");

        break;
        
        default:
        break;
    }

}


void prodtst_set_handler(int devid, int id, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "set: %.*s", payload_len, payload);
    prodtst_set_unpack_and_handle(id, payload, payload_len);
}

void prodtst_report_reply_handler(int devid, int id, int code, const char* payload, int payload_len)
{
     QM_LOGI(LOG_TAG, "report reply id: %d, code: %d", id, code);
}

void prodtst_config_handler(int devid, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "config: %.*s", payload_len, payload);

    prodtst_config_unpack_and_handle(payload, payload_len);
}

void prodtst_test_reply_handler(int devid, int id, int code, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "report reply id: %d, code: %d", id, code);
}

void prodtst_get_handler(int devid, int id, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "get id: %d, %.*s", id, payload_len, payload);
    prodtst_get_unpack_and_handle(id, payload, payload_len);
}

void prodtst_result_handler(int devid, int id, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "result: %.*s", payload_len, payload);
    prodtst_result_unpack_and_handle(id, payload, payload_len);
}

void prodtst_result_report_reply_handler(int devid, int id, int code, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "result reply id: %d, code: %d", id, code);
}

void prodtst_spcl_down_handler(int devid, int id, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "special down: %.*s", payload_len, payload);
    prodtst_spcl_down_unpack_and_handle(id, payload, payload_len);
}

void prodtst_spcl_up_reply_handler(int devid, int id, int code, const char* payload, int payload_len)
{
    QM_LOGI(LOG_TAG, "special up reply id: %d, code: %d", id, code);
}

int user_event_notify_handler(int devid, int event, const char* payload, int payload_len)
{
    int err_code = 0;

    if(payload){
        QM_LOGI(LOG_TAG, "notify: %.*s", payload_len, payload);
    }

    QM_LOGI(LOG_TAG, "event notify: %d", event);
    switch ( event ){

        case EVENT_VALUE_BIND_NOTIFY:

            QM_LOGI(LOG_TAG, "device bind");
            is_netconfig = 0;
            
        break;

        case EVENT_VALUE_UNBIND_NOTIFY:
            QM_LOGI(LOG_TAG, "device unbind");

        break;

        default:
        break;
    }
    
    return err_code;
}

static void network_status_changed_handler(network_status_t status, void* arg)
{
    qm_task_t cloud_task = {0};
    switch(status){

        case NETWORK_STATUS_UP:

        if(!xjiang_cloud_start){
            qm_task_new(&cloud_task, "cloud", cloud_thread, NULL, 6*1024, 20);
            xjiang_cloud_start = 1;
        }

        break;

        case NETWORK_STATUS_DOWN:

        break;

        default:
        break;
    }

}


static void retry_interval_ms_set(int interval_ms, int interval_max_ms)
{
    retry_interval_ms = interval_ms;
    retry_interval_max_ms = interval_max_ms;
}

static int retry_interval_ms_get(void)
{
    if (RETRY_INTERVAL_MAX_MS > retry_interval_ms) {
        retry_interval_ms *= 2;
    } else {
        retry_interval_ms = RETRY_INTERVAL_MAX_MS;
    }
    return retry_interval_ms;
}

static void cloud_thread(void *arg)
{
    int rc = 0;
    int ret = 0;
    int invalid = 0;
    int id = 0;
    uint32_t pid;
    iot_dev_meta_info_t dev_meta_info = {0};

    int device_id_len = DEVICE_ID_LEN;
    int device_secret_len = DEVICE_SECRET_LEN;

    ret = hal_get_product_id(&dev_meta_info.product_id);
    if(ret != 0){
        QM_LOGE(LOG_TAG, "product id is invalid");
        invalid = 1;
    }
    ret = hal_get_device_id(dev_meta_info.device_id, &device_id_len);
    if(ret != 0){
        QM_LOGE(LOG_TAG, "device id is invalid");
        invalid = 1;
    }
    ret = hal_get_device_secret(dev_meta_info.device_secret, &device_secret_len);
    if(ret != 0){
        QM_LOGE(LOG_TAG, "device secret is invalid");
        invalid = 1;
    }

    if(invalid){
        while(1){
            qm_msleep(1000);
        }
    }

    QM_LOGI(LOG_TAG, "pi[%u]", dev_meta_info.product_id );
    QM_LOGI(LOG_TAG, "di[%s]", dev_meta_info.device_id );
    QM_LOGI(LOG_TAG, "ds[%s]", dev_meta_info.device_secret );

    id = cloud_open(IOT_DEV_TYPE_MASTER, &dev_meta_info);
    app_version_set(APP_VERSION);

    retry_interval_ms_set(500, RETRY_INTERVAL_MAX_MS);

    while(1){

        rc = cloud_connect();
        if( rc < 0 ){
            qm_msleep(retry_interval_ms_get());
        }else{
            break;
        }
    }

    while(1){
        cloud_loop();
    }
}


void xjiang_cloud_init(void)
{
    iot_event_regist_cb( cloud_event_monitor );

    QM_LOGI(LOG_TAG, "SDK verison %s",sdk_version_get());
    QM_LOGI(LOG_TAG, "app verison %s",APP_VERSION);

    qm_cli_register_command(&xjiang_key);
    qm_cli_register_command(&meminfo_key);

    iot_event_register(IOT_EVENT_PRODTST_SET, prodtst_set_handler);
    iot_event_register(IOT_EVENT_PRODTST_REPORT_REPLY, prodtst_report_reply_handler);
    iot_event_register(IOT_EVENT_PRODTST_CONFIG, prodtst_config_handler);
    iot_event_register(IOT_EVENT_PRODTST_TEST_REPLY, prodtst_test_reply_handler);
    iot_event_register(IOT_EVENT_PRODTST_GET, prodtst_get_handler);
    iot_event_register(IOT_EVENT_PRODTST_RESULT, prodtst_result_handler);
    iot_event_register(IOT_EVENT_PRODTST_RESULT_REPORT_REPLY, prodtst_result_report_reply_handler);
    iot_event_register(IOT_EVENT_PRODTST_SPCL_DOWN, prodtst_spcl_down_handler);
    iot_event_register(IOT_EVENT_PRODTST_SPCL_UP_REPLY, prodtst_spcl_up_reply_handler);

    iot_event_register(IOT_EVENT_NOTIFY, user_event_notify_handler);

    system_notify_register(notify_NETWORK_STATUS_CHANGED, network_status_changed_handler, NULL);


}
