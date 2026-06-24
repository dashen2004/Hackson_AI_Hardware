#include "bsp_camera.h"
#include "bsp_oss_upload.h"
#include "example_video_common.h"

#include "ai_main.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if !APP_HEADLESS_CAMERA_OSS
#include "esp_lvgl_port.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/videodev2.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define LOG_TAG "bsp_camera"

#ifndef CONFIG_EXAMPLE_CAMERA_VIDEO_BUFFER_NUMBER
#define CONFIG_EXAMPLE_CAMERA_VIDEO_BUFFER_NUMBER 2
#endif

#ifndef CONFIG_EXAMPLE_JPEG_COMPRESSION_QUALITY
#define CONFIG_EXAMPLE_JPEG_COMPRESSION_QUALITY 80
#endif

#define CAM_BUF_NUM             CONFIG_EXAMPLE_CAMERA_VIDEO_BUFFER_NUMBER
#define CAM_CAPTURE_STACK       (6 * 1024)
#define CAM_UPLOAD_STACK        (5 * 1024)
#define CAM_DQBUF_TIMEOUT_MS    10000
#define CAM_DQBUF_POLL_MS       20

typedef struct {
    uint8_t *data;
    uint32_t len;
} cam_upload_job_t;

extern BaseType_t qm_task_create_psram(TaskFunction_t fn,
                                       const char *name,
                                       uint32_t stack_words,
                                       void *arg,
                                       UBaseType_t priority,
                                       TaskHandle_t *out_handle);
extern void qm_task_delete_psram(TaskHandle_t handle);

typedef struct {
    int fd;
    uint8_t *buffer[CAM_BUF_NUM];
    size_t buffer_len[CAM_BUF_NUM];
    uint32_t buffer_size;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    bool stream_on;

    example_encoder_handle_t encoder;
    uint8_t *jpeg_buf;
    uint32_t jpeg_buf_size;
} bsp_cam_t;

static bsp_cam_t s_cam = {
    .fd = -1,
};
static bool s_video_ready;
static bool s_camera_available;
static volatile bool s_capture_busy;
static volatile bool s_upload_requested;
static TaskHandle_t s_camera_task;

static bool cam_ui_lock(void);
static void cam_ui_unlock(bool locked);

static uint32_t cam_stack_bytes_to_words(uint32_t stack_bytes)
{
    return (stack_bytes + sizeof(StackType_t) - 1) / sizeof(StackType_t);
}

static void cam_log_heap(const char *stage)
{
    ESP_LOGI(LOG_TAG, "heap@%s: free=%u largest_internal=%u largest_dma=%u largest_psram=%u",
             stage,
             (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}

static void cam_upload_task(void *arg)
{
    cam_upload_job_t *job = (cam_upload_job_t *)arg;
    bool ui_locked = false;
    qm_err_t qret;

    ui_locked = cam_ui_lock();
    ESP_LOGI(LOG_TAG, "OSS upload begin");
    qret = bsp_oss_upload_jpg(job->data, job->len);
    ESP_LOGI(LOG_TAG, "OSS upload end, ret=%ld", (long)qret);
    cam_ui_unlock(ui_locked);

    cam_log_heap("upload_done");
    free(job->data);
    free(job);
    s_capture_busy = false;
    qm_task_delete_psram(xTaskGetCurrentTaskHandle());
}

static bool cam_ui_lock(void)
{
#if !APP_HEADLESS_CAMERA_OSS
    return lvgl_port_lock(0);
#else
    return false;
#endif
}

static void cam_ui_unlock(bool locked)
{
#if !APP_HEADLESS_CAMERA_OSS
    if (locked) {
        lvgl_port_unlock();
    }
#else
    (void)locked;
#endif
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
    ESP_ERROR_CHECK(gpio_set_level(BSP_CAM_PWDN_GPIO, 0));
    ESP_LOGI(LOG_TAG, "PWDN GPIO%d held low", BSP_CAM_PWDN_GPIO);
}

static esp_err_t cam_video_init_once(void)
{
    if (s_video_ready) {
        return ESP_OK;
    }

    ESP_LOGI(LOG_TAG, "video init begin");
    ESP_RETURN_ON_ERROR(example_video_init(), LOG_TAG, "example_video_init failed");
    s_video_ready = true;
    ESP_LOGI(LOG_TAG, "video init done");
    return ESP_OK;
}

static void cam_close_stream(void)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (s_cam.fd >= 0 && s_cam.stream_on) {
        ESP_LOGI(LOG_TAG, "VIDIOC_STREAMOFF begin");
        ioctl(s_cam.fd, VIDIOC_STREAMOFF, &type);
        s_cam.stream_on = false;
    }

    if (s_cam.jpeg_buf && s_cam.encoder) {
        example_encoder_free_output_buffer(s_cam.encoder, s_cam.jpeg_buf);
        s_cam.jpeg_buf = NULL;
        s_cam.jpeg_buf_size = 0;
    }

    if (s_cam.encoder) {
        example_encoder_deinit(s_cam.encoder);
        s_cam.encoder = NULL;
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

    s_cam.buffer_size = 0;
    s_cam.width = 0;
    s_cam.height = 0;
    s_cam.pixel_format = 0;
}

static esp_err_t cam_open_stream(void)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_format format;
    struct v4l2_streamparm sparm;
    struct v4l2_requestbuffers req;
    esp_err_t ret = ESP_OK;

    if (s_cam.stream_on) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(cam_video_init_once(), LOG_TAG, "video init not ready");

    ESP_LOGI(LOG_TAG, "open %s begin", EXAMPLE_CAM_DEV_PATH);
    s_cam.fd = open(EXAMPLE_CAM_DEV_PATH, O_RDWR | O_NONBLOCK);
    ESP_RETURN_ON_FALSE(s_cam.fd >= 0, ESP_ERR_NOT_FOUND, LOG_TAG,
                        "open %s failed, errno=%d", EXAMPLE_CAM_DEV_PATH, errno);
    ESP_LOGI(LOG_TAG, "open %s ok, fd=%d", EXAMPLE_CAM_DEV_PATH, s_cam.fd);

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_GOTO_ON_ERROR(ioctl(s_cam.fd, VIDIOC_G_FMT, &format), fail, LOG_TAG, "VIDIOC_G_FMT failed, errno=%d", errno);

    memset(&sparm, 0, sizeof(sparm));
    sparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_GOTO_ON_ERROR(ioctl(s_cam.fd, VIDIOC_G_PARM, &sparm), fail, LOG_TAG, "VIDIOC_G_PARM failed, errno=%d", errno);

    s_cam.width = format.fmt.pix.width;
    s_cam.height = format.fmt.pix.height;
    s_cam.pixel_format = format.fmt.pix.pixelformat;
    ESP_LOGI(LOG_TAG, "format: %" PRIu32 "x%" PRIu32 " fourcc=0x%08" PRIx32,
             s_cam.width, s_cam.height, s_cam.pixel_format);

    memset(&req, 0, sizeof(req));
    req.count = CAM_BUF_NUM;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_LOGI(LOG_TAG, "VIDIOC_REQBUFS begin, count=%d", CAM_BUF_NUM);
    ESP_GOTO_ON_ERROR(ioctl(s_cam.fd, VIDIOC_REQBUFS, &req), fail, LOG_TAG, "VIDIOC_REQBUFS failed, errno=%d", errno);
    ESP_LOGI(LOG_TAG, "VIDIOC_REQBUFS ok, count=%u", (unsigned)req.count);

    for (int i = 0; i < CAM_BUF_NUM; i++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ESP_LOGI(LOG_TAG, "VIDIOC_QUERYBUF[%d] begin", i);
        ESP_GOTO_ON_ERROR(ioctl(s_cam.fd, VIDIOC_QUERYBUF, &buf), fail, LOG_TAG,
                          "VIDIOC_QUERYBUF[%d] failed, errno=%d", i, errno);

        s_cam.buffer[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, s_cam.fd, buf.m.offset);
        ESP_GOTO_ON_FALSE(s_cam.buffer[i] != MAP_FAILED, ESP_ERR_NO_MEM, fail, LOG_TAG,
                          "mmap[%d] failed, len=%u errno=%d", i, (unsigned)buf.length, errno);
        s_cam.buffer_len[i] = buf.length;
        s_cam.buffer_size = buf.length;
        ESP_LOGI(LOG_TAG, "mmap[%d] ok, len=%u", i, (unsigned)buf.length);

        ESP_LOGI(LOG_TAG, "VIDIOC_QBUF[%d] begin", i);
        ESP_GOTO_ON_ERROR(ioctl(s_cam.fd, VIDIOC_QBUF, &buf), fail, LOG_TAG,
                          "VIDIOC_QBUF[%d] failed, errno=%d", i, errno);
        ESP_LOGI(LOG_TAG, "VIDIOC_QBUF[%d] ok", i);
    }

    if (s_cam.pixel_format != V4L2_PIX_FMT_JPEG) {
        example_encoder_config_t enc_cfg = {
            .width = s_cam.width,
            .height = s_cam.height,
            .pixel_format = s_cam.pixel_format,
            .quality = CONFIG_EXAMPLE_JPEG_COMPRESSION_QUALITY,
        };

        ESP_LOGI(LOG_TAG, "jpeg encoder init begin");
        ESP_GOTO_ON_ERROR(example_encoder_init(&enc_cfg, &s_cam.encoder), fail, LOG_TAG, "jpeg encoder init failed");
        ESP_GOTO_ON_ERROR(example_encoder_alloc_output_buffer(s_cam.encoder, &s_cam.jpeg_buf, &s_cam.jpeg_buf_size),
                          fail, LOG_TAG, "jpeg output alloc failed");
        ESP_LOGI(LOG_TAG, "jpeg encoder ready, out_size=%u", (unsigned)s_cam.jpeg_buf_size);
    }

    ESP_LOGI(LOG_TAG, "VIDIOC_STREAMON begin");
    ESP_GOTO_ON_ERROR(ioctl(s_cam.fd, VIDIOC_STREAMON, &type), fail, LOG_TAG, "VIDIOC_STREAMON failed, errno=%d", errno);
    s_cam.stream_on = true;
    s_camera_available = true;
    ESP_LOGI(LOG_TAG, "VIDIOC_STREAMON ok, spi_port=%d", CONFIG_EXAMPLE_SPI_CAM_0_SPI_PORT);
    ESP_LOGI(LOG_TAG, "wait first frames after streamon");
    vTaskDelay(pdMS_TO_TICKS(500));
    cam_log_heap("camera_open_ready");
    return ESP_OK;

fail:
    cam_close_stream();
    return ret;
}

static esp_err_t cam_dequeue_frame(struct v4l2_buffer *buf)
{
    memset(buf, 0, sizeof(*buf));
    buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;

    if (ioctl(s_cam.fd, VIDIOC_DQBUF, buf) == 0) {
        ESP_LOGD(LOG_TAG, "VIDIOC_DQBUF ok, index=%u flags=0x%x bytesused=%u",
                 (unsigned)buf->index, (unsigned)buf->flags, (unsigned)buf->bytesused);
        return ESP_OK;
    }

    if (errno == EAGAIN || errno == EINTR) {
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGE(LOG_TAG, "VIDIOC_DQBUF failed, errno=%d", errno);
    return ESP_FAIL;
}

static void cam_queue_frame(struct v4l2_buffer *buf)
{
    ESP_LOGI(LOG_TAG, "VIDIOC_QBUF return frame begin, index=%u", (unsigned)buf->index);
    if (ioctl(s_cam.fd, VIDIOC_QBUF, buf) != 0) {
        ESP_LOGE(LOG_TAG, "VIDIOC_QBUF return frame failed, errno=%d", errno);
    } else {
        ESP_LOGI(LOG_TAG, "VIDIOC_QBUF return frame ok");
    }
}

static void cam_log_y_stats(const uint8_t *buf, uint32_t len)
{
    uint32_t min_y = 255;
    uint32_t max_y = 0;
    uint32_t sum_y = 0;
    uint32_t count = 0;

    if (buf == NULL || len < 4) {
        return;
    }

    for (uint32_t i = 1; i + 2 < len; i += 4) {
        uint32_t y0 = buf[i];
        uint32_t y1 = buf[i + 2];

        if (y0 < min_y) {
            min_y = y0;
        }
        if (y1 < min_y) {
            min_y = y1;
        }
        if (y0 > max_y) {
            max_y = y0;
        }
        if (y1 > max_y) {
            max_y = y1;
        }

        sum_y += y0 + y1;
        count += 2;
    }

    ESP_LOGI(LOG_TAG, "frame Y stats: min=%u max=%u avg=%u samples=%u",
             (unsigned)min_y, (unsigned)max_y,
             (unsigned)(count ? (sum_y / count) : 0),
             (unsigned)count);
}

static qm_err_t cam_encode_upload_frame(struct v4l2_buffer *buf)
{
    uint8_t *jpg = NULL;
    uint8_t *upload_buf = NULL;
    cam_upload_job_t *job = NULL;
    uint32_t jpg_len = 0;
    esp_err_t ret = ESP_OK;

    if (!(buf->flags & V4L2_BUF_FLAG_DONE)) {
        ESP_LOGE(LOG_TAG, "frame not done, flags=0x%x", (unsigned)buf->flags);
        cam_queue_frame(buf);
        cam_close_stream();
        return -QM_ERROR;
    }

    if (buf->index >= CAM_BUF_NUM || s_cam.buffer[buf->index] == NULL) {
        ESP_LOGE(LOG_TAG, "invalid frame index=%u", (unsigned)buf->index);
        cam_close_stream();
        return -QM_ERROR;
    }

    if (s_cam.pixel_format == V4L2_PIX_FMT_JPEG) {
        jpg = s_cam.buffer[buf->index];
        jpg_len = buf->bytesused;
        ESP_LOGI(LOG_TAG, "camera frame is jpeg, size=%u", (unsigned)jpg_len);
    } else {
        ESP_LOGI(LOG_TAG, "jpeg encode begin, src_size=%u", (unsigned)s_cam.buffer_size);
        if (s_cam.pixel_format == V4L2_PIX_FMT_UYVY) {
            cam_log_y_stats(s_cam.buffer[buf->index], buf->bytesused);
        }
        ret = example_encoder_process(s_cam.encoder, s_cam.buffer[buf->index], s_cam.buffer_size,
                                      s_cam.jpeg_buf, s_cam.jpeg_buf_size, &jpg_len);
        if (ret != ESP_OK) {
            ESP_LOGE(LOG_TAG, "jpeg encode failed: %s", esp_err_to_name(ret));
            cam_queue_frame(buf);
            return -QM_ERROR;
        }
        jpg = s_cam.jpeg_buf;
        ESP_LOGI(LOG_TAG, "jpeg encode ok, size=%u", (unsigned)jpg_len);
        cam_log_heap("jpeg_done");
    }

    upload_buf = heap_caps_malloc(jpg_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (upload_buf == NULL) {
        ESP_LOGE(LOG_TAG, "upload buffer alloc failed, size=%u", (unsigned)jpg_len);
        cam_queue_frame(buf);
        return -QM_ERROR;
    }
    memcpy(upload_buf, jpg, jpg_len);

    cam_queue_frame(buf);

    job = heap_caps_malloc(sizeof(*job), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (job == NULL) {
        ESP_LOGE(LOG_TAG, "upload job alloc failed");
        free(upload_buf);
        return -QM_ERROR;
    }

    job->data = upload_buf;
    job->len = jpg_len;
    cam_log_heap("before_upload_task");
    if (qm_task_create_psram(cam_upload_task, "cam_upload",
                             cam_stack_bytes_to_words(CAM_UPLOAD_STACK),
                             job, 7, NULL) != pdPASS) {
        ESP_LOGE(LOG_TAG, "upload task create failed");
        free(upload_buf);
        free(job);
        return -QM_ERROR;
    }

    return QM_EOK;
}

static void cam_stream_task(void *arg)
{
    struct v4l2_buffer buf;
    TickType_t next_idle_log = xTaskGetTickCount() + pdMS_TO_TICKS(5000);
    TickType_t request_start = 0;
    bool request_logged = false;

    (void)arg;

    ESP_LOGI(LOG_TAG, "camera stream task started");
    for (;;) {
        if (s_upload_requested) {
            if (!request_logged) {
                ESP_LOGI(LOG_TAG, "capture dequeue frame begin");
                request_logged = true;
                request_start = xTaskGetTickCount();
            }
            if (cam_open_stream() != ESP_OK) {
                ESP_LOGE(LOG_TAG, "camera stream open failed");
                s_upload_requested = false;
                s_capture_busy = false;
                request_logged = false;
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        } else {
            TickType_t now = xTaskGetTickCount();
            if ((int32_t)(now - next_idle_log) >= 0) {
                ESP_LOGI(LOG_TAG, "camera stream idle, fd=%d stream_on=%d",
                         s_cam.fd, s_cam.stream_on ? 1 : 0);
                next_idle_log = now + pdMS_TO_TICKS(5000);
            }
            vTaskDelay(pdMS_TO_TICKS(CAM_DQBUF_POLL_MS));
            continue;
        }

        esp_err_t ret = cam_dequeue_frame(&buf);
        if (ret == ESP_ERR_TIMEOUT) {
            if (request_logged && (xTaskGetTickCount() - request_start) >= pdMS_TO_TICKS(CAM_DQBUF_TIMEOUT_MS)) {
                ESP_LOGE(LOG_TAG, "capture wait frame timeout");
                s_upload_requested = false;
                s_capture_busy = false;
                request_logged = false;
            }
            vTaskDelay(pdMS_TO_TICKS(CAM_DQBUF_POLL_MS));
            continue;
        }
        if (ret != ESP_OK) {
            ESP_LOGE(LOG_TAG, "camera dequeue failed");
            cam_close_stream();
            s_upload_requested = false;
            s_capture_busy = false;
            request_logged = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (!s_upload_requested) {
            cam_queue_frame(&buf);
        } else {
            s_upload_requested = false;
            request_logged = false;
            ESP_LOGI(LOG_TAG, "capture got frame, index=%u bytesused=%u",
                     (unsigned)buf.index, (unsigned)buf.bytesused);
            cam_log_heap("frame_got");
            if (cam_encode_upload_frame(&buf) != QM_EOK) {
                s_capture_busy = false;
            }
        }
    }
}

qm_err_t bsp_camera_early_init(void)
{
    cam_pwdn_hold_low();
    if (cam_video_init_once() != ESP_OK) {
        ESP_LOGW(LOG_TAG, "video init failed");
        return -QM_ERROR;
    }
    return QM_EOK;
}

qm_err_t bsp_camera_stream_prepare(void)
{
    if (cam_open_stream() != ESP_OK) {
        ESP_LOGW(LOG_TAG, "stream open failed");
        return -QM_ERROR;
    }
    return QM_EOK;
}

bool bsp_camera_is_available(void)
{
    return s_camera_available;
}

qm_err_t bsp_camera_stream_start(void)
{
    if (s_camera_task != NULL) {
        return QM_EOK;
    }

    if (qm_task_create_psram(cam_stream_task, "cam_stream",
                             cam_stack_bytes_to_words(CAM_CAPTURE_STACK),
                             NULL, 8, &s_camera_task) != pdPASS) {
        s_camera_task = NULL;
        ESP_LOGE(LOG_TAG, "camera stream task create failed, free=%u largest_internal=%u",
                 (unsigned)esp_get_free_heap_size(),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        return -QM_ERROR;
    }

    ESP_LOGI(LOG_TAG, "camera stream task create ok");
    return QM_EOK;
}

void bsp_camera_capture_upload(void)
{
    if (!s_camera_available) {
        ESP_LOGW(LOG_TAG, "camera not available");
        return;
    }

    if (s_capture_busy) {
        ESP_LOGW(LOG_TAG, "capture already running");
        return;
    }

    s_capture_busy = true;
    s_upload_requested = true;
    cam_log_heap("capture_request");
    if (s_camera_task == NULL) {
        s_capture_busy = false;
        s_upload_requested = false;
        ESP_LOGE(LOG_TAG, "camera stream task not running");
    } else {
        ESP_LOGI(LOG_TAG, "capture upload requested");
    }
}

bool bsp_camera_capture_busy(void)
{
    return s_capture_busy;
}
