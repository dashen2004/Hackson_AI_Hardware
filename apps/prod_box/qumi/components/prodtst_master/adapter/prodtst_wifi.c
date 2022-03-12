#include "prodtst_adapter.h"
#include "qm_log.h"
#include "qm_errno.h"
#include "prodtst_core.h"
#include "prodtst_tcp_mgr.h"
#include "prodtst_config.h"
#include "prodtst_hal_wifi.h"
#include "prodtst_hal_tcp.h"

#define LOG_TAG  "prodtst wifi"

typedef struct
{
    int fd;
}channel_info_t;

int prodtst_wifi_ap_event_handler(uint32_t ip)
{
    char *ip_str = NULL;

    //ip_str = inet_ntoa(ip);
    QM_LOGI(LOG_TAG, "device found ip: %s\n", ip_str);
    return prodtst_ip_notify(ip_str);
}

void prodtst_wifi_event_callback(prodtst_wifi_event_t *event, void *arg)
{
    


}

static int wifi_prodtst_init(void *param)
{
    int max_conn = 0;
    qm_err_t ret = QM_EOK;
    char ssid[32] = {0};
    uint8_t mac[6] = {0};

    prodtst_wifi_sta_mac_get(mac);

    max_conn = prodtst_device_num_get();

    snprintf(ssid, 32, "%s_%02x%02x", CONFIG_QM_PRODTST_AP_SSID, mac[4], mac[5]);

    prodtst_wifi_ap_event_register(prodtst_wifi_event_callback, NULL);
    
    return prodtst_wifi_ap_init(ssid, CONFIG_QM_PRODTST_AP_PASSWD, CONFIG_QM_PRODTST_WIFI_CHANNEL, max_conn);
}


static int wifi_prodtst_deinit(void)
{
    prodtst_wifi_ap_deinit();

    return QM_EOK;
}


static int channel_send(void *handle, char *buf, int size)
{
    int ret = 0;
    channel_info_t *channel = (channel_info_t*)handle;

    if(handle == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    return prodtst_tcp_write(channel->fd, buf, size, 0); 
}

static int channel_recv(void *handle, char *buf, int size, int timeout)
{
    channel_info_t *channel = (channel_info_t*)handle;

    if(handle == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    if(timeout == QM_WAIT_FOREVER){
        timeout = 0;
    }

    return prodtst_tcp_read(channel->fd, buf, size, timeout);
}


static void *channel_establish(void *param)
{
    int fd = 0;
    qm_err_t ret = QM_EOK;
    prodtst_ip_info_t *ip_info = (prodtst_ip_info_t*)param;

    channel_info_t *channel = (channel_info_t*)qm_malloc(sizeof(channel_info_t));
    if(channel == NULL){
        return NULL;
    }

    QM_LOGI(LOG_TAG, "tcp connecting...");

    fd = prodtst_tcp_establish(ip_info->ip, CONFIG_QM_PRODTST_TCP_PORT);
    if(fd >= 0){
        QM_LOGI(LOG_TAG, "tcp connected, fd: %d", fd);
    }else{
        QM_LOGI(LOG_TAG, "tcp connect failed");
        ret = -QM_EIO;
        goto __exit;
    }

    return (void*)channel;

__exit:
    if(channel){
        qm_free(channel);
        channel = NULL;
    }
    return NULL;
}

static int channel_destroy(void *handle)
{
    channel_info_t *channel = (channel_info_t*)handle;
    prodtst_tcp_mgr_destroy(channel->fd);

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


