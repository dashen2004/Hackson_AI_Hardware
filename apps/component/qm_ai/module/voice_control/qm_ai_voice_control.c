/*
 * qm_module_interaction.c - Implementation of the Interaction Module
 */

#include "qm_ai_voice_control.h"
#include "qm_iot_api.h"
#include "qm_log.h"
#include "qm_event.h"
#include "qm_ai_api.h"
#include <string.h>
#include <stdlib.h>
#include "qm_work.h"

#define LOG_TAG "INTERACTION"

// Voice Control Service
#ifndef CONFIG_AUDIO_VOICE_CONTROL_SIID
#define CONFIG_AUDIO_VOICE_CONTROL_SIID     6
#endif

#ifndef CONFIG_AUDIO_VOICE_CORPUS_PIID
#define CONFIG_AUDIO_VOICE_CORPUS_PIID      1
#endif
#ifndef CONFIG_AUDIO_VOICE_ABORT_PIID
#define CONFIG_AUDIO_VOICE_ABORT_PIID       2
#endif
#ifndef CONFIG_AUDIO_VOICE_EMOTION_PIID
#define CONFIG_AUDIO_VOICE_EMOTION_PIID     3
#endif

// Chat Service
#ifndef CONFIG_AUDIO_EXIT_CHAT_SIID
#define CONFIG_AUDIO_EXIT_CHAT_SIID         5
#endif
#ifndef CONFIG_AUDIO_CHAT_STATE_PIID
#define CONFIG_AUDIO_CHAT_STATE_PIID        1
#endif

typedef struct {
    qm_ai_license_config_t config;
    bool is_inited;
} interaction_context_t;

static interaction_context_t g_int_ctx = {0};

static void voice_control_send_request_iot(uint8_t service_siid, uint8_t service_piid, uint8_t param);
static void voice_control_register_service_iot(void); 

static void voice_control_iot_service_handler(void *handle, int service_id, qm_iot_service_type_t type, 
                                                qm_spec_property_operation_t *property_operation, void *userdata) 
{
    if (!property_operation || type != QM_IOT_SERVICE_TYPE_SET) return;

    qm_spec_property_t *prop = NULL;
    uint8_t siid = 0;
    uint16_t piid = 0;

    while (1) {
        prop = qm_spec_property_next(property_operation, prop);
        if (prop == NULL) {
            break;
        }

        qm_spec_property_unpack_xiid(prop, &siid, &piid);
        if (siid != CONFIG_AUDIO_VOICE_CONTROL_SIID) {
            break;
        }

        switch (piid) {
            case CONFIG_AUDIO_VOICE_CORPUS_PIID:
            case CONFIG_AUDIO_VOICE_ABORT_PIID:
            break;

            case CONFIG_AUDIO_VOICE_EMOTION_PIID:
                char *emotion = NULL;
                int len = 0;
                if(prop == NULL) {
                    return;
                }
                qm_spec_property_unpack_string(prop, NULL, &len);
                if(len == 0) {
                    return;
                }
                emotion = (char *)qm_malloc(len);
                if(emotion == NULL) {
                    return;
                }
                qm_spec_property_unpack_string(prop, emotion, &len);
                if(emotion == NULL) {
                    return;
                }

                qm_event_ext_post(QM_EVENT_QM_AI, QM_AI_EVENT_AGENT_LLM, emotion, len, g_int_ctx.config.ai_handle);
            break;
        }
    }
}

int32_t qm_ai_voice_control_init(qm_ai_license_config_t *config) 
{
    if (config == NULL) {
        return QM_EINIT;
    }

    if (g_int_ctx.is_inited) {
        return QM_EINIT;
    }

    memcpy(&g_int_ctx.config, config, sizeof(qm_ai_license_config_t));

    if (g_int_ctx.config.transport_handle && g_int_ctx.config.ai_handle) {
        if (g_int_ctx.config.type == QM_LICENSE_RTC_QM_IOT) {
            voice_control_register_service_iot();
        } else if (g_int_ctx.config.type == QM_LICENSE_RTC_HTTP) {
            QM_LOGW(LOG_TAG, "Not supported now");
            return QM_EINVAL;
        }
    }

    g_int_ctx.is_inited = 1;
    
    return QM_EOK;
}

int32_t qm_ai_voice_control_start(void)
{
    return QM_EOK;
}

int32_t qm_ai_voice_control_stop(void)
{
    return QM_EOK;
}

int32_t qm_ai_voice_control_deinit(void) 
{
    g_int_ctx.is_inited = 0;
    memset(&g_int_ctx, 0, sizeof(interaction_context_t));

    return QM_EOK;
}

int32_t qm_ai_voice_control_cmd(qm_ai_voice_control_cmd_t cmd, qm_ai_voice_control_param_t param)
{
    switch (cmd) {
        case QM_AI_VOICE_CONTROL_REQ_CORPUS:
            if (param == QM_AI_VOICE_CONTROL_PARAM_CORPUS_OPEN) {
                voice_control_send_request_iot(CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_CORPUS_PIID, 1);
            } else if (param == QM_AI_VOICE_CONTROL_PARAM_CORPUS_CLOSE) {
                voice_control_send_request_iot(CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_CORPUS_PIID, 2);
            }
        break;

        case QM_AI_VOICE_CONTROL_REQ_ABORT:
            if (param == QM_AI_VOICE_CONTROL_PARAM_ABORT) {
                voice_control_send_request_iot(CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_ABORT_PIID, 1);
            } else if (param == QM_AI_VOICE_CONTROL_PARAM_ABORT_WITH_WORD) {
                voice_control_send_request_iot(CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_ABORT_PIID, 2);
            }
        break;

        default:
        break;
    }

    return QM_EOK;
}

static void voice_control_send_request_iot(uint8_t service_siid, uint8_t service_piid, uint8_t param)
{
    if (g_int_ctx.config.type != QM_LICENSE_RTC_QM_IOT || !g_int_ctx.config.transport_handle) return;

    qm_spec_property_operation_t *op = qm_spec_property_operation_creat();
    if (!op) return;
    
    qm_spec_property_t *prop = qm_spec_property_creat();
    if (!prop) {
        // qm_spec_property_operation_free(op);
        return;
    }

    qm_spec_property_pack_xiid(prop, service_siid, service_piid);
    qm_spec_property_pack_number(prop, param);
    prop->did = qm_iot_did_get();
    
    qm_spec_property_add(op, prop);
    
    qm_iot_property_report(g_int_ctx.config.transport_handle, op);
}

static void voice_control_register_service_iot(void) 
{
    if (g_int_ctx.config.type != QM_LICENSE_RTC_QM_IOT || !g_int_ctx.config.transport_handle) {
        return;
    }

    qm_iot_service_iid_t service_corpus[] = {
        {CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_CORPUS_PIID},
    };

    qm_iot_service_iid_t service_abort[] = {
        {CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_ABORT_PIID},
    };

    qm_iot_service_iid_t service_emotion[] = {
        {CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_EMOTION_PIID},
    };

    qm_iot_service_register(g_int_ctx.config.transport_handle, QM_AI_SERVICE_ID_VOICE_CRTL_CORPUS, 
                            service_corpus, 1,
                           voice_control_iot_service_handler, NULL);

    qm_iot_service_register(g_int_ctx.config.transport_handle, QM_AI_SERVICE_ID_VOICE_CRTL_ABORT, 
                            service_abort, 1,
                           voice_control_iot_service_handler, NULL);

    qm_iot_service_register(g_int_ctx.config.transport_handle, QM_AI_SERVICE_ID_VOICE_CRTL_EMOTION,
                            service_emotion, 1,
                           voice_control_iot_service_handler, NULL);
}
