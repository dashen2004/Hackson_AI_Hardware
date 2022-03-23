#include "prodtst_adapter.h"
#include "qm_log.h"
#include "qm_errno.h"
#include "prodtst_core.h"
#include "prodtst_tcp_mgr.h"
#include "prodtst_config.h"
#include "prodtst_hal_wifi.h"
#include "prodtst_hal_tcp.h"
#include "qm_kernel.h"

#define LOG_TAG  "prodtst wifi"

typedef struct
{
    int fd;
}channel_info_t;

static int accept_exit = 0;
static qm_task_t accept_task = {0};

static uint8_t mac[6];
static char buffer[16] = {0};
static char prodtst_ssid[32] = {0};

#define CPRODTST_ACCEPT_TASK_SIZE   2048
#define PRODTST_ACCEPT_TASK_PRIO     18

static char *  __inet_ntoa( unsigned int in )
{
    unsigned char *bytes = (unsigned char *) &in;
    snprintf( buffer, sizeof (buffer), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3] );
    return buffer;
}

int prodtst_wifi_ap_name_get(char ssid[32])
{
    if(ssid == NULL){
        return -QM_EINVAL;
    }

    memcpy(ssid, prodtst_ssid, strlen(prodtst_ssid));

    return QM_EOK;
}

static void prodtst_accept_task(void *arg)
{
    int server_fd = -1;
    int ret = 0;
    server_fd = prodtst_tcp_server_creat(CONFIG_QM_PRODTST_TCP_PORT);
    if(server_fd == -1){
        QM_LOGE(LOG_TAG, "tcp server creat fail");
        goto __exit;
    }

    while(1){
        ret = prodtst_tcp_accept(server_fd, 1000);
        if(ret >= 0){
             prodtst_wifi_dut_found_notify(ret);
        }else if(ret == -QM_EIO){
            break;
        }
        if(accept_exit){
            close(server_fd);
            break;
        }
    }

__exit:
    qm_task_exit(&accept_exit);
}

static int wifi_prodtst_init(void *param)
{
    int max_conn = 0;
    qm_err_t ret = QM_EOK;
    uint8_t mac[6] = {0};

    accept_exit = 0;

    prodtst_wifi_sta_mac_get(mac);

    max_conn = prodtst_device_num_get();

    snprintf(prodtst_ssid, 32, "%s_%02x%02x", CONFIG_QM_PRODTST_AP_SSID, mac[4], mac[5]);
    
    ret = prodtst_wifi_ap_init(prodtst_ssid, CONFIG_QM_PRODTST_AP_PASSWD, CONFIG_QM_PRODTST_WIFI_CHANNEL, max_conn);
    if(ret != QM_EOK){
        return ret;
    }

    ret = qm_task_new(&accept_task, "prodtst accept", prodtst_accept_task, NULL, CPRODTST_ACCEPT_TASK_SIZE, PRODTST_ACCEPT_TASK_PRIO);
    if(ret != QM_EOK){
        return ret;
    }

    return QM_EOK;
}


static int wifi_prodtst_deinit(void)
{
    accept_exit = 1;
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
    prodtst_wifi_dut_info_t *wifi_dut_info = (prodtst_wifi_dut_info_t*)param;

    channel_info_t *channel = (channel_info_t*)qm_malloc(sizeof(channel_info_t));
    if(channel == NULL){
        return NULL;
    }

    channel->fd = wifi_dut_info->fd;

    return (void*)channel;
}

static int channel_destroy(void *handle)
{
    channel_info_t *channel = (channel_info_t*)handle;
    if(channel == NULL){
        return -QM_EINVAL;
    }
    prodtst_tcp_destroy(channel->fd);
    qm_free(handle);
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


