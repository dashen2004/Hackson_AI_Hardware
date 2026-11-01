#!/usr/bin/env python3
"""Run real RGB control, routing and property codec C code against host devices.

Hardware, mutexes, CLI transport, cloud and unrelated volume actions are supplied
by a host harness. The SDK's actual qm_spec codec remains in the test, including
its permissive unpackers. This checks software behavior, not physical lights,
platform speech routing, a complete ESP32 build, or a cloud/device connection.
Requires a POSIX host C compiler and pthreads (macOS or Linux).
"""

import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


QM_HEADER = r"""
#ifndef RGB_HOST_QM_H
#define RGB_HOST_QM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
typedef int qm_err_t;
typedef uint8_t bool_t;
typedef int qm_bool;
typedef struct { pthread_mutex_t native; int valid; } qm_mutex_t;
#define QM_TRUE 1
#define QM_FALSE 0
#define QM_EOK 0
#define QM_EIO 5
#define QM_ENOMEM 12
#define QM_EINVAL 22
#define QM_ENOSYS 38
#define QM_EINIT 48
#define QM_EBUSY 16
#define QM_WAIT_FOREVER (-1)
#define CONFIG_QM_CLI_SUPPORT 1
#define QM_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define QM_LOGI(...) ((void)0)
#define QM_LOGW(...) ((void)0)
#define QM_LOGE(...) ((void)0)
#define QM_LOGD(...) ((void)0)
#define cpu_to_be16(v) htons(v)
#define cpu_to_be32(v) htonl(v)
#define be16_to_cpu(v) ntohs(v)
#define be32_to_cpu(v) ntohl(v)
void *rgb_host_malloc(size_t size);
void rgb_host_free(void *pointer);
#define qm_malloc rgb_host_malloc
#define qm_free rgb_host_free
int qm_mutex_new(qm_mutex_t *mutex);
int qm_mutex_lock(qm_mutex_t *mutex, int timeout);
int qm_mutex_unlock(qm_mutex_t *mutex);
int qm_mutex_is_valid(qm_mutex_t *mutex);
#endif
"""

CLOUD_HEADER = r"""
#ifndef RGB_HOST_CLOUD_H
#define RGB_HOST_CLOUD_H
#include "qm_spec_api.h"
typedef void (*ljyd_cloud_prop_cb_t)(qm_spec_property_t *property);
typedef int (*ljyd_cloud_prop_get_cb_t)(
    const qm_spec_property_operation_t *requests,
    qm_spec_property_operation_t *responses);
void ljyd_cloud_set_prop_cb(ljyd_cloud_prop_cb_t callback);
void ljyd_cloud_set_prop_get_cb(ljyd_cloud_prop_get_cb_t callback);
void *ljyd_cloud_get_handle(void);
int qm_iot_report(void *handle, qm_spec_property_operation_t *operation);
#endif
"""

CLI_HEADER = r"""
#ifndef RGB_HOST_CLI_H
#define RGB_HOST_CLI_H
struct qm_cli_command {
    const char *name;
    const char *help;
    void (*function)(char *write_buffer, int length, int argc, char **argv);
};
int qm_cli_register_command(const struct qm_cli_command *command);
void qm_cli_printf(const char *format, ...);
#endif
"""

# The existing control cases never schedule focus work. The dedicated breathing
# harness supplies a deterministic queue/clock instead of these idle adapters.
WORK_HEADER = r"""
#ifndef RGB_HOST_WORK_H
#define RGB_HOST_WORK_H
#include "qm.h"
typedef struct {
    void (*fn)(void *);
    void *arg;
    uint32_t delay;
    int active;
} qm_work_t;
#ifdef RGB_HOST_FOCUS_TEST
uint32_t qm_now_ms(void);
int32_t qm_post_delayed_action(qm_work_t *, void (*)(void *), void *, uint32_t);
int32_t qm_cancel_delayed_action(qm_work_t *);
#else
static inline uint32_t qm_now_ms(void) { return 0U; }
static inline int32_t qm_post_delayed_action(qm_work_t *work,
    void (*fn)(void *), void *arg, uint32_t delay)
{
    work->fn = fn; work->arg = arg; work->delay = delay; work->active = 1;
    return QM_EOK;
}
static inline int32_t qm_cancel_delayed_action(qm_work_t *work)
{
    work->active = 0; return QM_EOK;
}
#endif
#endif
"""

TEST_SOURCE = r"""
#define _DEFAULT_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "qm.h"
#include "qm_spec_api.h"
#include "iot_control.h"
#include "iot_control_light_rgb.h"
#include "iot_control_light_rgb_cli.h"
#include "iot_control_focus.h"
#include "ljyd_cloud.h"
#include "qm_cli.h"

static ljyd_cloud_prop_cb_t set_callback;
static ljyd_cloud_prop_get_cb_t get_callback;
static int cloud_object;
static int have_cloud = 1;
static int mutex_failure;
static int hardware_failure;
static int report_failure;
static int malloc_after = -1;
static int live_allocations;
static int driver_calls;
static int fill_calls;
static int clear_calls;
static int report_calls;
static int volume_calls;
static int driver_active;
static int add_delay;
static int lock_calls;
static int cli_register_calls, cli_register_failure;
static const struct qm_cli_command *cli_command;
static char cli_output[4096];
static uint8_t physical_r, physical_g, physical_b;
static iot_control_light_rgb_state_t last_report;

static uint8_t scaled(uint8_t channel, uint8_t brightness)
{
    return (uint8_t)(((uint32_t)channel * brightness * 64U + 12750U) / 25500U);
}

static void assert_physical(const iot_control_light_rgb_state_t *state)
{
    uint8_t r = state->on ? scaled((state->color >> 16) & 255U, state->brightness) : 0;
    uint8_t g = state->on ? scaled((state->color >> 8) & 255U, state->brightness) : 0;
    uint8_t b = state->on ? scaled(state->color & 255U, state->brightness) : 0;
    assert(physical_r == r && physical_g == g && physical_b == b);
    assert(physical_r <= 64 && physical_g <= 64 && physical_b <= 64);
}

void *rgb_host_malloc(size_t size)
{
    if (malloc_after == 0) return NULL;
    if (malloc_after > 0) --malloc_after;
    void *pointer = malloc(size);
    if (pointer) __atomic_add_fetch(&live_allocations, 1, __ATOMIC_SEQ_CST);
    return pointer;
}

void rgb_host_free(void *pointer)
{
    if (pointer) {
        __atomic_sub_fetch(&live_allocations, 1, __ATOMIC_SEQ_CST);
        free(pointer);
    }
}

int qm_mutex_new(qm_mutex_t *mutex)
{
    if (mutex_failure) return mutex_failure;
    assert(!mutex->valid);
    assert(pthread_mutex_init(&mutex->native, NULL) == 0);
    mutex->valid = 1;
    return QM_EOK;
}

int qm_mutex_lock(qm_mutex_t *mutex, int timeout)
{
    assert(mutex && mutex->valid && timeout == QM_WAIT_FOREVER);
    assert(pthread_mutex_lock(&mutex->native) == 0);
    __atomic_add_fetch(&lock_calls, 1, __ATOMIC_SEQ_CST);
    return QM_EOK;
}

int qm_mutex_unlock(qm_mutex_t *mutex)
{
    assert(mutex && mutex->valid);
    assert(pthread_mutex_unlock(&mutex->native) == 0);
    return QM_EOK;
}

int qm_mutex_is_valid(qm_mutex_t *mutex) { return mutex && mutex->valid; }

static int hardware_apply(uint8_t r, uint8_t g, uint8_t b, int clear)
{
    assert(r <= 64 && g <= 64 && b <= 64);
    assert(__atomic_add_fetch(&driver_active, 1, __ATOMIC_SEQ_CST) == 1);
    __atomic_add_fetch(&driver_calls, 1, __ATOMIC_SEQ_CST);
    __atomic_add_fetch(clear ? &clear_calls : &fill_calls, 1, __ATOMIC_SEQ_CST);
    if (add_delay) usleep(100);
    int result = hardware_failure;
    if (!result) { physical_r = r; physical_g = g; physical_b = b; }
    else { physical_r = r; } /* Failed hardware may already have partly changed. */
    __atomic_sub_fetch(&driver_active, 1, __ATOMIC_SEQ_CST);
    return result;
}

int bsp_ws2812_fill(uint8_t r, uint8_t g, uint8_t b) { return hardware_apply(r, g, b, 0); }
int bsp_ws2812_clear(void) { return hardware_apply(0, 0, 0, 1); }
int bsp_ws2812_init(void) { return QM_EOK; }

void ljyd_cloud_set_prop_cb(ljyd_cloud_prop_cb_t callback) { set_callback = callback; }
void ljyd_cloud_set_prop_get_cb(ljyd_cloud_prop_get_cb_t callback) { get_callback = callback; }
void *ljyd_cloud_get_handle(void) { return have_cloud ? &cloud_object : NULL; }

int qm_cli_register_command(const struct qm_cli_command *command)
{
    ++cli_register_calls;
    if (cli_register_failure) return cli_register_failure;
    assert(command && strcmp(command->name, "rgb") == 0 && command->function);
    cli_command = command;
    return QM_EOK;
}

void qm_cli_printf(const char *format, ...)
{
    size_t used = strlen(cli_output);
    assert(used < sizeof(cli_output));
    va_list args;
    va_start(args, format);
    int n = vsnprintf(cli_output + used, sizeof(cli_output) - used, format, args);
    va_end(args);
    assert(n >= 0 && (size_t)n < sizeof(cli_output) - used);
}

static int number(qm_spec_property_t *property)
{
    int result = 0;
    assert(property && property->value.len == 4);
    assert(property->value.property_format == QM_SPEC_PROPERTY_FORMAT_NUMBER ||
           property->value.property_format == QM_SPEC_PROPERTY_FORMAT_INT32 ||
           property->value.property_format == QM_SPEC_PROPERTY_FORMAT_UINT32);
    assert(qm_spec_property_unpack_number(property, &result) == QM_EOK);
    return result;
}

static iot_control_light_rgb_state_t operation_state(qm_spec_property_operation_t *operation)
{
    iot_control_light_rgb_state_t state = {0};
    assert(operation && operation->element_num == 3);
    qm_spec_property_t *p = qm_spec_property_find(operation, 4, 1);
    assert(p && p->value.property_format == QM_SPEC_PROPERTY_FORMAT_BOOL && p->value.len == 1);
    assert(qm_spec_property_unpack_bool(p, &state.on) == QM_EOK);
    state.brightness = (uint8_t)number(qm_spec_property_find(operation, 4, 2));
    state.color = (uint32_t)number(qm_spec_property_find(operation, 4, 3));
    assert(state.on <= 1 && state.brightness >= 1 && state.brightness <= 100 && state.color <= 0xFFFFFF);
    return state;
}

int qm_iot_report(void *handle, qm_spec_property_operation_t *operation)
{
    assert(handle == &cloud_object);
    iot_control_light_rgb_state_t state = operation_state(operation);
    /* This runs while the real module serializes execution, commit and report. */
    assert_physical(&state);
    last_report = state;
    __atomic_add_fetch(&report_calls, 1, __ATOMIC_SEQ_CST);
    if (report_failure) return report_failure; /* Failure retains caller ownership. */
    assert(qm_spec_property_operation_delete(operation) == QM_EOK);
    return QM_EOK;
}

int iot_control_volume_set_value(qm_spec_property_t *property)
{
    assert(property && property->siid == 2 && property->piid == 1);
    ++volume_calls;
    return QM_EOK;
}

int iot_control_volume_set_adjust(qm_spec_property_t *property)
{
    assert(property && property->siid == 2 && property->piid == 2);
    ++volume_calls;
    return QM_EOK;
}

#ifndef RGB_HOST_FOCUS_CONTROL_TEST
/* Focus has its own real adapter/codec suite; unrelated legacy RGB cases keep
 * an unavailable timer while preserving routing/link dependencies. */
int iot_control_focus_get_state(iot_control_focus_state_t *state)
{ (void)state; return -QM_EINIT; }
int iot_control_focus_pack_property(qm_spec_property_t *property,
                                   const iot_control_focus_state_t *state)
{ (void)property; (void)state; return -QM_ENOSYS; }
int iot_control_focus_set_duration(qm_spec_property_t *property)
{ (void)property; return -QM_ENOSYS; }
int iot_control_focus_set_plugin_start(qm_spec_property_t *property)
{ (void)property; return -QM_ENOSYS; }
int iot_control_focus_set_action(qm_spec_property_t *property)
{ (void)property; return -QM_ENOSYS; }
#endif

static qm_spec_property_t value(uint8_t siid, uint16_t piid, int integer)
{
    qm_spec_property_t result = {0};
    assert(qm_spec_property_pack_xiid(&result, siid, piid) == QM_EOK);
    assert(qm_spec_property_pack_number(&result, integer) == QM_EOK);
    return result;
}

static qm_spec_property_t boolean(int on)
{
    qm_spec_property_t result = {0};
    assert(qm_spec_property_pack_xiid(&result, 4, 1) == QM_EOK);
    assert(qm_spec_property_pack_bool(&result, (bool_t)on) == QM_EOK);
    return result;
}

static int set_number(uint16_t piid, int integer)
{
    qm_spec_property_t p = value(4, piid, integer);
    return piid == 2 ? iot_control_light_rgb_set_brightness(&p) : iot_control_light_rgb_set_color(&p);
}

static int set_on(int on)
{
    qm_spec_property_t p = boolean(on);
    return iot_control_light_rgb_set_onoff(&p);
}

static iot_control_light_rgb_state_t snapshot(void)
{
    iot_control_light_rgb_state_t result;
    assert(iot_control_light_rgb_get_state(&result) == QM_EOK);
    return result;
}

static void expect_state(int on, int brightness, uint32_t color)
{
    iot_control_light_rgb_state_t state = snapshot();
    assert(state.on == on && state.brightness == brightness && state.color == color);
    assert_physical(&state);
    assert(live_allocations == 0);
}

static void expect_unknown(void)
{
    iot_control_light_rgb_state_t output, before;
    memset(&output, 0xA5, sizeof(output)); before = output;
    assert(iot_control_light_rgb_get_state(&output) == -QM_EIO);
    assert(memcmp(&output, &before, sizeof(output)) == 0);
    assert(live_allocations == 0);
}

static void initialize(void)
{
    assert(iot_control_light_rgb_init() == QM_EOK);
    expect_state(1, 25, 0x00FF00);
    assert(physical_r == 0 && physical_g == 16 && physical_b == 0);
}

static void test_initialization(void)
{
    iot_control_light_rgb_state_t state;
    qm_spec_property_t p = boolean(0);
    assert(iot_control_light_rgb_get_state(NULL) == -QM_EINVAL);
    assert(iot_control_light_rgb_get_state(&state) == -QM_EINIT);
    assert(iot_control_light_rgb_set_onoff(&p) == -QM_EINIT);
    assert(driver_calls == 0 && report_calls == 0);
    mutex_failure = -73;
    assert(iot_control_light_rgb_init() == -73);
    assert(driver_calls == 0);
    mutex_failure = 0;
    hardware_failure = -74;
    assert(iot_control_light_rgb_init() == -74);
    assert(iot_control_light_rgb_get_state(&state) == -QM_EINIT);
    hardware_failure = 0;
    initialize();
    int before = driver_calls;
    assert(iot_control_light_rgb_init() == QM_EOK && driver_calls == before);
    puts("PASS: initialization failure is retryable and starts at existing green 16");
}

static void test_valid_controls(void)
{
    initialize();
    assert(set_number(2, 100) == QM_EOK);
    expect_state(1, 100, 0x00FF00);
    assert(physical_g == 64);
    assert(set_number(3, 0xFFFFFF) == QM_EOK);
    expect_state(1, 100, 0xFFFFFF);
    assert(physical_r == 64 && physical_g == 64 && physical_b == 64);
    assert(set_number(3, 0x804020) == QM_EOK);
    expect_state(1, 100, 0x804020);
    assert(physical_r == 32 && physical_g == 16 && physical_b == 8);
    assert(set_number(3, 0xFF0000) == QM_EOK);
    assert(set_number(2, 1) == QM_EOK);
    expect_state(1, 1, 0xFF0000);
    assert(physical_r == 1 && physical_g == 0 && physical_b == 0);
    assert(set_on(0) == QM_EOK);
    int fills = fill_calls;
    assert(set_number(2, 70) == QM_EOK && set_number(3, 0x0000FF) == QM_EOK);
    expect_state(0, 70, 0x0000FF);
    assert(fill_calls == fills && physical_r == 0 && physical_g == 0 && physical_b == 0);
    assert(set_on(1) == QM_EOK);
    expect_state(1, 70, 0x0000FF);
    assert(last_report.on && last_report.brightness == 70 && last_report.color == 0x0000FF);
    qm_spec_property_t p = {0};
    qm_spec_property_pack_xiid(&p, 4, 2);
    qm_spec_property_pack_uint8(&p, 50);
    assert(iot_control_light_rgb_set_brightness(&p) == QM_EOK);
    expect_state(1, 50, 0x0000FF);
    p = value(4, 3, 0);
    qm_spec_property_pack_uint32(&p, 0x102030);
    assert(iot_control_light_rgb_set_color(&p) == QM_EOK);
    expect_state(1, 50, 0x102030);
    assert(set_number(3, 0) == QM_EOK);
    expect_state(1, 50, 0);
    puts("PASS: switch/brightness/color drive bounded RGB and off-state edits stay dark");
}

static void test_invalid_controls(void)
{
    initialize();
    int drives = driver_calls, reports = report_calls;
    qm_spec_property_t p = value(4, 1, 1);
    assert(iot_control_light_rgb_set_onoff(&p) == -QM_EINVAL);
    p = boolean(2);
    assert(iot_control_light_rgb_set_onoff(&p) == -QM_EINVAL);
    p = boolean(1); p.value.len = 2;
    assert(iot_control_light_rgb_set_onoff(&p) == -QM_EINVAL);
    p = boolean(1); p.piid = 2;
    assert(iot_control_light_rgb_set_brightness(&p) == -QM_EINVAL);
    p = value(4, 2, 50); p.value.property_format = QM_SPEC_PROPERTY_FORMAT_STRING;
    assert(iot_control_light_rgb_set_brightness(&p) == -QM_EINVAL);
    p = value(4, 2, 50); p.value.property_format = QM_SPEC_PROPERTY_FORMAT_FLOAT32;
    assert(iot_control_light_rgb_set_brightness(&p) == -QM_EINVAL);
    p = value(4, 3, 0);
    qm_spec_property_pack_uint32(&p, 0xFFFFFFFF);
    assert(iot_control_light_rgb_set_color(&p) == -QM_EINVAL);
    for (int len = 0; len <= 5; ++len) {
        if (len == 4) continue;
        p = value(4, 2, 50); p.value.len = (uint16_t)len;
        assert(iot_control_light_rgb_set_brightness(&p) == -QM_EINVAL);
    }
    assert(set_number(2, 0) == -QM_EINVAL && set_number(2, 101) == -QM_EINVAL);
    assert(set_number(2, -1) == -QM_EINVAL);
    assert(set_number(3, -1) == -QM_EINVAL && set_number(3, 0x1000000) == -QM_EINVAL);
    assert(iot_control_light_rgb_set_onoff(NULL) == -QM_EINVAL);
    assert(iot_control_light_rgb_set_brightness(NULL) == -QM_EINVAL);
    assert(iot_control_light_rgb_set_color(NULL) == -QM_EINVAL);
    assert(driver_calls == drives && report_calls == reports);
    expect_state(1, 25, 0x00FF00);
    puts("PASS: SDK permissive unpackers cannot bypass RGB type/length/range checks");
}

static void test_device_failure(void)
{
    initialize();
    int reports = report_calls;
    hardware_failure = -75;
    assert(set_number(2, 50) == -75);
    expect_unknown();
    assert(set_number(2, 0) == -QM_EINVAL); /* Validation cannot certify hardware. */
    expect_unknown();
    assert(report_calls == reports);
    hardware_failure = 0;
    assert(set_number(3, 0xFF0000) == QM_EOK);
    expect_state(1, 25, 0xFF0000); /* Failed brightness was not committed. */
    reports = report_calls;
    hardware_failure = -75;
    assert(set_number(3, 0x0000FF) == -75);
    expect_unknown();
    assert(report_calls == reports);
    hardware_failure = 0;
    assert(set_number(2, 50) == QM_EOK);
    expect_state(1, 50, 0xFF0000); /* Failed color was not committed. */
    reports = report_calls;
    hardware_failure = -75;
    assert(set_on(0) == -75);
    expect_unknown();
    assert(report_calls == reports);
    hardware_failure = 0;
    assert(set_number(3, 0x00FF00) == QM_EOK);
    expect_state(1, 50, 0x00FF00); /* Failed off was not committed. */
    assert(set_on(0) == QM_EOK);
    reports = report_calls;
    hardware_failure = -76;
    assert(set_number(2, 80) == -76);
    expect_unknown();
    assert(report_calls == reports);
    hardware_failure = 0;
    assert(set_number(3, 0xFF0000) == QM_EOK);
    expect_state(0, 50, 0xFF0000);
    reports = report_calls;
    hardware_failure = -76;
    assert(set_number(3, 0x0000FF) == -76);
    expect_unknown();
    assert(report_calls == reports);
    hardware_failure = 0;
    assert(set_number(2, 25) == QM_EOK);
    expect_state(0, 25, 0xFF0000);
    puts("PASS: hardware errors retain intent, suppress stale GET/report and recover on a full apply");
}

static void test_reporting_failure(void)
{
    initialize();
    report_failure = -77;
    assert(set_number(2, 90) == -77);
    expect_state(1, 90, 0x00FF00);
    report_failure = 0;
    for (int fail = 0; fail < 4; ++fail) {
        int reports = report_calls;
        malloc_after = fail;
        assert(set_number(2, 50 + fail) == -QM_ENOMEM);
        malloc_after = -1;
        expect_state(1, 50 + fail, 0x00FF00);
        assert(report_calls == reports && live_allocations == 0);
    }
    assert(set_number(2, 75) == QM_EOK);
    expect_state(1, 75, 0x00FF00);
    have_cloud = 0;
    assert(set_number(3, 0xFF0000) == -QM_EINIT);
    expect_state(1, 75, 0xFF0000);
    have_cloud = 1;
    puts("PASS: report/allocation failures preserve applied hardware and free ownership");
}

static void test_routing_and_get(void)
{
    assert(iot_control_init(NULL) == -QM_EINVAL);
    assert(!set_callback && !get_callback);
    assert(iot_control_init(&cloud_object) == QM_EOK);
    assert(set_callback && get_callback);
    expect_state(1, 25, 0x00FF00);
    qm_spec_property_t p = value(2, 1, 50); set_callback(&p);
    p = value(2, 2, 1); set_callback(&p);
    assert(volume_calls == 2);
    int drives = driver_calls, reports = report_calls;
    p = value(3, 1, 1); set_callback(&p);
    p = value(4, 99, 1); set_callback(&p);
    p = value(9, 1, 90); set_callback(&p);
    set_callback(NULL);
    assert(driver_calls == drives && report_calls == reports);
    p = value(4, 3, 0x123456); set_callback(&p);
    expect_state(1, 25, 0x123456);
    p = boolean(0); set_callback(&p);
    expect_state(0, 25, 0x123456);
    qm_spec_property_operation_t *requests = qm_spec_property_operation_creat();
    qm_spec_property_operation_t *responses = qm_spec_property_operation_creat();
    assert(requests && responses);
    for (int piid = 1; piid <= 3; ++piid) {
        qm_spec_property_t *item = qm_spec_property_creat(); assert(item);
        qm_spec_property_pack_xiid(item, 4, (uint16_t)piid);
        assert(qm_spec_property_add(requests, item) == QM_EOK);
    }
    int locks = lock_calls;
    assert(get_callback(requests, responses) == QM_EOK);
    assert(lock_calls == locks + 1); /* One committed snapshot for all properties. */
    iot_control_light_rgb_state_t got = operation_state(responses);
    assert(got.on == 0 && got.brightness == 25 && got.color == 0x123456);
    assert(driver_calls == drives + 2 && report_calls == reports + 2);
    qm_spec_property_operation_delete(requests); qm_spec_property_operation_delete(responses);
    requests = qm_spec_property_operation_creat(); responses = qm_spec_property_operation_creat();
    for (int service = 3; service <= 4; ++service) {
        qm_spec_property_t *item = qm_spec_property_creat(); assert(item);
        qm_spec_property_pack_xiid(item, (uint8_t)service, 99);
        assert(qm_spec_property_add(requests, item) == QM_EOK);
    }
    assert(get_callback(requests, responses) == QM_EOK);
    assert(responses->element_num == 2);
    for (qm_spec_property_t *item = responses->property; item; item = item->next) {
        assert(item->code == -QM_ENOSYS && item->value.len == 0);
    }
    qm_spec_property_operation_delete(requests); qm_spec_property_operation_delete(responses);
    hardware_failure = -78;
    assert(set_number(3, 0xFF0000) == -78);
    expect_unknown();
    requests = qm_spec_property_operation_creat(); responses = qm_spec_property_operation_creat();
    for (int piid = 1; piid <= 3; ++piid) {
        qm_spec_property_t *item = qm_spec_property_creat(); assert(item);
        qm_spec_property_pack_xiid(item, 4, (uint16_t)piid);
        assert(qm_spec_property_add(requests, item) == QM_EOK);
    }
    assert(get_callback(requests, responses) == QM_EOK);
    assert(responses->element_num == 3);
    for (qm_spec_property_t *item = responses->property; item; item = item->next) {
        assert(item->code == -QM_EIO && item->value.len == 0);
    }
    qm_spec_property_operation_delete(responses);
    hardware_failure = 0;
    assert(set_number(2, 50) == QM_EOK);
    iot_control_light_rgb_state_t recovered = snapshot();
    assert(!recovered.on && recovered.brightness == 50 && recovered.color == 0x123456);
    assert_physical(&recovered);
    responses = qm_spec_property_operation_creat();
    malloc_after = 1; /* Cloud owns and must destroy the partially filled operation. */
    assert(get_callback(requests, responses) == -QM_ENOMEM);
    malloc_after = -1;
    assert(responses->element_num == 1);
    qm_spec_property_operation_delete(requests); qm_spec_property_operation_delete(responses);
    assert(live_allocations == 0);
    puts("PASS: RGB/volume routes stay separate and GET returns committed state or errors");
}

static void *writer(void *parameter)
{
    intptr_t kind = (intptr_t)parameter;
    for (int i = 0; i < 60; ++i) {
        if (kind == 0) assert(set_number(2, 1 + i % 100) == QM_EOK);
        else if (kind == 1) assert(set_number(3, (i & 1) ? 0xFF0000 : 0x0000FF) == QM_EOK);
        else assert(set_on(i & 1) == QM_EOK);
        iot_control_light_rgb_state_t state = snapshot();
        assert(state.on <= 1 && state.brightness >= 1 && state.brightness <= 100 && state.color <= 0xFFFFFF);
    }
    return NULL;
}

static void test_concurrency(void)
{
    initialize(); add_delay = 1;
    pthread_t workers[3];
    for (intptr_t i = 0; i < 3; ++i) assert(pthread_create(&workers[i], NULL, writer, (void *)i) == 0);
    for (int i = 0; i < 3; ++i) assert(pthread_join(workers[i], NULL) == 0);
    iot_control_light_rgb_state_t state = snapshot(); assert_physical(&state);
    assert(live_allocations == 0 && driver_active == 0);
    puts("PASS: concurrent setters serialize driver/commit/report and expose valid snapshots");
}

static void cli(int argc, char **argv)
{
    assert(cli_command && cli_command->function);
    cli_output[0] = '\0';
    cli_command->function(NULL, 0, argc, argv);
}

static void test_cli(void)
{
    cli_register_failure = -91;
    assert(iot_control_light_rgb_cli_init() == -91 && !cli_command);
    cli_register_failure = 0;
    assert(iot_control_light_rgb_cli_init() == QM_EOK);
    int registrations = cli_register_calls;
    assert(iot_control_light_rgb_cli_init() == QM_EOK && cli_register_calls == registrations);
    assert(iot_control_init(&cloud_object) == QM_EOK);
    char *status[] = {"rgb", "status"};
    char *color[] = {"rgb", "color", "FF0000"};
    char *off[] = {"rgb", "off"};
    char *on[] = {"rgb", "on"};
    char *bright[] = {"rgb", "brightness", "80"};
    int drives = driver_calls, reports = report_calls;
    cli(2, status);
    assert(strstr(cli_output, "on=1 brightness=25 color=00FF00 channel_cap=64"));
    assert(driver_calls == drives && report_calls == reports);
    cli(3, color); expect_state(1, 25, 0xFF0000);
    assert(strstr(cli_output, "rgb source=cli piid=3 ret=0"));
    cli(2, off); cli(3, bright); expect_state(0, 80, 0xFF0000);
    assert(strstr(cli_output, "rgb source=cli piid=2 ret=0"));
    drives = driver_calls; reports = report_calls;
    const char *invalid[][3] = {
        {"rgb", "brightness", "0"}, {"rgb", "brightness", "101"},
        {"rgb", "brightness", "-1"}, {"rgb", "brightness", "+1"},
        {"rgb", "brightness", "50oops"}, {"rgb", "brightness", "1.0"},
        {"rgb", "brightness", "999999999999999999999999999999999999"},
        {"rgb", "color", "1000000"}, {"rgb", "color", "-ff"},
        {"rgb", "color", "ffzz"}, {"rgb", "on", "extra"},
    };
    for (size_t i = 0; i < QM_ARRAY_SIZE(invalid); ++i) {
        char *args[] = {(char *)invalid[i][0], (char *)invalid[i][1], (char *)invalid[i][2]};
        cli(3, args); assert(strstr(cli_output, "usage:"));
    }
    char *empty[] = {"rgb"}; cli(1, empty);
    assert(strstr(cli_output, "usage:"));
    assert(driver_calls == drives && report_calls == reports && live_allocations == 0);
    report_failure = -77;
    bright[2] = "55"; cli(3, bright);
    assert(strstr(cli_output, "piid=2 ret=-77"));
    expect_state(0, 55, 0xFF0000);
    report_failure = 0;
    drives = driver_calls; reports = report_calls;
    malloc_after = 0; cli(2, on); malloc_after = -1;
    assert(strstr(cli_output, "allocation failed"));
    assert(driver_calls == drives && report_calls == reports);
    hardware_failure = -78; cli(2, on);
    assert(strstr(cli_output, "piid=1 ret=-78") && strstr(cli_output, "status unavailable"));
    expect_unknown();
    hardware_failure = 0; cli(2, off); expect_state(0, 55, 0xFF0000);
    puts("PASS: actual CLI parses commands, uses shared routes and distinguishes apply/report errors");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "initialization")) test_initialization();
    else if (!strcmp(argv[1], "controls")) test_valid_controls();
    else if (!strcmp(argv[1], "invalid")) test_invalid_controls();
    else if (!strcmp(argv[1], "device-failure")) test_device_failure();
    else if (!strcmp(argv[1], "report-failure")) test_reporting_failure();
    else if (!strcmp(argv[1], "routing-get")) test_routing_and_get();
    else if (!strcmp(argv[1], "concurrency")) test_concurrency();
    else if (!strcmp(argv[1], "cli")) test_cli();
    else assert(!"unknown test case");
    return 0;
}
"""

CASES = ("initialization", "controls", "invalid", "device-failure",
         "report-failure", "routing-get", "concurrency", "cli")


def main():
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", type=Path,
                        default=root / "docs/开发/ticko-物模型-v3-11服务-20261031-脱敏.json")
    parser.add_argument("--case", action="append", choices=CASES,
                        help="Run only named cases (repeatable; default: all)")
    args = parser.parse_args()
    if os.name == "nt":
        parser.error("Run this pthread host harness on macOS or Linux")
    model = json.loads(args.model.read_text(encoding="utf-8"))
    service = next(x for x in model["services"] if x["serviceName"] == "lightRGB")
    assert service["siid"] == 4
    assert {x["description"]: x["piid"] for x in service["properties"]} == {
        "OnOff": 1, "Brightness": 2, "Color": 3,
    }
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not compiler or not shutil.which(compiler[0]):
        parser.error("A host C compiler is required (set CC if necessary)")
    firmware = root / "src/firmware"
    controls = firmware / "apps/main/lj_components/iot_control"
    spec = firmware / "qumios-dev/components/qm_spec"
    sources = [controls / "iot_control.c", controls / "ctrl/iot_control_light_rgb.c",
               controls / "ctrl/iot_control_light_rgb_cli.c"]
    for path in sources + [spec / "src/qm_spec_api.c"]:
        if not path.is_file():
            parser.error(f"Required SDK source is unavailable: {path}")
    with tempfile.TemporaryDirectory(prefix="light-rgb-control-test-") as folder:
        temp = Path(folder)
        (temp / "qm.h").write_text(QM_HEADER)
        for header in ("qm_types.h", "qm_log.h"):
            (temp / header).write_text('#include "qm.h"\n')
        for header in ("qm_spec_api.h", "qm_spec_core.h", "qm_spec_log.h"):
            shutil.copyfile(spec / "include" / header, temp / header)
        shutil.copyfile(spec / "src/qm_spec_api.c", temp / "qm_spec_api.c")
        shutil.copyfile(firmware / "apps/main/peripheral/bsp_ws2812.h", temp / "bsp_ws2812.h")
        (temp / "ljyd_cloud.h").write_text(CLOUD_HEADER)
        (temp / "qm_cli.h").write_text(CLI_HEADER)
        (temp / "qm_work.h").write_text(WORK_HEADER)
        (temp / "qm_iot_api.h").write_text('#include "ljyd_cloud.h"\n')
        (temp / "test_light_rgb.c").write_text(TEST_SOURCE)
        binary = temp / ("test-light-rgb.exe" if os.name == "nt" else "test-light-rgb")
        command = compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread",
                              "-I", str(temp), "-I", str(controls), "-I", str(controls / "ctrl")]
        command += [str(x) for x in sources] + [str(temp / "qm_spec_api.c"),
                    str(temp / "test_light_rgb.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        print("Contract: lightRGB=4, switch=1, brightness=2, color=3; real SDK property codec", flush=True)
        for name in args.case or CASES:
            subprocess.run([str(binary), name], check=True, cwd=temp, timeout=20)


if __name__ == "__main__":
    main()
