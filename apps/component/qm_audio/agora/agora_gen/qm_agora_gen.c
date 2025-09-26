#include "qm.h"
#include "qm_kv.h"
#include "qm_agora.h"
#include "qm_agora_gen.h"

#include "util_httpc.h"
#include "json_parser.h"
#include "backoff_algorithm.h"

#include "qm_utils_string.h"
#include "qm_utils_hmac.h"

#include "qm_iot_config.h"

#define LOG_TAG "AGORA_GEN"

qm_agora_gen_handle_t *agora_gen_handle = NULL;

static int32_t qm_agora_gen_free(void *handle)
{
    qm_agora_gen_handle_t *agora_handle = (qm_agora_gen_handle_t *)handle;
    if(agora_handle == NULL) {
        return QM_ERROR;
    }
    agora_handle->exist_data = QM_FALSE;
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

char *qm_agora_gen_get_data(qm_agora_gen_data_type_t type) 
{
    char *data = NULL;
    if(agora_gen_handle->exist_data == QM_FALSE) {
        return NULL;
    }
    switch (type)
    {
        case QM_AGORA_GEN_TYPE_UID:
            data = agora_gen_handle->uid;
        break;
        case QM_AGORA_GEN_TYPE_APPID:
            data = agora_gen_handle->appId;
        break;
        case QM_AGORA_GEN_TYPE_CHANNAL_NAME:
            data = agora_gen_handle->channelName;
        break;
        case QM_AGORA_GEN_TYPE_TOKEN:
            data = agora_gen_handle->token;
        break;
        default:
        break;
    }
    return data;
}

int32_t qm_agora_gen_set_data(qm_agora_gen_data_type_t type, char *data, int len)
{
    if(agora_gen_handle->exist_data == QM_TRUE) {
        qm_agora_gen_free(agora_gen_handle);
    }
    switch (type)
    {
        case QM_AGORA_GEN_TYPE_UID:
            agora_gen_handle->uid = data;
            agora_gen_handle->uid_len = len;
            QM_LOGD(LOG_TAG, "uid:%.*s", agora_gen_handle->uid_len, agora_gen_handle->uid);
        break;
        case QM_AGORA_GEN_TYPE_APPID:
            agora_gen_handle->appId = data;
            agora_gen_handle->appId_len = len;
            QM_LOGD(LOG_TAG, "appId:%.*s", agora_gen_handle->appId_len, agora_gen_handle->appId);
        break;
        case QM_AGORA_GEN_TYPE_CHANNAL_NAME:
            agora_gen_handle->channelName = data;
            agora_gen_handle->channelName_len = len;
            QM_LOGD(LOG_TAG, "channelName:%.*s", agora_gen_handle->channelName_len, agora_gen_handle->channelName);
        break;
        case QM_AGORA_GEN_TYPE_TOKEN:
            agora_gen_handle->token = data;
            agora_gen_handle->token_len = len;
            QM_LOGD(LOG_TAG, "token:%.*s", agora_gen_handle->token_len, agora_gen_handle->token);
        break;
        default:
        break;
    }
    if(agora_gen_handle->appId && agora_gen_handle->channelName && 
        agora_gen_handle->token && agora_gen_handle->uid) {
        agora_gen_handle->exist_data = QM_TRUE;
    }
    return QM_EOK;
}

#if CONFIG_AGORA_GEN_USE_MQTT

#define CONFIG_AGORA_GEN_TIMEOUT_MS (12 * 60 * 60 * 1000)

extern qm_err_t voice_rtc_login_info_get(void);

int32_t qm_agora_gen_start(void *handle)
{
    voice_rtc_login_info_get();
    qm_timer_start(&agora_gen_handle->timer);
}

static void qm_agora_gen_ticks_timeout(qm_timer_t *timer, void *arg)
{
    voice_rtc_login_info_get();
}

void *qm_agora_gen_init(void)
{
    qm_agora_gen_handle_t *agora_handle = NULL;

    if(agora_gen_handle != NULL) {
        return agora_gen_handle;
    }

    agora_handle = (qm_agora_gen_handle_t*)qm_malloc(sizeof(qm_agora_gen_handle_t));
    if (agora_handle == NULL) {
        return NULL;
    }
    memset(agora_handle, 0, sizeof(qm_agora_gen_handle_t));
    agora_gen_handle = agora_handle;

    qm_timer_new(&agora_gen_handle->timer, qm_agora_gen_ticks_timeout, NULL, CONFIG_AGORA_GEN_TIMEOUT_MS, 1);
    return agora_handle;
}

#else

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

#define AGORA_GEN_BUFFER_MAX_LEN    (4096)
#define BACKOFF_BASE_TIME_MS        (1 * 1000)
#define BACKOFF_RECV_TIMEOUT_MS     (10 * 1000)
#define BACKOFF_RETRY_NUM           (50)

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
static int qm_agora_gen_recv_handler(void *handle, char *buff, int len)
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
    qm_agora_gen_handle_t *agora_handle = (qm_agora_gen_handle_t *)handle;

    qm_agora_gen_free(agora_handle);

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
    agora_handle->exist_data = QM_TRUE;
    QM_LOGD(LOG_TAG, "uid:%.*s", agora_handle->uid_len, agora_handle->uid);
    QM_LOGD(LOG_TAG, "appId:%.*s", agora_handle->appId_len, agora_handle->appId);
    QM_LOGD(LOG_TAG, "channelName:%.*s", agora_handle->channelName_len, agora_handle->channelName);
    QM_LOGD(LOG_TAG, "token:%.*s", agora_handle->token_len, agora_handle->token);

    return QM_EOK;
__FREE:
    qm_agora_gen_free(agora_handle);
    return -QM_ENOMEM;
}

void *qm_agora_gen_init(void)
{
    qm_agora_gen_handle_t *agora_handle = NULL;

    agora_handle = (qm_agora_gen_handle_t*)qm_malloc(sizeof(qm_agora_gen_handle_t));
    if (agora_handle == NULL) {
        return NULL;
    }
    memset(agora_handle, 0, sizeof(qm_agora_gen_handle_t));
    agora_gen_handle = agora_handle;
    return agora_handle;
}

int32_t qm_agora_gen_start(void *handle)
{
    int32_t ret = QM_EOK;
    char url[128] = {0};
    uint32_t url_len = 0;
    httpclient_t http = {0};
    httpclient_data_t http_data = {0};
    qm_agora_gen_handle_t *agora_handle = (qm_agora_gen_handle_t *)handle;
    backoff_algorithm_ctx_t backoff_ctx = {0};
    backoff_algorithm_status_t status = BACKOFF_ALGORITHM_SUCCESS;
    uint16_t next_backoff = 0; 

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

    QM_LOGD(LOG_TAG, "agora gen url: %s", url);

    http_data.response_buf = buf;
    http_data.response_buf_len = buf_len;
    backoff_algorithm_init(&backoff_ctx, BACKOFF_BASE_TIME_MS, BACKOFF_RECV_TIMEOUT_MS, BACKOFF_RETRY_NUM);

    while (1) {
        ret = http_client_common(&http, url, 80, NULL, HTTPCLIENT_GET, 3000, &http_data);
        if(ret == QM_EOK && http.response_code == 200 && http_data.response_content_len){
            break;
        }

        status = backoff_algorithm_get_next_delay(&backoff_ctx, &next_backoff);
        if(status != BACKOFF_ALGORITHM_SUCCESS){
            ret = -QM_ETIMEOUT;
            break;
        }

        qm_msleep(next_backoff);
    }
    
    http_client_close(&http);
    
    if(ret != QM_EOK){
        goto __FREE;
    }

    ret = qm_agora_gen_recv_handler(agora_handle, buf, http_data.response_content_len);

__FREE:
    if(buf) {
        qm_free(buf);
        buf = NULL;
    }
    return ret;
}
#endif
