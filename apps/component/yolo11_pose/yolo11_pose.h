#ifndef YOLO11_POSE_H
#define YOLO11_POSE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t yolo11_pose_init(void);
esp_err_t yolo11_pose_run_jpg_named(const uint8_t *jpg_data, size_t jpg_len, const char *image_name);

#ifdef __cplusplus
}
#endif

#endif
