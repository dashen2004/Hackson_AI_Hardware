#ifndef CAMERA_POSE_H
#define CAMERA_POSE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "yolo11_pose.h"

typedef struct {
    uint32_t frame_id;
    int64_t captured_ms;
    int64_t completed_ms;
    uint32_t processing_ms;
    esp_err_t error; /* 先检查 error；失败时不能把 pose 当成有效检测结果。 */
    yolo11_pose_frame_result_t pose;
} camera_pose_result_t;

/* 在摄像头启动之前加载模型、预留帧副本并创建唯一推理任务。 */
esp_err_t camera_pose_init(void);

/* 仅从采集任务调用；忙碌/限频时立即跳过，不持有摄像头缓冲。 */
bool camera_pose_submit_uyvy(const uint8_t *data, size_t len,
                             uint16_t width, uint16_t height, uint32_t frame_id);

/* 没有完成过检测时返回 false；否则返回最近一次完成（包括失败）的结果。 */
bool camera_pose_get_latest(camera_pose_result_t *out);

#endif
