#include "bsp_camera.h"
#include "qm.h"
#include "qm_cli.h"
#include "mbedtls/base64.h"
#include <stdatomic.h>
#if defined(CONFIG_QM_POSE_SUPPORT) && CONFIG_QM_POSE_SUPPORT
#include "camera_pose.h"
#endif

#include "esp_cam_ctlr_spi.h"
#include "esp_cam_sensor_xclk.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_video_device.h"
#include "esp_video_init.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define LOG_TAG "bsp_camera"

#define CAM_BUF_NUM             2
#define CAM_I2C_FREQ_HZ         100000
#define CAM_STREAM_PRIO         12
#define CAM_STREAM_CORE         1
#define CAM_DQBUF_POLL_MS       20
#define CAM_LOG_EVERY_FRAMES    30
#define CAM_STREAM_STACK        (6 * 1024)
#define CAM_STACK_CAPS          (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)

typedef struct {
    int fd;
    uint8_t *buffer[CAM_BUF_NUM];
    size_t buffer_len[CAM_BUF_NUM];
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    bool stream_on;
} bsp_cam_t;

static bsp_cam_t s_cam = {
    .fd = -1,
};
static bool s_available;
static bool s_video_ready;
static TaskHandle_t s_stream_task;

/* Small preview has its own ownership and task, so inference never blocks it.
 * RGB332 keeps serial bandwidth low; the browser expands it to RGB888. */
#define PREVIEW_W 80
#define PREVIEW_H 106
static uint8_t *s_preview;
static SemaphoreHandle_t s_preview_free;
static TaskHandle_t s_preview_task;
static atomic_bool s_preview_requested;
static uint32_t s_preview_frame;

static uint8_t preview_clamp(int value)
{
    return value < 0 ? 0 : value > 255 ? 255 : value;
}

static void preview_request(char *out, int len, int argc, char **argv)
{
    (void)out; (void)len; (void)argc; (void)argv;
    atomic_store(&s_preview_requested, true);
}

static const struct qm_cli_command s_preview_command = {
    .name = "camera_preview",
    .help = "Request one compact RGB332 browser preview frame",
    .function = preview_request,
};

static void preview_task(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("\nCAMERA_PREVIEW %" PRIu32 " %d %d\n", s_preview_frame, PREVIEW_W, PREVIEW_H);
        for (int y = 0; y < PREVIEW_H; y++) {
            unsigned char encoded[112];
            size_t encoded_len = 0;
            if (mbedtls_base64_encode(encoded, sizeof(encoded), &encoded_len,
                                      s_preview + y * PREVIEW_W, PREVIEW_W) == 0) {
                printf("CAMERA_PIXEL %" PRIu32 " %d %s\n", s_preview_frame, y, encoded);
            }
            vTaskDelay(1);
        }
        printf("CAMERA_PREVIEW_END %" PRIu32 "\n", s_preview_frame);
        xSemaphoreGive(s_preview_free);
    }
}

static esp_err_t preview_init(void)
{
    if (s_preview_task) return ESP_OK;
    s_preview = heap_caps_malloc(PREVIEW_W * PREVIEW_H, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_preview_free = xSemaphoreCreateBinary();
    if (!s_preview || !s_preview_free) goto fail;
    if (xTaskCreatePinnedToCoreWithCaps(preview_task, "cam_preview", 3072, NULL, 4,
                                       &s_preview_task, 1,
                                       MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) != pdPASS) goto fail;
    xSemaphoreGive(s_preview_free);
    if (qm_cli_register_command(&s_preview_command) != 0) {
        ESP_LOGW(LOG_TAG, "camera_preview registration failed");
    }
    return ESP_OK;
fail:
    if (s_preview_free) vSemaphoreDelete(s_preview_free);
    heap_caps_free(s_preview);
    s_preview_free = NULL;
    s_preview = NULL;
    return ESP_ERR_NO_MEM;
}

static void preview_submit(const uint8_t *uyvy, uint32_t frame)
{
    if (!s_preview_task || !atomic_load(&s_preview_requested) ||
        xSemaphoreTake(s_preview_free, 0) != pdTRUE) return;
    atomic_store(&s_preview_requested, false);
    for (int y = 0; y < PREVIEW_H; y++) {
        for (int x = 0; x < PREVIEW_W; x++) {
            unsigned sx = x * s_cam.width / PREVIEW_W;
            unsigned sy = y * s_cam.height / PREVIEW_H;
            const uint8_t *pair = uyvy + (sy * s_cam.width + (sx & ~1u)) * 2;
            int c = pair[(sx & 1) ? 3 : 1] - 16;
            int d = pair[0] - 128, e = pair[2] - 128;
            uint8_t r = preview_clamp((298 * c + 409 * e + 128) >> 8);
            uint8_t g = preview_clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
            uint8_t b = preview_clamp((298 * c + 516 * d + 128) >> 8);
            s_preview[y * PREVIEW_W + x] = (r & 0xe0) | ((g >> 3) & 0x1c) | (b >> 6);
        }
    }
    s_preview_frame = frame;
    xTaskNotifyGive(s_preview_task);
}

static void cam_pwdn_hold_low(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = 1ULL << BSP_CAM_PWDN_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&conf));
    ESP_ERROR_CHECK(gpio_set_level(BSP_CAM_PWDN_GPIO, 1));
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_ERROR_CHECK(gpio_set_level(BSP_CAM_PWDN_GPIO, 0));
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(LOG_TAG, "PWDN GPIO%d pulsed then held low", BSP_CAM_PWDN_GPIO);
}

static void cam_teardown(void)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (s_cam.fd >= 0 && s_cam.stream_on) {
        ioctl(s_cam.fd, VIDIOC_STREAMOFF, &type);
        s_cam.stream_on = false;
    }
    for (int i = 0; i < CAM_BUF_NUM; i++) {
        if (s_cam.buffer[i] && s_cam.buffer[i] != MAP_FAILED) {
            munmap(s_cam.buffer[i], s_cam.buffer_len[i]);
            s_cam.buffer[i] = NULL;
            s_cam.buffer_len[i] = 0;
        }
    }
    if (s_cam.fd >= 0) {
        close(s_cam.fd);
        s_cam.fd = -1;
    }
}

static esp_err_t cam_video_init(void)
{
    static const esp_video_init_spi_config_t spi_config = {
        .sccb_config = {
            .init_sccb = true,
            .i2c_config = {
                .port = BSP_CAM_I2C_PORT,
                .scl_pin = BSP_CAM_SCL_GPIO,
                .sda_pin = BSP_CAM_SDA_GPIO,
            },
            .freq = CAM_I2C_FREQ_HZ,
        },
        .intf = ESP_CAM_CTLR_SPI_CAM_INTF_SPI,
        .io_mode = ESP_CAM_CTLR_SPI_CAM_IO_MODE_1BIT,
        .spi_port = SPI2_HOST,
        .spi_cs_pin = BSP_CAM_CS_GPIO,
        .spi_sclk_pin = BSP_CAM_SCLK_GPIO,
        .spi_data0_io_pin = BSP_CAM_DATA0_GPIO,
        .spi_data1_io_pin = -1,
        .spi_data2_io_pin = -1,
        .spi_data3_io_pin = -1,
        .reset_pin = BSP_CAM_RESET_GPIO,
        .pwdn_pin = -1,
        .xclk_source = ESP_CAM_SENSOR_XCLK_LEDC,
        .xclk_freq = 0,
        .xclk_pin = BSP_CAM_XCLK_GPIO,
#if CONFIG_CAMERA_XCLK_USE_LEDC
        .xclk_ledc_cfg = {
            .timer = LEDC_TIMER_3,
            .clk_cfg = LEDC_AUTO_CLK,
            .channel = LEDC_CHANNEL_7,
        },
#endif
    };
    const esp_video_init_config_t cam_config = {
        .spi = &spi_config,
    };

    ESP_LOGI(LOG_TAG,
             "pins SDA=%d SCL=%d CS=%d SCLK=%d DATA0=%d PWDN=%d SPI_HOST=%d",
             BSP_CAM_SDA_GPIO, BSP_CAM_SCL_GPIO, BSP_CAM_CS_GPIO,
             BSP_CAM_SCLK_GPIO, BSP_CAM_DATA0_GPIO, BSP_CAM_PWDN_GPIO,
             (int)SPI2_HOST);
    return esp_video_init(&cam_config);
}

static esp_err_t cam_setup_buffers(void)
{
    struct v4l2_format format;
    struct v4l2_requestbuffers req;

    if (s_cam.fd >= 0 && s_cam.buffer[0] != NULL) {
        return ESP_OK;
    }

    if (s_cam.fd < 0) {
        s_cam.fd = open(ESP_VIDEO_SPI_DEVICE_NAME, O_RDWR);
        ESP_RETURN_ON_FALSE(s_cam.fd >= 0, ESP_ERR_NOT_FOUND, LOG_TAG,
                            "open %s failed, errno=%d", ESP_VIDEO_SPI_DEVICE_NAME, errno);
    }

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_RETURN_ON_ERROR(ioctl(s_cam.fd, VIDIOC_G_FMT, &format) == 0 ? ESP_OK : ESP_FAIL,
                        LOG_TAG, "VIDIOC_G_FMT failed, errno=%d", errno);
    s_cam.width = format.fmt.pix.width;
    s_cam.height = format.fmt.pix.height;
    s_cam.pixel_format = format.fmt.pix.pixelformat;
    ESP_LOGI(LOG_TAG, "format %" PRIu32 "x%" PRIu32 " fourcc=0x%08" PRIx32,
             s_cam.width, s_cam.height, s_cam.pixel_format);
    ESP_RETURN_ON_FALSE(s_cam.pixel_format == V4L2_PIX_FMT_UYVY &&
                        s_cam.width == 240 && s_cam.height == 320,
                        ESP_ERR_NOT_SUPPORTED, LOG_TAG, "expected 240x320 UYVY");

    memset(&req, 0, sizeof(req));
    req.count = CAM_BUF_NUM;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_RETURN_ON_ERROR(ioctl(s_cam.fd, VIDIOC_REQBUFS, &req) == 0 ? ESP_OK : ESP_FAIL,
                        LOG_TAG, "VIDIOC_REQBUFS failed, errno=%d", errno);

    for (int i = 0; i < CAM_BUF_NUM; i++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ESP_RETURN_ON_ERROR(ioctl(s_cam.fd, VIDIOC_QUERYBUF, &buf) == 0 ? ESP_OK : ESP_FAIL,
                            LOG_TAG, "VIDIOC_QUERYBUF[%d] failed, errno=%d", i, errno);
        s_cam.buffer[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                               s_cam.fd, buf.m.offset);
        ESP_RETURN_ON_FALSE(s_cam.buffer[i] != MAP_FAILED, ESP_ERR_NO_MEM, LOG_TAG,
                            "mmap[%d] failed, errno=%d", i, errno);
        s_cam.buffer_len[i] = buf.length;
        ESP_RETURN_ON_ERROR(ioctl(s_cam.fd, VIDIOC_QBUF, &buf) == 0 ? ESP_OK : ESP_FAIL,
                            LOG_TAG, "VIDIOC_QBUF[%d] failed, errno=%d", i, errno);
    }

    ESP_LOGI(LOG_TAG, "buffers ready, STREAMON deferred");
    return ESP_OK;
}

static esp_err_t cam_stream_on(void)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (s_cam.stream_on) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(ioctl(s_cam.fd, VIDIOC_STREAMON, &type) == 0 ? ESP_OK : ESP_FAIL,
                        LOG_TAG, "VIDIOC_STREAMON failed, errno=%d", errno);
    s_cam.stream_on = true;
    ESP_LOGI(LOG_TAG, "stream on, %s", ESP_VIDEO_SPI_DEVICE_NAME);
    return ESP_OK;
}

static void cam_stream_task(void *arg)
{
    uint32_t frames = 0;
    uint32_t good = 0;
    uint32_t empty = 0;
    TickType_t t0 = xTaskGetTickCount();
    TickType_t last_good = t0;
    TickType_t last_warning = t0;
    const uint32_t expect = s_cam.width * s_cam.height * 2;

    (void)arg;
    ESP_LOGI(LOG_TAG, "stream task on core %d", (int)xPortGetCoreID());
    /* 在有内部栈的采集任务内启动驱动，避免在云事件回调中启动 SPI。 */
    if (cam_stream_on() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "camera stream start failed; reboot after checking driver/wiring");
        /* 保留任务句柄防止重复创建，错误状态下低频等待复位。 */
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    for (;;) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(s_cam.fd, VIDIOC_DQBUF, &buf) != 0) {
            int dequeue_errno = errno;
            TickType_t now = xTaskGetTickCount();
            if ((now - last_good) >= pdMS_TO_TICKS(5000) &&
                (now - last_warning) >= pdMS_TO_TICKS(5000)) {
                ESP_LOGW(LOG_TAG, "camera no valid frame for >=5s; check SPI data/sync wiring");
                last_warning = now;
            }
            if (dequeue_errno != EAGAIN && dequeue_errno != EINTR) {
                ESP_LOGE(LOG_TAG, "VIDIOC_DQBUF failed, errno=%d", dequeue_errno);
                vTaskDelay(pdMS_TO_TICKS(1000));
            } else {
                vTaskDelay(pdMS_TO_TICKS(CAM_DQBUF_POLL_MS));
            }
            continue;
        }

        frames++;
        if (buf.index < CAM_BUF_NUM && buf.bytesused == expect &&
            buf.bytesused <= s_cam.buffer_len[buf.index]) {
            good++;
            last_good = xTaskGetTickCount();
            preview_submit(s_cam.buffer[buf.index], frames);
#if defined(CONFIG_QM_POSE_SUPPORT) && CONFIG_QM_POSE_SUPPORT
            camera_pose_submit_uyvy(s_cam.buffer[buf.index], buf.bytesused,
                                    s_cam.width, s_cam.height, frames);
#endif
        } else {
            empty++;
            TickType_t now = xTaskGetTickCount();
            if ((now - last_good) >= pdMS_TO_TICKS(5000) &&
                (now - last_warning) >= pdMS_TO_TICKS(5000)) {
                ESP_LOGW(LOG_TAG, "camera invalid frame bytes=%u expected=%u",
                         (unsigned)buf.bytesused, (unsigned)expect);
                last_warning = now;
            }
        }
        if ((frames % CAM_LOG_EVERY_FRAMES) == 0) {
            TickType_t now = xTaskGetTickCount();
            uint32_t ms = (uint32_t)((now - t0) * portTICK_PERIOD_MS);
            ESP_LOGI(LOG_TAG, "got %u frames good=%u empty=%u last_bytes=%u dt=%u ms",
                     (unsigned)frames, (unsigned)good, (unsigned)empty,
                     (unsigned)buf.bytesused, (unsigned)ms);
            t0 = now;
        }

        if (ioctl(s_cam.fd, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(LOG_TAG, "VIDIOC_QBUF return failed, errno=%d", errno);
        }
        vTaskDelay(1);
    }
}

esp_err_t bsp_camera_early_init(void)
{
    cam_pwdn_hold_low();

    if (!s_video_ready) {
        if (cam_video_init() != ESP_OK) {
            ESP_LOGE(LOG_TAG, "esp_video_init failed (check 3V3/GND/I2C/PWDN wiring)");
            return ESP_FAIL;
        }
        s_video_ready = true;
    }
    if (cam_setup_buffers() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "camera buffer alloc failed");
        cam_teardown();
        return ESP_FAIL;
    }
    s_available = true;
    ESP_LOGI(LOG_TAG, "early init ok, wait for stream_start");
    return ESP_OK;
}

esp_err_t bsp_camera_stream_start(void)
{
    if (s_stream_task != NULL) {
        return ESP_OK;
    }
    if (!s_available) {
        ESP_LOGE(LOG_TAG, "stream_start before early_init");
        return ESP_ERR_INVALID_STATE;
    }
    if (preview_init() != ESP_OK) {
        ESP_LOGW(LOG_TAG, "browser preview unavailable: allocation failed");
    }
    if (xTaskCreatePinnedToCoreWithCaps(cam_stream_task, "cam_stream", CAM_STREAM_STACK,
                                        NULL, CAM_STREAM_PRIO, &s_stream_task,
                                        CAM_STREAM_CORE, CAM_STACK_CAPS) != pdPASS) {
        s_stream_task = NULL;
        ESP_LOGE(LOG_TAG, "camera task create failed");
        return ESP_FAIL;
    }
    ESP_LOGI(LOG_TAG, "stream task created, internal stack=%d", CAM_STREAM_STACK);
    return ESP_OK;
}

esp_err_t bsp_camera_start(void)
{
    if (bsp_camera_early_init() != ESP_OK) {
        return ESP_FAIL;
    }
    return bsp_camera_stream_start();
}

bool bsp_camera_is_available(void)
{
    return s_available;
}
