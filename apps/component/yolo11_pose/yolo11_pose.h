#ifndef YOLO11_POSE_H
#define YOLO11_POSE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    YOLO11_POSE_ERR_NONE = 0,
    YOLO11_POSE_ERR_YAW = 1,         /* 歪头 */
    YOLO11_POSE_ERR_BODY_TILT = 2,   /* 身体倾斜 */
    YOLO11_POSE_ERR_TOO_CLOSE = 3,   /* 距离过近 */
    YOLO11_POSE_ERR_HEAD_DOWN = 4,   /* 趴下/低头过多 */
} yolo11_pose_err_type_t;

typedef enum {
    YOLO11_POSE_FRAME_NO_PERSON = 0,
    YOLO11_POSE_FRAME_OK,
    YOLO11_POSE_FRAME_BAD_POSTURE,
    YOLO11_POSE_FRAME_UNKNOWN,   /* 检出人体但关键点不足，无法判定坐姿 */
} yolo11_pose_frame_status_t;

typedef struct {
    yolo11_pose_frame_status_t status;
    yolo11_pose_err_type_t err_type;
    float yaw;
    float roll;
    float shoulder_diff;
    float lean;
    float eye_w_ratio;
    float head_down_ratio;
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
