#ifndef BSP_OSS_UPLOAD_H
#define BSP_OSS_UPLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include <stddef.h>

#define BSP_OSS_UPLOAD_ENABLE           1

#define BSP_OSS_BUCKET                  "wc-ihs"
#define BSP_OSS_ENDPOINT                "oss-cn-hangzhou.aliyuncs.com"
#define BSP_OSS_OBJECT_KEY              "test.jpg"
#define BSP_OSS_CONTENT_TYPE            "image/jpeg"

/* 填入阿里云 RAM 用户的 AccessKey，仅用于测试；正式产品请改用 STS 或预签名 URL */
#define BSP_OSS_ACCESS_KEY_ID           "LTAI5t9zos2KMkM4Lurhsp11"
#define BSP_OSS_ACCESS_KEY_SECRET       "mvum79Gmk25IqehQ9wYi0doL9h7PI3"

qm_err_t bsp_oss_upload_init(void);
qm_err_t bsp_oss_upload_jpg(const uint8_t *data, size_t len);
qm_err_t bsp_oss_upload_jpg_named(const uint8_t *data, size_t len, const char *object_key);
qm_err_t bsp_oss_upload_test_jpg(void);

#ifdef __cplusplus
}
#endif

#endif
