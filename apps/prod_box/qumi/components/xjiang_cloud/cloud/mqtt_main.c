#include "iot_mqtt_debug.h"
#include "iot_mqtt.h"
#include "cloud_common.h"
#include "cloudapi.h"
#include "iot_device.h"
#include "iot_guider.h"
#include "qm_utils_list.h"
#include "product.h"
#include "iot_event.h"
#include "qm_iot_export.h"
#include "json_parser.h"
#include "qm_utils_string.h"
#include "qm_utils_base64.h"


#if CONFIG_OTA_SUPPORT
#include "iot_ota.h"
#endif


static cloud_context_t *mqtt_cloud_context;
static iot_mqtt_topic_info_t topic_msg;

#ifdef CONFIG_OTA_SUPPORT
void *h_ota = NULL;
#endif
static void *pclient;


#define MQTT_MSGLEN             (1024*2)
#define MQTT_SEND_MSGLEN        (1024*6)
#define MAX_MQTT_TOPIC_LEN      (64)


#ifdef PRODTST_SUPPORT

#define MQTT_TOPIC_PRODTST_CONFIG_FORMAT             "/%u/%s/prodtst/config"
#define MQTT_TOPIC_PRODTST_CONFIG_REPLY_FORMAT       "/%u/%s/prodtst/config_reply"
#define MQTT_TOPIC_PRODTST_SET_FORMAT                "/%u/%s/prodtst/set"
#define MQTT_TOPIC_PRODTST_SET_REPLY_FORMAT          "/%u/%s/prodtst/set_reply"
#define MQTT_TOPIC_PRODTST_GET_FORMAT                "/%u/%s/prodtst/get"
#define MQTT_TOPIC_PRODTST_GET_REPLY_FORMAT          "/%u/%s/prodtst/get_reply"
#define MQTT_TOPIC_PRODTST_REPORT_FORMAT             "/%u/%s/prodtst/report"
#define MQTT_TOPIC_PRODTST_REPORT_REPLY_FORMAT       "/%u/%s/prodtst/report_reply"
#define MQTT_TOPIC_PRODTST_TEST_FORMAT               "/%u/%s/prodtst/test"
#define MQTT_TOPIC_PRODTST_TEST_REPLY_FORMAT         "/%u/%s/prodtst/test_reply"
#define MQTT_TOPIC_PRODTST_RESULT_FORMAT             "/%u/%s/prodtst/result"
#define MQTT_TOPIC_PRODTST_RESULT_REPLY_FORMAT       "/%u/%s/prodtst/result_reply"
#define MQTT_TOPIC_PRODTST_RESULT_REPORT_FORMAT      "/%u/%s/prodtst/result_report"
#define MQTT_TOPIC_PRODTST_RESULT_REPORT_REPLY_FORMAT "/%u/%s/prodtst/result_report_reply"
#define MQTT_TOPIC_PRODTST_SPCL_DOWN_FORMAT          "/%u/%s/prodtst/spcl_down"
#define MQTT_TOPIC_PRODTST_SPCL_DOWN_REPLY_FORMAT    "/%u/%s/prodtst/spcl_down_reply"
#define MQTT_TOPIC_PRODTST_SPCL_UP_FORMAT            "/%u/%s/prodtst/spcl_up"
#define MQTT_TOPIC_PRODTST_SPCL_UP_REPLY_FORMAT      "/%u/%s/prodtst/spcl_up_reply"

#endif


#ifdef RAW_SUPPORT

#define MQTT_TOPIC_UPRAW_FORMAT             "/%u/%s/up_raw"
#define MQTT_TOPIC_UPRAW_REPLY_FORMAT       "/%u/%s/up_raw_reply"
#define MQTT_TOPIC_DOWNRAW_FORMAT           "/%u/%s/down_raw"
#define MQTT_TOPIC_DOWNRAW_REPLY_FORMAT     "/%u/%s/down_raw_reply"

#endif

#ifdef STANDARD_SUPPORT

#define MQTT_TOPIC_REPORT_FORMAT            "/%u/%s/report"
#define MQTT_TOPIC_REPORT_REPLY_FORMAT      "/%u/%s/report_reply"
#define MQTT_TOPIC_SET_FORMAT               "/%u/%s/set"
#define MQTT_TOPIC_SET_REPLY_FORMAT         "/%u/%s/set_reply"



#endif


#define MQTT_TOPIC_BIND_TOKEN_FORMAT         "/%u/%s/event/bind_token"
#define MQTT_TOPIC_BIND_TOKEN_REPLY_FORMAT   "/%u/%s/event/bind_token_reply"

#define MQTT_TOPIC_EVENT_NOTIFY_FORMAT          "/%u/%s/event/notify"
#define MQTT_TOPIC_EVENT_NOTIFY_REPLY_FORMAT   "/%u/%s/event/notify_reply"


#define MQTT_TOPIC_RESET_FORMAT                 "/%u/%s/reset"
#define MQTT_TOPIC_RESET_REPLY_FORMAT          "/%u/%s/reset_reply"

#ifdef PRODTST_SUPPORT
static void mqtt_client_prodtst_set(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_report_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_config(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_test_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_get(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_result(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_result_report_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_spcl_down(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_prodtst_spcl_up_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
#endif


#ifdef RAW_SUPPORT
static void mqtt_client_down_raw(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_up_raw_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
#endif


#ifdef STANDARD_SUPPORT
static void mqtt_client_set(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
static void mqtt_client_report_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);
#endif

static void mqtt_client_token_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);

static void mqtt_client_event_notify(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);

static void mqtt_client_reset_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg);

typedef struct {
    const char *uri_name;
    void *callback;
    char topic[MAX_MQTT_TOPIC_LEN];
} mqtt_client_uri_map_t;


static mqtt_client_uri_map_t g_mqtt_client_uri_map[] = {

#ifdef RAW_SUPPORT

    {MQTT_TOPIC_DOWNRAW_FORMAT, mqtt_client_down_raw, {0} },
    
    {MQTT_TOPIC_UPRAW_REPLY_FORMAT, mqtt_client_up_raw_reply, {0} },

#endif

#ifdef STANDARD_SUPPORT

    {MQTT_TOPIC_REPORT_REPLY_FORMAT, mqtt_client_report_reply, {0} },    
    
    {MQTT_TOPIC_SET_FORMAT ,mqtt_client_set, {0} },
    
#endif

#ifdef PRODTST_SUPPORT

    {MQTT_TOPIC_PRODTST_CONFIG_FORMAT, mqtt_client_prodtst_config, {0} },    
    
    {MQTT_TOPIC_PRODTST_SET_FORMAT ,mqtt_client_prodtst_set, {0} },

    {MQTT_TOPIC_PRODTST_GET_FORMAT, mqtt_client_prodtst_get, {0} },    
    
    {MQTT_TOPIC_PRODTST_REPORT_REPLY_FORMAT ,mqtt_client_prodtst_report_reply, {0} },

    {MQTT_TOPIC_PRODTST_TEST_REPLY_FORMAT ,mqtt_client_prodtst_test_reply, {0} },

    {MQTT_TOPIC_PRODTST_RESULT_FORMAT ,mqtt_client_prodtst_result, {0} },

    {MQTT_TOPIC_PRODTST_RESULT_REPORT_REPLY_FORMAT ,mqtt_client_prodtst_result_report_reply, {0} },

    {MQTT_TOPIC_PRODTST_SPCL_DOWN_FORMAT ,mqtt_client_prodtst_spcl_down, {0} },
    
    {MQTT_TOPIC_PRODTST_SPCL_UP_REPLY_FORMAT ,mqtt_client_prodtst_spcl_up_reply, {0} },

#endif

    {MQTT_TOPIC_BIND_TOKEN_REPLY_FORMAT, mqtt_client_token_reply, {0} },

    {MQTT_TOPIC_EVENT_NOTIFY_FORMAT, mqtt_client_event_notify, {0} },
    
    {MQTT_TOPIC_RESET_REPLY_FORMAT, mqtt_client_reset_reply, {0} },
    
};


void event_handle(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    uintptr_t packet_id = (uintptr_t)msg->msg;
    iot_mqtt_topic_info_pt topic_info = (iot_mqtt_topic_info_pt)msg->msg;

    switch (msg->event_type) {
        case IOT_MQTT_EVENT_UNDEF:
            log_debug("undefined event occur.");
            break;

        case IOT_MQTT_EVENT_DISCONNECT:
            log_debug("MQTT disconnect.");

            mqtt_cloud_context->is_login = 0;

            iot_event_post(EVENT_VALUE_LOGOUT);

            break;

        case IOT_MQTT_EVENT_RECONNECT:
            log_debug("MQTT reconnect.");

            mqtt_cloud_context->is_login = 1;

            iot_event_post(EVENT_VALUE_LOGIN);

            break;

        case IOT_MQTT_EVENT_SUBCRIBE_SUCCESS:
            log_debug("subscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case IOT_MQTT_EVENT_SUBCRIBE_TIMEOUT:
            log_debug("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case IOT_MQTT_EVENT_SUBCRIBE_NACK:
            log_debug("subscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case IOT_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
            log_debug("unsubscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case IOT_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
            log_debug("unsubscribe timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case IOT_MQTT_EVENT_UNSUBCRIBE_NACK:
            log_debug("unsubscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case IOT_MQTT_EVENT_PUBLISH_SUCCESS:
            log_debug("publish success, packet-id=%u", (unsigned int)packet_id);
#if 0
            push_ack_info_remove(mqtt_cloud_context, (uint16_t)packet_id, &push_info);
            if(push_info)
            {
              if(mqtt_cloud_context->cbs.m_respcb){
                mqtt_cloud_context->cbs.m_respcb(push_info->packet_id, NO_ERR);
               }

              hal_free(push_info);
            }
#endif
            break;

        case IOT_MQTT_EVENT_PUBLISH_TIMEOUT:
            log_debug("publish timeout, packet-id=%u", (unsigned int)packet_id);
#if 0

            push_ack_info_remove(mqtt_cloud_context, (uint16_t)packet_id, &push_info);
            if(push_info)
            {
                if(mqtt_cloud_context->cbs.m_respcb){
                    mqtt_cloud_context->cbs.m_respcb(push_info->packet_id, PACKET_TIME_OUT);
                }
                hal_free(push_info);
            }
#endif
            break;

        case IOT_MQTT_EVENT_PUBLISH_NACK:
            log_debug("publish nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case IOT_MQTT_EVENT_PUBLISH_RECVEIVED:
            log_debug("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s",
                          topic_info->topic_len,
                          topic_info->ptopic,
                          topic_info->payload_len,
                          topic_info->payload);
            break;

        case IOT_MQTT_EVENT_BUFFER_OVERFLOW:
            log_debug("buffer overflow, %s", (char*)msg->msg);
            break;

        default:
            log_debug("Should NOT arrive here.");
            break;
    }
}


static void mqtt_client_event_callback(iot_event_t event, char *payload, int payload_len)
{
    int length = 0;
    char *val = NULL;
    void *callback = NULL;
    int id = 0;
    int devid = 0;
    int code = 0;
    char *params = NULL;
    uint8_t *raw_data = NULL;
    uint32_t raw_len = 0;
    char *id_str = NULL;
    char *method = NULL;
    int event_value = 0;
    char id_tmp[16] = {0};
    
    if(payload == NULL || payload_len == 0){
        goto exit;
    }

    if(payload_len >= MQTT_MSGLEN){
        goto exit;
    }
    *(payload + payload_len) = '\0';
    
    log_debug("Payload: %.*s",payload_len, payload);

    val = qm_json_get_value_by_name(payload, payload_len, "devid", &length, NULL);
    if (NULL == val) {
        goto exit;
    }
    
    devid = atoi(val);
    
    log_debug("devid: %d",devid);

    switch(event){
        
#ifdef STANDARD_SUPPORT   

    case IOT_EVENT_REPORT_REPLY:

        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        id = atoi(id_str);

        log_debug("id: %d",id);
        
        val = qm_json_get_value_by_name(payload, payload_len, "code", &length, NULL);
        if (NULL == val) {
            goto exit;
        }

        code = atoi(val);

        log_debug("code: %d",code);
        
        callback = iot_event_callback(IOT_EVENT_REPORT_REPLY);
        if (callback) {
            ((int (*)(const int, const int, const int, const char *, const int))callback)(devid, id, code, NULL, 0);
        }

    break;

    case IOT_EVENT_PROPERTY_SET:

        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        memcpy(id_tmp, id_str, length);
        
        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if (NULL == val || length == 0) {
            goto exit;
        }
        
        params = (char*)hal_malloc(length+1);
        if(!params){
            goto exit;
        }
        memset(params, 0, length+1);
        memcpy(params, val, length);
        
        log_debug("params: %s", params);

        cloud_generic_send(devid, MSG_TYPE_SET_REPLY, id_tmp, NULL, 200, NULL, 0);

        callback = iot_event_callback(IOT_EVENT_PROPERTY_SET);
        if (callback) {
            ((int (*)(const int, const char *, const int))callback)(devid, params, length);
        }

        hal_free(params);
        
    break;
    
#endif

    case IOT_EVENT_NOTIFY:
       
        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        memcpy(id_tmp, id_str, length);

        method = qm_json_get_value_by_name(payload, payload_len, "method", &length, NULL);
        if (NULL == method) {
            goto exit;
        }

        if(0 == memcmp(method, "bind", length)){
            cloud_event_reply(devid, id_tmp, "bind_reply", 200, NULL, 0);
            event_value = EVENT_VALUE_BIND_NOTIFY;
        }else if(0 == memcmp(method, "unbind", length)){
            cloud_event_reply(devid, id_tmp, "unbind_reply", 200, NULL, 0);
            event_value = EVENT_VALUE_UNBIND_NOTIFY;
        }else{
            goto exit;
        }

        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if(val && length){
            params = (char*)hal_malloc(length+1);
            if(!params){
                goto exit;
            }
            memset(params, 0, length+1);
            memcpy(params, val, length);
            
            log_debug("params: %s", params);
        }

        callback = iot_event_callback(IOT_EVENT_NOTIFY);
        if (callback) {
            if(val && length){
                ((int (*)(const int, const int, const char*, const int))callback)(devid, event_value, params, length);
                hal_free(params);
            }else{
                ((int (*)(const int, const int, const char*, const int))callback)(devid, event_value, NULL, 0);
            }        
        }
       

    break;
    
#ifdef RAW_SUPPORT

    case IOT_EVENT_RAWDATA_ARRIVED:

        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        memcpy(id_tmp, id_str, length);

        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if (NULL == val || length == 0) {
            goto exit;
        }

        cloud_generic_send(devid, MSG_TYPE_RAW_DATA_REPLY, id_tmp, NULL, 200, NULL, 0);

        raw_data = (uint8_t*)hal_malloc(length);
        if(!raw_data){
            goto exit;
        }
    
        qm_utils_base64decode((const uint8_t*)val, length, length, raw_data, &raw_len); 

        callback = iot_event_callback(IOT_EVENT_RAWDATA_ARRIVED);
        if (callback) {
            ((int (*)(const int, const unsigned char *, const int))callback)(devid, raw_data, raw_len);
        }

        hal_free(raw_data);
        
    break;

    case IOT_EVENT_RAWDATA_REPLY:

        val = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == val) {
            goto exit;
        }
        
        id = atoi(val);

        log_debug("id: %d",id);
        
        val = qm_json_get_value_by_name(payload, payload_len, "code", &length, NULL);
        if (NULL == val) {
            goto exit;
        }

        code = atoi(val);

        log_debug("code: %d",code);
        
        callback = iot_event_callback(IOT_EVENT_RAWDATA_REPLY);
        if (callback) {
            ((int (*)(const int, const int, const int, const char *, const int))callback)(devid, id, code, NULL, 0);
        }

    break;

#endif

#ifdef PRODTST_SUPPORT  
    case IOT_EVENT_PRODTST_SET:


        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        id = atoi(id_str);
        log_debug("id: %d",id);
        
        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if (NULL == val || length == 0) {
            goto exit;
        }
        
        params = (char*)hal_malloc(length+1);
        if(!params){
            goto exit;
        }
        memset(params, 0, length+1);
        memcpy(params, val, length);
        
        log_debug("params: %s", params);

        callback = iot_event_callback(IOT_EVENT_PRODTST_SET);
        if (callback) {
            ((int (*)(const int, const int, const char *, const int))callback)(devid, id, params, length);
        }

        hal_free(params);
        
    break;

    case IOT_EVENT_PRODTST_CONFIG:
    
        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        memcpy(id_tmp, id_str, length);
        
        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if (NULL == val || length == 0) {
            goto exit;
        }
        
        params = (char*)hal_malloc(length+1);
        if(!params){
            goto exit;
        }
        memset(params, 0, length+1);
        memcpy(params, val, length);
        
        log_debug("params: %s", params);

        cloud_generic_send(devid, MSG_TYPE_PRODTST_CONFIG_REPLY, id_tmp, NULL, 200, NULL, 0);

        callback = iot_event_callback(IOT_EVENT_PRODTST_CONFIG);
        if (callback) {
            ((int (*)(const int, const char *, const int))callback)(devid, params, length);
        }

        hal_free(params);

    break;

    case IOT_EVENT_PRODTST_GET:

        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        id = atoi(id_str);
        
        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if (NULL == val || length == 0) {
            goto exit;
        }
        
        params = (char*)hal_malloc(length+1);
        if(!params){
            goto exit;
        }
        memset(params, 0, length+1);
        memcpy(params, val, length);
        
        log_debug("params: %s", params);

        callback = iot_event_callback(IOT_EVENT_PRODTST_GET);
        if (callback) {
            ((int (*)(const int, const int, const char *, const int))callback)(devid, id, params, length);
        }

        hal_free(params);
        
    break;

    case IOT_EVENT_PRODTST_TEST_REPLY:
    case IOT_EVENT_PRODTST_REPORT_REPLY:
    case IOT_EVENT_PRODTST_SPCL_UP_REPLY:
    case IOT_EVENT_PRODTST_RESULT_REPORT_REPLY:

        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        id = atoi(id_str);

        log_debug("id: %d",id);
        
        val = qm_json_get_value_by_name(payload, payload_len, "code", &length, NULL);
        if (NULL == val) {
            goto exit;
        }

        code = atoi(val);

        log_debug("code: %d",code);
        
        callback = iot_event_callback(event);
        if (callback) {
            ((int (*)(const int, const int, const int, const char *, const int))callback)(devid, id, code, NULL, 0);
        }

    break;

     case IOT_EVENT_PRODTST_RESULT:

        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        id = atoi(id_str);
        log_debug("id: %d",id);
        
        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if (NULL == val || length == 0) {
            goto exit;
        }
        
        params = (char*)hal_malloc(length+1);
        if(!params){
            goto exit;
        }
        
        memset(params, 0, length+1);
        memcpy(params, val, length);
        
        log_debug("params: %s", params);

        callback = iot_event_callback(IOT_EVENT_PRODTST_RESULT);
        if (callback) {
            ((int (*)(const int, const int, const char *, const int))callback)(devid, id, params, length);
        }

        hal_free(params);
        
    break;

    case IOT_EVENT_PRODTST_SPCL_DOWN:
    
        id_str = qm_json_get_value_by_name(payload, payload_len, "id", &length, NULL);
        if (NULL == id_str) {
            goto exit;
        }

        id = atoi(id_str);
        log_debug("id: %d",id);
        
        val = qm_json_get_value_by_name(payload, payload_len, "params", &length, NULL);
        if (NULL == val || length == 0) {
            goto exit;
        }
        
        params = (char*)hal_malloc(length+1);
        if(!params){
            goto exit;
        }
        memset(params, 0, length+1);
        memcpy(params, val, length);
        
        log_debug("params: %s", params);

        callback = iot_event_callback(IOT_EVENT_PRODTST_SPCL_DOWN);
        if (callback) {
            ((int (*)(const int, const int, const char *, const int))callback)(devid, id, params, length);
        }

        hal_free(params);
        
    break; 

#endif

    case IOT_EVENT_TOKEN_REPLY:
        
        val = qm_json_get_value_by_name(payload, payload_len, "code", &length, NULL);
        if (NULL == val) {
            goto exit;
        }

        code = atoi(val);

        log_debug("code: %d",code);
        if(code == 200){
            iot_event_post(EVENT_VALUE_TOKEN_REPORT_SUC);
        }
    
    break;

    case IOT_EVENT_RESET_REPLY:
    
        val = qm_json_get_value_by_name(payload, payload_len, "code", &length, NULL);
        if (NULL == val) {
            goto exit;
        }

        code = atoi(val);

        log_debug("code: %d",code);  
        if(code == 200){
            iot_event_post(EVENT_VALUE_RESET);
        }

    break;
    
    default:
        break;
   }


exit:
     return;
}

static void mqtt_client_token_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_TOKEN_REPLY, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_event_notify(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_NOTIFY, ptopic_info->payload, ptopic_info->payload_len);
}


static void mqtt_client_reset_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{  
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_RESET_REPLY, ptopic_info->payload, ptopic_info->payload_len);
}


#ifdef PRODTST_SUPPORT
static void mqtt_client_prodtst_set(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_SET, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_report_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_REPORT_REPLY, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_result_report_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_RESULT_REPORT_REPLY, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_config(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_CONFIG, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_test_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_TEST_REPLY, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_get(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_GET, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_result(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_RESULT, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_spcl_down(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_SPCL_DOWN, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_prodtst_spcl_up_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    mqtt_client_event_callback(IOT_EVENT_PRODTST_SPCL_UP_REPLY, ptopic_info->payload, ptopic_info->payload_len);
}


#endif


#ifdef STANDARD_SUPPORT

static void mqtt_client_set(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    void *callback = NULL;

    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;

    mqtt_client_event_callback(IOT_EVENT_PROPERTY_SET, ptopic_info->payload, ptopic_info->payload_len);
}

static void mqtt_client_report_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    void *callback = NULL;

    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;

    mqtt_client_event_callback(IOT_EVENT_REPORT_REPLY, ptopic_info->payload, ptopic_info->payload_len);
}


#endif

#ifdef RAW_SUPPORT

static void mqtt_client_down_raw(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    void *callback = NULL;

    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;
    
    mqtt_client_event_callback(IOT_EVENT_RAWDATA_ARRIVED, ptopic_info->payload, ptopic_info->payload_len);
}


static void mqtt_client_up_raw_reply(void *pcontext, void *pclient, iot_mqtt_event_msg_pt msg)
{
    void *callback = NULL;

    iot_mqtt_topic_info_pt ptopic_info = (iot_mqtt_topic_info_pt) msg->msg;

    mqtt_client_event_callback(IOT_EVENT_RAWDATA_REPLY, ptopic_info->payload, ptopic_info->payload_len);

}


#endif


int iot_ota_reportversion(const char *version, iot_ota_type_t ota_type)
{
    if(h_ota == NULL){
        return NO_INIT_ERR;
    }

    if (0 != iot_ota_generic_reportversion(h_ota, version, ota_type)) {
       log_err("report OTA version failed");
    }

    return NO_ERR;
}


int mqtt_client(void *arg)
{
    int rc;
    iot_conn_info_pt pconn_info = {0};
    iot_mqtt_param_t mqtt_params = {0};
    char *msg_buf = NULL, *msg_readbuf = NULL;
    char mqtt_topic[MAX_MQTT_TOPIC_LEN] = {0};
    int topic_num = 0;
    int i = 0;

    mqtt_cloud_context = (cloud_context_t*)arg;

    if (NULL == (msg_buf = (char *)hal_malloc(MQTT_SEND_MSGLEN))) {
        log_debug("not enough memory");
        rc = -1;
        goto do_exit;
    }

    if (NULL == (msg_readbuf = (char *)hal_malloc(MQTT_MSGLEN))) {
        log_debug("not enough memory");
        rc = -1;
        goto do_exit;
    }

    /* Device AUTH */
    if (0 != iot_setup_conn_info(mqtt_cloud_context->product_info.product_id,
                               mqtt_cloud_context->product_info.device_id,
                               mqtt_cloud_context->product_info.device_secret,
                               (void **)&pconn_info))
    {
        log_debug("AUTH request failed!");
        rc = -1;
        goto do_exit;
    }
    

    /* Initialize MQTT parameter */
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    mqtt_params.port = pconn_info->port;
    mqtt_params.host = pconn_info->host_name;
    mqtt_params.client_id = pconn_info->client_id;
    mqtt_params.username = pconn_info->username;
    mqtt_params.password = pconn_info->password;
    mqtt_params.pub_key = pconn_info->pub_key;

    mqtt_params.request_timeout_ms = 3000;
    mqtt_params.clean_session = 1;
    mqtt_params.keepalive_interval_ms = 60000;
    mqtt_params.pread_buf = msg_readbuf;
    mqtt_params.read_buf_size = MQTT_MSGLEN;
    mqtt_params.pwrite_buf = msg_buf;
    mqtt_params.write_buf_size = MQTT_SEND_MSGLEN;

    mqtt_params.handle_event.h_fp = event_handle;
    mqtt_params.handle_event.pcontext = NULL;


    /* Construct a MQTT client with specify parameter */
    pclient = iot_mqtt_construct(&mqtt_params);
    if (NULL == pclient) {
        log_debug("MQTT construct failed");
        rc = -1;
        goto do_exit;
    }

    mqtt_cloud_context->pclient = pclient;

#if CONFIG_OTA_SUPPORT
    h_ota = iot_ota_init(mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id, pclient);
    if (NULL == h_ota) {
       rc = -1;
       log_err("initialize OTA failed");
       goto do_exit;
    }

    if (0 != iot_ota_generic_reportversion(h_ota, app_version_get(), IOT_OTA_TYPE_DEVICE)) {
       rc = -1;
       log_err("report OTA version failed");
       goto do_exit;
    }
#ifdef MCU_OTA_SUPPORT 

    if (0 != iot_ota_generic_reportversion(h_ota, mcu_version_get(), IOT_OTA_TYPE_MCU)) {
       rc = -1;
       log_err("report OTA mcu version failed");
       goto do_exit;
    }
#endif

#endif

    topic_num = sizeof(g_mqtt_client_uri_map)/sizeof(mqtt_client_uri_map_t);

    log_debug("topic num: %d", topic_num);

    for(i = 0; i<topic_num; i++){
        
        memset(g_mqtt_client_uri_map[i].topic, 0, MAX_MQTT_TOPIC_LEN);

        hal_snprintf(g_mqtt_client_uri_map[i].topic, MAX_MQTT_TOPIC_LEN, g_mqtt_client_uri_map[i].uri_name, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
        log_debug("topic: %s", g_mqtt_client_uri_map[i].topic);

        rc = iot_mqtt_auto_subscribe(pclient, g_mqtt_client_uri_map[i].topic, (iot_mqtt_event_handle_func_fpt)g_mqtt_client_uri_map[i].callback, NULL);
        if (rc < 0) {
            iot_mqtt_destroy(&pclient);
            log_debug("IOT_MQTT_Subscribe() failed, rc = %d", rc);
            rc = -1;
            goto do_exit;
        }
    }     

    mqtt_cloud_context->is_login = 1;

    iot_event_post(EVENT_VALUE_LOGIN);

    /* Initialize topic information */
    memset(&topic_msg, 0x0, sizeof(iot_mqtt_topic_info_t));

    topic_msg.retain = 0;
    topic_msg.dup = 0;

    return rc;

do_exit:
    if (NULL != msg_buf) {
        hal_free(msg_buf);
    }

    if (NULL != msg_readbuf) {
        hal_free(msg_readbuf);
    }

#if CONFIG_OTA_SUPPORT

    if (NULL != h_ota) {
        iot_ota_deinit(h_ota);
    }
#endif

    if (NULL != pclient) {
        iot_mqtt_destroy(&pclient);
    }

    return rc;
}


/*qm_list_node_t + packet_info_t + payload*/

int mqtt_loop(void *pclient)
{
    int rc = 0;  
    int need_send = 0;
    char mqtt_topic[MAX_MQTT_TOPIC_LEN] = {0};
    qm_list_node_t *node = NULL;
    packet_info_t *packet_info = NULL;

    if(!mqtt_cloud_context->is_login){
        return NO_ERR;
    }
      
    node = qm_list_lpop(mqtt_cloud_context->push_list);
    if(!node){
        return NO_ERR;
    }

    packet_info = (packet_info_t*)node->val;

    if(packet_info == NULL){
        return PARAM_ERR;
    }

    memset(mqtt_topic, 0, sizeof(mqtt_topic));

    switch(packet_info->msg_type)
    {
#ifdef STANDARD_SUPPORT          

    case MSG_TYPE_REPORT:
        
        hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_REPORT_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
        need_send = 1;

        break;
            
        case MSG_TYPE_SET_REPLY:
            
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_SET_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;

        break;
#endif

#ifdef RAW_SUPPORT
        case MSG_TYPE_RAW_DATA_REPORT:

            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_UPRAW_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
            
        break;
        
        case MSG_TYPE_RAW_DATA_REPLY:

            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_DOWNRAW_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
            
        break;
         
#endif

#ifdef PRODTST_SUPPORT

        case MSG_TYPE_PRODTST_REPORT:

            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_REPORT_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
                     
        break;

        case MSG_TYPE_PRODTST_CONFIG_REPLY:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_CONFIG_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
           
        break;

        case MSG_TYPE_PRODTST_TEST:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_TEST_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
                       
        break;
        
        case MSG_TYPE_PRODTST_GET_REPLY:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_GET_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;

        break;

        case MSG_TYPE_PRODTST_SET_REPLY:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_SET_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
        
        break;

        case MSG_TYPE_PRODTST_RESULT_REPLY:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_RESULT_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;

        break;

        case MSG_TYPE_PRODTST_SPCL_DOWN_REPLY:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_SPCL_DOWN_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1; 

        break;

        case MSG_TYPE_PRODTST_SPCL_UP:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_SPCL_UP_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1; 

        break;

        case MSG_TYPE_PRODTST_RESULT_REPORT:
            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_PRODTST_RESULT_REPORT_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1; 

        break;

#endif

        case MSG_TYPE_TOKEN_REPORT:

            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_BIND_TOKEN_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
        
        break;

        case MSG_TYPE_EVENT_NOTIFY_REPLY:

            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_EVENT_NOTIFY_REPLY_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;
        
        break;

        case MSG_TYPE_RESET:

            hal_snprintf(mqtt_topic, MAX_MQTT_TOPIC_LEN, MQTT_TOPIC_RESET_FORMAT, mqtt_cloud_context->product_info.product_id, mqtt_cloud_context->product_info.device_id);
            need_send = 1;

        break;
    
        default:
        break;
      }


    if(!need_send){
        return NO_ERR;
    }

    topic_msg.ptopic = mqtt_topic;
    topic_msg.payload = (char*)packet_info->data;
    topic_msg.payload_len = packet_info->len;

    topic_msg.qos = IOT_MQTT_QOS0;
    topic_msg.packet_id = packet_info->packet_id;

    rc = iot_mqtt_publish(pclient, mqtt_topic, &topic_msg);
    if (rc < 0) {
        log_debug("error occur when publish");
        hal_free(node);
        return NO_ERR;
    }

    hal_free(node);
        
    log_debug("packet-id=%u, len=%d, publish topic=%s, %.*s", topic_msg.packet_id, topic_msg.payload_len, topic_msg.ptopic, topic_msg.payload_len, topic_msg.payload);
     
    return NO_ERR;
}

void cloud_loop(void)
{

    mqtt_loop(pclient);

#if CONFIG_OTA_SUPPORT
    ota_loop(&h_ota,mqtt_cloud_context);
#endif
    /* handle the MQTT packet received from TCP or SSL connection */
    iot_mqtt_yield(pclient, mqtt_cloud_context->loop_timeout);
    
}

