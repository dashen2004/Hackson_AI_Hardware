#include "qm_ai_license.h"
#include "qm_utils_string.h"
#include "qm_event.h"
#include "qm_log.h"
#include "qm_kv.h"
#include "cJSON.h"
#include "qm_struct_mapper.h"
#include <string.h>
#include <stdlib.h>
#include "qm_event.h"
#include "qm_workqueue.h"
#include "qm_work.h"
#include "qm_iot_api.h"
#include "ai_main.h"
#include "qm_ai_api.h"

#define LOG_TAG "LICENSE"

#define KEY_LIC_UID         "lic_uid"
#define KEY_LIC_APPID       "lic_appid"
#define KEY_LIC_CHANNEL     "lic_channel"
#define KEY_LIC_TOKEN       "lic_token"
#define KEY_LIC_KEY         "lic_key"

typedef struct {
    // Credentials
    int uid;
    char *appid;
    char *channel_name;
    char *token;
    char *license_key;
    uint32_t pid;
    char *secret;

    // Status flags
    bool has_uid;
    bool has_appid;
    bool has_channel;
    bool has_token;
    bool has_license;
    bool has_pid;
    bool has_secret;

    // Lifecycle state
    bool is_inited;

    // Config & Context
    qm_ai_license_config_t config;

} license_context_t;

static license_context_t g_ctx = {0};

static void license_send_request_iot(void);
static void license_register_service_iot(void);

QM_STRUCT_BEGIN(qm_ai_rtc_auth_config_t)
    QM_FIELD_INT(qm_ai_rtc_auth_config_t, uid)
    QM_FIELD_STRING(qm_ai_rtc_auth_config_t, app_id)
    QM_FIELD_STRING(qm_ai_rtc_auth_config_t, channel_name)
    QM_FIELD_STRING(qm_ai_rtc_auth_config_t, token)
    QM_FIELD_STRING(qm_ai_rtc_auth_config_t, license)
QM_STRUCT_END(qm_ai_rtc_auth_config_t)

static void free_if_not_null(char **ptr) 
{
    if (*ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

static bool license_is_ready(void) 
{
    // Basic check - exact requirements depend on business logic
    if (!g_ctx.has_uid || !g_ctx.has_appid || 
        !g_ctx.has_channel || !g_ctx.has_token) {
        return false;
    }

    return true;
}

static int license_get_config(qm_ai_rtc_auth_config_t *config_out) 
{
    if (!config_out) return -QM_EINVAL;
    if (!license_is_ready()) return -QM_ERROR;

    config_out->uid = g_ctx.uid;
    config_out->app_id = g_ctx.appid;
    config_out->channel_name = g_ctx.channel_name;
    config_out->token = g_ctx.token;
    config_out->license = g_ctx.license_key;

    return QM_EOK;
}

static cJSON *license_convert_to_json(void) 
{
    if (!license_is_ready()) return NULL;

    qm_ai_rtc_auth_config_t config;
    if (license_get_config(&config) != QM_EOK) {
        return NULL;
    }

    return qm_struct_to_json(&config, &QM_STRUCT_DESC_NAME(qm_ai_rtc_auth_config_t));
}

static void license_post_ready_event(void) 
{
    cJSON *json = license_convert_to_json();
    if (json) {
        char *json_str = cJSON_PrintUnformatted(json);
        if (json_str) {
            qm_event_ext_post(QM_EVENT_QM_AI, QM_AI_EVENT_CLOUD_LICENSE, json_str, strlen(json_str) + 1, g_ctx.config.ai_handle);
        }

        cJSON_Delete(json);
    }
}

static void license_load_from_kv(void) 
{
    int len;
    int temp_int;
    char buffer[512]; 

    QM_LOGD(LOG_TAG, "Loading license from KV...");

    // UID
    len = sizeof(int);
    if (qm_kv_get(KEY_LIC_UID, &temp_int, &len) == 0) {
        g_ctx.uid = temp_int;
        g_ctx.has_uid = true;
    }

    // AppID
    len = sizeof(buffer);
    if (qm_kv_get(KEY_LIC_APPID, buffer, &len) == 0 && len > 0) {
        free_if_not_null(&g_ctx.appid);
        g_ctx.appid = strdup(buffer);
        g_ctx.has_appid = true;
    }

    // Channel
    len = sizeof(buffer);
    if (qm_kv_get(KEY_LIC_CHANNEL, buffer, &len) == 0 && len > 0) {
        free_if_not_null(&g_ctx.channel_name);
        g_ctx.channel_name = strdup(buffer);
        g_ctx.has_channel = true;
    }

    // Token
    len = sizeof(buffer);
    if (qm_kv_get(KEY_LIC_TOKEN, buffer, &len) == 0 && len > 0) {
        free_if_not_null(&g_ctx.token);
        g_ctx.token = strdup(buffer);
        g_ctx.has_token = true;
    }

    // License Key
    len = sizeof(buffer);
    if (qm_kv_get(KEY_LIC_KEY, buffer, &len) == 0 && len > 0) {
        free_if_not_null(&g_ctx.license_key);
        g_ctx.license_key = strdup(buffer);
        g_ctx.has_license = true;
    }
}

static void lic_iot_service_handler(void *handle, int service_id, qm_iot_service_type_t type, qm_spec_property_operation_t *property_operation, void *userdata) 
{
    if (!property_operation || type != QM_IOT_SERVICE_TYPE_GET_RSP) {
        return;
    }

    qm_spec_property_t *prop = NULL;
    uint8_t siid = 0;
    uint16_t piid = 0;

    while (1) {
        prop = qm_spec_property_next(property_operation, prop);
        if (prop == NULL) {
            break;
        }

        qm_spec_property_unpack_xiid(prop, &siid, &piid);
        if (siid != CONFIG_AUDIO_RTC_LOGIN_SIID) {
            break;
        }

        switch (piid) {
            case CONFIG_AUDIO_RTC_UID_PIID: {
                int val = 0;
                qm_spec_property_unpack_number(prop, &val);
                g_ctx.uid = val;
                g_ctx.has_uid = true;
                qm_kv_set(KEY_LIC_UID, &val, sizeof(int), 1);
                break;
            }

            case CONFIG_AUDIO_RTC_APPID_PIID: {
                char val[128] = {0};
                int len = 0;
                qm_spec_property_unpack_string(prop, val, &len);
                if (len > 0) {
                    free_if_not_null(&g_ctx.appid);
                    g_ctx.appid = strdup(val);
                    g_ctx.has_appid = true;
                    qm_kv_set(KEY_LIC_APPID, val, len, 1);
                }
                break;
            }

            case CONFIG_AUDIO_RTC_CHANNAL_NAME_PIID: {
                char val[128] = {0};
                int len = 0;
                qm_spec_property_unpack_string(prop, val, &len);
                if (len > 0) {
                    free_if_not_null(&g_ctx.channel_name);
                    g_ctx.channel_name = strdup(val);
                    g_ctx.has_channel = true;
                    qm_kv_set(KEY_LIC_CHANNEL, val, len, 1);
                }
                break;
            }

            case CONFIG_AUDIO_RTC_TOKEN_PIID: {
                char val[512] = {0};
                int len = 0;
                qm_spec_property_unpack_string(prop, val, &len);
                if (len > 0) {
                    free_if_not_null(&g_ctx.token);
                    g_ctx.token = strdup(val);
                    g_ctx.has_token = true;
                    qm_kv_set(KEY_LIC_TOKEN, val, len, 1);
                }
                break;
            }

            case CONFIG_AUDIO_RTC_LICENSE_VALUE_PIID: {
                char val[256] = {0};
                int len = 0;
                qm_spec_property_unpack_string(prop, val, &len);
                if (len > 0) {
                    free_if_not_null(&g_ctx.license_key);
                    g_ctx.license_key = strdup(val);
                    g_ctx.has_license = true;
                    qm_kv_set(KEY_LIC_KEY, val, len, 1);
                }
                break;
            }

            default:
                break;
        }
    }

    // Check if ready after processing all properties
    if (license_is_ready()) {
        license_post_ready_event();
    }
}

int32_t qm_ai_license_init(qm_ai_license_config_t *config) 
{
    if (config == NULL) {
        return QM_EINIT;
    }

    if (g_ctx.is_inited) {
        return QM_EINIT;
    }

    memcpy(&g_ctx.config, config, sizeof(qm_ai_license_config_t));

    if (g_ctx.config.transport_handle && g_ctx.config.ai_handle) {
        if (g_ctx.config.type == QM_LICENSE_RTC_QM_IOT) {
            license_register_service_iot();
        } else {
            QM_LOGW(LOG_TAG, "Not supported now");
            return QM_EINVAL;
        }
    }

    g_ctx.is_inited = true;
    
    return QM_EOK;
}

int32_t qm_ai_license_start(void)
{
    if (!g_ctx.is_inited) {
        return QM_EINIT;
    }

    // load_license_from_kv();

    if (license_is_ready()) {
        license_post_ready_event();
    } else {
        if (g_ctx.config.type == QM_LICENSE_RTC_QM_IOT) {
            license_send_request_iot();
        } else if (g_ctx.config.type == QM_LICENSE_RTC_HTTP) {
            QM_LOGW(LOG_TAG, "Not supported now");
            return QM_EINVAL;
        }
    }

    return QM_EOK;
}

int32_t qm_ai_license_stop(void)
{
    return QM_EOK;
}

int32_t qm_ai_license_deinit(void) 
{
    free_if_not_null(&g_ctx.appid);
    free_if_not_null(&g_ctx.channel_name);
    free_if_not_null(&g_ctx.token);
    free_if_not_null(&g_ctx.license_key);
    free_if_not_null(&g_ctx.secret);

    g_ctx.is_inited = 0;
    
    memset(&g_ctx, 0, sizeof(license_context_t));
    return QM_EOK;
}

// use iot to get license
static void license_send_request_iot(void)
{
    if (g_ctx.config.type != QM_LICENSE_RTC_QM_IOT || !g_ctx.config.transport_handle) return;
    
    // Create property operation for GET request
    qm_spec_property_operation_t *op = qm_spec_property_operation_creat();
    if (!op) return;
    
    qm_spec_property_t *prop = qm_spec_property_creat();
    if (!prop) {
        // qm_spec_property_operation_free(op);
        return;
    }

    qm_spec_property_pack_xiid(prop, CONFIG_AUDIO_RTC_LOGIN_SIID, 0);
    prop->did = qm_iot_did_get();
    
    qm_spec_property_add(op, prop);
    
    qm_iot_property_get_request(g_ctx.config.transport_handle, op);
}

static void license_register_service_iot(void)
{
    if (g_ctx.config.type != QM_LICENSE_RTC_QM_IOT || !g_ctx.config.transport_handle) {
        return;
    }

    qm_iot_service_iid_t iids[] = {
        {CONFIG_AUDIO_RTC_LOGIN_SIID, CONFIG_AUDIO_RTC_UID_PIID},
        {CONFIG_AUDIO_RTC_LOGIN_SIID, CONFIG_AUDIO_RTC_APPID_PIID},
        {CONFIG_AUDIO_RTC_LOGIN_SIID, CONFIG_AUDIO_RTC_CHANNAL_NAME_PIID},
        {CONFIG_AUDIO_RTC_LOGIN_SIID, CONFIG_AUDIO_RTC_TOKEN_PIID},
        {CONFIG_AUDIO_RTC_LOGIN_SIID, CONFIG_AUDIO_RTC_LICENSE_VALUE_PIID}
    };

    qm_iot_service_register(g_ctx.config.transport_handle, CONFIG_AUDIO_RTC_LOGIN_SIID, 
                           iids, sizeof(iids)/sizeof(qm_iot_service_iid_t),
                           lic_iot_service_handler, NULL);
}
