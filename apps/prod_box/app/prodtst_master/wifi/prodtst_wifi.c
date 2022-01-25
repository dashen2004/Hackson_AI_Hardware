#include "prodtst_adapter.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "sys_notify.h"
#include "esp_event_loop.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "qm_log.h"
#include "qm_errno.h"
#include "prodtst_core.h"
#include "prodtst_tcp_mgr.h"

#define LOG_TAG  "prodtst wifi"

#ifndef CONFIG_QM_PRODTST_AP_SSID
#define CONFIG_QM_PRODTST_AP_SSID       "xj_prodtst"
#endif

#ifndef CONFIG_QM_PRODTST_AP_PASSWD
#define CONFIG_QM_PRODTST_AP_PASSWD     "asdf062516"
#endif

#ifndef CONFIG_QM_PRODTST_WIFI_CHANNEL
#define CONFIG_QM_PRODTST_WIFI_CHANNEL     1
#endif

typedef struct{
    int fd;
    prodtst_ip_info_t ip_info;
}wifi_handle_t;

static wifi_handle_t* wifi_handle_get(void);

static wifi_handle_t g_wifi_handle[CONFIG_QM_PRODTST_OBJ_NUM] = {0};

static int wifi_handle_init(void);


int prodtst_wifi_ap_event_handler(uint32_t ip)
{
    char *ip_str = NULL;
    prodtst_ip_info_t ip_info = {0};

    wifi_handle_t *wifi_handle = wifi_handle_get();
    if(wifi_handle == NULL){
        QM_LOGE(LOG_TAG, "device num is exceed");
        return -QM_EFULL;
    }
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

    wifi_handle_init();

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


static wifi_handle_t* wifi_handle_get(void)
{
    for(int i = 0; i < CONFIG_QM_PRODTST_OBJ_NUM; i++){
        if(g_wifi_handle[i].fd == -1){
            return &g_wifi_handle[i];
        }
    }
    return NULL;
}

static int wifi_handle_reset(wifi_handle_t *handle)
{
    if(handle == NULL){
        return -QM_EINVAL;
    }

    handle->fd = -1;
    return QM_EOK;
}

static int wifi_handle_init(void)
{
    for(int i = 0; i < CONFIG_QM_PRODTST_OBJ_NUM; i++){
        g_wifi_handle[i].fd = -1;
    }
    return QM_EOK;
}

static int wifi_prodtst_deinit(void)
{
    esp_wifi_stop();
    esp_wifi_deinit();
    prodtst_tcp_mgr_deinit();
    return QM_EOK;
}


static int channel_send(void *handle, char *buf, int size)
{
    int ret = 0;
    wifi_handle_t *wifi_handle = (wifi_handle_t*)handle;
    if(handle == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    ret = prodtst_tcp_write(wifi_handle->fd, buf, size, 0);
    if(ret > 0){
        return ret;
    }else{
        close(wifi_handle->fd);
        wifi_handle->fd = -1;
        return -QM_EIO;
    }
}

static int channel_recv(void *handle, char *buf, int size, int timeout)
{
    int ret = 0;
    wifi_handle_t *wifi_handle = (wifi_handle_t*)handle;
    if(handle == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    ret = prodtst_tcp_read(wifi_handle->fd, buf, size, timeout);
    if(ret > 0){
        return ret;
    }else{
        close(wifi_handle->fd);
        wifi_handle->fd = -1;
        return -QM_EIO;
    }
}

static int channel_establish(void *param, void **handle)
{
    int fd = 0;
    qm_err_t ret = QM_EOK;
    wifi_handle_t *wifi_handle = NULL;

    wifi_handle = wifi_handle_get();
    if(wifi_handle == NULL){
        QM_LOGE(LOG_TAG, "device num is exceed");
        return -QM_EINVAL;
    }

    QM_LOGI(LOG_TAG, "tcp connecting...");

    fd = prodtst_tcp_establish(param, CONFIG_QM_PRODTST_TCP_PORT);
    if(fd >= 0){
        QM_LOGI(LOG_TAG, "tcp connected, fd: %d", fd);
        wifi_handle->fd = fd;
    }else{
        QM_LOGI(LOG_TAG, "tcp connect failed");
        ret = -QM_EIO;
        goto __exit;
    }

    *handle = (void*)wifi_handle;

__exit:
    if(param){
        qm_free(param);
        param = NULL;
    }
    return ret;
}

static int channel_destroy(void *handle)
{
    wifi_handle_t *wifi_handle = (wifi_handle_t*)handle;
    if(handle == NULL){
        return -QM_EINVAL;
    }

    prodtst_tcp_destroy(wifi_handle->fd);
    wifi_handle_reset(wifi_handle);

    QM_LOGI(LOG_TAG, "tcp closed");
    return QM_EOK;
}

prodtst_adapter_t wifi_prodtst_adapter = {

    wifi_prodtst_init,
    wifi_prodtst_deinit,
    channel_establish,
    channel_destroy,
    channel_send,
    channel_recv,
};


