#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "qm_iot_import.h"
#include "iot_mqtt_debug.h"

#include "openssl/ssl.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "esp_system.h"

#define platform_log(M, ...) mqtt_log("tcp", M, ##__VA_ARGS__)

typedef struct _TLSDataParams
{
    int server_fd;
    SSL_CTX *ctx;
    SSL *ssl;
} TLSDataParams_t, *TLSDataParams_pt;

#define SSL_READ_BUFFER_LEN 8120

/**
 * @brief Establish a SSL connection.
 *
 * @param [in] host: @n Specify the hostname(IP) of the SSL server
 * @param [in] port: @n Specify the SSL port of SSL server
 * @param [in] ca_crt @n Specify the root certificate which is PEM format.
 * @param [in] ca_crt_len @n Length of root certificate, in bytes.
 * @return SSL handle.
 * @see None.
 * @note None.
 */
uintptr_t hal_ssl_establish(
    _IN_ const char *host,
    _IN_ uint16_t port,
    _IN_ const char *ca_crt,
    _IN_ size_t ca_crt_len)
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

    platform_log("host name: %s", host);
    hostent_content = gethostbyname(host);
    if (hostent_content == NULL)
    {
        platform_log("gethostbyname err");
        return 0;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0)
    {
        platform_log("socket creat err");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(uint32_t *)(*hostent_content->h_addr_list);
    addr.sin_port = htons(port);

    platform_log("ip %s port %d fd %d\n", inet_ntoa(addr.sin_addr), port, server_fd);

    pTlsData = malloc(sizeof(TLSDataParams_t));

    if (NULL == pTlsData)
    {
        return (uintptr_t)NULL;
    }

    ret = connect(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret)
    {
        platform_log("connect failed");
        goto err_exit;
    }

    platform_log("server_fd %d", server_fd);

    ctx = SSL_CTX_new(TLSv1_1_client_method());
    if (!ctx)
    {
        platform_log("SSL_CTX_new, ret: %d, free_heap :%u", (uint32_t)ctx, esp_get_free_heap_size());
        goto err_exit;
    }
    platform_log("set SSL context read buffer size free_heap: %d", esp_get_free_heap_size());
    SSL_CTX_set_default_read_buffer_len(ctx, SSL_READ_BUFFER_LEN);
    ssl = SSL_new(ctx);

    if (!ssl)
    {
        platform_log("SSL_new failed free_heap :%u", esp_get_free_heap_size());
        goto err_exit;
    }

    SSL_set_fd(ssl, server_fd);

    pTlsData->server_fd = server_fd;
    pTlsData->ctx = ctx;
    pTlsData->ssl = ssl;

    ret = SSL_connect(ssl);

    if (!ret)
    {
        platform_log("SSL_connect, ret: %d,free_heap :%u", ret, esp_get_free_heap_size());
        goto err_exit;
    }

    platform_log("ssl connect success");

    platform_log("SSL_new, ret: %d, free_heap :%u", (uint32_t)ssl, esp_get_free_heap_size());

    return (uintptr_t)pTlsData;

err_exit:

    if (pTlsData)
    {
        free(pTlsData);
        pTlsData = NULL;
    }

    if (ctx)
    {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (ssl)
    {
        SSL_free(ssl);
        ssl = NULL;
    }

    if (server_fd >= 0)
    {
        close(server_fd);
    }

    platform_log("platform_ssl_connect end free_heap :%u", esp_get_free_heap_size());
    return (uintptr_t)NULL;
}

/**
 * @brief Destroy the specific SSL connection.
 *
 * @param[in] handle: @n Handle of the specific connection.
 *
 * @return The result of destroy ssl
 *
 * @retval < 0 : Fail.
 * @retval   0 : Success.
 */
int32_t hal_ssl_destroy(_IN_ uintptr_t handle)

{
    TLSDataParams_pt pTlsData;
    if ((uint32_t)NULL == handle)
    {
        platform_log("handle is NULL");
        return 0;
    }
    pTlsData = (TLSDataParams_t *)handle;

    close(pTlsData->server_fd);

    if (pTlsData->ssl)
    {
        SSL_free(pTlsData->ssl);
    }
    if (pTlsData->ctx)
    {
        SSL_CTX_free(pTlsData->ctx);
    }
    if (pTlsData)
    {
        free(pTlsData);
    }

    platform_log("ssl_disconnect");
    return 0;
}

/**
 * @brief Write data into the specific SSL connection.
 *        The API will return immediately if 'len' be written into the specific SSL connection.
 *
 * @param [in] handle @n A descriptor identifying a connection.
 * @param [in] buf @n A pointer to a buffer containing the data to be transmitted.
 * @param [in] len @n The length, in bytes, of the data pointed to by the 'buf' parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 * @retval      < 0 : SSL connection error occur..
 * @retval        0 : No any data be write into the SSL connection in 'timeout_ms' timeout period.
 * @retval (0, len] : The total number of bytes be written in 'timeout_ms' timeout period.
 * @see None.
 */
int32_t hal_ssl_write(_IN_ uintptr_t handle, _IN_ const char *buf, _IN_ int len, _IN_ uint32_t timeout_ms)
{
    TLSDataParams_t *pTlsData = (TLSDataParams_t *)handle;
    int length = 0, ret;
    while (length < len)
    {
        ret = SSL_write(pTlsData->ssl, (char *)(buf + length), (len - length));
        platform_log("ssl send =%d", ret);
        if (ret < 0)
        {
            length = ret;
            break;
        }
        else if (ret > 0)
        {
            length += ret;
        }
        else
        {
            return length;
        }
    }

    return (length > 0) ? length : -1;
}

/**
 * @brief Read data from the specific SSL connection with timeout parameter.
 *        The API will return immediately if 'len' be received from the specific SSL connection.
 *
 * @param [in] handle @n A descriptor identifying a SSL connection.
 * @param [out] buf @n A pointer to a buffer to receive incoming data.
 * @param [out] len @n The length, in bytes, of the data pointed to by the 'buf' parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 *
 * @retval       -2 : SSL connection error occur.
 * @retval       -1 : SSL connection be closed by remote server.
 * @retval        0 : No any data be received in 'timeout_ms' timeout period.
 * @retval (0, len] : The total number of bytes be received in 'timeout_ms' timeout period.
 * @see None.
 */
int32_t hal_ssl_read(_IN_ uintptr_t handle, _OU_ char *buf, _OU_ int len, _IN_ uint32_t timeout_ms)
{
    TLSDataParams_t *pTlsData = (TLSDataParams_t *)handle;
    int rxLen = 0;
    int ret = 0;
    int fd = pTlsData->server_fd;
    fd_set readfds;
    struct timeval t;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    t.tv_sec = timeout_ms / 1000;
    t.tv_usec = (timeout_ms % 1000) * 1000;

    while (len > 0)
    {
        if (SSL_pending(pTlsData->ssl))
        {
            ret = SSL_read(pTlsData->ssl, buf, len);
        }
        else
        {

            if (timeout_ms == 0)
            {
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
                        platform_log("select ret %d", ret);
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
            platform_log("socket read err");
            return -1;
        }

        if (len == 0)
        {
            platform_log("recv len %d", rxLen);
            return rxLen;
        }
    }

    return rxLen;
}
