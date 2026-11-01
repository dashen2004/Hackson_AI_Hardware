#!/usr/bin/env python3
"""Exercise real volume routing with the SDK's actual property codec.

Audio, KV and cloud I/O are mocked. qm_spec_api.c and its headers are copied
unchanged into a temporary host build, so integer widths, byte order and bad
format/length rejection are not simulated. No board, credentials or network.
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
#ifndef VOLUME_HOST_QM_H
#define VOLUME_HOST_QM_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "qm_errno.h"
typedef uint8_t bool_t;
typedef int qm_err_t;
#define QM_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define cpu_to_be16(v) htons(v)
#define cpu_to_be32(v) htonl(v)
#define be16_to_cpu(v) ntohs(v)
#define be32_to_cpu(v) ntohl(v)
void volume_host_log(const char *format, ...);
#define QM_LOGI(tag, ...) volume_host_log(__VA_ARGS__)
#define QM_LOGW(tag, ...) volume_host_log(__VA_ARGS__)
#define QM_LOGE(tag, ...) volume_host_log(__VA_ARGS__)
#define QM_LOGD(tag, ...) volume_host_log(__VA_ARGS__)
void *volume_host_malloc(size_t size);
void volume_host_free(void *pointer);
#define qm_malloc volume_host_malloc
#define qm_free volume_host_free
#endif
"""

IO_HEADER = r"""
#ifndef VOLUME_HOST_IO_H
#define VOLUME_HOST_IO_H
#include "qm.h"
#include "qm_spec_api.h"
typedef void (*property_callback_t)(qm_spec_property_t *);
typedef int (*property_get_callback_t)(const qm_spec_property_operation_t *,
                                      qm_spec_property_operation_t *);
void ljyd_cloud_set_prop_cb(property_callback_t callback);
void ljyd_cloud_set_prop_get_cb(property_get_callback_t callback);
void *ljyd_cloud_get_handle(void);
int voice_audio_ready(void);
int qm_audio_set_volume(uint8_t volume);
int qm_kv_get(const char *key, void *value, int *length);
int qm_kv_set(const char *key, const void *value, int length, int save);
int qm_iot_report(void *handle, qm_spec_property_operation_t *operation);
#endif
"""

TEST_SOURCE = r"""
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "host_io.h"
#include "iot_control.h"
#include "iot_control_volume.h"
#include "iot_control_light_rgb.h"
#include "iot_control_focus.h"

static property_callback_t set_callback;
static int cloud_object;
static int ready = 1, have_cloud = 1, audio_result, kv_result, report_result;
static int audio_calls, save_calls, report_calls, successful_reports, rgb_calls;
static int hw_success_logs, hw_failed_logs, save_failed_logs;
static int hardware_volume = -1, persisted_volume = -1, persisted_len = 1;
static int reported_volume = -1, reported_siid = -1, reported_piid = -1;
static int live_allocations, allocation_call, fail_allocation = -1;

void volume_host_log(const char *format, ...)
{
    if (strcmp(format, "hw vol=%u") == 0) ++hw_success_logs;
    if (strstr(format, "hw volume failed")) ++hw_failed_logs;
    if (strstr(format, "kv save failed")) ++save_failed_logs;
}
void *volume_host_malloc(size_t size)
{
    int call = allocation_call++;
    if (call == fail_allocation) return NULL;
    void *p = malloc(size);
    if (p) ++live_allocations;
    return p;
}
void volume_host_free(void *p)
{
    if (p) { assert(live_allocations > 0); --live_allocations; free(p); }
}
void ljyd_cloud_set_prop_cb(property_callback_t cb) { set_callback = cb; }
void ljyd_cloud_set_prop_get_cb(property_get_callback_t cb) { assert(cb == iot_control_on_get); }
void *ljyd_cloud_get_handle(void) { return have_cloud ? &cloud_object : NULL; }
int voice_audio_ready(void) { return ready; }
int qm_audio_set_volume(uint8_t volume)
{
    ++audio_calls;
    if (audio_result == QM_EOK) hardware_volume = volume;
    return audio_result;
}
int qm_kv_get(const char *key, void *value, int *length)
{
    assert(strcmp(key, "volume") == 0 && *length == 1);
    if (persisted_volume < 0) return -QM_ERROR;
    *(uint8_t *)value = (uint8_t)persisted_volume;
    *length = persisted_len;
    return QM_EOK;
}
int qm_kv_set(const char *key, const void *value, int length, int save)
{
    assert(strcmp(key, "volume") == 0 && length == 1 && save == 1);
    ++save_calls;
    if (kv_result == QM_EOK) persisted_volume = *(const uint8_t *)value;
    return kv_result;
}
int qm_iot_report(void *handle, qm_spec_property_operation_t *op)
{
    assert(handle == &cloud_object && op && op->property && !op->property->next);
    ++report_calls;
    if (report_result != QM_EOK) return report_result;
    int value = -1;
    assert(qm_spec_property_unpack_number(op->property, &value) == QM_EOK);
    reported_volume = value;
    reported_siid = op->property->siid;
    reported_piid = op->property->piid;
    ++successful_reports;
    assert(qm_spec_property_operation_delete(op) == QM_EOK);
    return QM_EOK;
}
/* RGB implementation has its own real driver/codec regression suite. Here we
 * check that adding the legacy volume rows leaves all three RGB routes intact. */
int iot_control_light_rgb_init(void) { return QM_EOK; }
int iot_control_light_rgb_get_state(iot_control_light_rgb_state_t *s)
{ (void)s; return -QM_EINIT; }
int iot_control_light_rgb_pack_property(qm_spec_property_t *p,
                                      const iot_control_light_rgb_state_t *s)
{ (void)p; (void)s; return -QM_ENOSYS; }
int iot_control_light_rgb_set_onoff(qm_spec_property_t *p)
{ assert(p->siid == 4 && p->piid == 1); ++rgb_calls; return -QM_EIO; }
int iot_control_light_rgb_set_brightness(qm_spec_property_t *p)
{ assert(p->siid == 4 && p->piid == 2); ++rgb_calls; return -QM_EIO; }
int iot_control_light_rgb_set_color(qm_spec_property_t *p)
{ assert(p->siid == 4 && p->piid == 3); ++rgb_calls; return -QM_EIO; }

/* The focus adapter has an independent real-code suite. */
int iot_control_focus_get_state(iot_control_focus_state_t *s)
{ (void)s; return -QM_EINIT; }
int iot_control_focus_pack_property(qm_spec_property_t *p,
                                   const iot_control_focus_state_t *s)
{ (void)p; (void)s; return -QM_ENOSYS; }
int iot_control_focus_set_duration(qm_spec_property_t *p)
{ (void)p; return -QM_ENOSYS; }
int iot_control_focus_set_plugin_start(qm_spec_property_t *p)
{ (void)p; return -QM_ENOSYS; }
int iot_control_focus_set_action(qm_spec_property_t *p)
{ (void)p; return -QM_ENOSYS; }

static qm_spec_property_t property(int siid, int piid, int format, int64_t value)
{
    qm_spec_property_t p = {0};
    assert(qm_spec_property_pack_xiid(&p, (uint8_t)siid, (uint16_t)piid) == QM_EOK);
    int ret = -QM_EINVAL;
    switch (format) {
    case QM_SPEC_PROPERTY_FORMAT_NUMBER: ret = qm_spec_property_pack_number(&p, (int)value); break;
    case QM_SPEC_PROPERTY_FORMAT_INT32: ret = qm_spec_property_pack_int32(&p, (int32_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_UINT32: ret = qm_spec_property_pack_uint32(&p, (uint32_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_INT16: ret = qm_spec_property_pack_int16(&p, (int16_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_UINT16: ret = qm_spec_property_pack_uint16(&p, (uint16_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_INT8: ret = qm_spec_property_pack_int8(&p, (int8_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_UINT8: ret = qm_spec_property_pack_uint8(&p, (uint8_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_BOOL: ret = qm_spec_property_pack_bool(&p, (bool_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_FLOAT32: ret = qm_spec_property_pack_float32(&p, (float)value); break;
    default: assert(0);
    }
    assert(ret == QM_EOK);
    return p;
}
static int dispatch(qm_spec_property_t *p)
{
    qm_spec_property_t before = *p;
    p->code = 99;
    set_callback(p);
    assert(p->siid == before.siid && p->piid == before.piid);
    assert(memcmp(&p->value, &before.value, sizeof(p->value)) == 0);
    return p->code;
}
static int number(int siid, int piid, int value)
{
    qm_spec_property_t p = property(siid, piid, QM_SPEC_PROPERTY_FORMAT_NUMBER, value);
    return dispatch(&p);
}
static void expect_volume(int value)
{
    assert(hardware_volume == value && persisted_volume == value);
    assert(reported_volume == value && reported_siid == 2 && reported_piid == 1);
    assert(live_allocations == 0);
}
static void initialize(void)
{
    assert(iot_control_init(NULL) == -QM_EINVAL && !set_callback);
    assert(iot_control_init(&cloud_object) == QM_EOK && set_callback);
}
static void rejected(qm_spec_property_t *p)
{
    int audio = audio_calls, saves = save_calls, reports = report_calls;
    assert(dispatch(p) == -QM_EINVAL);
    assert(audio_calls == audio && save_calls == saves && report_calls == reports);
}
static void test_absolute(void)
{
    initialize();
    assert(number(PLATFORM_VOLUME_SIID, PLATFORM_VOLUME_PIID, 50) == QM_EOK);
    expect_volume(50);
    const int formats[] = {QM_SPEC_PROPERTY_FORMAT_NUMBER, QM_SPEC_PROPERTY_FORMAT_INT32,
        QM_SPEC_PROPERTY_FORMAT_UINT8, QM_SPEC_PROPERTY_FORMAT_UINT16,
        QM_SPEC_PROPERTY_FORMAT_UINT32, QM_SPEC_PROPERTY_FORMAT_INT8,
        QM_SPEC_PROPERTY_FORMAT_INT16};
    for (int siid = 2; siid <= 3; ++siid) {
        for (size_t i = 0; i < QM_ARRAY_SIZE(formats); ++i) {
            qm_spec_property_t p = property(siid, 1, formats[i], 37 + (int)i);
            assert(dispatch(&p) == QM_EOK);
            expect_volume(37 + (int)i);
        }
    }
    puts("PASS: primary/legacy absolute values use real signed/unsigned property codecs and report 2.1");
}
static void test_relative(void)
{
    initialize();
    for (int siid = 2; siid <= 3; ++siid) {
        assert(number(siid, 1, 50) == QM_EOK);
        qm_spec_property_t p = property(siid, 2, QM_SPEC_PROPERTY_FORMAT_UINT16, 1);
        assert(dispatch(&p) == QM_EOK); expect_volume(60);
        p = property(siid, 2, QM_SPEC_PROPERTY_FORMAT_UINT32, 2);
        assert(dispatch(&p) == QM_EOK); expect_volume(50);
        assert(number(siid, 2, 3) == QM_EOK); expect_volume(100);
        assert(number(siid, 2, 1) == QM_EOK); expect_volume(100);
        assert(number(siid, 2, 4) == QM_EOK); expect_volume(10);
        assert(number(siid, 2, 2) == QM_EOK); expect_volume(10);
    }
    puts("PASS: both SIIDs preserve increment/decrement 10 and saturating min/max");
}
static void test_bounds(void)
{
    initialize();
    const int inputs[] = {INT_MIN, -1, 0, 9, 10, 100, 101, INT_MAX};
    const int expected[] = {10, 10, 10, 10, 10, 100, 100, 100};
    for (int siid = 2; siid <= 3; ++siid) {
        for (size_t i = 0; i < QM_ARRAY_SIZE(inputs); ++i) {
            assert(number(siid, 1, inputs[i]) == QM_EOK); expect_volume(expected[i]);
        }
        qm_spec_property_t p = property(siid, 1, QM_SPEC_PROPERTY_FORMAT_UINT32, UINT32_MAX);
        assert(dispatch(&p) == QM_EOK); expect_volume(100);
    }
    puts("PASS: bounds retain clamp 10..100, including unsigned values above INT32_MAX");
}
static void test_types(void)
{
    initialize();
    assert(number(2, 1, 50) == QM_EOK);
    for (int siid = 2; siid <= 3; ++siid) {
        for (int piid = 1; piid <= 2; ++piid) {
            qm_spec_property_t p = property(siid, piid, QM_SPEC_PROPERTY_FORMAT_BOOL, 1);
            rejected(&p);
            p = property(siid, piid, QM_SPEC_PROPERTY_FORMAT_FLOAT32, 50);
            rejected(&p);
            p = (qm_spec_property_t){0};
            assert(qm_spec_property_pack_xiid(&p, (uint8_t)siid, (uint16_t)piid) == QM_EOK);
            assert(qm_spec_property_pack_string(&p, "50", 2) == QM_EOK);
            rejected(&p);
            for (int format = QM_SPEC_PROPERTY_FORMAT_INT32; format <= QM_SPEC_PROPERTY_FORMAT_UINT32; ++format) {
                const int lengths[] = {0, 1, 2, 3, 5, 8, 65535};
                for (size_t i = 0; i < QM_ARRAY_SIZE(lengths); ++i) {
                    p = property(siid, piid, format, 1);
                    p.value.len = (uint16_t)lengths[i]; rejected(&p);
                }
            }
            p = property(siid, piid, QM_SPEC_PROPERTY_FORMAT_UINT8, 1);
            p.value.len = 4; rejected(&p);
            p = property(siid, piid, QM_SPEC_PROPERTY_FORMAT_UINT16, 1);
            p.value.len = 4; rejected(&p);
        }
    }
    expect_volume(50);
    puts("PASS: BOOL/string/float and malformed integer widths never touch volume (including legacy 3.1 BOOL)");
}
static void test_invalid_adjust(void)
{
    initialize();
    assert(number(2, 1, 50) == QM_EOK);
    const int values[] = {-1, 0, 5, 8, 50, 100, INT_MAX};
    for (int siid = 2; siid <= 3; ++siid) {
        for (size_t i = 0; i < QM_ARRAY_SIZE(values); ++i) {
            qm_spec_property_t p = property(siid, 2, QM_SPEC_PROPERTY_FORMAT_NUMBER, values[i]);
            rejected(&p);
        }
        qm_spec_property_t p = property(siid, 2, QM_SPEC_PROPERTY_FORMAT_UINT32, UINT32_MAX);
        rejected(&p);
    }
    expect_volume(50);
    puts("PASS: legacy 3.2 brightness 8/50/100 and all non-enum adjustments return EINVAL");
}
static void test_hardware_errors(void)
{
    initialize();
    assert(number(2, 1, 50) == QM_EOK);
    int calls = audio_calls, saves = save_calls, reports = report_calls, successes = hw_success_logs;
    ready = 0;
    assert(number(3, 1, 80) == -QM_EINIT);
    assert(audio_calls == calls && save_calls == saves && report_calls == reports);
    ready = 1; audio_result = -QM_EIO;
    assert(number(2, 1, 80) == -QM_EIO);
    assert(audio_calls == calls + 1 && save_calls == saves && report_calls == reports);
    assert(hw_success_logs == successes && hw_failed_logs == 1);
    expect_volume(50);
    audio_result = QM_EOK;
    assert(number(3, 2, 1) == QM_EOK); expect_volume(60);
    puts("PASS: unready/failed hardware propagates error without false hw success, KV/report or state commit");
}
static void test_followup_errors(void)
{
    initialize();
    assert(number(2, 1, 50) == QM_EOK);
    kv_result = -QM_EIO;
    assert(number(3, 1, 70) == -QM_EIO);
    assert(hardware_volume == 70 && persisted_volume == 50 && reported_volume == 70);
    assert(save_failed_logs == 1 && live_allocations == 0);
    kv_result = QM_EOK;
    assert(number(2, 2, 1) == QM_EOK); expect_volume(80);
    report_result = -QM_EIO;
    assert(number(2, 1, 60) == -QM_EIO);
    assert(hardware_volume == 60 && persisted_volume == 60 && live_allocations == 0);
    report_result = QM_EOK;
    assert(number(3, 2, 1) == QM_EOK); expect_volume(70);
    have_cloud = 0;
    assert(number(3, 1, 65) == -QM_EINIT);
    assert(hardware_volume == 65 && persisted_volume == 65 && live_allocations == 0);
    have_cloud = 1;
    assert(number(2, 2, 1) == QM_EOK); expect_volume(75);
    puts("PASS: KV/report/cloud failures are returned, free caller-owned reports and retain applied state");
}
static void test_report_allocation(void)
{
    initialize();
    for (int fail = 0; fail < 2; ++fail) {
        allocation_call = 0; fail_allocation = fail;
        assert(number(2, 1, 40 + fail) == -QM_ENOMEM);
        fail_allocation = -1;
        assert(hardware_volume == 40 + fail && persisted_volume == 40 + fail);
        assert(report_calls == 0 && live_allocations == 0);
    }
    assert(number(3, 2, 1) == QM_EOK); expect_volume(51);
    puts("PASS: each report allocation failure frees ownership and leaves the applied volume available for adjustment");
}
static void test_kv_load(int invalid)
{
    initialize();
    persisted_volume = 30;
    persisted_len = invalid ? 2 : 1;
    assert(number(3, 2, 1) == QM_EOK);
    expect_volume(invalid ? 90 : 40);
    puts(invalid ? "PASS: malformed saved-value length uses default without accepting corrupt KV"
                 : "PASS: first relative command reads the existing saved volume");
}
static void test_routing(void)
{
    initialize();
    assert(number(2, 1, 50) == QM_EOK);
    int audio = audio_calls, saves = save_calls, reports = report_calls;
    assert(number(2, 3, 1) == 99); /* Unsupported mute stays unsupported. */
    assert(number(3, 3, 1) == 99);
    assert(number(9, 1, 90) == 99);
    for (int piid = 1; piid <= 3; ++piid) {
        qm_spec_property_t p = property(4, piid, QM_SPEC_PROPERTY_FORMAT_NUMBER, 50);
        assert(dispatch(&p) == -QM_EIO);
    }
    set_callback(NULL);
    assert(rgb_calls == 3 && audio_calls == audio && save_calls == saves && report_calls == reports);
    assert(iot_control_volume_set_value(NULL) == -QM_EINVAL);
    assert(iot_control_volume_set_adjust(NULL) == -QM_EINVAL);
    qm_spec_property_t wrong = property(4, 1, QM_SPEC_PROPERTY_FORMAT_NUMBER, 50);
    assert(iot_control_volume_set_value(&wrong) == -QM_EINVAL);
    expect_volume(50);
    puts("PASS: RGB routes/error codes stay isolated; unsupported mute/unknown IDs do not mutate volume");
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *name = argv[1];
    if (!strcmp(name, "absolute")) test_absolute();
    else if (!strcmp(name, "relative")) test_relative();
    else if (!strcmp(name, "bounds")) test_bounds();
    else if (!strcmp(name, "types")) test_types();
    else if (!strcmp(name, "adjust-invalid")) test_invalid_adjust();
    else if (!strcmp(name, "hardware-errors")) test_hardware_errors();
    else if (!strcmp(name, "followup-errors")) test_followup_errors();
    else if (!strcmp(name, "report-allocation")) test_report_allocation();
    else if (!strcmp(name, "kv-load")) test_kv_load(0);
    else if (!strcmp(name, "kv-invalid")) test_kv_load(1);
    else if (!strcmp(name, "routing")) test_routing();
    else assert(0);
    assert(live_allocations == 0);
    return 0;
}
"""

CASES = ("absolute", "relative", "bounds", "types", "adjust-invalid", "hardware-errors",
         "followup-errors", "report-allocation", "kv-load", "kv-invalid", "routing")


def main():
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", type=Path,
                        default=root / "docs/开发/萌护坐姿台灯-物模型-v2-20261030-脱敏.json")
    parser.add_argument("--voice-siid", type=int)
    parser.add_argument("--light-siid", type=int)
    parser.add_argument("--sanitize", action="store_true", help="Also enable AddressSanitizer/UBSan")
    parser.add_argument("--case", action="append", choices=CASES)
    args = parser.parse_args()
    model_path = args.model.resolve()
    model = json.loads(model_path.read_text())
    voice = next(s for s in model["services"] if s["serviceName"] == "voice")
    light = next(s for s in model["services"] if s["serviceName"] == "light")
    piids = {p["description"]: p["piid"] for p in voice["properties"]}
    expected = {
        "PLATFORM_VOLUME_SIID": voice["siid"] if args.voice_siid is None else args.voice_siid,
        "PLATFORM_VOLUME_PIID": piids["Volume"],
        "PLATFORM_ADJUST_PIID": piids["VolumeAdjust"],
        "PLATFORM_MUTE_PIID": piids["MuteMode"],
        "PLATFORM_LIGHT_SIID": light["siid"] if args.light_siid is None else args.light_siid,
    }
    if any(not isinstance(v, int) or v <= 0 for v in expected.values()):
        parser.error("Service/property IDs must be positive integers")
    if expected["PLATFORM_VOLUME_SIID"] == expected["PLATFORM_LIGHT_SIID"]:
        parser.error("Voice and light must have different service IDs")
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if os.name == "nt" or not compiler or not shutil.which(compiler[0]):
        parser.error("A POSIX host C compiler is required (set CC if necessary)")
    sdk = root / "src/firmware"
    controls = sdk / "apps/main/lj_components/iot_control"
    spec = sdk / "qumios-dev/components/qm_spec"
    sources = [controls / "iot_control.c", controls / "ctrl/iot_control_volume.c"]
    print(f"Contract: {model_path.name}, version={model.get('version')}, "
          f"voice={expected['PLATFORM_VOLUME_SIID']}; primary report=2.1, legacy voice=3; real SDK codec", flush=True)
    with tempfile.TemporaryDirectory(prefix="volume-control-test-") as folder:
        temp = Path(folder)
        (temp / "qm.h").write_text(QM_HEADER)
        for name in ("qm_types.h", "qm_log.h"):
            (temp / name).write_text('#include "qm.h"\n')
        shutil.copyfile(sdk / "qumios-dev/include/common/qm_errno.h", temp / "qm_errno.h")
        for name in ("qm_spec_api.h", "qm_spec_core.h", "qm_spec_log.h"):
            shutil.copyfile(spec / "include" / name, temp / name)
        shutil.copyfile(spec / "src/qm_spec_api.c", temp / "qm_spec_api.c")
        (temp / "host_io.h").write_text(IO_HEADER)
        for name in ("ljyd_cloud.h", "qm_iot_api.h", "qm_kv.h", "voice.h"):
            (temp / name).write_text('#include "host_io.h"\n')
        (temp / "test_volume.c").write_text(TEST_SOURCE)
        binary = temp / "test-volume"
        command = compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror",
                              "-I", str(temp), "-I", str(controls), "-I", str(controls / "ctrl")]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        command += [f"-D{k}={v}" for k, v in expected.items()]
        command += [str(p) for p in sources] + [str(temp / "qm_spec_api.c"), str(temp / "test_volume.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        env = os.environ.copy()
        if args.sanitize:
            env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
            env["UBSAN_OPTIONS"] = "halt_on_error=1"
        for case in args.case or CASES:
            subprocess.run([str(binary), case], check=True, cwd=temp, env=env, timeout=20)


if __name__ == "__main__":
    main()
