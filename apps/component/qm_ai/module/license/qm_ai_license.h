#ifndef __QM_AI_LICENSE_H__
#define __QM_AI_LICENSE_H__

#include "qm.h"

typedef enum {
    QM_LICENSE_RTC_QM_IOT=0,
    QM_LICENSE_RTC_HTTP,
} qm_ai_license_type_t;

typedef struct {
    qm_ai_license_type_t type;
    void *transport_handle;
    void *ai_handle;
} qm_ai_license_config_t;

typedef struct {
    int uid;
    char *app_id;
    char *channel_name;
    char *token;
    char *license;
} qm_ai_rtc_auth_config_t;

int32_t qm_ai_license_init(qm_ai_license_config_t *config);
int32_t qm_ai_license_start(void);
int32_t qm_ai_license_stop(void);
int32_t qm_ai_license_deinit(void);

#endif // __QM_AI_LICENSE_H__