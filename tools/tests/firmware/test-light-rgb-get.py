#!/usr/bin/env python3
"""Host regression of the real synchronous cloud GET and shadow queue C paths.

Compiles unmodified function bodies from SDK/application sources, the complete
property codec, real cJSON, real outer JSON parser and real list implementation.
Only MQTT, timers, mutex/device state and unrelated pub message paths are host
adapters. No board, credentials, platform connection or firmware build is used.
"""

import argparse
from pathlib import Path
import re
import runpy
import shlex
import shutil
import subprocess
import tempfile


def function_source(source, name):
    match = re.search(r"(?m)^(?:static\s+)?(?:const\s+)?\w+\s*\*?\s*" + re.escape(name)
                      + r"\s*\([^;]*?\)\s*\{", source)
    if match is None:
        # C permits the pointer star adjacent to a function name.
        match = re.search(r"(?m)^(?:static\s+)?[\w]+\s+\*" + re.escape(name)
                          + r"\s*\([^;]*?\)\s*\{", source)
    if match is None:
        raise ValueError(f"C function not found: {name}")
    index, depth, state = match.end() - 1, 0, "code"
    while index < len(source):
        char, pair = source[index], source[index:index + 2]
        if state == "line":
            if char == "\n":
                state = "code"
        elif state == "comment":
            if pair == "*/":
                state = "code"
                index += 1
        elif state in ('"', "'"):
            if char == "\\":
                index += 1
            elif char == state:
                state = "code"
        elif pair == "//":
            state = "line"
            index += 1
        elif pair == "/*":
            state = "comment"
            index += 1
        elif char in ('"', "'"):
            state = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():index + 1]
        index += 1
    raise ValueError(f"Unclosed C function: {name}")


PREAMBLE = r"""
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "qm.h"
#include "qm_spec_core.h"
#include "qm_iot_pub.h"
#include "qm_utils_list.h"
#include "json_parser.h"
#include "cJSON.h"
#include "iot_control_light_rgb.h"
#include "iot_control_focus.h"
#define CONFIG_QM_IOT_SPEC_SUPPORT 1
#define CONFIG_QM_IOT_PUB_SUPPORT 1
#define QM_CLOUD_LOGE(...) ((void)0)
#define QM_CLOUD_LOGD(...) ((void)0)
#define QM_IOT_COMMON_ID_KEY "id"
#define QM_IOT_COMMON_PARAMS_KEY "params"
#define QM_IOT_COMMON_PARAMS_GET_KEY "get"
#define QM_IOT_COMMON_DID_MAX_LEN 32
#define QM_IOT_COMMON_MSG_ID_MAX_NUM 65535
#define QM_IOT_COMMON_TOPIC_MAX_LEN 128
#define QM_MQTT_QoS1 1
#define qm_snprintf snprintf
#define SPEC_JSON_KEY_DID "did"
#define SPEC_JSON_KEY_SIID "siid"
#define SPEC_JSON_KEY_PIID "piid"
#define SPEC_JSON_KEY_CODE "code"
#define SPEC_JSON_KEY_VALUE "value"
typedef struct { int scheduled; } qm_work_t;
typedef enum { QM_IOT_MQTTRECV_PUB } qm_iot_mqtt_recv_type_t;
typedef struct {
    qm_iot_mqtt_recv_type_t type;
    union { struct { uint8_t *payload; unsigned payload_len; } pub; } data;
} qm_iot_mqtt_recv_t;
typedef enum {
    QM_IOT_RECV_TYPE_PROPERTY_SET, QM_IOT_RECV_TYPE_PROPERTY_GET_RSP,
    QM_IOT_RECV_TYPE_PROPERTY_GET_REQ
} qm_iot_recv_type_t;
typedef struct {
    qm_iot_recv_type_t type;
    uint32_t did; uint8_t siid; uint16_t piid;
    union {
        struct { qm_spec_property_t *property; } set, get_rsp;
        struct { qm_spec_property_t *property; qm_spec_property_operation_t *operation; } get_req;
    } data;
} qm_iot_recv_t;
typedef struct {
    void *pub_handle;
    void (*recv_handler)(void *, const qm_iot_recv_t *, void *);
    void *user_data;
} qm_iot_session_t;
typedef void (*ljyd_cloud_prop_cb_t)(qm_spec_property_t *);
typedef int (*ljyd_cloud_prop_get_cb_t)(const qm_spec_property_operation_t *, qm_spec_property_operation_t *);
static ljyd_cloud_prop_cb_t cloud_prop_cb;
static ljyd_cloud_prop_get_cb_t cloud_prop_get_cb;
static int qm_after=-1, json_after=-1, live, arm_qm=-1, arm_json=-1;
static int qm_attempts, json_attempts, counting, post_calls, post_failure;
static int snapshot_reads, snapshot_failure, pub_calls, mqtt_result=77;
static char payload_capture[8192], topic_capture[128];
static iot_control_light_rgb_state_t hardware_state={1,25,0x00ff00};

static void *tracked_alloc(size_t size, int *after, int *attempts)
{
    if (counting) ++*attempts;
    if (*after==0) { *after=-1; return NULL; }
    if (*after>0) --*after;
    void *result=malloc(size);
    if(result) ++live;
    return result;
}
void *rgb_host_malloc(size_t size) { return tracked_alloc(size,&qm_after,&qm_attempts); }
static void *json_malloc(size_t size) { return tracked_alloc(size,&json_after,&json_attempts); }
void rgb_host_free(void *value) { if(value) { --live; free(value); } }
int qm_mutex_lock(qm_mutex_t *mutex,int timeout) { (void)mutex;(void)timeout;return 0; }
int qm_mutex_unlock(qm_mutex_t *mutex) { (void)mutex;return 0; }
static uint32_t qm_iot_pid_get(void *handle) { assert(handle);return 11878; }
static uint32_t qm_iot_did_get(void *handle) { assert(handle);return 42; }
static int int_str_to_num(const char *string,int length)
{ char text[32];assert(length>=0 && length<32);memcpy(text,string,length);text[length]=0;return (int)strtol(text,NULL,10); }
static int qm_iot_mqtt_pub(void *handle,const char *topic,uint8_t *payload,size_t size,int qos)
{
    assert(handle && qos==1 && size<sizeof(payload_capture));
    snprintf(topic_capture,sizeof(topic_capture),"%s",topic);
    memcpy(payload_capture,payload,size);payload_capture[size]=0;++pub_calls;
    return mqtt_result;
}
int iot_control_light_rgb_get_state(iot_control_light_rgb_state_t *state)
{ ++snapshot_reads;if(snapshot_failure)return snapshot_failure;*state=hardware_state;return 0; }
int iot_control_focus_get_state(iot_control_focus_state_t *state)
{ (void)state;return -QM_EINIT; }
int iot_control_focus_pack_property(qm_spec_property_t *property,
                                   const iot_control_focus_state_t *state)
{ (void)property;(void)state;return -QM_ENOSYS; }
static unsigned qm_now_ms(void) { return 7; }
static void qm_srandom(unsigned value) { (void)value; }
static unsigned qm_random_get(unsigned maximum) { assert(maximum);return 1; }
static void qm_iot_pub_action(void *argument) { (void)argument;assert(!"timer execution outside this queue test"); }
static int qm_post_delayed_action(qm_work_t *work,void (*fn)(void *),void *arg,unsigned delay)
{ (void)fn;(void)arg;assert(delay>0);++post_calls;if(post_failure)return post_failure;work->scheduled=1;return 0; }
static int qm_iot_pub_msg_send(void *h,void *m) { (void)h;(void)m;assert(0);return -5; }
static int qm_iot_prop_pub_msg_send(void *h,void *m) { (void)h;(void)m;assert(0);return -5; }
static int qm_iot_prop_pub_msg_push(void *h,void *m) { (void)h;(void)m;assert(0);return -5; }
static int qm_iot_prop_pub_msg_get_req(void *h,void *m) { (void)h;(void)m;assert(0);return -5; }
static int32_t qm_iot_property_get_rsponse(void *,qm_spec_property_operation_t *);
"""


ADAPTERS = r"""
int qm_spec_data_unpack(qm_spec_data_type_t type,qm_spec_msg_type_t message,uint8_t *data,int length,qm_spec_data_info_t *info)
{ assert(type==QM_SPEC_DATA_TYPE_JSON && message==QM_SPEC_MSG_TYPE_CLOUD_GET);return qm_spec_json_cloud_get_unpack(data,length,info); }
int qm_spec_data_pack(qm_spec_data_type_t type,qm_spec_msg_type_t message,qm_spec_data_info_t *info)
{ assert(type==QM_SPEC_DATA_TYPE_JSON && message==QM_SPEC_MSG_TYPE_CLOUD_GET);return qm_spec_json_cloud_get_pack(info); }
int qm_spec_data_destroy(qm_spec_data_type_t type,qm_spec_msg_type_t message,qm_spec_data_info_t *info)
{ assert(type==QM_SPEC_DATA_TYPE_JSON && message==QM_SPEC_MSG_TYPE_CLOUD_GET);return qm_spec_json_cloud_get_destroy(info); }
static void receive_bridge(void *handle,qm_iot_pub_recv_t *received,void *userdata)
{
    (void)handle;qm_after=arm_qm;json_after=arm_json;qm_attempts=json_attempts=0;counting=1;
    qm_iot_pub_recv_comm_handler(userdata,received);
    counting=0;qm_after=json_after=-1;
}
static qm_iot_pub_handle_t pub;
static qm_iot_session_t session;
static void init_session(void)
{
    memset(&pub,0,sizeof(pub));memset(&session,0,sizeof(session));
    pub.mqtt_handle=&pub;pub.userdata=&session;pub.recv_handler=receive_bridge;pub.delay_pub=50;
    pub.pub_list=qm_list_new();assert(pub.pub_list);pub.pub_list->match=list_match;
    session.pub_handle=&pub;session.recv_handler=cloud_iot_recv_handler;
    cloud_prop_get_cb=iot_control_on_get;
}
static void clean_session(void)
{
    qm_list_node_t *node;
    while((node=qm_list_lpop(pub.pub_list))!=NULL) {
        qm_iot_pub_msg_handle_t *pending=node->val;
        qm_spec_property_operation_delete(pending->property_operation);qm_list_node_destroy(node);
    }
    qm_list_destroy(pub.pub_list);assert(live==0);
}
static void receive(const char *id,const char *properties)
{
    char input[4096];int count=snprintf(input,sizeof(input),"{\"id\":%s,\"params\":%s}",id,properties);
    assert(count>0 && count<(int)sizeof(input));
    qm_iot_mqtt_recv_t packet={.type=QM_IOT_MQTTRECV_PUB};
    packet.data.pub.payload=(uint8_t *)input;packet.data.pub.payload_len=(unsigned)count;
    qm_iot_get_recv_handler(&pub,&packet,&pub);
}
static const char *three="[{\"did\":\"42\",\"siid\":4,\"piid\":1},{\"did\":\"42\",\"siid\":4,\"piid\":2},{\"did\":\"42\",\"siid\":4,\"piid\":3}]";
static cJSON *assert_reply(double id,int count)
{
    assert(strcmp(topic_capture,"/11878/42/cloud/getReply")==0);
    cJSON *root=cJSON_Parse(payload_capture);assert(root);
    assert(cJSON_GetObjectItem(root,"id")->valuedouble==id);
    assert(strcmp(cJSON_GetObjectItem(root,"method")->valuestring,"get")==0);
    assert(cJSON_GetArraySize(cJSON_GetObjectItem(root,"params"))==count);
    return root;
}
static void normal_and_errors(void)
{
    int baseline=live;
    receive("401",three);assert(pub_calls==1 && snapshot_reads==1 && live==baseline);
    cJSON *root=assert_reply(401,3),*array=cJSON_GetObjectItem(root,"params");
    for(int i=0;i<3;++i) {
        cJSON *item=cJSON_GetArrayItem(array,i);assert(cJSON_GetObjectItem(item,"code")->valueint==0);
        cJSON *value=cJSON_GetObjectItem(item,"value");assert(value);
        if(i==0) assert(cJSON_IsTrue(value));
        else assert(cJSON_IsNumber(value) && value->valueint==(i==1?25:0x00ff00));
    }
    cJSON_Delete(root);
    receive("909",three);assert(pub_calls==2 && snapshot_reads==2);root=assert_reply(909,3);cJSON_Delete(root);
    receive("4294967295",three);assert(pub_calls==3);root=assert_reply(4294967295.0,3);cJSON_Delete(root);
    const char *invalid[]={"4294967296","-1","1.5","1e3","12345678901","null","true"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
        receive(invalid[i],three);
        if(pub_calls!=3)fprintf(stderr,"unexpected GET accepted: %s (%s)\n",invalid[i],payload_capture);
        assert(pub_calls==3);
    }
    assert(pub_calls==3);
    receive("0",three);assert(pub_calls==4);root=assert_reply(0,3);cJSON_Delete(root);
    receive("0",three);assert(pub_calls==4);
    receive("\"4294967294\"",three);assert(pub_calls==5);root=assert_reply(4294967294.0,3);cJSON_Delete(root);
    receive("910","[{\"did\":\"42\",\"siid\":4,\"piid\":2},{\"did\":\"42\",\"siid\":4,\"piid\":99},{\"did\":\"42\",\"siid\":3,\"piid\":1}]");
    root=assert_reply(910,3);array=cJSON_GetObjectItem(root,"params");
    for(int i=0;i<3;++i) { cJSON *item=cJSON_GetArrayItem(array,i);int code=cJSON_GetObjectItem(item,"code")->valueint;
        assert(i==0?code==0:code==-QM_ENOSYS);assert(i==0 || !cJSON_GetObjectItem(item,"value")); }
    cJSON_Delete(root);
    snapshot_failure=-QM_EIO;receive("911",three);snapshot_failure=0;
    root=assert_reply(911,3);array=cJSON_GetObjectItem(root,"params");
    for(int i=0;i<3;++i) { cJSON *item=cJSON_GetArrayItem(array,i);assert(cJSON_GetObjectItem(item,"code")->valueint==-QM_EIO);assert(!cJSON_GetObjectItem(item,"value")); }
    cJSON_Delete(root);
    cloud_prop_get_cb=NULL;receive("912",three);cloud_prop_get_cb=iot_control_on_get;
    root=assert_reply(912,3);array=cJSON_GetObjectItem(root,"params");
    for(int i=0;i<3;++i) { cJSON *item=cJSON_GetArrayItem(array,i);assert(cJSON_GetObjectItem(item,"code")->valueint==-QM_ENOSYS);assert(!cJSON_GetObjectItem(item,"value")); }
    cJSON_Delete(root);assert(live==baseline && !pub.cloud_get_reply_active);
    qm_spec_property_t no_value={0};char log[20];format_spec_value(&no_value,log,sizeof(log));assert(strcmp(log,"none")==0);
    puts("PASS GET wire: IDs, batch, types, unknown properties, unavailable state, no-value log");
}
static void wire_allocation_failures(void)
{
    int baseline=live;
    receive("1000",three);int qm_count=qm_attempts,json_count=json_attempts;
    assert(qm_count>=5 && json_count>20);
    for(int n=0;n<qm_count;++n) {
        char id[20];snprintf(id,sizeof(id),"%d",1100+n);int before=pub_calls;
        arm_qm=n;receive(id,three);arm_qm=-1;
        assert(pub_calls==before && live==baseline);
    }
    for(int n=0;n<json_count;++n) {
        char id[20];snprintf(id,sizeof(id),"%d",2100+n);int before=pub_calls;
        arm_json=n;receive(id,three);arm_json=-1;
        assert(pub_calls==before && live==baseline);
    }
    printf("PASS GET allocation failure: %d SDK + %d real cJSON pack allocations\n",qm_count,json_count);
}
static qm_spec_property_operation_t *make_properties(int second,int dynamic)
{
    qm_spec_property_operation_t *operation=qm_spec_property_operation_creat();assert(operation);
    qm_spec_property_t *property=qm_spec_property_creat();assert(property);
    property->did=42;property->siid=4;property->piid=second?2:1;
    assert(qm_spec_property_pack_number(property,second?55:1)==0);assert(qm_spec_property_add(operation,property)==0);
    if(dynamic) {
        property=qm_spec_property_creat();assert(property);property->did=42;property->siid=9;property->piid=second?2:1;
        char text[]="a dynamic value longer than inline";
        assert(qm_spec_property_pack_bytes(property,(uint8_t *)text,(int)strlen(text))==0);
        property->value.property_format=QM_SPEC_PROPERTY_FORMAT_STRING;assert(qm_spec_property_add(operation,property)==0);
    }
    return operation;
}
static void response_ownership(void)
{
    int baseline=live,before=pub_calls;
    qm_spec_property_operation_t *operation=make_properties(0,0);
    assert(qm_iot_property_get_rsponse(&session,operation)==-QM_EINVAL && live==baseline && pub_calls==before);
    operation=make_properties(0,0);assert(qm_iot_property_get_rsponse(NULL,operation)==-QM_EINVAL && live==baseline);
    operation=make_properties(0,0);session.pub_handle=NULL;
    assert(qm_iot_property_get_rsponse(&session,operation)==-QM_EINVAL && live==baseline);session.pub_handle=&pub;
    pub.cloud_get_reply_active=1;pub.cloud_get_reply_id=3000;mqtt_result=-QM_EIO;
    operation=make_properties(0,0);assert(qm_iot_property_get_rsponse(&session,operation)==-QM_EIO && live==baseline);
    pub.cloud_get_reply_active=0;mqtt_result=77;
    char *payload=NULL;payload_malloc_and_copy(&pub,"get",QM_IOT_PUB_ID_DEVICE_GET,(uint8_t *)"[]",2,&payload);
    assert(payload);cJSON *json=cJSON_Parse(payload);assert(json && cJSON_GetObjectItem(json,"id")->valueint==1);
    cJSON_Delete(json);qm_free(payload);assert(live==baseline);
    puts("PASS GET ownership: invalid/deferred response, missing session, MQTT failure, unrelated uplink IDs");
}
static void clear_queue(void)
{
    qm_list_node_t *node;
    while((node=qm_list_lpop(pub.pub_list))!=NULL) {
        qm_iot_pub_msg_handle_t *pending=node->val;
        qm_spec_property_operation_delete(pending->property_operation);qm_list_node_destroy(node);
    }
}
static int queue_input(qm_spec_property_operation_t *operation)
{
    qm_iot_pub_msg_t message={.did=42,.type=QM_IOT_PUBMSG_SHADOW_UPDATE};message.data.update.property_operation=operation;
    return qm_iot_pub_send(&pub,&message,0);
}
static void queue_faults(int existing)
{
    int baseline=live,allocation_count=0;
    for(int fault=-1;;++fault) {
        if(fault>=allocation_count)break;
        if(existing)assert(queue_input(make_properties(0,1))==0);
        qm_iot_pub_msg_handle_t *old=existing?pub.pub_list->head->val:NULL;
        qm_spec_property_operation_t *previous=old?old->property_operation:NULL;
        qm_spec_property_operation_t *input=make_properties(existing==2?0:1,1);
        int pending_live=live,before_posts=post_calls;
        qm_after=fault;qm_attempts=0;counting=1;
        int result=queue_input(input);counting=0;qm_after=-1;
        if(fault==-1) {
            allocation_count=qm_attempts;assert(result==0);
            assert(pub.pub_list->len==1);
            qm_iot_pub_msg_handle_t *pending=pub.pub_list->head->val;
            assert(pending->property_operation->element_num==(existing==1?4:2));
            assert(post_calls==before_posts+(existing?0:1));
        } else {
            if(result!=-QM_ENOMEM || live!=pending_live)fprintf(stderr,"queue fault existing=%d point=%d total=%d ret=%d live=%d expected=%d\n",existing,fault,allocation_count,result,live,pending_live);
            assert(result==-QM_ENOMEM && live==pending_live);
            assert(pub.pub_list->len==(existing?1:0) && post_calls==before_posts);
            assert(input->element_num==2);
            if(existing)assert(old->property_operation==previous && previous->element_num==2 && old->pub_work.scheduled);
            qm_spec_property_operation_delete(input);
        }
        clear_queue();assert(live==baseline);
    }
    printf("PASS SHADOW queue %s: %d allocation points, immutable input/pending/deadline\n",existing==2?"replacement":existing?"merge":"new",allocation_count);
}
static void queue_post_and_replace(void)
{
    int baseline=live,before=post_calls;
    qm_spec_property_operation_t *input=make_properties(0,1);
    post_failure=-QM_EIO;assert(queue_input(input)==-QM_EIO);post_failure=0;
    assert(pub.pub_list->len==0 && post_calls==before+1 && input->element_num==2);
    qm_spec_property_operation_delete(input);assert(live==baseline);
    assert(queue_input(make_properties(0,1))==0);before=post_calls;
    assert(queue_input(make_properties(0,1))==0);assert(post_calls==before);
    qm_iot_pub_msg_handle_t *pending=pub.pub_list->head->val;
    assert(pending->property_operation->element_num==2);
    clear_queue();assert(live==baseline);
    puts("PASS SHADOW queue: post failure rollback, same-property replacement, first deadline retained");
}
int main(void)
{
    cJSON_Hooks hooks={json_malloc,rgb_host_free};cJSON_InitHooks(&hooks);init_session();
    normal_and_errors();wire_allocation_failures();response_ownership();queue_faults(0);queue_faults(1);queue_faults(2);queue_post_and_replace();
    clean_session();puts("All real C GET/queue regression groups passed.");return 0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    sdk = root / "src/firmware"
    cloud = sdk / "qumios-dev/components/qm_cloud"
    spec = sdk / "qumios-dev/components/qm_spec"
    app = sdk / "apps/main"
    pub = (cloud / "pub/qm_iot_pub.c").read_text()
    api = (cloud / "api/qm_iot_api.c").read_text()
    ljyd = (app / "cloud/ljyd_cloud.c").read_text()
    core = (spec / "src/qm_spec_core.c").read_text()
    control = (app / "lj_components/iot_control/iot_control.c").read_text()
    rgb = (app / "lj_components/iot_control/ctrl/iot_control_light_rgb.c").read_text()
    header = runpy.run_path(str(root / "tools/tests/firmware/test-light-rgb-control.py"))["QM_HEADER"]
    # Reuse source-owned private pub types/macros, rather than a divergent mirror.
    pub_types = pub[pub.index("#define QM_IOT_PUB_WORK_RANDOM_MS"):pub.index("/**\n * @brief 从 pub 会话")]
    message_type = pub[pub.index("typedef struct\n{\n    uint32_t did;"):pub.index("#if CONFIG_QM_IOT_SPEC_SUPPORT\nstatic int msg_id_get")]
    sources = [PREAMBLE, pub_types, message_type]
    for name in ("pub_iot_handle", "list_match", "msg_id_get", "payload_malloc_and_copy"):
        sources.append(function_source(pub, name))
    for name in ("qm_spec_json_cloud_get_unpack", "qm_spec_json_cloud_get_pack", "qm_spec_json_cloud_get_destroy"):
        sources.append(function_source(core, name))
    sources.append(function_source(rgb, "iot_control_light_rgb_pack_property"))
    sources.append(function_source(control, "iot_control_on_get"))
    for name in ("qm_iot_pub_msg_creat", "shadow_candidate_merge", "shadow_pending_find", "qm_iot_pub_msg_push", "qm_iot_prop_pub_msg_get_rsp", "qm_iot_pub_send"):
        sources.append(function_source(pub, name))
    sources.append(function_source(api, "qm_iot_property_get_rsponse"))
    sources.append(function_source(api, "qm_iot_pub_recv_comm_handler"))
    for name in ("iot_recv_type_name", "iot_recv_property", "format_spec_value", "cloud_get_unsupported", "cloud_iot_get_request", "cloud_iot_recv_handler"):
        sources.append(function_source(ljyd, name))
    for name in ("cloud_get_id_parse", "qm_iot_get_recv_handler"):
        sources.append(function_source(pub, name))
    sources.append(ADAPTERS)
    with tempfile.TemporaryDirectory(prefix="ticko-rgb-get-") as temporary:
        tmp = Path(temporary)
        (tmp / "qm.h").write_text(header + "\n#define qm_snprintf snprintf\n")
        for name in ("qm_types.h", "qm_kernel.h", "qm_log.h"):
            (tmp / name).write_text('#include "qm.h"\n')
        for source in (spec / "include/qm_spec_api.h", spec / "include/qm_spec_core.h",
                       spec / "include/qm_spec_log.h", cloud / "pub/qm_iot_pub.h",
                       sdk / "qumios-dev/components/cjson/cJSON.h",
                       sdk / "qumios-dev/utility/json_parser/json_parser.h",
                       sdk / "qumios-dev/utility/list/qm_utils_list.h",
                       app / "lj_components/iot_control/ctrl/iot_control_light_rgb.h",
                       app / "lj_components/iot_control/ctrl/iot_control_focus.h"):
            shutil.copyfile(source, tmp / source.name)
        (tmp / "wire.c").write_text("\n\n".join(sources))
        command = shlex.split(args.cc) + ["-std=c11", "-g", "-O1", "-pthread", "-I", str(tmp),
            str(tmp / "wire.c"), str(spec / "src/qm_spec_api.c"),
            str(sdk / "qumios-dev/components/cjson/cJSON.c"),
            str(sdk / "qumios-dev/utility/json_parser/json_parser.c"),
            str(sdk / "qumios-dev/utility/list/qm_utils_list.c"), "-lm", "-o", str(tmp / "wire")]
        if args.sanitize:
            command[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(command, check=True)
        subprocess.run([str(tmp / "wire")], check=True)


if __name__ == "__main__":
    main()
