/*
 * qm_module_volume.c - Implementation of the Volume Management Module
 */

#include "qm_ai_volume.h"
#include "qm_utils_string.h"
#include "qm_event.h"
#include "qm_log.h"
#include "qm_kv.h"
#include "qm_iot_api.h"
#include "qm_ai_api.h"
#include <string.h>
#include <stdlib.h>
#include "qm_event.h"
#include "qm_workqueue.h"
#include "qm_work.h"

#define LOG_TAG "VOLUME"

// Reuse SIID/PIID from audio_car.c / config
#ifndef CONFIG_AUDIO_VOLUME_SIID
#define CONFIG_AUDIO_VOLUME_SIID        3
#endif
#ifndef CONFIG_AUDIO_VOLUME_PIID
#define CONFIG_AUDIO_VOLUME_PIID        1
#endif
#ifndef CONFIG_AUDIO_VOLUME_CMD_PIID
#define CONFIG_AUDIO_VOLUME_CMD_PIID    2
#endif
#ifndef CONFIG_AUDIO_VOLUME_MUTE_PIID
#define CONFIG_AUDIO_VOLUME_MUTE_PIID   3
#endif

typedef struct {
    qm_ai_license_config_t config;
    bool is_mute;
} volume_context_t;

static volume_context_t g_vol_ctx = {0};

static void vol_iot_service_handler(void *handle, int service_id, qm_iot_service_type_t type, qm_spec_property_operation_t *property_operation, void *userdata) {
    // if (!property_operation || type != QM_IOT_SERVICE_TYPE_GET_RSP) return;

    QM_LOGI("1", "vol set");
    qm_spec_property_t *prop = NULL;
    uint8_t siid = 0;
    uint16_t piid = 0;

    while (1) {
        prop = qm_spec_property_next(property_operation, prop);
        if (prop == NULL) {
            break;
        }

        qm_spec_property_unpack_xiid(prop, &siid, &piid);
        if (siid != CONFIG_AUDIO_VOLUME_SIID) {
            break;
        }

        switch (piid) {
            case CONFIG_AUDIO_VOLUME_PIID: {
                int val = 0;
                qm_spec_property_unpack_number(prop, &val);

                qm_event_ext_post(QM_EVENT_QM_AI, QM_AI_EVENT_VOLUME_CTRL, &val, sizeof(int), g_vol_ctx.config.ai_handle);
                break;
            }
            case CONFIG_AUDIO_VOLUME_MUTE_PIID: {
                bool_t mute = false;
                qm_spec_property_unpack_bool(prop, &mute);

                qm_event_ext_post(QM_EVENT_QM_AI, QM_AI_EVENT_MUTE_CTRL, &mute, sizeof(bool_t), g_vol_ctx.config.ai_handle);
                break;
            }
            case CONFIG_AUDIO_VOLUME_CMD_PIID: {
                int cmd = 0;
                qm_spec_property_unpack_number(prop, &cmd);

                qm_event_ext_post(QM_EVENT_QM_AI, QM_AI_EVENT_VOLUME_ADJUST, &cmd, sizeof(int), g_vol_ctx.config.ai_handle);
                break;
            }
            default: break;
        }
    }
}

static void register_iot_services(void) {
    if (!g_vol_ctx.config.transport_handle) return;
    
    qm_iot_service_iid_t iids1[] = {
        {CONFIG_AUDIO_VOLUME_SIID, CONFIG_AUDIO_VOLUME_PIID},
    };

    qm_iot_service_iid_t iids2[] = {
        {CONFIG_AUDIO_VOLUME_SIID, CONFIG_AUDIO_VOLUME_MUTE_PIID},
    };

    qm_iot_service_iid_t iids3[] = {
        {CONFIG_AUDIO_VOLUME_SIID, CONFIG_AUDIO_VOLUME_CMD_PIID}
    };
    
    qm_iot_service_register(g_vol_ctx.config.transport_handle, CONFIG_AUDIO_VOLUME_PIID, 
                           iids1, sizeof(iids1)/sizeof(qm_iot_service_iid_t),
                           vol_iot_service_handler, NULL);

    qm_iot_service_register(g_vol_ctx.config.transport_handle, CONFIG_AUDIO_VOLUME_MUTE_PIID, 
                           iids2, sizeof(iids2)/sizeof(qm_iot_service_iid_t),
                           vol_iot_service_handler, NULL);

    qm_iot_service_register(g_vol_ctx.config.transport_handle, CONFIG_AUDIO_VOLUME_CMD_PIID, 
                           iids3, sizeof(iids3)/sizeof(qm_iot_service_iid_t),
                           vol_iot_service_handler, NULL);
}


int32_t qm_ai_volume_init(qm_ai_license_config_t *config)
{
    if (!config) return -QM_EINVAL;
    qm_ai_volume_deinit();
    memcpy(&g_vol_ctx.config, config, sizeof(qm_ai_license_config_t));
    return QM_EOK;
}

int32_t qm_ai_volume_deinit(void)
{
    memset(&g_vol_ctx, 0, sizeof(volume_context_t));
    return QM_EOK;
}

int32_t qm_ai_volume_start(void)
{
    if (g_vol_ctx.config.transport_handle && g_vol_ctx.config.type == QM_LICENSE_RTC_QM_IOT) {
        register_iot_services();
    }

    return QM_EOK;
}

int32_t qm_ai_volume_stop(void)
{
    return QM_EOK;
}