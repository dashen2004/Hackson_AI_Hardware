#include "bsp_oss_upload.h"

#if BSP_OSS_UPLOAD_ENABLE

#include "qm_utils_base64.h"
#include "qm_utils_sha1.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define LOG_TAG "oss_upload"

#define OSS_HMAC_KEY_SIZE   64
#define OSS_HMAC_DIGEST_LEN 20

static int oss_hmac_sha1_base64(const char *msg, int msg_len,
                                const char *key, int key_len,
                                char *b64_out, size_t b64_out_len)
{
    qm_sha1_context ctx;
    unsigned char k_ipad[OSS_HMAC_KEY_SIZE];
    unsigned char k_opad[OSS_HMAC_KEY_SIZE];
    unsigned char inner[OSS_HMAC_DIGEST_LEN];
    unsigned char digest[OSS_HMAC_DIGEST_LEN];
    uint32_t out_len = 0;
    int i;

    if (key_len > OSS_HMAC_KEY_SIZE) {
        return -1;
    }

    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    for (i = 0; i < OSS_HMAC_KEY_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    qm_utils_sha1_init(&ctx);
    qm_utils_sha1_starts(&ctx);
    qm_utils_sha1_update(&ctx, k_ipad, OSS_HMAC_KEY_SIZE);
    qm_utils_sha1_update(&ctx, (const unsigned char *)msg, msg_len);
    qm_utils_sha1_finish(&ctx, inner);

    qm_utils_sha1_init(&ctx);
    qm_utils_sha1_starts(&ctx);
    qm_utils_sha1_update(&ctx, k_opad, OSS_HMAC_KEY_SIZE);
    qm_utils_sha1_update(&ctx, inner, OSS_HMAC_DIGEST_LEN);
    qm_utils_sha1_finish(&ctx, digest);

    if (qm_utils_base64encode(digest, OSS_HMAC_DIGEST_LEN, (uint32_t)b64_out_len,
                              (uint8_t *)b64_out, &out_len) != 0) {
        return -1;
    }
    b64_out[out_len] = '\0';
    return 0;
}

static bool oss_credentials_ready(void)
{
    return BSP_OSS_ACCESS_KEY_ID[0] != '\0' && BSP_OSS_ACCESS_KEY_SECRET[0] != '\0';
}

static int oss_build_gmt_date(char *date_buf, size_t date_buf_len)
{
    time_t now = time(NULL);
    struct tm gmt = {0};

    if (now <= 0) {
        return -1;
    }

    if (gmtime_r(&now, &gmt) == NULL) {
        return -1;
    }

    if (strftime(date_buf, date_buf_len, "%a, %d %b %Y %H:%M:%S GMT", &gmt) == 0) {
        return -1;
    }

    return 0;
}

static int oss_build_signature(const char *date_gmt, const char *object_key,
                               char *sig_b64, size_t sig_b64_len)
{
    char canonical[256];
    char resource[128];
    int canonical_len;

    snprintf(resource, sizeof(resource), "/%s/%s", BSP_OSS_BUCKET, object_key);
    canonical_len = snprintf(canonical, sizeof(canonical),
                             "PUT\n\n%s\n%s\n%s",
                             BSP_OSS_CONTENT_TYPE, date_gmt, resource);
    if (canonical_len <= 0 || canonical_len >= (int)sizeof(canonical)) {
        return -1;
    }

    return oss_hmac_sha1_base64(canonical, canonical_len,
                                BSP_OSS_ACCESS_KEY_SECRET,
                                (int)strlen(BSP_OSS_ACCESS_KEY_SECRET),
                                sig_b64, sig_b64_len);
}

static int oss_send_all(int fd, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t sent = 0;

    while (sent < len) {
        int n = send(fd, p + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        sent += (size_t)n;
    }

    return 0;
}

static int oss_recv_header(int fd, char *buf, size_t buf_len)
{
    size_t used = 0;

    if (buf_len == 0) {
        return -1;
    }
    buf[0] = '\0';

    while (used + 1 < buf_len) {
        int n = recv(fd, buf + used, buf_len - used - 1, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            break;
        }

        used += (size_t)n;
        buf[used] = '\0';
        if (strstr(buf, "\r\n\r\n") != NULL) {
            return (int)used;
        }
    }

    return strstr(buf, "\r\n\r\n") ? (int)used : -1;
}

static int oss_raw_put_jpg(const uint8_t *data, size_t data_len,
                           const char *object_key,
                           const char *date_gmt, const char *sig_b64,
                           int *status_code, char *resp, size_t resp_len)
{
    char host[128];
    char request[512];
    struct addrinfo hints = {0};
    struct addrinfo *addr_list = NULL;
    struct timeval tv = {
        .tv_sec = 3,
        .tv_usec = 0,
    };
    int fd = -1;
    int req_len;
    int ret = -1;

    snprintf(host, sizeof(host), "%s.%s", BSP_OSS_BUCKET, BSP_OSS_ENDPOINT);
    req_len = snprintf(request, sizeof(request),
                       "PUT /%s HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "Authorization: OSS %s:%s\r\n"
                       "Date: %s\r\n"
                       "Content-Type: %s\r\n"
                       "Content-Length: %u\r\n"
                       "Connection: close\r\n"
                       "\r\n",
                       object_key, host,
                       BSP_OSS_ACCESS_KEY_ID, sig_b64,
                       date_gmt, BSP_OSS_CONTENT_TYPE,
                       (unsigned)data_len);
    if (req_len <= 0 || req_len >= (int)sizeof(request)) {
        ESP_LOGE(LOG_TAG, "build HTTP request failed");
        return -1;
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    ret = getaddrinfo(host, "80", &hints, &addr_list);
    if (ret != 0 || addr_list == NULL) {
        ESP_LOGE(LOG_TAG, "DNS failed for %s, ret=%d", host, ret);
        return -1;
    }

    fd = socket(addr_list->ai_family, addr_list->ai_socktype, addr_list->ai_protocol);
    if (fd < 0) {
        ESP_LOGE(LOG_TAG, "socket failed, errno=%d", errno);
        goto out;
    }

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    ESP_LOGI(LOG_TAG, "connect %s:80", host);
    if (connect(fd, addr_list->ai_addr, addr_list->ai_addrlen) != 0) {
        ESP_LOGE(LOG_TAG, "connect failed, errno=%d", errno);
        goto out;
    }

    if (oss_send_all(fd, request, (size_t)req_len) != 0 ||
        oss_send_all(fd, data, data_len) != 0) {
        ESP_LOGE(LOG_TAG, "send PUT failed, errno=%d", errno);
        goto out;
    }

    ESP_LOGI(LOG_TAG, "PUT sent, waiting response header");
    if (oss_recv_header(fd, resp, resp_len) < 0) {
        ESP_LOGE(LOG_TAG, "recv response header failed, errno=%d", errno);
        goto out;
    }

    *status_code = 0;
    if (sscanf(resp, "HTTP/%*s %d", status_code) != 1) {
        ESP_LOGE(LOG_TAG, "parse HTTP status failed: %.40s", resp);
        goto out;
    }

    ret = 0;

out:
    if (fd >= 0) {
        close(fd);
    }
    if (addr_list) {
        freeaddrinfo(addr_list);
    }
    return ret;
}

qm_err_t bsp_oss_upload_jpg_named(const uint8_t *data, size_t data_len, const char *object_key)
{
    char date_gmt[40];
    char sig_b64[48];
    char response_buf[768];
    int status_code = 0;
    int ret;

    if (!oss_credentials_ready()) {
        ESP_LOGW(LOG_TAG, "skip upload: set BSP_OSS_ACCESS_KEY_ID/SECRET in bsp_oss_upload.h");
        return -QM_EINVAL;
    }

    if (data == NULL || data_len == 0 || object_key == NULL || object_key[0] == '\0') {
        ESP_LOGE(LOG_TAG, "invalid jpg buffer or object key");
        return -QM_EINVAL;
    }

    if (oss_build_gmt_date(date_gmt, sizeof(date_gmt)) != 0) {
        ESP_LOGE(LOG_TAG, "system time not ready, wait for NTP sync");
        return -QM_EIO;
    }

    if (oss_build_signature(date_gmt, object_key, sig_b64, sizeof(sig_b64)) != 0) {
        ESP_LOGE(LOG_TAG, "build OSS signature failed");
        return -QM_ERROR;
    }

    ESP_LOGI(LOG_TAG, "uploading jpg (%u bytes) -> http://%s.%s/%s",
             (unsigned)data_len, BSP_OSS_BUCKET, BSP_OSS_ENDPOINT, object_key);

    ret = oss_raw_put_jpg(data, data_len, object_key, date_gmt, sig_b64,
                          &status_code, response_buf, sizeof(response_buf));

    if (ret != 0) {
        ESP_LOGE(LOG_TAG, "upload failed");
        return -QM_EIO;
    }

    ESP_LOGI(LOG_TAG, "upload done, HTTP %d", status_code);
    return (status_code == 200) ? QM_EOK : -QM_EIO;
}

qm_err_t bsp_oss_upload_jpg(const uint8_t *data, size_t data_len)
{
    return bsp_oss_upload_jpg_named(data, data_len, BSP_OSS_OBJECT_KEY);
}

qm_err_t bsp_oss_upload_test_jpg(void)
{
    ESP_LOGW(LOG_TAG, "embedded test.jpg removed; use bsp_camera_capture_upload()");
    return -QM_ENOSYS;
}

#else

qm_err_t bsp_oss_upload_jpg(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
    return -QM_ENOSYS;
}

qm_err_t bsp_oss_upload_jpg_named(const uint8_t *data, size_t len, const char *object_key)
{
    (void)data;
    (void)len;
    (void)object_key;
    return -QM_ENOSYS;
}

qm_err_t bsp_oss_upload_test_jpg(void)
{
    return -QM_ENOSYS;
}

#endif
