#include "qm.h"
#include "qm_kv.h"
#include "qm_agora_gen.h"

#include "util_httpc.h"
#include "json_parser.h"
#include "backoff_algorithm.h"

#include "qm_utils_string.h"
#include "qm_utils_hmac.h"

#include "qm_iot_config.h"

#define LOG_TAG "AGORA_GEN"

#define AGORA_GEN_TOLKEN_FORMAT  "?did=%lu&pid=%lu"

#define AGORA_STATUS_KEY         "status"
#define AGORA_MSG_KEY            "msg"
#define AGORA_TIMESTAMP_KEY      "timestamp"
#define AGORA_RESULT_KEY         "result"
#define AGORA_UID_KEY            "uid"
#define AGORA_APPID_KEY          "appId"
#define AGORA_CHANNELNAME_KEY    "channelName"
#define AGORA_TOKEN_KEY          "token"
#define AGORA_OK_KEY             "ok"

#define CONFIG_QM_AGORA_GEN_KV_HEADER   "agora/"

#define UID_KEY                 (CONFIG_QM_AGORA_GEN_KV_HEADER"uid")
#define APPID_KEY               (CONFIG_QM_AGORA_GEN_KV_HEADER"appId")
#define CHANNELNAME_KEY         (CONFIG_QM_AGORA_GEN_KV_HEADER"channelName")
#define TOKEN_KEY               (CONFIG_QM_AGORA_GEN_KV_HEADER"token")

#define AGORA_GEN_BUFFER_MAX_LEN 4096


int qm_iot_agora_uid_set(char *uid, uint32_t len)
{
    return qm_kv_set(UID_KEY, uid, len, 1);
}

int32_t qm_iot_agora_uid_get(char *uid, uint32_t *len)
{
    return qm_kv_get(UID_KEY, uid, (int *)len);
}

int qm_iot_agora_appid_set(char *appid, uint32_t len)
{
    return qm_kv_set(APPID_KEY, appid, len, 1);
}

int32_t qm_iot_agora_appid_get(char *appid, uint32_t *len)
{
    return qm_kv_get(APPID_KEY, appid, (int *)len);
}

int qm_iot_agora_channelName_set(char *channelName, uint32_t len)
{       
    return qm_kv_set(CHANNELNAME_KEY, channelName, len, 1);
}

int32_t qm_iot_agora_channelName_get(char *channelName, uint32_t *len)
{
    return qm_kv_get(CHANNELNAME_KEY, channelName, (int *)len);
}

int qm_iot_agora_token_set(char *token, uint32_t len)
{
    return qm_kv_set(TOKEN_KEY, token, len, 1);
}

int32_t qm_iot_agora_token_get(char *token, uint32_t *len)
{
    return qm_kv_get(TOKEN_KEY, token, (int *)len);
}

int32_t qm_iot_agora_gen_free(void **handle)
{
    qm_iot_agora_gen_handle_t *agora_handle = (qm_iot_agora_gen_handle_t *)*handle;
    if(agora_handle == NULL) {
        return QM_ERROR;
    }
    if(agora_handle->appId) {
        qm_free(agora_handle->appId);
        agora_handle->appId = NULL;
    }
    if(agora_handle->uid) {
        qm_free(agora_handle->uid);
        agora_handle->uid = NULL;
    }
    if(agora_handle->channelName) {
        qm_free(agora_handle->channelName);
        agora_handle->channelName = NULL;
    }
    if(agora_handle->token) {
        qm_free(agora_handle->token);
        agora_handle->token = NULL;
    }
    return QM_EOK;
}

/*
{
    "status":0,
    "msg":"OK",
    "timestamp":1756717466244,
    "result":{
        "uid":1971367809,
        "appId":"9942a7948c3f4c4f8970d54ef6adfb24",
        "channelName":"10290_19569330_1756717466243",
        "token":"007eJxTYNiiOM2Xveny7q8Hz+7bwuHHMFP1ZHLI+T8vGt4pB4kGOH1TYLC0NDFKNLc0sUg2TjNJNkmzsDQ3SDE1SU0zS0xJSzIymZW4NaMhkJGBheu9ESMDkAZiEJ8JTDKDSRYwKcNgaGBkaRBvaGlqZmlsDGSYm5qZG5qbmJkZmRhzMRhamhsam5lbGFgCAPagKVo="
    },
    "ok":true
}
*/ 
static int qm_iot_agora_gen_recv_handler(void *handle, char *buff, int len)
{
    int type;
    int code = 0;
    char *status = NULL;
    int status_len = 0;
    char *result = NULL;
    int result_len = 0;
    char *uid = NULL;
    int uid_len = 0;
    char *appId = NULL;
    int appId_len = 0;
    char *channelName = NULL;
    int channelName_len = 0;
    char *token = NULL;
    int token_len = 0;
    qm_iot_agora_gen_handle_t *agora_handle = (qm_iot_agora_gen_handle_t *)handle;

    status = qm_json_get_value_by_name(buff, len, AGORA_STATUS_KEY, &status_len, &type);
    if(status == NULL){
        return -QM_EINVAL;
    }
    code = int_str_to_num(status, status_len);
    if(code != QM_EOK){
        return -QM_ERROR;
    }

    result = qm_json_get_value_by_name(buff, len, AGORA_RESULT_KEY, &result_len, &type);
    if(result == NULL){
        QM_LOGE(LOG_TAG, "[result] NULL!!");
        return -QM_EINVAL;
    }
    uid = qm_json_get_value_by_name(result, result_len, AGORA_UID_KEY, (int *)&uid_len, &type);
    if(uid == NULL || !uid_len){
        QM_LOGE(LOG_TAG, "[uid] NULL!!");
        return -QM_EINVAL;
    }

    appId = qm_json_get_value_by_name(result, result_len, AGORA_APPID_KEY, (int *)&appId_len, &type);
    if(appId == NULL || !appId_len){
        QM_LOGE(LOG_TAG, "[appId] NULL!!");
        return -QM_EINVAL;
    }

    channelName = qm_json_get_value_by_name(result, result_len, AGORA_CHANNELNAME_KEY, (int *)&channelName_len, &type);
    if(channelName == NULL || !channelName_len){
        QM_LOGE(LOG_TAG, "[channelName] NULL!!");
        return -QM_EINVAL;
    }

    token = qm_json_get_value_by_name(result, result_len, AGORA_TOKEN_KEY, (int *)&token_len, &type);
    if(token == NULL || !token_len){
        QM_LOGE(LOG_TAG, "[token] NULL!!");
        return -QM_EINVAL;
    }
    
    agora_handle->uid = (char *)qm_malloc(uid_len+1);
    agora_handle->appId = (char *)qm_malloc(appId_len+1);
    agora_handle->channelName = (char *)qm_malloc(channelName_len+1);
    agora_handle->token = (char *)qm_malloc(token_len+1);

    if(agora_handle->appId == NULL || agora_handle->uid == NULL || agora_handle->channelName == NULL || agora_handle->token == NULL){
        goto __FREE;
    }
    
    memcpy(agora_handle->appId, appId, appId_len);
    agora_handle->appId_len = appId_len;
    agora_handle->appId[agora_handle->appId_len] = '\0';
    memcpy(agora_handle->uid, uid, uid_len);
    agora_handle->uid_len = uid_len;
    agora_handle->uid[agora_handle->uid_len] = '\0';
    memcpy(agora_handle->channelName, channelName, channelName_len);
    agora_handle->channelName_len = channelName_len;
    agora_handle->channelName[agora_handle->channelName_len] = '\0';
    memcpy(agora_handle->token, token, token_len);
    agora_handle->token_len = token_len;
    agora_handle->token[agora_handle->token_len] = '\0';

    QM_LOGD("1", "uid:%.*s", agora_handle->uid_len, agora_handle->uid);
    QM_LOGD("1", "appId:%.*s", agora_handle->appId_len, agora_handle->appId);
    QM_LOGD("1", "channelName:%.*s", agora_handle->channelName_len, agora_handle->channelName);
    QM_LOGD("1", "token:%.*s", agora_handle->token_len, agora_handle->token);
#if 0
    qm_iot_agora_uid_set(uid, uid_len);
    qm_iot_agora_appid_set(appId, appId_len);
    qm_iot_agora_channelName_set(channelName, channelName_len);
    qm_iot_agora_token_set(token, token_len);
#endif

    return QM_EOK;
__FREE:
    qm_iot_agora_gen_free((void *)&agora_handle);
    return -QM_ENOMEM;
}

int32_t qm_iot_agora_gen_start(void *handle)
{
    int32_t ret = QM_EOK;
    httpclient_t http = {0};              /* http client */
    httpclient_data_t http_data = {0};    /* http client data */
    char url[128];
    uint32_t url_len = 0;
    qm_iot_agora_gen_handle_t *agora_handle = (qm_iot_agora_gen_handle_t *)handle;

    char *buf = NULL;
    uint32_t buf_len = AGORA_GEN_BUFFER_MAX_LEN;
    buf = qm_malloc(buf_len);
    if(buf == NULL) {
        QM_LOGE(LOG_TAG, "allocate for buf failed");
        return -QM_ENOMEM;
    }

    qm_iot_agora_uri_get(url, &url_len);
    if(url[0] == '\0') {
        QM_LOGW(LOG_TAG, "agora uri is NULL");
        ret = -QM_EEMPTY;
        goto __FREE;
    }

    snprintf(url + url_len, sizeof(url) - url_len, AGORA_GEN_TOLKEN_FORMAT, qm_iot_did_get(), qm_iot_pid_get());

    http_data.response_buf = buf;
    http_data.response_buf_len = buf_len;
    if (0 != http_client_common(&http, url, 80, NULL, HTTPCLIENT_GET, 3000, &http_data)){
        QM_LOGE(LOG_TAG ,"http client failed");
        ret = -QM_EIO;
        goto __FREE;
    }
    
    ret = qm_iot_agora_gen_recv_handler(agora_handle, buf, http_data.response_content_len);

__FREE:
    if(buf) {
        qm_free(buf);
        buf = NULL;
    }
    http_client_close(&http);
    return ret;
}
