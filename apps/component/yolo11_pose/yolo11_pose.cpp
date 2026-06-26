#include "coco_pose.hpp"
#include "dl_image_jpeg.hpp"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <cstdint>
#include <cstdio>
#include <new>

#include "yolo11_pose.h"

static const char *TAG = "yolo11n-pose";
static COCOPose *s_pose = nullptr;

extern "C" esp_err_t yolo11_pose_init(void)
{
#if CONFIG_COCO_POSE_MODEL_IN_SDCARD
    ESP_LOGE(TAG, "SD card model loading is not wired in this project");
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (s_pose != nullptr) {
        return ESP_OK;
    }

    s_pose = new (std::nothrow) COCOPose(static_cast<COCOPose::model_type_t>(CONFIG_DEFAULT_COCO_POSE_MODEL), false);
    if (s_pose == nullptr) {
        ESP_LOGE(TAG, "alloc pose model failed");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
#endif
}

extern "C" esp_err_t yolo11_pose_rotate_jpg_cw90(const uint8_t *jpg_data,
                                                 size_t jpg_len,
                                                 uint8_t **out_jpg,
                                                 size_t *out_len)
{
    if (jpg_data == nullptr || jpg_len == 0 || out_jpg == nullptr || out_len == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_jpg = nullptr;
    *out_len = 0;

    dl::image::jpeg_img_t jpeg_img = {.data = (void *)jpg_data, .data_len = jpg_len};
    auto src = dl::image::sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    if (src.data == nullptr || src.width == 0 || src.height == 0) {
        ESP_LOGE(TAG, "rotate decode jpg failed");
        return ESP_FAIL;
    }

    uint16_t src_w = src.width;
    uint16_t src_h = src.height;
    uint16_t dst_w = src_h;
    uint16_t dst_h = src_w;
    size_t dst_size = (size_t)dst_w * dst_h * 3;
    uint8_t *dst_buf = (uint8_t *)heap_caps_aligned_alloc(16, dst_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (dst_buf == nullptr) {
        dst_buf = (uint8_t *)heap_caps_aligned_alloc(16, dst_size, MALLOC_CAP_DEFAULT);
    }
    if (dst_buf == nullptr) {
        ESP_LOGE(TAG, "rotate dst alloc failed, %ux%u", dst_w, dst_h);
        heap_caps_free(src.data);
        return ESP_ERR_NO_MEM;
    }

    const uint8_t *src_buf = (const uint8_t *)src.data;
    for (uint16_t y = 0; y < src_h; y++) {
        for (uint16_t x = 0; x < src_w; x++) {
            size_t src_idx = ((size_t)y * src_w + x) * 3;
            size_t dst_idx = ((size_t)x * dst_w + (dst_w - 1 - y)) * 3;
            dst_buf[dst_idx] = src_buf[src_idx];
            dst_buf[dst_idx + 1] = src_buf[src_idx + 1];
            dst_buf[dst_idx + 2] = src_buf[src_idx + 2];
        }
    }
    heap_caps_free(src.data);

    dl::image::img_t dst = {
        .data = dst_buf,
        .width = dst_w,
        .height = dst_h,
        .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888,
    };
    auto rotated_jpg = dl::image::sw_encode_jpeg(dst, 0, 80);
    heap_caps_free(dst_buf);
    if (rotated_jpg.data == nullptr || rotated_jpg.data_len == 0) {
        ESP_LOGE(TAG, "rotate encode jpg failed");
        return ESP_FAIL;
    }

    *out_jpg = (uint8_t *)rotated_jpg.data;
    *out_len = rotated_jpg.data_len;
    ESP_LOGI(TAG, "rotate jpg cw90 %ux%u -> %ux%u, %u -> %u bytes",
             src_w, src_h, dst_w, dst_h, (unsigned)jpg_len, (unsigned)*out_len);
    return ESP_OK;
}

extern "C" esp_err_t yolo11_pose_run_jpg_named(const uint8_t *jpg_data, size_t jpg_len, const char *image_name)
{
#if CONFIG_COCO_POSE_MODEL_IN_SDCARD
    ESP_LOGE(TAG, "SD card model loading is not wired in this project");
    return ESP_ERR_NOT_SUPPORTED;
#else
    const char *name = (image_name != nullptr && image_name[0] != '\0') ? image_name : "-";
    const char *kpt_names[17] = {"nose",
                                 "left eye",
                                 "right eye",
                                 "left ear",
                                 "right ear",
                                 "left shoulder",
                                 "right shoulder",
                                 "left elbow",
                                 "right elbow",
                                 "left wrist",
                                 "right wrist",
                                 "left hip",
                                 "right hip",
                                 "left knee",
                                 "right knee",
                                 "left ankle",
                                 "right ankle"};

    if (jpg_data == nullptr || jpg_len == 0) {
        ESP_LOGE(TAG, "%s invalid jpg", name);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = yolo11_pose_init();
    if (ret != ESP_OK) {
        return ret;
    }

    dl::image::jpeg_img_t jpeg_img = {.data = (void *)jpg_data, .data_len = jpg_len};
    auto img = dl::image::sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    if (img.data == nullptr) {
        ESP_LOGE(TAG, "%s decode jpg failed", name);
        return ESP_FAIL;
    }

    auto &pose_results = s_pose->run(img);
    if (pose_results.empty()) {
        ESP_LOGI(TAG, "%s no pose detected", name);
    }
    for (const auto &res : pose_results) {
        ESP_LOGI(TAG,
                 "%s [score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                 name,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);

        char log_buf[512];
        char *p = log_buf;
        char *end = log_buf + sizeof(log_buf);
        for (int i = 0; i < 17; ++i) {
            size_t left = (size_t)(end - p);
            int written = snprintf(p, left, "%s: [%d, %d] ", kpt_names[i], res.keypoint[2 * i], res.keypoint[2 * i + 1]);
            if (written < 0 || written >= (int)left) {
                break;
            }
            p += written;
        }
        ESP_LOGI(TAG, "%s %s", name, log_buf);
    }
    heap_caps_free(img.data);

    return ESP_OK;
#endif
}
