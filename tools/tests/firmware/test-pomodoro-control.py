#!/usr/bin/env python3
"""Run actual app wrapper, CLI, timer core and cJSON on a deterministic host OS.

Mock only OS timer/work queue, RGB driver boundary, network and reminder enqueue.
Checks protocol, durations, single-round completion, concurrency and rollback;
no real FreeRTOS scheduling, MQTT delivery, TTS, voice or physical LED claims.
"""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

CONFIG = r'''
#ifndef POMODORO_HOST_CONFIG_H
#define POMODORO_HOST_CONFIG_H
#define CONFIG_QM_WORK_SUPPORT 1
#define CONFIG_QM_WORKQUEUE_NO_TASK 0
#define CONFIG_QM_OS_SUPPORT 1
#define CONFIG_QM_OS_MUTEX_SUPPORT 1
#define CONFIG_QM_POMODORO_SUPPORT 1
#define CONFIG_QM_CLI_SUPPORT 1
#define CONFIG_QM_POMODORO_FOCUS_SEC 1500
#define CONFIG_QM_POMODORO_SHORT_BREAK_SEC 5
#define CONFIG_QM_POMODORO_LONG_BREAK_SEC 10
#define CONFIG_QM_POMODORO_LONG_BREAK_INTERVAL 2
#define CONFIG_QM_POMODORO_FOCUS_TARGET 1
#define CONFIG_QM_POMODORO_TICK_MS 1000
#define CONFIG_APP_POMODORO_AUTO_START 0
#endif
'''

STUB_HEADER = r'''
#ifndef POMODORO_HOST_STUB_H
#define POMODORO_HOST_STUB_H
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "qm_errno.h"
#include "qm_kernel.h"
#include "qm_work.h"
/* OS allocation substitute: SDK unsigned-int size is 32-bit on ESP32,
 * while the host cJSON hook requires size_t (64-bit on this Mac). */
#define qm_malloc malloc
#ifndef QM_ARRAY_SIZE
#define QM_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
#define QM_LOGI(...) host_log(__VA_ARGS__)
#define QM_LOGW(...) host_log(__VA_ARGS__)
#define QM_LOGE(...) host_log(__VA_ARGS__)
#define QM_LOGD(...) host_log(__VA_ARGS__)
void host_log(const char *tag, const char *format, ...);
typedef enum {
    QM_IOT_MQTTRECV_PUB, QM_IOT_MQTTRECV_SUB_ACK,
    QM_IOT_MQTTRECV_UNSUB_ACK, QM_IOT_MQTTRECV_PUB_ACK
} qm_iot_mqtt_recv_type_t;
typedef struct {
    qm_iot_mqtt_recv_type_t type;
    union {
        struct {
            uint8_t qos;
            char *topic;
            uint16_t topic_len;
            uint8_t *payload;
            uint32_t payload_len;
        } pub;
    } data;
} qm_iot_mqtt_recv_t;
typedef void (*qm_iot_mqtt_recv_handler_t)(void *, const qm_iot_mqtt_recv_t *, void *);
typedef struct { const char *method; const char *submethod; uint32_t msg_id; } qm_iot_down_req_t;
int32_t qm_iot_down_register(void *, char *, qm_iot_mqtt_recv_handler_t, void *);
int32_t qm_iot_down_unregister(void *, char *);
int32_t qm_iot_up_report_ex(void *, char *, char *, uint32_t, char *, int);
int32_t qm_iot_down_reply_ex(void *, char *, char *, uint32_t, int, char *, int);
int32_t qm_iot_down_reply(void *, const qm_iot_down_req_t *, int, char *, int);
#endif
'''

TYPES = r'''
#ifndef HOST_QM_TYPES_H
#define HOST_QM_TYPES_H
#include <stdint.h>
typedef int qm_bool;
#define QM_TRUE 1
#define QM_FALSE 0
#endif
'''

KERNEL = r'''
#ifndef HOST_QM_KERNEL_H
#define HOST_QM_KERNEL_H
#include <stddef.h>
#include <pthread.h>
#include "qm_types.h"
typedef pthread_mutex_t qm_mutex_t;
typedef struct qm_timer qm_timer_t;
struct qm_timer { void (*cb)(qm_timer_t *, void *); void *arg; int period; int active; };
#define QM_WAIT_FOREVER (-1)
void *qm_calloc(size_t, size_t);
void qm_free(void *);
int qm_mutex_new(qm_mutex_t *);
int qm_mutex_lock(qm_mutex_t *, int);
int qm_mutex_unlock(qm_mutex_t *);
int qm_mutex_free(qm_mutex_t *);
int qm_timer_new(qm_timer_t *, void (*)(qm_timer_t *, void *), void *, int, int);
int qm_timer_start(qm_timer_t *);
int qm_timer_stop(qm_timer_t *);
int qm_timer_free(qm_timer_t *);
int qm_msleep(int);
#endif
'''

RGB_HEADER = r'''#ifndef HOST_RGB_H
#define HOST_RGB_H
#include <stdint.h>
typedef struct {int active;int restore_pending;uint32_t owner;int last_error;} iot_control_light_rgb_focus_status_t;
int iot_control_light_rgb_focus_begin(uint32_t);
int iot_control_light_rgb_focus_end(uint32_t);
int iot_control_light_rgb_focus_get_status(iot_control_light_rgb_focus_status_t *);
#endif
'''

TEST_SOURCE = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_stubs.h"
#include "qm_pomodoro.h"
#include "pomodoro.h"
#include "pomodoro_cli.h"
#include "qm_cli.h"
#include "cJSON.h"
#include "iot_control_light_rgb.h"
#include <pthread.h>

static qm_timer_t *real_timer;
static int timer_start_failure, create_failure, timer_start_calls;
static _Thread_local int event_depth;
static int begin_failure, begin_reject_before_owner, end_failure, lamp_begin_calls, lamp_end_calls;
static int notify_failure, notify_calls;
static int state_report_failure, state_report_calls;
static qm_pomodoro_status_t reported_status;
static uint32_t notified_generation;
static iot_control_light_rgb_focus_status_t lamp;
static pthread_mutex_t work_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lamp_gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t lamp_cond = PTHREAD_COND_INITIALIZER;
static int block_begin, begin_entered, begin_release;
static int cloud_token, wire_connected = 1;
static qm_iot_mqtt_recv_handler_t down_callback;
static void *down_userdata;
static int register_failure, register_calls;
static int next_publish_failure, next_pause_publish_failure, persistent_publish_failure, next_post_failure;
static uint32_t auto_id = 8000;
static uint64_t now_ms;

typedef struct {
    qm_work_t *handle;
    void (*fn)(void *);
    void *arg;
    uint64_t due;
    int active;
} queued_work_t;
static queued_work_t work_queue[32];

typedef struct {
    char method[64], submethod[32], params[1024];
    uint32_t id;
    int code, reply, publish_return;
} published_t;
static published_t published[128];
static size_t publish_count;
static const struct qm_cli_command *cli_command;
static int cli_register_calls, cli_register_failure;
static char cli_output[2048];

static char captured_logs[65536];
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
void host_log(const char *tag, const char *format, ...) {
    (void)tag;va_list args;pthread_mutex_lock(&log_lock);
    size_t used=strlen(captured_logs);assert(used+2<sizeof(captured_logs));
    va_start(args,format);int n=vsnprintf(captured_logs+used,sizeof(captured_logs)-used,format,args);va_end(args);
    assert(n>=0&&(size_t)n<sizeof(captured_logs)-used-1);used+=(size_t)n;
    captured_logs[used]='\n';captured_logs[used+1]='\0';pthread_mutex_unlock(&log_lock);
}
int qm_cli_register_command(const struct qm_cli_command *command) {
    ++cli_register_calls;
    if (cli_register_failure) return cli_register_failure;
    assert(command && strcmp(command->name, "pomodoro") == 0 && command->function);
    cli_command = command;
    return QM_EOK;
}
int qm_cli_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    size_t used = strlen(cli_output);
    (void)vsnprintf(cli_output + used, sizeof(cli_output) - used, format, args);
    va_end(args);
    return QM_EOK;
}

void *qm_calloc(size_t n, size_t size) { return calloc(n, size); }
void qm_free(void *p) { free(p); }
int qm_mutex_new(qm_mutex_t *m) { return pthread_mutex_init(m, NULL); }
int qm_mutex_free(qm_mutex_t *m) { return pthread_mutex_destroy(m); }
int qm_mutex_lock(qm_mutex_t *m, int timeout) { (void)timeout; return pthread_mutex_lock(m); }
int qm_mutex_unlock(qm_mutex_t *m) { return pthread_mutex_unlock(m); }
int qm_timer_new(qm_timer_t *p, void (*cb)(qm_timer_t *,void *),void *arg,int ms,int periodic) {
    if (create_failure) return -QM_ENOMEM;
    assert(periodic == 1); *p=(qm_timer_t){ .cb=cb,.arg=arg,.period=ms }; real_timer=p; return 0;
}
int qm_timer_start(qm_timer_t *p) { ++timer_start_calls;if(timer_start_failure){timer_start_failure=0;return -QM_EIO;} p->active=1;return 0; }
int qm_timer_stop(qm_timer_t *p) { p->active=0;return 0; }
int qm_timer_free(qm_timer_t *p) { assert(p==real_timer);real_timer=NULL;return 0; }
int qm_msleep(int ms) { (void)ms;return 0; }
int iot_control_light_rgb_focus_begin(uint32_t owner) {
    assert(!event_depth && owner); ++lamp_begin_calls;
    pthread_mutex_lock(&lamp_gate);
    if (block_begin) { begin_entered=1;pthread_cond_broadcast(&lamp_cond);while(!begin_release)pthread_cond_wait(&lamp_cond,&lamp_gate); }
    pthread_mutex_unlock(&lamp_gate);
    if(begin_reject_before_owner) return begin_reject_before_owner;
    lamp.owner=owner;
    if(begin_failure){lamp.last_error=begin_failure;return begin_failure;}
    lamp.active=1;lamp.restore_pending=0;lamp.last_error=0;return 0;
}
int iot_control_light_rgb_focus_end(uint32_t owner) {
    assert(!event_depth && owner);++lamp_end_calls;
    assert(lamp.owner==owner);lamp.active=0;
    if(end_failure){lamp.restore_pending=1;lamp.last_error=end_failure;return end_failure;}
    lamp.restore_pending=0;lamp.last_error=0;return 0;
}
int iot_control_light_rgb_focus_get_status(iot_control_light_rgb_focus_status_t *s) {
    assert(!event_depth);*s=lamp;return 0;
}
int pomodoro_report_state(void) {
    assert(!event_depth);++state_report_calls;
    /* This status query would deadlock if sync invoked the hook under op lock. */
    assert(pomodoro_get_local_status(&reported_status)==0);
    return state_report_failure;
}
int pomodoro_notify_finished(uint32_t owner) {
    assert(!event_depth);++notify_calls;
    if(notify_failure)return notify_failure;
    notified_generation=owner;return 0;
}

int32_t qm_post_delayed_action(qm_work_t *work, void (*fn)(void *), void *arg, uint32_t ms) {
    if (next_post_failure) { int ret = next_post_failure; next_post_failure = 0; return ret; }
    assert(work && fn);
    pthread_mutex_lock(&work_lock);
    queued_work_t *slot = NULL;
    for (size_t i = 0; i < QM_ARRAY_SIZE(work_queue); ++i) {
        if (work_queue[i].active && work_queue[i].handle == work) { slot = &work_queue[i]; break; }
        if (!slot && !work_queue[i].active) slot = &work_queue[i];
    }
    assert(slot);
    *slot = (queued_work_t){ .handle = work, .fn = fn, .arg = arg, .due = now_ms + ms, .active = 1 };
    work->arg = arg; work->fn = fn; work->dly = ms; work->active = 1; ++work->seq;
    pthread_mutex_unlock(&work_lock);
    return QM_EOK; /* Deliberately never runs inline. Same-handle posting replaces work. */
}
int32_t qm_cancel_delayed_action(qm_work_t *work) {
    assert(work);
    pthread_mutex_lock(&work_lock);
    for (size_t i = 0; i < QM_ARRAY_SIZE(work_queue); ++i)
        if (work_queue[i].handle == work) work_queue[i].active = 0;
    work->active = 0; ++work->seq;
    pthread_mutex_unlock(&work_lock);
    return QM_EOK;
}
static void pump(void) {
    for (unsigned iteration = 0; iteration < 200; ++iteration) {
        pthread_mutex_lock(&work_lock);
        queued_work_t *slot = NULL;
        for (size_t i = 0; i < QM_ARRAY_SIZE(work_queue); ++i)
            if (work_queue[i].active && work_queue[i].due <= now_ms) { slot = &work_queue[i]; break; }
        if (!slot) { pthread_mutex_unlock(&work_lock); return; }
        queued_work_t item = *slot;
        slot->active = 0; item.handle->active = 0;
        pthread_mutex_unlock(&work_lock);
        item.fn(item.arg);
    }
    assert(!"unbounded immediate work repost");
}
static void advance(uint32_t ms) { now_ms += ms; pump(); }

static qm_pomodoro_status_t status(void) {
    qm_pomodoro_status_t s;assert(pomodoro_get_local_status(&s)==0);return s;
}
static void tick(unsigned n) {
    assert(real_timer);
    while(n--) { assert(real_timer->active);++event_depth;real_timer->cb(real_timer,real_timer->arg);--event_depth;now_ms+=1000;pump(); }
}
static void raw_tick(void) { ++event_depth;real_timer->cb(real_timer,real_timer->arg);--event_depth; }

int32_t qm_iot_down_register(void *handle, char *method, qm_iot_mqtt_recv_handler_t callback, void *user) {
    assert(handle == &cloud_token && strcmp(method, "pomodoro") == 0 && callback);
    ++register_calls;
    if (register_failure) return register_failure;
    down_callback = callback; down_userdata = user;
    return QM_EOK;
}
int32_t qm_iot_down_unregister(void *handle, char *method) {
    assert(handle == &cloud_token && strcmp(method, "pomodoro") == 0);
    down_callback = NULL; return QM_EOK;
}
static int publish(void *handle, const char *method, const char *submethod,
                   uint32_t id, int code, const char *payload, int length, int reply) {
    assert(event_depth == 0); /* Never publish while a timer event callback is on stack. */
    assert(handle == &cloud_token && method && submethod);
    assert(length >= 0 && (payload || length == 0));
    assert(publish_count < QM_ARRAY_SIZE(published));
    published_t *record = &published[publish_count++];
    memset(record, 0, sizeof(*record));
    size_t n = strlen(method);
    snprintf(record->method, sizeof(record->method), "%s%s", method,
             reply && (n < 5 || strcmp(method + n - 5, "Reply") != 0) ? "Reply" : "");
    snprintf(record->submethod, sizeof(record->submethod), "%s", submethod);
    record->id = id ? id : ++auto_id; /* Actual SDK allocates a new id when the argument is zero. */
    record->code = code; record->reply = reply;
    assert((size_t)length < sizeof(record->params));
    if (length) memcpy(record->params, payload, (size_t)length);
    if (!wire_connected) record->publish_return = -QM_EIO;
    else if (persistent_publish_failure) record->publish_return = persistent_publish_failure;
    else if (!reply && !strcmp(submethod,"pause") && next_pause_publish_failure) {
        record->publish_return = next_pause_publish_failure; next_pause_publish_failure = 0;
    } else if (next_publish_failure) { record->publish_return = next_publish_failure; next_publish_failure = 0; }
    return record->publish_return;
}
int32_t qm_iot_up_report_ex(void *handle, char *method, char *submethod,
                          uint32_t id, char *payload, int length) {
    return publish(handle, method, submethod, id, -999, payload, length, 0);
}
int32_t qm_iot_down_reply_ex(void *handle, char *method, char *submethod,
                           uint32_t id, int code, char *payload, int length) {
    return publish(handle, method, submethod, id, code, payload, length, 1);
}
int32_t qm_iot_down_reply(void *handle, const qm_iot_down_req_t *request,
                        int code, char *payload, int length) {
    assert(request);
    return publish(handle, request->method, request->submethod, request->msg_id, code, payload, length, 1);
}

static void receive(const char *json) {
    assert(down_callback);
    size_t length = strlen(json);
    uint8_t *copy = malloc(length + 1);
    assert(copy); memcpy(copy, json, length + 1);
    qm_iot_mqtt_recv_t packet = { .type = QM_IOT_MQTTRECV_PUB,
        .data.pub = { .payload = copy, .payload_len = (uint32_t)length } };
    down_callback(&cloud_token, &packet, down_userdata);
    memset(copy, 0xa5, length); free(copy); /* Catch deferred borrowing of MQTT payload storage. */
}
static void command(uint32_t id, const char *submethod, const char *source) {
    char json[256];
    const char *duration=!strcmp(submethod,"start")&&!strcmp(source,"cloud") ? ",\"focus_sec\":1500" : "";
    snprintf(json, sizeof(json), "{\"method\":\"pomodoro\",\"subMethod\":\"%s\",\"id\":%u,\"params\":{\"source\":\"%s\"%s}}",
             submethod, (unsigned)id, source,duration);
    receive(json); pump();
}
static const published_t *reply_for(uint32_t id, const char *submethod) {
    for (size_t i = publish_count; i; --i) {
        const published_t *r = &published[i - 1];
        if (r->reply && r->id == id && strcmp(r->submethod, submethod) == 0) return r;
    }
    return NULL;
}
static void expect_reply(uint32_t id, const char *submethod, int success) {
    const published_t *r = reply_for(id, submethod);
    assert(r && strcmp(r->method, "pomodoroReply") == 0);
    assert(success ? r->code == 0 : r->code != 0);
}
static int local_report_count(const char *submethod) {
    int count = 0;
    for (size_t i = 0; i < publish_count; ++i) {
        const published_t *r = &published[i];
        if (r->reply || r->publish_return || strcmp(r->method, "pomodoro") || strcmp(r->submethod, submethod)) continue;
        cJSON *params = cJSON_Parse(r->params);
        assert(params);
        const cJSON *source = cJSON_GetObjectItemCaseSensitive(params, "source");
        if (cJSON_IsString(source) && strcmp(source->valuestring, "local") == 0) ++count;
        cJSON_Delete(params);
    }
    return count;
}

static void initialize(void) {
    pomodoro_init();assert(real_timer && !real_timer->active);
    assert(pomodoro_on_connected(NULL)!=0);
    assert(pomodoro_on_connected(&cloud_token)==0);pump();
}
static void start_json(uint32_t id, const char *value) {
    char json[256];snprintf(json,sizeof(json),"{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":%u,\"params\":{\"source\":\"cloud\",\"focus_sec\":%s}}",id,value);
    receive(json);pump();
}
static void test_basics(void) {
    initialize();command(1,"start","cloud");expect_reply(1,"start",1);
    assert(status().duration_sec==1500 && status().focus_target==1 && lamp.active);
    tick(1);assert(status().remaining_sec==1499);
    command(2,"pause","cloud");expect_reply(2,"pause",1);assert(status().paused&&!lamp.active);
    raw_tick();pump();assert(status().remaining_sec==1499);
    command(3,"resume","cloud");expect_reply(3,"resume",1);assert(!status().paused&&lamp.active);
    command(4,"stop","cloud");expect_reply(4,"stop",1);assert(status().state==QM_POMODORO_STATE_IDLE&&!lamp.active);
}
static void test_duration(void) {
    initialize();assert(pomodoro_start_for(0)!=0 && pomodoro_start_for(86401)!=0);
    assert(pomodoro_start_for(7)==0);tick(2);assert(status().remaining_sec==5);
    int begins=lamp_begin_calls;assert(pomodoro_start_for(9)==-QM_EBUSY && lamp_begin_calls==begins);
    assert(status().remaining_sec==5 && status().duration_sec==7);
    assert(pomodoro_execute("pause")==0);assert(pomodoro_start_for(8)==-QM_EBUSY);
    assert(pomodoro_execute("stop")==0);assert(pomodoro_start_for(86400)==0 && status().duration_sec==86400);
    assert(pomodoro_execute("stop")==0);assert(pomodoro_execute("start")==0 && status().duration_sec==1500);pump();
}
static void test_duplicate(void) {
    initialize();start_json(10,"7");expect_reply(10,"start",1);tick(2);
    start_json(10,"7");expect_reply(10,"start",1);assert(status().remaining_sec==5&&lamp_begin_calls==1);
    start_json(10,"8");expect_reply(10,"start",0);assert(status().remaining_sec==5);
    start_json(11,"9");expect_reply(11,"start",0);assert(status().remaining_sec==5);
    command(12,"pause","cloud");command(12,"pause","cloud");expect_reply(12,"pause",1);
    assert(status().paused);
}
static void test_finished(void) {
    initialize();assert(pomodoro_start_for(2)==0);pump();uint32_t owner=lamp.owner;
    int stops=local_report_count("stop");tick(2);
    assert(status().state==QM_POMODORO_STATE_FINISHED&&!status().paused&&!real_timer->active&&!lamp.active);
    assert(notify_calls==1&&notified_generation==owner&&pomodoro_is_finished_round(owner));
    assert(local_report_count("stop")==stops); /* Custom timer never entered native learning. */
    assert(reported_status.state==QM_POMODORO_STATE_FINISHED&&reported_status.remaining_sec==0);
    raw_tick();pump();advance(3000);assert(notify_calls==1);
    assert(pomodoro_start_for(3)==0);assert(!pomodoro_is_finished_round(owner));pump();
    assert(status().state==QM_POMODORO_STATE_FOCUS&&lamp.active&&lamp.owner!=owner&&notify_calls==1);
    assert(pomodoro_execute("stop")==0);pump();assert(notify_calls==1);
}
static void test_late(void) {
    initialize();assert(pomodoro_start_for(1)==0);pump();uint32_t old=lamp.owner;
    raw_tick(); /* FINISHED work is pending, but a new run starts before it drains. */
    assert(status().state==QM_POMODORO_STATE_FINISHED);assert(pomodoro_start_for(4)==0);
    pump();assert(status().state==QM_POMODORO_STATE_FOCUS&&status().remaining_sec==4&&lamp.active&&lamp.owner!=old);
    assert(notify_calls==0&&!pomodoro_is_finished_round(old));
}
static void test_lamp_failures(void) {
    initialize();begin_failure=-QM_EIO;start_json(20,"3");expect_reply(20,"start",0);
    assert(status().state==QM_POMODORO_STATE_IDLE&&!real_timer->active&&!lamp.active);
    begin_failure=0;begin_reject_before_owner=-QM_EIO;
    assert(pomodoro_start_for(3)==-QM_EIO);
    begin_reject_before_owner=0;
    timer_start_failure=1;assert(pomodoro_start_for(3)==-QM_EIO&&!lamp.active);
    assert(pomodoro_start_for(3)==0);pump();end_failure=-QM_EIO;
    assert(pomodoro_execute("pause")==-QM_EIO);assert(status().state==QM_POMODORO_STATE_IDLE&&lamp.restore_pending);
    end_failure=0;advance(1000);assert(!lamp.restore_pending);
    assert(pomodoro_start_for(3)==0);pump();assert(pomodoro_execute("pause")==0);
    begin_failure=-QM_EIO;assert(pomodoro_execute("resume")==-QM_EIO);assert(status().state==QM_POMODORO_STATE_IDLE);
}
static void test_watchdog(void) {
    initialize();assert(pomodoro_start_for(4)==0);pump();int starts=local_report_count("start");
    int reports=state_report_calls;
    tick(1);tick(1);assert(local_report_count("start")==starts&&state_report_calls==reports);
    lamp.active=0;lamp.last_error=-QM_ETIMEOUT;tick(1);
    assert(status().state==QM_POMODORO_STATE_IDLE&&!lamp.active&&!real_timer->active);
    pomodoro_local_status_t s;assert(pomodoro_get_state(&s)==0&&s.last_error==-QM_ETIMEOUT);
}
static void test_restore_retry(void) {
    initialize();assert(pomodoro_start_for(1)==0);pump();end_failure=-QM_EIO;tick(1);
    assert(status().state==QM_POMODORO_STATE_FINISHED&&lamp.restore_pending&&notify_calls==1);
    assert(pomodoro_start_for(3)==-QM_EIO);end_failure=0;advance(1000);assert(!lamp.restore_pending&&notify_calls==1);
    pomodoro_local_status_t s;assert(pomodoro_get_state(&s)==0&&s.last_error==-QM_EIO);
    assert(pomodoro_start_for(3)==0);pump();assert(status().remaining_sec==3&&lamp.active);
}
static void test_notify_retry(void) {
    initialize();assert(pomodoro_start_for(1)==0);pump();notify_failure=-QM_EFULL;tick(1);assert(notify_calls==1);
    notify_failure=0;advance(1000);assert(notify_calls==2);advance(1000);assert(notify_calls==2);
}
static void test_params(void) {
    initialize();const char *bad[]={"0","86401","-1","1.5","true","\"60\"","null"};
    uint32_t id=30;for(size_t i=0;i<QM_ARRAY_SIZE(bad);i++){start_json(++id,bad[i]);expect_reply(id,"start",0);}
    const char *requests[]={
      "{\"id\":50,\"method\":\"pomodoro\",\"subMethod\":\"start\",\"params\":{\"duration\":60}}",
      "{\"id\":51,\"method\":\"pomodoro\",\"subMethod\":\"start\",\"params\":{\"focus_sec\":1,\"focus_sec\":2}}",
      "{\"id\":52,\"method\":\"pomodoro\",\"subMethod\":\"start\",\"focus_sec\":60}",
      "{\"id\":53,\"method\":\"pomodoro\",\"subMethod\":\"stop\",\"params\":{\"focus_sec\":1}}"};
    for(size_t i=0;i<QM_ARRAY_SIZE(requests);i++){receive(requests[i]);pump();expect_reply(50+(uint32_t)i,i==3?"stop":"start",0);}
    assert(status().state==QM_POMODORO_STATE_IDLE&&lamp_begin_calls==0);
    start_json(60,"60");expect_reply(60,"start",1);assert(status().duration_sec==60);
}
static void test_wire_diagnostics(void) {
    initialize();captured_logs[0]='\0';
    receive("{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":95,\"params\":{\"source\":\"cloud\",\"duration\":7,\"token\":\"SECRET_WIRE_TOKEN_55\",\"big\":86401,\"fraction\":1.5,\"negative\":-1,\"flag\":true,\"nested\":{\"codebody\":\"SECRET_NESTED_BODY_66\"}},\"productSecret\":\"SECRET_PRODUCT_77\",\"01234567890123456789012345678901SECRET_KEY_SUFFIX_88\":\"SECRET_LONG_KEY_VALUE_99\",\"odd\\nkey\":\"SECRET_CONTROL_VALUE_11\"}");
    pump();expect_reply(95,"start",0);
    assert(status().state==QM_POMODORO_STATE_IDLE);
    assert(strstr(captured_logs,"layer=params field=duration type=number integer=7"));
    assert(strstr(captured_logs,"layer=params field=source type=string enum=cloud"));
    assert(strstr(captured_logs,"layer=params field=token type=string length="));
    assert(strstr(captured_logs,"layer=params field=big type=number value=hidden"));
    assert(strstr(captured_logs,"layer=params field=fraction type=number value=hidden"));
    assert(strstr(captured_logs,"layer=params field=negative type=number value=hidden"));
    assert(strstr(captured_logs,"layer=params field=flag type=bool"));
    assert(strstr(captured_logs,"layer=params field=nested type=object"));
    assert(strstr(captured_logs,"layer=root field=odd?key type=string length="));
    assert(strstr(captured_logs,"layer=root field=01234567890123456789012345678901 type=string length="));
    assert(strstr(captured_logs,"invalid layer=params reason=unknown_field"));
    assert(strstr(captured_logs,"invalid layer=root reason=unknown_field"));
    assert(!strstr(captured_logs,"SECRET_")&&!strstr(captured_logs,"86401")&&!strstr(captured_logs,"1.5"));
    captured_logs[0]='\0';
    receive("{\"method\":\"pomodoro\",\"subMethod\":\"SECRET_ACTION_33\",\"id\":96,\"params\":{\"source\":\"SECRET_SOURCE_22\"}}");
    pump();expect_reply(96,"SECRET_ACTION_33",0);
    assert(strstr(captured_logs,"invalid layer=params reason=source_enum"));
    assert(!strstr(captured_logs,"SECRET_"));
    captured_logs[0]='\0';
    start_json(97,"420");expect_reply(97,"start",1);
    assert(strstr(captured_logs,"layer=params field=focus_sec type=number integer=420"));
    assert(status().duration_sec==420);
}
static void test_native_metadata(void) {
    initialize();captured_logs[0]='\0';
    receive("{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":101,\"params\":{\"source\":\"cloud\",\"focus_sec\":1500,\"speakText\":\"\"}}");
    pump();expect_reply(101,"start",1);assert(status().duration_sec==1500&&lamp.active);
    assert(strstr(captured_logs,"field=speakText type=string length=0"));
    tick(2);uint32_t owner=lamp.owner;int begins=lamp_begin_calls;
    receive("{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":101,\"params\":{\"source\":\"cloud\",\"focus_sec\":1500,\"speakText\":\"SECRET_METADATA_BODY_101\"}}");
    pump();expect_reply(101,"start",1);
    assert(status().remaining_sec==1498&&lamp.owner==owner&&lamp_begin_calls==begins&&notify_calls==0);
    assert(!strstr(captured_logs,"SECRET_METADATA_BODY_101"));
    receive("{\"method\":\"pomodoro\",\"subMethod\":\"stop\",\"id\":102,\"params\":{\"source\":\"cloud\",\"speakText\":\"SECRET_METADATA_STOP_102\"}}");
    pump();expect_reply(102,"stop",1);assert(status().state==QM_POMODORO_STATE_IDLE&&!lamp.active&&notify_calls==0);
    assert(!strstr(captured_logs,"SECRET_METADATA_STOP_102"));
    const char *bad[]={"null","true","7","{}","[]"};
    char json[1200];uint32_t id=103;
    for(size_t i=0;i<QM_ARRAY_SIZE(bad);i++) {
        snprintf(json,sizeof(json),"{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":%u,\"params\":{\"speakText\":%s}}",id,bad[i]);
        receive(json);pump();expect_reply(id++,"start",0);
        assert(status().state==QM_POMODORO_STATE_IDLE);
    }
    receive("{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":108,\"params\":{\"speakText\":\"\",\"speakText\":\"\"}}");
    pump();expect_reply(108,"start",0);assert(status().state==QM_POMODORO_STATE_IDLE);
    assert(strstr(captured_logs,"reason=speakText_type")&&strstr(captured_logs,"reason=speakText_duplicate"));

    /* The existing entire-wire cap also bounds optional metadata. Exercise a
     * nonempty string at exactly the admitted payload boundary, then +1. */
    const char *prefix="{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":109,\"params\":{\"focus_sec\":1500,\"speakText\":\"";
    const char *suffix="\"}}";
    size_t n=strlen(prefix),body=1024-n-strlen(suffix);
    memcpy(json,prefix,n);memset(json+n,'Z',body);memcpy(json+n+body,suffix,strlen(suffix)+1);
    assert(strlen(json)==1024);receive(json);pump();expect_reply(109,"start",1);
    assert(status().duration_sec==1500&&notify_calls==0);
    assert(pomodoro_execute("stop")==0);pump();
    memcpy(json,prefix,n);memset(json+n,'Z',body+1);memcpy(json+n+body+1,suffix,strlen(suffix)+1);
    assert(strlen(json)==1025);int calls=lamp_begin_calls;receive(json);pump();assert(lamp_begin_calls==calls&&status().state==QM_POMODORO_STATE_IDLE);
    assert(!strstr(captured_logs,"ZZZZZZZZZZZZZZZZ"));
}
static void test_duration_required(void) {
    initialize();captured_logs[0]='\0';int timers=timer_start_calls,begins=lamp_begin_calls;
    const char *bare="{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":110,\"params\":{\"source\":\"cloud\",\"speakText\":\"\"}}";
    receive(bare);pump();expect_reply(110,"start",0);
    assert(status().state==QM_POMODORO_STATE_IDLE&&!real_timer->active&&!lamp.active);
    assert(timer_start_calls==timers&&lamp_begin_calls==begins&&strstr(captured_logs,"duration_required id=110"));
    receive(bare);pump();expect_reply(110,"start",0); /* Failed request stays cached, without any action. */
    assert(timer_start_calls==timers&&lamp_begin_calls==begins&&status().state==QM_POMODORO_STATE_IDLE);
    receive("{\"method\":\"pomodoro\",\"subMethod\":\"start\",\"id\":111}");
    pump();expect_reply(111,"start",0); /* Missing params/source is also cloud-side, never a default. */
    assert(timer_start_calls==timers&&lamp_begin_calls==begins);
    command(112,"start","local");expect_reply(112,"start",1);
    assert(timer_start_calls==timers&&lamp_begin_calls==begins&&status().state==QM_POMODORO_STATE_IDLE);
    start_json(113,"420");expect_reply(113,"start",1);tick(1);
    assert(status().duration_sec==420&&status().remaining_sec==419&&lamp.active);
    begins=lamp_begin_calls;timers=timer_start_calls;
    receive(bare);pump();expect_reply(110,"start",0);
    assert(status().remaining_sec==419&&lamp_begin_calls==begins&&timer_start_calls==timers);
    assert(pomodoro_execute("stop")==0);pump();
    assert(pomodoro_execute("start")==0&&status().duration_sec==1500);pump(); /* Explicit developer default remains. */
}
static void test_loopback(void) {
    initialize();int starts=local_report_count("start");command(70,"start","local");expect_reply(70,"start",1);
    assert(status().state==QM_POMODORO_STATE_IDLE&&lamp_begin_calls==0&&local_report_count("start")==starts);
}
static void test_panel_plugin_start(void) {
    initialize();
    int starts=local_report_count("start"),stops=local_report_count("stop");
    assert(pomodoro_start_for_plugin(0)==-QM_EINVAL);
    assert(pomodoro_start_for_plugin(86401)==-QM_EINVAL);
    assert(pomodoro_start_for_plugin(4)==QM_EOK);
    assert(status().duration_sec==4&&status().remaining_sec==4&&lamp.active);
    pump();
    assert(local_report_count("start")==starts+1);
    assert(pomodoro_start_for(3)==-QM_EBUSY);
    assert(pomodoro_start_for_plugin(3)==-QM_EBUSY);
    assert(status().duration_sec==4);
    pump();
    starts=local_report_count("start");
    int timer_starts=timer_start_calls;
    command(115,"start","local");expect_reply(115,"start",1);
    assert(timer_start_calls==timer_starts&&status().remaining_sec==4);
    assert(local_report_count("start")==starts);
    assert(pomodoro_execute("pause")==QM_EOK);pump();
    assert(status().paused&&!lamp.active&&local_report_count("start")==starts&&local_report_count("pause")==1);
    assert(pomodoro_execute("resume")==QM_EOK);pump();
    assert(!status().paused&&lamp.active&&local_report_count("start")==starts&&local_report_count("resume")==1);
    assert(pomodoro_execute("stop")==QM_EOK);pump();
    assert(status().state==QM_POMODORO_STATE_IDLE&&!lamp.active&&local_report_count("stop")==stops+1);
    starts=local_report_count("start");stops=local_report_count("stop");
    assert(pomodoro_start_for_plugin(1)==QM_EOK);pump();
    assert(local_report_count("start")==starts+1);
    tick(1);
    assert(status().state==QM_POMODORO_STATE_FINISHED&&!lamp.active&&local_report_count("stop")==stops+1);
    starts=local_report_count("start");
    assert(pomodoro_start_for(2)==QM_EOK);pump();
    assert(local_report_count("start")==starts); /* 12.1 stays outside native learning. */
    assert(pomodoro_execute("stop")==QM_EOK);pump();
    begin_failure=-QM_EIO;
    starts=local_report_count("start");
    assert(pomodoro_start_for_plugin(2)==-QM_EIO);pump();
    assert(status().state==QM_POMODORO_STATE_IDLE&&local_report_count("start")==starts);
    begin_failure=0;
    pomodoro_on_disconnected();wire_connected=0;
    size_t before=publish_count;
    assert(pomodoro_start_for_plugin(3)==QM_EOK);pump();
    assert(publish_count==before&&status().remaining_sec==3);
    wire_connected=1;
    assert(pomodoro_on_connected(&cloud_token)==QM_EOK);pump();
    assert(local_report_count("start")==starts+1);
}
static void test_reconnect(void) {
    initialize();int starts=local_report_count("start"),pauses=local_report_count("pause");
    int stops=local_report_count("stop");
    assert(pomodoro_start_for(3)==0);pump();
    assert(status().state==QM_POMODORO_STATE_FOCUS&&status().remaining_sec==3&&lamp.active);
    assert(local_report_count("start")==starts&&local_report_count("stop")==stops);
    tick(1);pomodoro_on_disconnected();wire_connected=0;
    size_t before=publish_count;assert(pomodoro_execute("pause")==0);pump();assert(publish_count==before&&status().paused);
    wire_connected=1;stops=local_report_count("stop");assert(pomodoro_on_connected(&cloud_token)==0);pump();
    assert(status().remaining_sec==2&&status().paused);
    assert(local_report_count("start")==starts&&local_report_count("pause")==pauses&&local_report_count("stop")==stops+1);
    assert(pomodoro_execute("resume")==0);pump();assert(local_report_count("start")==starts);
    tick(2);assert(notify_calls==1&&status().state==QM_POMODORO_STATE_FINISHED);

    /* Native cloud starts retain native learning across pause/reconnect. */
    start_json(90,"4");expect_reply(90,"start",1);tick(1);
    assert(pomodoro_start_for(8)==-QM_EBUSY);pump(); /* Busy custom start must not steal origin. */
    starts=local_report_count("start");pauses=local_report_count("pause");
    pomodoro_on_disconnected();wire_connected=0;assert(pomodoro_execute("pause")==0);pump();
    wire_connected=1;assert(pomodoro_on_connected(&cloud_token)==0);pump();
    assert(local_report_count("start")==starts+1&&local_report_count("pause")==pauses+1&&status().paused);
    starts=local_report_count("start");int resumes=local_report_count("resume");
    assert(pomodoro_execute("resume")==0);pump();
    assert(local_report_count("start")==starts&&local_report_count("resume")==resumes+1&&
           !status().paused&&status().remaining_sec==3);
    stops=local_report_count("stop");assert(pomodoro_execute("stop")==0);pump();assert(local_report_count("stop")==stops+1);

    /* An earlier native run must never resurrect learning for a custom run. */
    starts=local_report_count("start");assert(pomodoro_start_for(7)==0);pump();
    pomodoro_on_disconnected();assert(pomodoro_on_connected(&cloud_token)==0);pump();
    start_json(90,"4");expect_reply(90,"start",1); /* cached native ACK only */
    assert(local_report_count("start")==starts&&status().remaining_sec==7&&lamp.active);
    assert(pomodoro_execute("stop")==0);pump();

    /* A native natural completion still explicitly exits learning once. */
    start_json(91,"1");expect_reply(91,"start",1);stops=local_report_count("stop");tick(1);
    assert(status().state==QM_POMODORO_STATE_FINISHED&&local_report_count("stop")==stops+1);
    starts=local_report_count("start");pomodoro_on_disconnected();assert(pomodoro_on_connected(&cloud_token)==0);pump();
    assert(local_report_count("start")==starts&&notify_calls==2);
}

static void test_post_failure(void) {
    initialize();next_post_failure=-QM_ENOMEM;command(80,"start","cloud");assert(!reply_for(80,"start")&&status().state==QM_POMODORO_STATE_IDLE);
    command(80,"start","cloud");expect_reply(80,"start",1);
}
static void test_publish_failure(void) {
    initialize();assert(pomodoro_start_for_plugin(3)==0);pump();persistent_publish_failure=-QM_EIO;
    size_t before=publish_count;assert(pomodoro_execute("stop")==0);pump();for(int i=0;i<6;i++)advance(1000);
    assert(publish_count-before>=1&&publish_count-before<=4);size_t bounded=publish_count;advance(10000);assert(publish_count==bounded);
    persistent_publish_failure=0;pomodoro_on_disconnected();assert(pomodoro_on_connected(&cloud_token)==0);pump();assert(local_report_count("stop")>=1);
    state_report_failure=-QM_EIO;
    assert(pomodoro_start_for(8)==0);pump();assert(status().state==QM_POMODORO_STATE_FOCUS&&status().remaining_sec==8);
    assert(reported_status.state==QM_POMODORO_STATE_FOCUS&&reported_status.remaining_sec==8);
}
static void test_partial_sync_retry(void) {
    initialize();
    int reports=state_report_calls, starts=local_report_count("start");
    state_report_failure=-QM_EIO;
    assert(pomodoro_start_for_plugin(8)==0);pump();
    assert(state_report_calls==reports+1&&local_report_count("start")==starts+1);
    state_report_failure=0;advance(1000);
    assert(state_report_calls==reports+2&&local_report_count("start")==starts+1);
    reports=state_report_calls;advance(1000);assert(state_report_calls==reports);

    int stops=local_report_count("stop");
    next_publish_failure=-QM_EIO;
    assert(pomodoro_execute("stop")==0);pump();
    assert(state_report_calls==reports+1&&local_report_count("stop")==stops);
    reports=state_report_calls;advance(1000);
    assert(state_report_calls==reports&&local_report_count("stop")==stops+1);
    advance(1000);assert(state_report_calls==reports&&local_report_count("stop")==stops+1);
}
static void test_plugin_transitions(void) {
    initialize();
    int starts=local_report_count("start"), stops=local_report_count("stop");
    int pauses=local_report_count("pause"), resumes=local_report_count("resume");
    assert(pomodoro_start_for_plugin(6)==0);pump();
    assert(local_report_count("start")==starts+1);
    next_publish_failure=-QM_EIO;
    assert(pomodoro_execute("pause")==0);pump();
    assert(local_report_count("start")==starts+1&&local_report_count("pause")==pauses);
    advance(1000);
    assert(local_report_count("start")==starts+1&&local_report_count("pause")==pauses+1);
    assert(pomodoro_execute("resume")==0);pump();
    assert(local_report_count("start")==starts+1&&local_report_count("resume")==resumes+1);
    assert(pomodoro_execute("stop")==0);pump();
    assert(local_report_count("stop")==stops+1);

    /* Cloud ACK establishes learning; local pause must not publish a new start. */
    starts=local_report_count("start");int reports=state_report_calls;
    start_json(133,"5");expect_reply(133,"start",1);
    assert(local_report_count("start")==starts&&state_report_calls==reports+1);
    starts=local_report_count("start");pauses=local_report_count("pause");
    assert(pomodoro_execute("pause")==0);pump();
    assert(local_report_count("start")==starts&&local_report_count("pause")==pauses+1);
    pomodoro_on_disconnected();wire_connected=0;
    wire_connected=1;assert(pomodoro_on_connected(&cloud_token)==0);
    starts=local_report_count("start");pauses=local_report_count("pause");pump();
    assert(local_report_count("start")==starts+1&&local_report_count("pause")==pauses+1);
    assert(pomodoro_execute("stop")==0);pump();

    /* Reconnect while paused: successful start is remembered if pause fails. */
    assert(pomodoro_start_for_plugin(3)==0);pump();
    assert(pomodoro_execute("pause")==0);pump();
    pomodoro_on_disconnected();wire_connected=0;
    wire_connected=1;next_pause_publish_failure=-QM_EIO;
    assert(pomodoro_on_connected(&cloud_token)==0);
    starts=local_report_count("start");pauses=local_report_count("pause");pump();
    assert(local_report_count("start")==starts+1&&local_report_count("pause")==pauses);
    advance(1000);
    assert(local_report_count("start")==starts+1&&local_report_count("pause")==pauses+1);
    assert(pomodoro_execute("stop")==0);pump();
}
static void invoke_cli(int argc, const char *action, const char *extra) {
    char *args[3]={"pomodoro",(char *)action,(char *)extra};assert(cli_command);cli_output[0]='\0';cli_command->function(NULL,0,argc,args);
}
static void test_cli(void) {
    assert(pomodoro_cli_init()==0);initialize();invoke_cli(3,"start","3");assert(strstr(cli_output,"applied_ret=0")&&status().duration_sec==3);
    invoke_cli(2,"pause",NULL);assert(status().paused);invoke_cli(2,"resume",NULL);assert(!status().paused);
    invoke_cli(2,"stop",NULL);assert(status().state==QM_POMODORO_STATE_IDLE);
    const char *bad[]={"0","86401","-1","1.5","","+5"," 5","9999999999999999999999"};
    for(size_t i=0;i<QM_ARRAY_SIZE(bad);i++){invoke_cli(3,"start",bad[i]);assert(status().state==QM_POMODORO_STATE_IDLE);}
    invoke_cli(2,"start",NULL);assert(status().duration_sec==1500);pump();
}
static int thread_start_ret, thread_stop_ret;
static void *start_thread(void *unused){(void)unused;thread_start_ret=pomodoro_start_for(6);return NULL;}
static void *stop_thread(void *unused){(void)unused;thread_stop_ret=pomodoro_execute("stop");return NULL;}
static void test_concurrent(void) {
    initialize();pthread_t a,b;block_begin=1;assert(pthread_create(&a,NULL,start_thread,NULL)==0);
    pthread_mutex_lock(&lamp_gate);while(!begin_entered)pthread_cond_wait(&lamp_cond,&lamp_gate);pthread_mutex_unlock(&lamp_gate);
    assert(pthread_create(&b,NULL,stop_thread,NULL)==0);
    pthread_mutex_lock(&lamp_gate);begin_release=1;pthread_cond_broadcast(&lamp_cond);pthread_mutex_unlock(&lamp_gate);
    assert(pthread_join(a,NULL)==0&&pthread_join(b,NULL)==0);assert(thread_start_ret==0&&thread_stop_ret==0);pump();
    assert(status().state==QM_POMODORO_STATE_IDLE&&!lamp.active&&!real_timer->active);
}
static void test_uninitialized(void) {
    create_failure=1;pomodoro_init();assert(pomodoro_start_for(4)!=0);create_failure=0;
    pomodoro_init();assert(pomodoro_start_for(4)==0);pump();assert(status().duration_sec==4);
}
int main(int argc,char **argv) {
    assert(argc==2);
    if(!strcmp(argv[1],"basics"))test_basics();else if(!strcmp(argv[1],"duration"))test_duration();
    else if(!strcmp(argv[1],"duplicate"))test_duplicate();else if(!strcmp(argv[1],"finished"))test_finished();
    else if(!strcmp(argv[1],"late"))test_late();else if(!strcmp(argv[1],"lamp-failures"))test_lamp_failures();
    else if(!strcmp(argv[1],"watchdog"))test_watchdog();else if(!strcmp(argv[1],"restore-retry"))test_restore_retry();
    else if(!strcmp(argv[1],"notify-retry"))test_notify_retry();else if(!strcmp(argv[1],"params"))test_params();
    else if(!strcmp(argv[1],"loopback"))test_loopback();else if(!strcmp(argv[1],"panel-plugin-start"))test_panel_plugin_start();else if(!strcmp(argv[1],"reconnect"))test_reconnect();
    else if(!strcmp(argv[1],"post-failure"))test_post_failure();else if(!strcmp(argv[1],"publish-failure"))test_publish_failure();
    else if(!strcmp(argv[1],"partial-sync-retry"))test_partial_sync_retry();
    else if(!strcmp(argv[1],"plugin-transitions"))test_plugin_transitions();
    else if(!strcmp(argv[1],"cli"))test_cli();else if(!strcmp(argv[1],"concurrent"))test_concurrent();
    else if(!strcmp(argv[1],"uninitialized"))test_uninitialized();
    else if(!strcmp(argv[1],"wire-diagnostics"))test_wire_diagnostics();
    else if(!strcmp(argv[1],"native-metadata"))test_native_metadata();
    else if(!strcmp(argv[1],"duration-required"))test_duration_required();else return 2;
    printf("PASS %s\n",argv[1]);return 0;
}
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    scenarios = ['basics', 'duration', 'duplicate', 'finished', 'late', 'lamp-failures', 'watchdog', 'restore-retry', 'notify-retry', 'params', 'loopback', 'panel-plugin-start', 'reconnect', 'post-failure', 'publish-failure', 'partial-sync-retry', 'plugin-transitions', 'cli', 'concurrent', 'uninitialized', 'wire-diagnostics', 'native-metadata', 'duration-required']
    parser.add_argument("--scenario", choices=scenarios)
    parser.add_argument("--sanitize", action="store_true", help="Enable host AddressSanitizer/UBSan")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    sdk = root / "src/firmware"
    application = sdk / "apps/main/lj_components/pomodoro"
    cjson = sdk / "qumios-dev/components/cjson"
    source = application / "pomodoro.c"
    cli_source = application / "pomodoro_cli.c"
    if not source.is_file() or not cli_source.is_file() or not (cjson / "cJSON.c").is_file():
        raise SystemExit("Complete SDK sources are required; initialize the pinned submodule first")
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not compiler or not shutil.which(compiler[0]):
        raise SystemExit("A host C compiler is required (set CC if necessary)")
    with tempfile.TemporaryDirectory(prefix="pomodoro-control-test-") as directory:
        temporary = Path(directory)
        (temporary / "qm_config.h").write_text(CONFIG, encoding="utf-8")
        (temporary / "qm_types.h").write_text(TYPES, encoding="utf-8")
        shutil.copy2(sdk / "qumios-dev/include/common/qm_errno.h", temporary / "qm_errno.h")
        (temporary / "qm_kernel.h").write_text(KERNEL, encoding="utf-8")
        (temporary / "iot_control_light_rgb.h").write_text(RGB_HEADER, encoding="utf-8")
        (temporary / "host_stubs.h").write_text(STUB_HEADER, encoding="utf-8")
        for name in ["qm.h", "qm_iot_api.h", "qm_iot_mqtt.h"]:
            (temporary / name).write_text('#include "host_stubs.h"\n', encoding="utf-8")
        (temporary / "qm_event.h").write_text("/* OS event dispatch is substituted by the fixture. */\n", encoding="utf-8")
        fixture = temporary / "test_pomodoro_control.c"
        fixture.write_text(TEST_SOURCE, encoding="utf-8")
        binary = temporary / ("test-pomodoro-control.exe" if os.name == "nt" else "test-pomodoro-control")
        includes = [temporary, application, cjson, sdk / "qumios-dev/include/common",
                    sdk / "qumios-dev/include/qmos", sdk / "qumios-dev/components/qm_event/include",
                    sdk / "qumios-dev/components/qm_pomodoro/include", sdk / "qumios-dev/components/cli"]
        command = compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread"]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
        for directory in includes:
            command += ["-I", str(directory)]
        json_object = temporary / "cJSON.o"
        # Only the unchanged upstream parser may use deprecated sprintf on macOS.
        # Keep strict diagnostics for the actual application, CLI and harness.
        subprocess.run(command + ["-Wno-deprecated-declarations", "-c", str(cjson / "cJSON.c"),
                                  "-o", str(json_object)], check=True)
        command += [str(source), str(cli_source), str(sdk / "qumios-dev/components/qm_pomodoro/src/qm_pomodoro.c"), str(fixture), str(json_object), "-o", str(binary)]
        subprocess.run(command, check=True)
        scenarios = [args.scenario] if args.scenario else scenarios
        for scenario in scenarios:
            env = os.environ.copy()
            if args.sanitize:
                env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
                env["UBSAN_OPTIONS"] = "halt_on_error=1"
            subprocess.run([str(binary), scenario], check=True, cwd=temporary, env=env, timeout=20)


if __name__ == "__main__":
    main()
