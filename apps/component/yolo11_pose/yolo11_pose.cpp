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
