#include "qm_tls.h"
#include "qm_log.h"
#include "qm_types.h"
#include "qm_errno.h"

#include "openssl/ssl.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "esp_system.h"

#define LOG_TAG "hal tls"

typedef struct _TLSDataParams
{
    int server_fd;
    SSL_CTX *ctx;
    SSL *ssl;
} TLSDataParams_t, *TLSDataParams_pt;

#define SSL_READ_BUFFER_LEN 8120

void *qm_ssl_establish(const char *host, uint16_t port, const char *ca_crt, uint32_t ca_crt_len)
{
    char port_str[6] = {0};
    TLSDataParams_pt pTlsData = NULL;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    int ret = -1;
    int server_fd = -1;
    X509 *cacrt = NULL;

    struct sockaddr_in addr;
    struct hostent *hostent_content = NULL;

    QM_LOGD(LOG_TAG, "host name: %s", host);
    hostent_content = gethostbyname(host);
    if (hostent_content == NULL)
    {
        QM_LOGE(LOG_TAG, "gethostbyname err");
        return NULL;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0)
    {
        QM_LOGE(LOG_TAG, "socket creat err");
        return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(uint32_t *)(*hostent_content->h_addr_list);
    addr.sin_port = htons(port);

    QM_LOGD(LOG_TAG, "ip %s port %d fd %d\n", inet_ntoa(addr.sin_addr), port, server_fd);

    pTlsData = malloc(sizeof(TLSDataParams_t));

    if (NULL == pTlsData)
    {
        return NULL;
    }

    ret = connect(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret)
    {
        QM_LOGE(LOG_TAG, "connect failed");
        goto err_exit;
    }

    QM_LOGD(LOG_TAG, "server_fd %d", server_fd);

    ctx = SSL_CTX_new(TLSv1_1_client_method());
    if (!ctx)
    {
        QM_LOGE(LOG_TAG, "SSL_CTX_new, ret: %d, free_heap :%u", (uint32_t)ctx, esp_get_free_heap_size());
        goto err_exit;
    }
    QM_LOGD(LOG_TAG, "set SSL context read buffer size free_heap: %d", esp_get_free_heap_size());
    SSL_CTX_set_default_read_buffer_len(ctx, SSL_READ_BUFFER_LEN);
    ssl = SSL_new(ctx);
    if (!ssl)
    {
        QM_LOGE(LOG_TAG, "SSL_new failed free_heap :%u", esp_get_free_heap_size());
        goto err_exit;
    }

    SSL_set_fd(ssl, server_fd);

    pTlsData->server_fd = server_fd;
    pTlsData->ctx = ctx;
    pTlsData->ssl = ssl;

    ret = SSL_connect(ssl);
    if (!ret)
    {
        QM_LOGE(LOG_TAG, "SSL_connect, ret: %d,free_heap :%u", ret, esp_get_free_heap_size());
        goto err_exit;
    }

    QM_LOGD(LOG_TAG, "ssl connect success");

    QM_LOGD(LOG_TAG, "SSL_new, ret: %d, free_heap :%u", (uint32_t)ssl, esp_get_free_heap_size());

    return (void*)pTlsData;

err_exit:

    if (pTlsData){
        free(pTlsData);
        pTlsData = NULL;
    }

    if (ctx){
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (ssl){
        SSL_free(ssl);
        ssl = NULL;
    }

    if (server_fd >= 0){
        close(server_fd);
    }

    QM_LOGD(LOG_TAG, "platform_ssl_connect end free_heap :%u", esp_get_free_heap_size());
    return NULL;
}

int32_t qm_ssl_destroy(void *handle)
{
    TLSDataParams_pt pTlsData = NULL;
    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }
    pTlsData = (TLSDataParams_t *)handle;

    close(pTlsData->server_fd);

    if (pTlsData->ssl){
        SSL_free(pTlsData->ssl);
    }
    if (pTlsData->ctx){
        SSL_CTX_free(pTlsData->ctx);
    }
    if (pTlsData){
        free(pTlsData);
    }

    QM_LOGD(LOG_TAG, "ssl_disconnect");
    return QM_EOK;
}

int32_t qm_ssl_write(void *handle, const char *buf, int len, uint32_t timeout_ms)
{
    TLSDataParams_t *pTlsData = (TLSDataParams_t *)handle;
    int length = 0, ret = 0;
    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }

    while (length < len)
    {
        ret = SSL_write(pTlsData->ssl, (char *)(buf + length), (len - length));
        QM_LOGD(LOG_TAG, "ssl send =%d", ret);
        if (ret < 0){
            length = ret;
            break;
        }
        else if (ret > 0){
            length += ret;
        }
        else{
            return length;
        }
    }

    return (length > 0) ? length : -1;
}

int32_t hal_ssl_read(void * handle, char *buf, int len, uint32_t timeout_ms)
{
    TLSDataParams_t *pTlsData = (TLSDataParams_t *)handle;
    int rxLen = 0;
    int ret = 0;
    int fd = pTlsData->server_fd;
    fd_set readfds;
    struct timeval t;

    if (NULL == handle){
        QM_LOGE(LOG_TAG, "handle is NULL");
        return -QM_EINVAL;
    }

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    t.tv_sec = timeout_ms / 1000;
    t.tv_usec = (timeout_ms % 1000) * 1000;

    while (len > 0)
    {
        if (SSL_pending(pTlsData->ssl)){
            ret = SSL_read(pTlsData->ssl, buf, len);
        }
        else{

            if (timeout_ms == 0){
                break;
            }
            //platform_log("timeout_ms %u, cur %u", timeout_ms, HAL_UptimeMs());

            ret = select(fd + 1, &readfds, NULL, NULL, &t);
            if (ret > 0)
            {
                if (FD_ISSET(fd, &readfds))
                {
                    ret = SSL_read(pTlsData->ssl, buf, len);
                    if (ret < 0)
                    {
                        QM_LOGE(LOG_TAG, "select ret %d", ret);
                    }
                }
            }
            else if (ret == 0)
            {
                break;
            }
        }

        if (ret >= 0)
        {
            rxLen += ret;
            buf += ret;
            len -= ret;
        }

        if (ret < 0)
        {
            QM_LOGE(LOG_TAG, "socket read err");
            return -QM_EIO;
        }

        if (len == 0)
        {
            QM_LOGD(LOG_TAG, "recv len %d", rxLen);
            return rxLen;
        }
    }

    return rxLen;
}