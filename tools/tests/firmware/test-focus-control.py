#!/usr/bin/env python3
"""Test real focus property adapter/routes and SDK codec with a mock timer.

The timer mock implements the synchronous public API; the suite does not test
timer internals, speech, a real device, cloud delivery, or a firmware build.
Existing RGB and volume suites separately retain their original assertions.
"""
import argparse
import os
from pathlib import Path
import runpy
import shlex
import shutil
import subprocess
import tempfile


SOURCE = r'''
#include "pomodoro.h"

static qm_pomodoro_status_t timer_state;
static int start_result, action_result_mock, status_result;
static int start_calls, plugin_start_calls, action_calls, status_calls, focus_reports;
static int mutate_on_read;
static uint32_t last_seconds;
static char last_action[16];
static iot_control_focus_state_t focus_report;

int pomodoro_start_for(uint32_t seconds)
{
    ++start_calls; last_seconds = seconds;
    if (start_result) return start_result;
    timer_state.state = QM_POMODORO_STATE_FOCUS;
    timer_state.paused = QM_FALSE;
    timer_state.duration_sec = timer_state.remaining_sec = seconds;
    return QM_EOK;
}

int pomodoro_start_for_plugin(uint32_t seconds)
{
    ++plugin_start_calls;
    return pomodoro_start_for(seconds);
}

int pomodoro_execute(const char *action)
{
    ++action_calls;
    snprintf(last_action, sizeof(last_action), "%s", action);
    if (action_result_mock) return action_result_mock;
    if (!strcmp(action, "stop")) {
        timer_state.state = QM_POMODORO_STATE_IDLE;
        timer_state.paused = QM_FALSE;
        timer_state.duration_sec = timer_state.remaining_sec = 0;
    } else if (!strcmp(action, "pause")) timer_state.paused = QM_TRUE;
    else if (!strcmp(action, "resume")) timer_state.paused = QM_FALSE;
    else assert(!"unexpected action");
    return QM_EOK;
}

int pomodoro_get_local_status(qm_pomodoro_status_t *status)
{
    ++status_calls;
    if (status_result) return status_result;
    *status = timer_state;
    if (mutate_on_read) {
        ++timer_state.duration_sec;
        --timer_state.remaining_sec;
        timer_state.paused = !timer_state.paused;
    }
    return QM_EOK;
}

static uint32_t u32(qm_spec_property_t *property)
{
    uint32_t value = 0;
    assert(property && property->value.property_format == QM_SPEC_PROPERTY_FORMAT_UINT32);
    assert(property->value.len == sizeof(value));
    assert(qm_spec_property_unpack_uint32(property, &value) == QM_EOK);
    return value;
}

static uint8_t u8(qm_spec_property_t *property)
{
    uint8_t value = 0;
    assert(property && property->value.property_format == QM_SPEC_PROPERTY_FORMAT_UINT8);
    assert(property->value.len == sizeof(value));
    assert(qm_spec_property_unpack_uint8(property, &value) == QM_EOK);
    return value;
}

int qm_iot_report(void *handle, qm_spec_property_operation_t *operation)
{
    assert(handle && operation && operation->property);
    if (operation->property->siid == 4) return rgb_legacy_report(handle, operation);
    assert(handle == &cloud_object && operation->element_num == 3);
    assert(!qm_spec_property_find(operation, 12, 2));
    focus_report.duration_sec = u32(qm_spec_property_find(operation, 12, 1));
    focus_report.state = u8(qm_spec_property_find(operation, 12, 3));
    focus_report.remaining_sec = u32(qm_spec_property_find(operation, 12, 4));
    ++focus_reports;
    if (report_failure) return report_failure; /* Ownership stays with caller. */
    assert(qm_spec_property_operation_delete(operation) == QM_EOK);
    return QM_EOK;
}

static qm_spec_property_t focus_value(int piid, int format, int64_t value)
{
    qm_spec_property_t property = {0};
    assert(qm_spec_property_pack_xiid(&property, 12, piid) == QM_EOK);
    int ret = -QM_EINVAL;
    switch (format) {
    case QM_SPEC_PROPERTY_FORMAT_NUMBER: ret = qm_spec_property_pack_number(&property, (int)value); break;
    case QM_SPEC_PROPERTY_FORMAT_INT8: ret = qm_spec_property_pack_int8(&property, (int8_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_UINT8: ret = qm_spec_property_pack_uint8(&property, (uint8_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_INT16: ret = qm_spec_property_pack_int16(&property, (int16_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_UINT16: ret = qm_spec_property_pack_uint16(&property, (uint16_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_INT32: ret = qm_spec_property_pack_int32(&property, (int32_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_UINT32: ret = qm_spec_property_pack_uint32(&property, (uint32_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_BOOL: ret = qm_spec_property_pack_bool(&property, (bool_t)value); break;
    case QM_SPEC_PROPERTY_FORMAT_FLOAT32: ret = qm_spec_property_pack_float32(&property, (float)value); break;
    case QM_SPEC_PROPERTY_FORMAT_STRING: ret = qm_spec_property_pack_string(&property, "60", 2); break;
    default: assert(!"unsupported fixture format");
    }
    assert(ret == QM_EOK);
    return property;
}

static int dispatch_focus(qm_spec_property_t *property)
{
    qm_spec_property_t before = *property;
    property->code = 99;
    iot_control_on_set(property);
    assert(before.siid == property->siid && before.piid == property->piid);
    assert(memcmp(&before.value, &property->value, sizeof(before.value)) == 0);
    return property->code;
}

static int duration(uint32_t seconds)
{
    qm_spec_property_t property = focus_value(1, QM_SPEC_PROPERTY_FORMAT_UINT32, seconds);
    return dispatch_focus(&property);
}

static int action(int value)
{
    qm_spec_property_t property = focus_value(2, QM_SPEC_PROPERTY_FORMAT_NUMBER, value);
    return dispatch_focus(&property);
}

static int plugin_start(uint32_t seconds)
{
    qm_spec_property_t property = focus_value(5, QM_SPEC_PROPERTY_FORMAT_UINT32, seconds);
    return dispatch_focus(&property);
}

static void test_plugin_start(void)
{
    initialize();
    assert(plugin_start(1) == QM_EOK && plugin_start_calls == 1 && last_seconds == 1);
    assert(focus_report.state == 1 && focus_report.duration_sec == 1);
    assert(plugin_start(86400) == QM_EOK && plugin_start_calls == 2 && last_seconds == 86400);
    int calls = plugin_start_calls, reports = focus_reports;
    assert(plugin_start(0) == -QM_EINVAL && plugin_start(86401) == -QM_EINVAL);
    qm_spec_property_t invalid = focus_value(5, QM_SPEC_PROPERTY_FORMAT_INT32, -1);
    assert(dispatch_focus(&invalid) == -QM_EINVAL);
    invalid = focus_value(5, QM_SPEC_PROPERTY_FORMAT_STRING, 60);
    assert(dispatch_focus(&invalid) == -QM_EINVAL);
    assert(calls == plugin_start_calls && reports == focus_reports);
    start_result = -QM_EBUSY;
    assert(plugin_start(30) == -QM_EBUSY && plugin_start_calls == calls + 1 && focus_reports == reports);
    assert(iot_control_focus_set_plugin_start(NULL) == -QM_EINVAL);
    invalid = focus_value(1, QM_SPEC_PROPERTY_FORMAT_UINT32, 30);
    assert(iot_control_focus_set_plugin_start(&invalid) == -QM_EINVAL);
    puts("PASS: panel-only plugin start uses explicit seconds, reports actual state, rejects invalid/busy writes");
}

static void test_duration(void)
{
    initialize();
    assert(duration(1) == QM_EOK && last_seconds == 1 && start_calls == 1);
    assert(duration(86400) == QM_EOK && last_seconds == 86400 && start_calls == 2);
    assert(focus_report.duration_sec == 86400 && focus_report.remaining_sec == 86400);
    assert(focus_report.state == 1 && focus_reports == 2);
    int calls = start_calls, reports = focus_reports;
    assert(duration(0) == -QM_EINVAL);
    assert(duration(86401) == -QM_EINVAL);
    assert(duration(UINT32_MAX) == -QM_EINVAL);
    qm_spec_property_t negative = focus_value(1, QM_SPEC_PROPERTY_FORMAT_INT32, -1);
    assert(dispatch_focus(&negative) == -QM_EINVAL);
    assert(start_calls == calls && focus_reports == reports);
    assert(live_allocations == 0);
    puts("PASS: duration is exact seconds 1..86400; zero/negative/overflow are rejected, never clamped");
}

static void test_encoding(void)
{
    initialize();
    static const int accepted[] = {QM_SPEC_PROPERTY_FORMAT_NUMBER,
        QM_SPEC_PROPERTY_FORMAT_INT8, QM_SPEC_PROPERTY_FORMAT_UINT8,
        QM_SPEC_PROPERTY_FORMAT_INT16, QM_SPEC_PROPERTY_FORMAT_UINT16,
        QM_SPEC_PROPERTY_FORMAT_INT32, QM_SPEC_PROPERTY_FORMAT_UINT32};
    for (size_t i = 0; i < QM_ARRAY_SIZE(accepted); ++i) {
        qm_spec_property_t p = focus_value(1, accepted[i], 60);
        assert(dispatch_focus(&p) == QM_EOK && last_seconds == 60);
        uint16_t correct = p.value.len;
        for (int delta = -1; delta <= 1; delta += 2) {
            p.value.len = (uint16_t)(correct + delta);
            int calls = start_calls;
            assert(dispatch_focus(&p) == -QM_EINVAL && calls == start_calls);
        }
    }
    static const int rejected[] = {QM_SPEC_PROPERTY_FORMAT_BOOL,
        QM_SPEC_PROPERTY_FORMAT_FLOAT32, QM_SPEC_PROPERTY_FORMAT_STRING};
    for (size_t i = 0; i < QM_ARRAY_SIZE(rejected); ++i) {
        qm_spec_property_t p = focus_value(1, rejected[i], 60);
        int calls = start_calls;
        assert(dispatch_focus(&p) == -QM_EINVAL && calls == start_calls);
    }
    qm_spec_property_t p = focus_value(1, QM_SPEC_PROPERTY_FORMAT_NUMBER, 60);
    p.value.property_format = QM_SPEC_PROPERTY_FORMAT_UINT64; p.value.len = 8;
    assert(dispatch_focus(&p) == -QM_EINVAL);
    p.value.property_format = QM_SPEC_PROPERTY_FORMAT_NONE; p.value.len = 0;
    assert(dispatch_focus(&p) == -QM_EINVAL);
    assert(iot_control_focus_set_duration(NULL) == -QM_EINVAL);
    p = focus_value(1, QM_SPEC_PROPERTY_FORMAT_UINT32, 60); p.siid = 11;
    assert(iot_control_focus_set_duration(&p) == -QM_EINVAL);
    puts("PASS: actual SDK integer widths are accepted; permissive unpackers cannot bypass type/length checks");
}

static void test_actions(void)
{
    initialize();
    assert(duration(600) == QM_EOK);
    assert(action(2) == QM_EOK && !strcmp(last_action, "pause"));
    assert(focus_report.state == 2 && focus_report.remaining_sec == 600);
    assert(action(3) == QM_EOK && !strcmp(last_action, "resume"));
    assert(focus_report.state == 1);
    assert(action(1) == QM_EOK && !strcmp(last_action, "stop"));
    assert(focus_report.state == 0 && focus_report.duration_sec == 0 && focus_report.remaining_sec == 0);
    int calls = action_calls, reports = focus_reports;
    assert(action(0) == -QM_EINVAL && action(4) == -QM_EINVAL && action(-1) == -QM_EINVAL);
    qm_spec_property_t p = focus_value(2, QM_SPEC_PROPERTY_FORMAT_BOOL, 1);
    assert(dispatch_focus(&p) == -QM_EINVAL);
    p = focus_value(2, QM_SPEC_PROPERTY_FORMAT_NUMBER, 1); p.value.len = 1;
    assert(dispatch_focus(&p) == -QM_EINVAL);
    for (int piid = 3; piid <= 4; ++piid) {
        p = focus_value(piid, QM_SPEC_PROPERTY_FORMAT_NUMBER, 1);
        assert(dispatch_focus(&p) == -QM_ENOSYS);
    }
    p = focus_value(99, QM_SPEC_PROPERTY_FORMAT_NUMBER, 1);
    assert(dispatch_focus(&p) == -QM_ENOSYS);
    assert(calls == action_calls && reports == focus_reports);
    assert(iot_control_focus_set_action(NULL) == -QM_EINVAL);
    p = focus_value(1, QM_SPEC_PROPERTY_FORMAT_NUMBER, 1);
    assert(iot_control_focus_set_action(&p) == -QM_EINVAL);
    /* Existing route IDs still dispatch to their original handlers. */
    p = value(2, 1, 50); iot_control_on_set(&p); assert(volume_calls == 1);
    p = value(3, 2, 1); iot_control_on_set(&p); assert(volume_calls == 2);
    p = value(4, 2, 80); iot_control_on_set(&p); assert(p.code == QM_EOK);
    expect_state(1, 80, 0x00FF00);
    puts("PASS: stop/pause/resume map exactly; read-only/unknown focus SETs fail and RGB/volume routes remain intact");
}

static void test_errors(void)
{
    initialize();
    start_result = -QM_EBUSY;
    assert(duration(60) == -QM_EBUSY && focus_reports == 0 && status_calls == 0);
    start_result = -QM_EIO;
    assert(duration(60) == -QM_EIO && focus_reports == 0);
    action_result_mock = -QM_EINIT;
    assert(action(1) == -QM_EINIT && focus_reports == 0);
    start_result = action_result_mock = 0;
    report_failure = -71;
    assert(duration(60) == QM_EOK && timer_state.duration_sec == 60);
    assert(live_allocations == 0); /* Report failure retains caller ownership. */
    report_failure = 0; have_cloud = 0;
    assert(action(2) == QM_EOK && timer_state.paused);
    have_cloud = 1; status_result = -QM_EINIT;
    assert(action(3) == QM_EOK && !timer_state.paused);
    assert(live_allocations == 0);
    puts("PASS: actual action errors reach property.code; report/offline/status errors do not disguise successful actions");
}

static void append_request(qm_spec_property_operation_t *requests, int siid, int piid)
{
    qm_spec_property_t *p = qm_spec_property_creat();
    assert(p && qm_spec_property_pack_xiid(p, siid, piid) == QM_EOK);
    p->did = 0xDEADBEEFU;
    assert(qm_spec_property_add(requests, p) == QM_EOK);
}

static void test_get(void)
{
    initialize();
    timer_state.state = QM_POMODORO_STATE_FOCUS;
    timer_state.duration_sec = 600; timer_state.remaining_sec = 321;
    qm_spec_property_operation_t *requests = qm_spec_property_operation_creat();
    qm_spec_property_operation_t *responses = qm_spec_property_operation_creat();
    append_request(requests, 12, 1); append_request(requests, 12, 3);
    append_request(requests, 12, 4); append_request(requests, 12, 2);
    append_request(requests, 12, 5); append_request(requests, 12, 99); append_request(requests, 4, 2);
    append_request(requests, 99, 1);
    mutate_on_read = 1; status_calls = 0;
    assert(iot_control_on_get(requests, responses) == QM_EOK && status_calls == 1);
    assert(responses->element_num == requests->element_num);
    assert(u32(qm_spec_property_find(responses, 12, 1)) == 600);
    assert(u8(qm_spec_property_find(responses, 12, 3)) == 1);
    assert(u32(qm_spec_property_find(responses, 12, 4)) == 321);
    for (qm_spec_property_t *p = responses->property; p; p = p->next) {
        assert(p->did == 0xDEADBEEFU);
        if ((p->siid == 12 && (p->piid == 2 || p->piid == 5 || p->piid == 99)) || p->siid == 99) {
            assert(p->code == -QM_ENOSYS && p->value.len == 0);
        } else assert(p->code == QM_EOK);
    }
    assert(number(qm_spec_property_find(responses, 4, 2)) == 25);
    qm_spec_property_operation_delete(responses);
    responses = qm_spec_property_operation_creat();
    mutate_on_read = 0; status_result = -QM_EINIT;
    assert(iot_control_on_get(requests, responses) == QM_EOK);
    assert(qm_spec_property_find(responses, 12, 1)->code == -QM_EINIT);
    assert(qm_spec_property_find(responses, 12, 1)->value.len == 0);
    assert(qm_spec_property_find(responses, 12, 2)->code == -QM_ENOSYS);
    assert(qm_spec_property_find(responses, 12, 5)->code == -QM_ENOSYS);
    assert(number(qm_spec_property_find(responses, 4, 2)) == 25);
    qm_spec_property_operation_delete(requests); qm_spec_property_operation_delete(responses);
    assert(live_allocations == 0);
    puts("PASS: mixed GET uses one timer snapshot, typed seconds/state, preserved IDs and explicit unsupported/unavailable errors");
}

static void test_packing(void)
{
    initialize();
    iot_control_focus_state_t state, before;
    static const struct { qm_pomodoro_state_t phase; int paused; int expected; } cases[] = {
        {QM_POMODORO_STATE_IDLE, 0, 0}, {QM_POMODORO_STATE_FOCUS, 0, 1},
        {QM_POMODORO_STATE_FOCUS, 1, 2}, {QM_POMODORO_STATE_FINISHED, 1, 3},
        {QM_POMODORO_STATE_SHORT_BREAK, 0, 0}, {QM_POMODORO_STATE_LONG_BREAK, 0, 0},
    };
    for (size_t i = 0; i < QM_ARRAY_SIZE(cases); ++i) {
        timer_state.state = cases[i].phase; timer_state.paused = cases[i].paused;
        assert(iot_control_focus_get_state(&state) == QM_EOK && state.state == cases[i].expected);
    }
    assert(iot_control_focus_get_state(NULL) == -QM_EINVAL);
    memset(&state, 0xA5, sizeof(state)); before = state;
    status_result = -QM_EIO;
    assert(iot_control_focus_get_state(&state) == -QM_EIO && !memcmp(&state, &before, sizeof(state)));
    status_result = 0; timer_state.duration_sec = 86401;
    assert(iot_control_focus_get_state(&state) == -QM_EIO && !memcmp(&state, &before, sizeof(state)));
    timer_state.duration_sec = 0; timer_state.remaining_sec = 86401;
    assert(iot_control_focus_get_state(&state) == -QM_EIO);
    state.duration_sec = state.remaining_sec = 0; state.state = 3;
    qm_spec_property_t p = focus_value(1, QM_SPEC_PROPERTY_FORMAT_NUMBER, 99);
    assert(iot_control_focus_pack_property(&p, &state) == QM_EOK && u32(&p) == 0);
    assert(iot_control_focus_pack_property(NULL, &state) == -QM_EINVAL);
    assert(iot_control_focus_pack_property(&p, NULL) == -QM_EINVAL);
    p.piid = 2; assert(iot_control_focus_pack_property(&p, &state) == -QM_ENOSYS);
    p.piid = 5; assert(iot_control_focus_pack_property(&p, &state) == -QM_ENOSYS);
    p.piid = 1; state.state = 4;
    assert(iot_control_focus_pack_property(&p, &state) == -QM_EIO);
    puts("PASS: state mapping prioritizes finished/paused; unknown status is never published as a valid value");
}

static void test_allocations(void)
{
    initialize();
    for (int fail = 0; fail <= 4; ++fail) {
        int reports = focus_reports;
        malloc_after = fail;
        assert(duration(60) == QM_EOK && last_seconds == 60);
        malloc_after = -1;
        assert(focus_reports == reports + (fail >= 4));
        assert(live_allocations == 0);
    }
    for (int fail = 0; fail < 3; ++fail) {
        qm_spec_property_operation_t *requests = qm_spec_property_operation_creat();
        qm_spec_property_operation_t *responses = qm_spec_property_operation_creat();
        append_request(requests, 12, 1); append_request(requests, 12, 3); append_request(requests, 12, 4);
        malloc_after = fail;
        assert(iot_control_on_get(requests, responses) == -QM_ENOMEM);
        malloc_after = -1;
        qm_spec_property_operation_delete(requests); qm_spec_property_operation_delete(responses);
        assert(live_allocations == 0);
    }
    puts("PASS: every report allocation and partial GET allocation fails without leaks or fake action failure");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "duration")) test_duration();
    else if (!strcmp(argv[1], "plugin-start")) test_plugin_start();
    else if (!strcmp(argv[1], "encoding")) test_encoding();
    else if (!strcmp(argv[1], "actions")) test_actions();
    else if (!strcmp(argv[1], "errors")) test_errors();
    else if (!strcmp(argv[1], "get")) test_get();
    else if (!strcmp(argv[1], "packing")) test_packing();
    else if (!strcmp(argv[1], "allocations")) test_allocations();
    else assert(!"unknown case");
    assert(live_allocations == 0);
    return 0;
}
'''

CASES = ("duration", "plugin-start", "encoding", "actions", "errors", "get", "packing", "allocations")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    legacy = runpy.run_path(str(root / "tools/tests/firmware/test-light-rgb-control.py"))
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not compiler or not shutil.which(compiler[0]):
        parser.error("A POSIX host C compiler is required")
    sdk = root / "src/firmware"
    controls = sdk / "apps/main/lj_components/iot_control"
    spec = sdk / "qumios-dev/components/qm_spec"
    with tempfile.TemporaryDirectory(prefix="focus-control-") as folder:
        temp = Path(folder)
        (temp / "qm.h").write_text(legacy["QM_HEADER"])
        for name in ("qm_types.h", "qm_log.h", "qm_errno.h"):
            (temp / name).write_text('#include "qm.h"\n')
        for name in ("qm_spec_api.h", "qm_spec_core.h", "qm_spec_log.h"):
            shutil.copyfile(spec / "include" / name, temp / name)
        shutil.copyfile(sdk / "qumios-dev/components/qm_pomodoro/include/qm_pomodoro.h", temp / "qm_pomodoro.h")
        shutil.copyfile(sdk / "apps/main/lj_components/pomodoro/pomodoro.h", temp / "pomodoro.h")
        shutil.copyfile(sdk / "apps/main/peripheral/bsp_ws2812.h", temp / "bsp_ws2812.h")
        for name, key in (("ljyd_cloud.h", "CLOUD_HEADER"), ("qm_cli.h", "CLI_HEADER"),
                          ("qm_work.h", "WORK_HEADER")):
            (temp / name).write_text(legacy[key])
        (temp / "qm_iot_api.h").write_text('#include "ljyd_cloud.h"\n')
        fixture = legacy["TEST_SOURCE"].replace("int main(int argc, char **argv)",
            "int rgb_legacy_main(int argc, char **argv)")
        fixture = fixture.replace("int qm_iot_report(", "int rgb_legacy_report(")
        (temp / "focus_control.c").write_text(fixture + SOURCE)
        sources = [controls / "iot_control.c", controls / "ctrl/iot_control_focus.c",
            controls / "ctrl/iot_control_light_rgb.c", controls / "ctrl/iot_control_light_rgb_cli.c",
            spec / "src/qm_spec_api.c", temp / "focus_control.c"]
        command = compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-DRGB_HOST_FOCUS_CONTROL_TEST", "-I", str(temp), "-I", str(controls),
            "-I", str(controls / "ctrl")] + [str(p) for p in sources] + ["-o", str(temp / "focus_control")]
        if args.sanitize:
            command[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(command, check=True)
        for name in args.case or CASES:
            subprocess.run([str(temp / "focus_control"), name], check=True, timeout=20)


if __name__ == "__main__":
    main()
