#ifndef YOLO11_POSE_H
#define YOLO11_POSE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    YOLO11_POSE_FRAME_NO_PERSON = 0,
    YOLO11_POSE_FRAME_OK,
    YOLO11_POSE_FRAME_BAD_POSTURE,
} yolo11_pose_frame_status_t;

typedef struct {
    yolo11_pose_frame_status_t status;
    float yaw;
    float roll;
    float shoulder_diff;
    float lean;
} yolo11_pose_frame_result_t;

esp_err_t yolo11_pose_init(void);
esp_err_t yolo11_pose_rotate_jpg_cw90(const uint8_t *jpg_data,
                                      size_t jpg_len,
                                      uint8_t **out_jpg,
                                      size_t *out_len);
esp_err_t yolo11_pose_analyze_jpg_named(const uint8_t *jpg_data,
                                        size_t jpg_len,
                                        const char *image_name,
                                        yolo11_pose_frame_result_t *out);
esp_err_t yolo11_pose_run_jpg_named(const uint8_t *jpg_data, size_t jpg_len, const char *image_name);

#ifdef __cplusplus
}
#endif

#endif
