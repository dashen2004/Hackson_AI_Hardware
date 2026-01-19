#include "qm.h"
#include "qm_kv.h"
#include "qm_agora.h"
#include "qm_agora_gen.h"

#include "util_httpc.h"
#include "json_parser.h"

#include "qm_utils_string.h"
#include "qm_utils_hmac.h"
#include "cJSON.h"

#include "qm_iot_config.h"

#define LOG_TAG "AGORA_GEN"

#define DEFAULT_LICENSE_JSON \
"{\
\"appId\":\"e19c736eef894f21b192aadea9665484\",\
\"channelName\":\"10506_100000180_1768824671372\",\
\"token\":\"007eJxTYLh/qFxSxNJgg6H7lstGduLmp5cXyUg8X5InLbDk28sOdjsFhlRDy2RzY7PU1DQLS5M0I8MkQ0ujxMSU1ERLMzNTEwuTePm8zIZARoa6z7vaGRkYGViAGMRnApPMYJIFTMoyGBqYGpjFGxqAgKGFQbyhuZmFhZGJmbmhsbkRJwNcAgCkISbv\",\
\"uid\":100000180\
}"

qm_agora_gen_handle_t *agora_gen_handle = NULL;

static int32_t rtc_agora_gen_free(void *handle)
{
    qm_agora_gen_handle_t *agora_handle = (qm_agora_gen_handle_t *)handle;
    if(agora_handle == NULL) {
        return QM_ERROR;
    }
    agora_handle->exist_data = QM_FALSE;
    agora_gen_handle->uid = 0;
    if(agora_handle->appId) {
        qm_free(agora_handle->appId);
        agora_handle->appId = NULL;
    }
    if(agora_handle->channelName) {
        qm_free(agora_handle->channelName);
        agora_handle->channelName = NULL;
    }
    if(agora_handle->token) {
        qm_free(agora_handle->token);
        agora_handle->token = NULL;
    }
#if CONFIG_AGORA_USE_LICENSE
    if(agora_handle->licenseValue) {
        qm_free(agora_handle->licenseValue);
        agora_handle->licenseValue = NULL;
    }
#endif
    return QM_EOK;
}

int32_t rtc_agora_gen_set_data(qm_agora_gen_data_type_t type, void *data, int len)
{
    if (agora_gen_handle->exist_data == QM_TRUE) {
        rtc_agora_gen_free(agora_gen_handle);
    }

    switch (type)
    {
        case QM_AGORA_GEN_TYPE_UID:
            agora_gen_handle->uid = *(int *)data;
            QM_LOGD(LOG_TAG, "uid:%d", agora_gen_handle->uid);
        break;
        case QM_AGORA_GEN_TYPE_APPID:
            agora_gen_handle->appId = (char *)data;
            agora_gen_handle->appId_len = len;
            agora_gen_handle->appId[agora_gen_handle->appId_len] = '\0';
            QM_LOGD(LOG_TAG, "appId:%.*s", agora_gen_handle->appId_len, agora_gen_handle->appId);
        break;
        case QM_AGORA_GEN_TYPE_CHANNEL_NAME:
            agora_gen_handle->channelName = (char *)data;
            agora_gen_handle->channelName_len = len;
            agora_gen_handle->channelName[agora_gen_handle->channelName_len] = '\0';
            QM_LOGD(LOG_TAG, "channelName:%.*s", agora_gen_handle->channelName_len, agora_gen_handle->channelName);
        break;
        case QM_AGORA_GEN_TYPE_TOKEN:
            agora_gen_handle->token = (char *)data;
            agora_gen_handle->token_len = len;
            agora_gen_handle->token[agora_gen_handle->token_len] = '\0';
            QM_LOGD(LOG_TAG, "token:%.*s", agora_gen_handle->token_len, agora_gen_handle->token);
        break;
        case QM_AGORA_GEN_TYPE_LICENSE_VALUE:
        #if CONFIG_AGORA_USE_LICENSE
            agora_gen_handle->licenseValue = (char *)data;
            agora_gen_handle->licenseValue_len = len;
            agora_gen_handle->licenseValue[agora_gen_handle->licenseValue_len] = '\0';
            QM_LOGD(LOG_TAG, "licesenValue:%.*s", agora_gen_handle->licenseValue_len, agora_gen_handle->licenseValue);
        #endif
        break;
        default:
        break;
    }

    if (agora_gen_handle->appId && agora_gen_handle->channelName && 
        agora_gen_handle->token && agora_gen_handle->uid) {
        #if CONFIG_AGORA_USE_LICENSE
            if (agora_gen_handle->licenseValue) {
                agora_gen_handle->exist_data = QM_TRUE;
            }
        #else
        agora_gen_handle->exist_data = QM_TRUE;
        #endif
    }

    return QM_EOK;
}

int32_t rtc_agora_gen_unpack_license(char *license)
{
    cJSON *root = NULL;
    cJSON *item = NULL;
    if (license == NULL) {
        return QM_EINVAL;
    }

    root = cJSON_Parse(license);
    if (root == NULL) {
        QM_LOGW(LOG_TAG, "license cJSON_Parse failed");
        return QM_EINVAL;
    }

    item = cJSON_GetObjectItem(root, "appId");
    if (cJSON_IsString(item) == true) {
        rtc_agora_gen_set_data(QM_AGORA_GEN_TYPE_APPID, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "channelName");
    if (cJSON_IsString(item) == true) {
        rtc_agora_gen_set_data(QM_AGORA_GEN_TYPE_CHANNEL_NAME, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "token");
    if (cJSON_IsString(item) == true) {
        rtc_agora_gen_set_data(QM_AGORA_GEN_TYPE_TOKEN, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "uid");
    if (cJSON_IsNumber(item) == true) {
        int uid = item->valueint;
        rtc_agora_gen_set_data(QM_AGORA_GEN_TYPE_UID, &uid, sizeof(int));
    }

    // cJSON_Delete(root);
    return QM_EOK;
}

void *qm_agora_gen_get_data(qm_agora_gen_data_type_t type) 
{
    void *data = NULL;
    if (agora_gen_handle->exist_data == QM_FALSE) {
        return NULL;
    }

    switch (type)
    {
        case QM_AGORA_GEN_TYPE_UID:
            data = (void *)&agora_gen_handle->uid;
        break;
        case QM_AGORA_GEN_TYPE_APPID:
            data = (void *)agora_gen_handle->appId;
        break;
        case QM_AGORA_GEN_TYPE_CHANNEL_NAME:
            data = (void *)agora_gen_handle->channelName;
        break;
        case QM_AGORA_GEN_TYPE_TOKEN:
            data = (void *)agora_gen_handle->token;
        break;
        case QM_AGORA_GEN_TYPE_LICENSE_VALUE:
        #if CONFIG_AGORA_USE_LICENSE
            data = (void *)agora_gen_handle->licenseValue;
        #else
            data = NULL;
        #endif
        break;
        default:
        break;
    }

    return data;
}

void *qm_agora_gen_init(char *license)
{
    qm_agora_gen_handle_t *agora_handle = NULL;

    if (agora_gen_handle != NULL) {
        return agora_gen_handle;
    }

    agora_handle = (qm_agora_gen_handle_t*)qm_malloc(sizeof(qm_agora_gen_handle_t));
    if (agora_handle == NULL) {
        return NULL;
    }

    memset(agora_handle, 0, sizeof(qm_agora_gen_handle_t));
    agora_gen_handle = agora_handle;

    rtc_agora_gen_unpack_license(DEFAULT_LICENSE_JSON);
    // rtc_agora_gen_unpack_license(license);

    return agora_handle;
}
