#include "prodtst_adapter.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "qm_log.h"
#include "qm_errno.h"
#include "prodtst_core.h"
#include "prodtst_tcp_mgr.h"
#include "prodtst_config.h"

#define LOG_TAG  "prodtst wifi"


int prodtst_wifi_ap_event_handler(uint32_t ip)
{
    char *ip_str = NULL;

    ip_str = inet_ntoa(ip);
    QM_LOGI(LOG_TAG, "device found ip: %s\n", ip_str);
    return prodtst_ip_notify(ip_str);
}

static int wifi_init(void)
{

#if 0
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    ESP_ERROR_CHECK(esp_wifi_start() );
#endif

    return QM_EOK;
}


static int wifi_prodtst_init(void *param)
{
    int max_conn = 0;

    max_conn = prodtst_device_num_get();

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_QM_PRODTST_AP_SSID,
            .ssid_len = strlen(CONFIG_QM_PRODTST_AP_SSID),
            .channel = CONFIG_QM_PRODTST_WIFI_CHANNEL,
            .password = CONFIG_QM_PRODTST_AP_PASSWD,
            .max_connection = max_conn,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    wifi_init();

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    QM_LOGI(LOG_TAG, "softap finished. SSID:%s, password:%s, channel:%d, max connection:%d",
             CONFIG_QM_PRODTST_AP_SSID, CONFIG_QM_PRODTST_AP_PASSWD, CONFIG_QM_PRODTST_WIFI_CHANNEL, 1);


    prodtst_tcp_mgr_init();

    return QM_EOK;
}



static int wifi_prodtst_deinit(void)
{
    esp_wifi_stop();
    esp_wifi_deinit();
    return QM_EOK;
}


static int channel_send(void *handle, char *buf, int size)
{
    int ret = 0;
    if(handle == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    return prodtst_tcp_mgr_write(handle, (uint8_t*)buf, size); 
}

static int channel_recv(void *handle, char *buf, int size, int timeout)
{
    if(handle == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    if(timeout == QM_WAIT_FOREVER){
        timeout = 0;
    }

    return prodtst_tcp_mgr_read(handle, (uint8_t*)buf, size, timeout);
}

static int wifi_channel_get(void **handle)
{
    *handle = prodtst_tcp_mgr_handle();
    return QM_EOK;
}

static int channel_establish(void *handle, void *param)
{
    int fd = 0;
    qm_err_t ret = QM_EOK;

    QM_LOGI(LOG_TAG, "tcp connecting...");

    fd = prodtst_tcp_mgr_establish(handle, param, CONFIG_QM_PRODTST_TCP_PORT);
    if(fd >= 0){
        QM_LOGI(LOG_TAG, "tcp connected, fd: %d", fd);
    }else{
        QM_LOGI(LOG_TAG, "tcp connect failed");
        ret = -QM_EIO;
        goto __exit;
    }

__exit:
    if(param){
        qm_free(param);
        param = NULL;
    }
    return ret;
}

static int channel_destroy(void *handle)
{
    prodtst_tcp_mgr_destroy(handle);

    QM_LOGI(LOG_TAG, "tcp closed");
    return QM_EOK;
}

prodtst_adapter_t wifi_prodtst_adapter = {

    wifi_prodtst_init,
    wifi_prodtst_deinit,
    wifi_channel_get,
    channel_establish,
    channel_destroy,
    channel_send,
    channel_recv,
};


