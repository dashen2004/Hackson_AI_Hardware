#include "camera_pose.h"

#include "qm.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "qm_cli.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>

#define TAG "camera_pose"
#define FRAME_WIDTH 240
#define FRAME_HEIGHT 320
#define FRAME_BYTES (FRAME_WIDTH * FRAME_HEIGHT * 2)
#define POSE_STACK_BYTES (12 * 1024)

static uint8_t *s_frame;
static SemaphoreHandle_t s_free;
static TaskHandle_t s_task;
static uint32_t s_frame_id;
static int64_t s_captured_ms;
static int64_t s_next_capture_ms;
static portMUX_TYPE s_result_lock = portMUX_INITIALIZER_UNLOCKED;
static camera_pose_result_t s_latest;
static bool s_has_result;
static atomic_bool s_snapshot_requested;

static void snapshot_command(char *out, int out_len, int argc, char **argv)
{
    (void)out;
    (void)out_len;
    (void)argc;
    (void)argv;
    atomic_store(&s_snapshot_requested, true);
}

static const struct qm_cli_command s_snapshot_command = {
    .name = "camera_snapshot",
    .help = "Export one RGB thumbnail as CAMERA_RGB/ROW/END lines",
    .function = snapshot_command,
};

/* Diagnostic export runs only on demand, from this frame's exclusive owner.
 * Use numbered text rows so other tasks' logs can be ignored by the receiver. */
static void export_snapshot(const uint8_t *rgb, uint16_t width, uint16_t height)
{
    static const char hex[] = "0123456789abcdef";
    const unsigned out_w = width / 3, out_h = height / 3;
    printf("\nCAMERA_RGB %u %u %" PRIu32 "\n", out_w, out_h, s_frame_id);
    for (unsigned y = 0; y < out_h; y++) {
        char row[720];
        int pos = snprintf(row, sizeof(row), "CAMERA_ROW %u ", y);
        for (unsigned x = 0; x < out_w; x++) {
            const uint8_t *pixel = rgb + ((y * 3) * width + x * 3) * 3;
            for (int c = 0; c < 3; c++) {
                row[pos++] = hex[pixel[c] >> 4];
                row[pos++] = hex[pixel[c] & 15];
            }
        }
        row[pos++] = '\n';
        row[pos] = '\0';
        printf("%s", row);
        vTaskDelay(1);
    }
    printf("CAMERA_END\n");
}

static esp_err_t convert_frame(uint8_t **rgb, uint16_t *w, uint16_t *h)
{
#if CONFIG_APP_CAMERA_POSE_ROTATE_CW90
    return yolo11_pose_uyvy_to_rgb888_cw90(s_frame, FRAME_BYTES,
                                         FRAME_WIDTH, FRAME_HEIGHT, rgb, w, h);
#else
    return yolo11_pose_uyvy_to_rgb888(s_frame, FRAME_BYTES,
                                    FRAME_WIDTH, FRAME_HEIGHT, rgb, w, h);
#endif
}

static const char *status_name(yolo11_pose_frame_status_t status)
{
    switch (status) {
    case YOLO11_POSE_FRAME_NO_PERSON: return "no_person";
    case YOLO11_POSE_FRAME_OK: return "ok";
    case YOLO11_POSE_FRAME_BAD_POSTURE: return "bad_posture";
    default: return "unknown";
    }
}

static const char *reason_name(yolo11_pose_err_type_t reason)
{
    switch (reason) {
    case YOLO11_POSE_ERR_YAW: return "head_tilt";
    case YOLO11_POSE_ERR_BODY_TILT: return "body_tilt";
    case YOLO11_POSE_ERR_TOO_CLOSE: return "too_close";
    case YOLO11_POSE_ERR_HEAD_DOWN: return "head_down";
    default: return "none";
    }
}

static void pose_task(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        const int64_t started_ms = esp_timer_get_time() / 1000;
        camera_pose_result_t result = {
            .frame_id = s_frame_id,
            .captured_ms = s_captured_ms,
            .pose.status = YOLO11_POSE_FRAME_UNKNOWN,
        };
        uint8_t *rgb = NULL;
        uint16_t w = 0, h = 0;
        result.error = convert_frame(&rgb, &w, &h);
        if (result.error == ESP_OK) {
            if (atomic_exchange(&s_snapshot_requested, false)) {
                export_snapshot(rgb, w, h);
            }
            result.error = yolo11_pose_analyze_rgb888(rgb, w, h, "camera", &result.pose);
        }
        if (result.error != ESP_OK) {
            memset(&result.pose, 0, sizeof(result.pose));
            result.pose.status = YOLO11_POSE_FRAME_UNKNOWN;
        }
        result.processing_ms = (uint32_t)(esp_timer_get_time() / 1000 - started_ms);
        result.completed_ms = esp_timer_get_time() / 1000;
        portENTER_CRITICAL(&s_result_lock);
        s_latest = result;
        s_has_result = true;
        portEXIT_CRITICAL(&s_result_lock);

        if (result.error == ESP_OK) {
            /* This SDK uses nano printf, which does not support 64-bit integer
             * formats. Keep the full timestamp in the snapshot, log its low 32 bits. */
            ESP_LOGI(TAG, "camera pose frame=%" PRIu32 " captured_ms=%" PRIu32
                     " status=%s reason=%s processing_ms=%" PRIu32,
                     result.frame_id, (uint32_t)result.captured_ms, status_name(result.pose.status),
                     reason_name(result.pose.err_type), result.processing_ms);
        } else {
            ESP_LOGE(TAG, "camera pose frame=%" PRIu32 " error=%s processing_ms=%" PRIu32,
                     result.frame_id, esp_err_to_name(result.error), result.processing_ms);
        }
        xSemaphoreGive(s_free);
        /* 即使模型运行很快，也让出 CPU；忙碌时采集端不会排队旧画面。 */
        vTaskDelay(1);
    }
}

esp_err_t camera_pose_init(void)
{
    if (s_task != NULL) {
        return ESP_OK;
    }
    esp_err_t ret = yolo11_pose_init();
    if (ret != ESP_OK) {
        return ret;
    }
    s_frame = heap_caps_calloc(1, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_frame == NULL) {
        return ESP_ERR_NO_MEM;
    }
    /* 提前分配本方向唯一需要的 RGB 常驻缓冲，不额外分配 JPEG 工作区。 */
    uint8_t *rgb = NULL;
    uint16_t w = 0, h = 0;
    ret = convert_frame(&rgb, &w, &h);
    if (ret != ESP_OK) {
        goto fail;
    }
    s_free = xSemaphoreCreateBinary();
    if (s_free == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }
    if (xTaskCreatePinnedToCoreWithCaps(pose_task, "camera_pose", POSE_STACK_BYTES,
                                      NULL, 4, &s_task, 1,
                                      MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) != pdPASS) {
        ret = ESP_ERR_NO_MEM;
        vSemaphoreDelete(s_free);
        s_free = NULL;
        goto fail;
    }
    xSemaphoreGive(s_free);
    if (qm_cli_register_command(&s_snapshot_command) != 0) {
        ESP_LOGW(TAG, "camera_snapshot command registration failed");
    }
    ESP_LOGI(TAG, "camera pose ready interval_ms=%d rotate_cw90=%d input=%dx%d",
             CONFIG_APP_CAMERA_POSE_INTERVAL_MS, CONFIG_APP_CAMERA_POSE_ROTATE_CW90,
             FRAME_WIDTH, FRAME_HEIGHT);
    return ESP_OK;
fail:
    heap_caps_free(s_frame);
    s_frame = NULL;
    return ret;
}

bool camera_pose_submit_uyvy(const uint8_t *data, size_t len,
                             uint16_t width, uint16_t height, uint32_t frame_id)
{
    if (s_task == NULL || data == NULL || width != FRAME_WIDTH ||
        height != FRAME_HEIGHT || len != FRAME_BYTES) {
        return false;
    }
    int64_t now_ms = esp_timer_get_time() / 1000;
    if (now_ms < s_next_capture_ms || xSemaphoreTake(s_free, 0) != pdTRUE) {
        return false;
    }
    memcpy(s_frame, data, FRAME_BYTES);
    s_frame_id = frame_id;
    s_captured_ms = now_ms;
    s_next_capture_ms = now_ms + CONFIG_APP_CAMERA_POSE_INTERVAL_MS;
    xTaskNotifyGive(s_task);
    return true;
}

bool camera_pose_get_latest(camera_pose_result_t *out)
{
    if (out == NULL) {
        return false;
    }
    portENTER_CRITICAL(&s_result_lock);
    bool available = s_has_result;
    if (available) {
        *out = s_latest;
    }
    portEXIT_CRITICAL(&s_result_lock);
    return available;
}
