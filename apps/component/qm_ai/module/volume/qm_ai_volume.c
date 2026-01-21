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

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

#define KEY_SYS_VOLUME      "sys_vol"
#define KEY_SYS_MUTE        "sys_mute"

#define DEFAULT_VOLUME      60
#define MIN_VOLUME          0
#define MAX_VOLUME          100

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

// -------------------------------------------------------------------------
// Internal Context
// -------------------------------------------------------------------------

typedef struct {
    int volume;
    bool is_mute;
    qm_ai_license_config_t config;
} volume_context_t;

static volume_context_t g_vol_ctx = {0};

// -------------------------------------------------------------------------
// Helper Functions
// -------------------------------------------------------------------------

static void save_volume_to_kv(void) {
    qm_kv_set(KEY_SYS_VOLUME, &g_vol_ctx.volume, sizeof(int), 1);
    qm_kv_set(KEY_SYS_MUTE, &g_vol_ctx.is_mute, sizeof(bool), 1);
}

// -------------------------------------------------------------------------
// IoT Callbacks
// -------------------------------------------------------------------------
static void vol_iot_service_handler(void *handle, int service_id, qm_iot_service_type_t type, qm_spec_property_operation_t *property_operation, void *userdata) {
    // if (!property_operation || type != QM_IOT_SERVICE_TYPE_GET_RSP) return;

    QM_LOGI("1", "vol set");
    qm_spec_property_t *prop = property_operation->property;
    bool vol_changed = false;
    bool mute_changed = false;

    while (prop) {
        uint8_t siid = 0;
        uint16_t piid = 0;
        qm_spec_property_unpack_xiid(prop, &siid, &piid);

        if (siid == CONFIG_AUDIO_VOLUME_SIID) {
            switch (piid) {
                case CONFIG_AUDIO_VOLUME_PIID: { // Set Volume
                    int val = 0;
                    qm_spec_property_unpack_number(prop, &val);
                    if (val < MIN_VOLUME) val = MIN_VOLUME;
                    if (val > MAX_VOLUME) val = MAX_VOLUME;
                    
                    if (g_vol_ctx.volume != val) {
                        g_vol_ctx.volume = val;
                        vol_changed = true;
                    }
                    break;
                }
                case CONFIG_AUDIO_VOLUME_MUTE_PIID: { // Set Mute
                    bool_t mute = false;

                    int mute_val = 0;
                    if (qm_spec_property_unpack_number(prop, &mute_val) == 0) {
                         if (mute_val == 2) mute = true;
                         else mute = false;
                    } else {
                        // Try bool
                        qm_spec_property_unpack_bool(prop, &mute);
                    }

                    if (g_vol_ctx.is_mute != mute) {
                        g_vol_ctx.is_mute = mute;
                        mute_changed = true;
                    }
                    break;
                }
                case CONFIG_AUDIO_VOLUME_CMD_PIID: { // Volume Command (Up/Down)
                    int cmd = 0;
                    qm_spec_property_unpack_number(prop, &cmd);
                    int current = g_vol_ctx.volume;
                    switch (cmd) {
                        case 1: current += 10; break;
                        case 2: current -= 10; break;
                        case 3: current = MAX_VOLUME; break;
                        case 4: current = MIN_VOLUME; break;
                    }
                    if (current < MIN_VOLUME) current = MIN_VOLUME;
                    if (current > MAX_VOLUME) current = MAX_VOLUME;
                    
                    if (g_vol_ctx.volume != current) {
                        g_vol_ctx.volume = current;
                        vol_changed = true;
                    }
                    break;
                }
                default: break;
            }
        }
        prop = prop->next;
    }
    
    if (vol_changed) {
        qm_event_ext_post(QM_EVENT_QM_AI, QM_AI_EVENT_VOLUME_CTRL, &g_vol_ctx.volume, sizeof(int), g_vol_ctx.config.ai_handle);
    }
    
    if (mute_changed) {
        // Post separate event for mute
        qm_event_ext_post(QM_EVENT_QM_AI, QM_AI_EVENT_MUTE_CTRL, &g_vol_ctx.is_mute, sizeof(bool), g_vol_ctx.config.ai_handle);
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


int32_t qm_ai_volume_init(qm_ai_license_config_t *config) {
    if (!config) return -QM_EINVAL;
    qm_ai_volume_deinit();
    memcpy(&g_vol_ctx.config, config, sizeof(qm_ai_license_config_t));
    return QM_EOK;
}

int32_t qm_ai_volume_deinit(void) {
    memset(&g_vol_ctx, 0, sizeof(volume_context_t));
    return QM_EOK;
}

int32_t qm_ai_volume_start(void)
{
    // Register IoT Services
    if (g_vol_ctx.config.transport_handle && g_vol_ctx.config.type == QM_LICENSE_RTC_QM_IOT) {
        register_iot_services();
    }

    QM_LOGI(LOG_TAG, "Volume Initialized: %d (Mute: %d)", g_vol_ctx.volume, g_vol_ctx.is_mute);
    // Sync with cloud

    return QM_EOK;
}

int32_t qm_ai_volume_stop(void)
{
    return QM_EOK;
}