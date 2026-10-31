#include "camera_web.h"
#include "qm.h"
#include "qm_cli.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#if CONFIG_QM_POSE_SUPPORT
#include "camera_pose.h"
#endif

#define VIDEO_WIDTH 120
#define VIDEO_HEIGHT 160
#define FRAME_BYTES (VIDEO_WIDTH * VIDEO_HEIGHT * 2)
#define SOURCE_BYTES (240 * 320 * 2)
#define HEADER_BYTES 24
#define FRAME_PERIOD_MS 100
static const char *TAG = "camera_web";
static SemaphoreHandle_t s_lock;
static uint8_t *s_frame, *s_tx;
static uint32_t s_frame_id, s_captured_ms;
static atomic_uint s_valid, s_invalid, s_viewer_until_ms;
static uint32_t s_next_ms;
static httpd_handle_t s_server;
static bool s_video_power_set;

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void endpoint_command(char *out, int len, int argc, char **argv)
{
    (void)out; (void)len; (void)argc; (void)argv;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip = {0};
    if (netif && esp_netif_get_ip_info(netif, &ip) == ESP_OK && ip.ip.addr) {
        printf("\nCAMERA_ENDPOINT http://" IPSTR ":8080\n", IP2STR(&ip.ip));
    } else {
        printf("\nCAMERA_ENDPOINT offline\n");
    }
}

static const struct qm_cli_command s_endpoint_command = {
    .name = "camera_endpoint",
    .help = "Show camera LAN video endpoint (no Wi-Fi credentials)",
    .function = endpoint_command,
};

static esp_err_t frame_get(httpd_req_t *req)
{
    if (!s_video_power_set && esp_wifi_set_ps(WIFI_PS_NONE) == ESP_OK) {
        s_video_power_set = true;
    }
    atomic_store(&s_viewer_until_ms, now_ms() + 3000);
    uint32_t header[6];
    xSemaphoreTake(s_lock, portMAX_DELAY);
    header[0] = 0x314d4143; /* CAM1, little endian */
    header[1] = s_frame_id;
    header[2] = s_captured_ms;
    header[3] = VIDEO_WIDTH;
    header[4] = VIDEO_HEIGHT;
    header[5] = FRAME_BYTES;
    if (s_frame_id) {
        memcpy(s_tx, header, sizeof(header));
        memcpy(s_tx + HEADER_BYTES, s_frame, FRAME_BYTES);
    }
    xSemaphoreGive(s_lock);
    if (!header[1]) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        return httpd_resp_send(req, "waiting for camera frame", HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    /* HTTP task owns s_tx; the capture task can immediately publish a new frame. */
    return httpd_resp_send(req, (char *)s_tx, FRAME_BYTES + HEADER_BYTES);
}

static esp_err_t status_get(httpd_req_t *req)
{
    char json[768];
    int n;
#if CONFIG_QM_POSE_SUPPORT
    camera_pose_result_t result;
    if (camera_pose_get_latest(&result)) {
        static const char *statuses[] = {"no_person", "ok", "bad_posture", "unknown"};
        static const char *reasons[] = {"none", "head_tilt", "body_tilt", "too_close", "head_down"};
        unsigned status = result.pose.status, reason = result.pose.err_type;
        n = snprintf(json, sizeof(json),
                     "{\"uptime_ms\":%" PRIu32 ",\"valid\":%u,\"invalid\":%u,\"pose\":{"
                     "\"frame_id\":%" PRIu32 ",\"captured_ms\":%" PRIu32 ","
                     "\"processing_ms\":%" PRIu32 ",\"completed_ms\":%" PRIu32 ","
                     "\"status\":\"%s\",\"reason\":\"%s\",\"error\":%d}}",
                     now_ms(), atomic_load(&s_valid), atomic_load(&s_invalid),
                     result.frame_id, (uint32_t)result.captured_ms, result.processing_ms,
                     (uint32_t)result.completed_ms,
                     result.error == ESP_OK ? statuses[status < 4 ? status : 3] : "error",
                     reasons[reason < 5 ? reason : 0], (int)result.error);
    } else
#endif
    {
        n = snprintf(json, sizeof(json), "{\"uptime_ms\":%" PRIu32
                     ",\"valid\":%u,\"invalid\":%u,\"pose\":null}",
                     now_ms(), atomic_load(&s_valid), atomic_load(&s_invalid));
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, json, n);
}

esp_err_t camera_web_init(void)
{
    if (s_server) return ESP_OK;
    s_frame = heap_caps_malloc(SOURCE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_tx = heap_caps_malloc(SOURCE_BYTES + HEADER_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_lock = xSemaphoreCreateMutex();
    if (!s_frame || !s_tx || !s_lock) goto fail;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8080;
    config.ctrl_port = 32769;
    config.core_id = 0;
    config.task_priority = 4;
    config.stack_size = 6144;
    config.max_open_sockets = 3;
    config.lru_purge_enable = true;
    config.send_wait_timeout = 2;
    config.recv_wait_timeout = 2;
    if (httpd_start(&s_server, &config) != ESP_OK) goto fail;
    const httpd_uri_t frame_uri = {.uri = "/frame", .method = HTTP_GET, .handler = frame_get};
    const httpd_uri_t status_uri = {.uri = "/status", .method = HTTP_GET, .handler = status_get};
    if (httpd_register_uri_handler(s_server, &frame_uri) != ESP_OK ||
        httpd_register_uri_handler(s_server, &status_uri) != ESP_OK) {
        httpd_stop(s_server);
        s_server = NULL;
        goto fail;
    }
    qm_cli_register_command(&s_endpoint_command);
    ESP_LOGI(TAG, "LAN camera server ready port=8080 max_fps=10");
    return ESP_OK;
fail:
    heap_caps_free(s_frame);
    heap_caps_free(s_tx);
    if (s_lock) vSemaphoreDelete(s_lock);
    s_frame = s_tx = NULL;
    s_lock = NULL;
    return ESP_FAIL;
}

void camera_web_submit_uyvy(const uint8_t *data, size_t size, uint32_t frame_id,
                            uint32_t valid, uint32_t invalid)
{
    atomic_store(&s_valid, valid);
    atomic_store(&s_invalid, invalid);
    uint32_t now = now_ms();
    if (!s_server || size != SOURCE_BYTES || (int32_t)(atomic_load(&s_viewer_until_ms) - now) <= 0 ||
        (int32_t)(now - s_next_ms) < 0 || xSemaphoreTake(s_lock, 0) != pdTRUE) return;
    /* Keep native UYVY colors, sample at half resolution for the video only.
     * Inference continues to receive the original 240x320 frame. */
    for (unsigned y = 0; y < VIDEO_HEIGHT; y++) {
        for (unsigned x = 0; x < VIDEO_WIDTH; x += 2) {
            const uint8_t *src = data + (y * 2 * 240 + x * 2) * 2;
            uint8_t *dst = s_frame + (y * VIDEO_WIDTH + x) * 2;
            dst[0] = (src[0] + src[4]) / 2;
            dst[1] = src[1];
            dst[2] = (src[2] + src[6]) / 2;
            dst[3] = src[5];
        }
    }
    s_frame_id = frame_id;
    s_captured_ms = now;
    s_next_ms = now + FRAME_PERIOD_MS;
    xSemaphoreGive(s_lock);
}
