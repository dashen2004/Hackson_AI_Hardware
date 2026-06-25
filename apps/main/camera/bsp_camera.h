#ifndef BSP_CAMERA_H
#define BSP_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include <stdbool.h>

/** BF3901 PWDN is active-low; held low outside the sensor driver. */
#define BSP_CAM_PWDN_GPIO   41

qm_err_t bsp_camera_early_init(void);
qm_err_t bsp_camera_stream_prepare(void);
qm_err_t bsp_camera_stream_start(void);
bool bsp_camera_is_available(void);
void bsp_camera_capture_upload(void);
void bsp_camera_capture_upload_named(const char *object_key);
bool bsp_camera_capture_busy(void);

#ifdef __cplusplus
}
#endif

#endif
