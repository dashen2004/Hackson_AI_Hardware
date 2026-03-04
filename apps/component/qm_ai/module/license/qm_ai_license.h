#ifndef __QM_AI_LICENSE_H__
#define __QM_AI_LICENSE_H__

#include "qm.h"

#define QM_AI_LICENSE_TYPE      0

//RTC登录信息
#define CONFIG_AUDIO_RTC_LOGIN_SIID         (7)
//RTC登录信息-uid
#define CONFIG_AUDIO_RTC_UID_PIID           (1)
#define CONFIG_AUDIO_RTC_UID_TYPE           (QM_SPEC_PROPERTY_FORMAT_NUMBER)

//RTC登录信息-appid
#define CONFIG_AUDIO_RTC_APPID_PIID         (2)
#define CONFIG_AUDIO_RTC_APPID_TYPE         (QM_SPEC_PROPERTY_FORMAT_NUMBER)

//RTC登录信息-channalname
#define CONFIG_AUDIO_RTC_CHANNAL_NAME_PIID  (3)
#define CONFIG_AUDIO_RTC_CHANNAL_NAME_TYPE  (QM_SPEC_PROPERTY_FORMAT_NUMBER)

//RTC登录信息-token
#define CONFIG_AUDIO_RTC_TOKEN_PIID         (4)
#define CONFIG_AUDIO_RTC_TOKEN_TYPE         (QM_SPEC_PROPERTY_FORMAT_NUMBER)

//RTC登录信息-licenseValue
#define CONFIG_AUDIO_RTC_LICENSE_VALUE_PIID (5)
#define CONFIG_AUDIO_RTC_LICENSE_VALUE_TYPE (QM_SPEC_PROPERTY_FORMAT_NUMBER)

typedef enum {
    QM_AI_SERVICE_ID_NONE = 0,

    QM_AI_SERVICE_ID_LICENSE = 0x10,

    QM_AI_SERVICE_ID_VOLUME = 0x20,
    QM_AI_SERVICE_ID_VOLUME_SET,
    QM_AI_SERVICE_ID_VOLUME_CTRL,
    QM_AI_SERVICE_ID_VOLUME_MUTE,

    QM_AI_SERVICE_ID_VOICE_CRTL = 0x30,
    QM_AI_SERVICE_ID_VOICE_CRTL_EMOTION,
    QM_AI_SERVICE_ID_VOICE_CRTL_CORPUS,
    QM_AI_SERVICE_ID_VOICE_CRTL_ABORT,
} qm_ai_service_id_t;

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