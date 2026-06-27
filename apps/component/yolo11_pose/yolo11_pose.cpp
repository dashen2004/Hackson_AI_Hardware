#include "coco_pose.hpp"
#include "dl_image_jpeg.hpp"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>

#include "yolo11_pose.h"

static const char *TAG = "yolo11n-pose";
static COCOPose *s_pose = nullptr;

#define POSE_YAW_BAD_MAX        0.90f
#define POSE_ROLL_BAD_MIN       0.10f
#define POSE_SHOULDER_BAD_MIN   0.03f
#define POSE_LEAN_BAD_MIN       0.50f
#define POSE_CLOSE_EYE_RATIO    0.20f
#define POSE_HEAD_DOWN_RATIO    -0.05f

static const char *kKptNames[17] = {"nose",
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

static bool kpt_valid(int x, int y)
{
    return x != 0 || y != 0;
}

static void log_pose_keypoints(const char *name, const dl::detect::result_t &res)
{
    char log_buf[512];
    char *p = log_buf;
    char *end = log_buf + sizeof(log_buf);

    for (int i = 0; i < 17; ++i) {
        size_t left = (size_t)(end - p);
        int written = snprintf(p,
                               left,
                               "%s: [%d, %d] ",
                               kKptNames[i],
                               res.keypoint[2 * i],
                               res.keypoint[2 * i + 1]);
        if (written < 0 || written >= (int)left) {
            break;
        }
        p += written;
    }
    ESP_LOGI(TAG, "%s %s", name, log_buf);
}

static yolo11_pose_err_type_t pick_primary_pose_err(bool yaw_bad,
                                                    bool tilt_bad,
                                                    bool close_bad,
                                                    bool head_down_bad)
{
    if (head_down_bad) {
        return YOLO11_POSE_ERR_HEAD_DOWN;
    }
    if (close_bad) {
        return YOLO11_POSE_ERR_TOO_CLOSE;
    }
    if (yaw_bad) {
        return YOLO11_POSE_ERR_YAW;
    }
    if (tilt_bad) {
        return YOLO11_POSE_ERR_BODY_TILT;
    }
    return YOLO11_POSE_ERR_NONE;
}

static void analyze_pose_result(const dl::detect::result_t &res,
                                int img_w,
                                int img_h,
                                yolo11_pose_frame_result_t *out)
{
    const auto &kp = res.keypoint;
    int nose_x = kp[0];
    int nose_y = kp[1];
    int le_x = kp[2];
    int le_y = kp[3];
    int re_x = kp[4];
    int re_y = kp[5];
    int ls_x = kp[10];
    int ls_y = kp[11];
    int rs_x = kp[12];
    int rs_y = kp[13];

    out->status = YOLO11_POSE_FRAME_OK;
    out->err_type = YOLO11_POSE_ERR_NONE;
    out->yaw = 0.0f;
    out->roll = 0.0f;
    out->shoulder_diff = 0.0f;
    out->lean = 0.0f;
    out->eye_w_ratio = 0.0f;
    out->head_down_ratio = 0.0f;

    if (img_w <= 0 || img_h <= 0) {
        return;
    }

    bool yaw_bad = false;
    bool tilt_bad = false;
    bool close_bad = false;
    bool head_down_bad = false;
    bool measured = false;

    if (kpt_valid(le_x, le_y) && kpt_valid(re_x, re_y)) {
        float dx = (float)(re_x - le_x);
        float dy = (float)(re_y - le_y);
        float eye_w = sqrtf(dx * dx + dy * dy);
        if (eye_w >= 1.0f) {
            measured = true;
            out->eye_w_ratio = eye_w / (float)img_w;
            out->yaw = fabsf((float)(le_x - re_x)) / eye_w;
            out->roll = fabsf((float)(le_y - re_y)) / eye_w;
            if (out->yaw < POSE_YAW_BAD_MAX) {
                yaw_bad = true;
            }
            if (out->roll > POSE_ROLL_BAD_MIN) {
                tilt_bad = true;
            }
            if (out->eye_w_ratio > POSE_CLOSE_EYE_RATIO) {
                close_bad = true;
            }
        }
    }

    if (kpt_valid(ls_x, ls_y) && kpt_valid(rs_x, rs_y)) {
        measured = true;
        out->shoulder_diff = fabsf((float)(ls_y - rs_y)) / (float)img_h;
        if (out->shoulder_diff > POSE_SHOULDER_BAD_MIN) {
            tilt_bad = true;
        }
    }

    if (kpt_valid(nose_x, nose_y) && kpt_valid(ls_x, ls_y) && kpt_valid(rs_x, rs_y)) {
        measured = true;
        float shoulder_mid_x = ((float)ls_x + (float)rs_x) * 0.5f;
        float shoulder_mid_y = ((float)ls_y + (float)rs_y) * 0.5f;
        out->lean = fabsf((float)nose_x - shoulder_mid_x) / (float)img_w;
        out->head_down_ratio = ((float)nose_y - shoulder_mid_y) / (float)img_h;
        if (out->lean > POSE_LEAN_BAD_MIN) {
            tilt_bad = true;
        }
        if (out->head_down_ratio > POSE_HEAD_DOWN_RATIO) {
            head_down_bad = true;
        }
    }

    out->err_type = pick_primary_pose_err(yaw_bad, tilt_bad, close_bad, head_down_bad);
    if (out->err_type != YOLO11_POSE_ERR_NONE) {
        out->status = YOLO11_POSE_FRAME_BAD_POSTURE;
    } else if (!measured) {
        out->status = YOLO11_POSE_FRAME_UNKNOWN;
    } else {
        out->status = YOLO11_POSE_FRAME_OK;
    }
}

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

extern "C" esp_err_t yolo11_pose_analyze_jpg_named(const uint8_t *jpg_data,
                                                   size_t jpg_len,
                                                   const char *image_name,
                                                   yolo11_pose_frame_result_t *out)
{
#if CONFIG_COCO_POSE_MODEL_IN_SDCARD
    ESP_LOGE(TAG, "SD card model loading is not wired in this project");
    return ESP_ERR_NOT_SUPPORTED;
#else
    const char *name = (image_name != nullptr && image_name[0] != '\0') ? image_name : "-";

    if (out != nullptr) {
        memset(out, 0, sizeof(*out));
        out->status = YOLO11_POSE_FRAME_NO_PERSON;
    }

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
        heap_caps_free(img.data);
        return ESP_OK;
    }

    const auto &res = pose_results.front();
    ESP_LOGI(TAG,
             "%s [score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
             name,
             res.score,
             res.box[0],
             res.box[1],
             res.box[2],
             res.box[3]);
    log_pose_keypoints(name, res);

    if (out != nullptr) {
        analyze_pose_result(res, (int)img.width, (int)img.height, out);
        ESP_LOGI(TAG,
                 "%s pose metrics yaw=%.3f roll=%.3f shoulder=%.3f lean=%.3f eye=%.3f head=%.3f status=%d err=%d",
                 name,
                 out->yaw,
                 out->roll,
                 out->shoulder_diff,
                 out->lean,
                 out->eye_w_ratio,
                 out->head_down_ratio,
                 (int)out->status,
                 (int)out->err_type);
    }

    heap_caps_free(img.data);
    return ESP_OK;
#endif
}

extern "C" esp_err_t yolo11_pose_run_jpg_named(const uint8_t *jpg_data, size_t jpg_len, const char *image_name)
{
    return yolo11_pose_analyze_jpg_named(jpg_data, jpg_len, image_name, nullptr);
}
