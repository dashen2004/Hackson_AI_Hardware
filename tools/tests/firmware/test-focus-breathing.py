#!/usr/bin/env python3
"""Compile the real RGB control C code with a deterministic clock/work queue.

Checks physical-write ordering, owner lifetimes, restoration, GET/SET semantics
and injected driver/queue faults. This does not test an ESP32, cloud speech,
optical brightness, FreeRTOS timing, or the complete firmware build.
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
#include "qm_work.h"
#include <limits.h>

static uint32_t fake_now;
static qm_work_t *pending_work;
static int post_failure;
static int post_calls;
static int stop_warnings;

void rgb_focus_warning(const char *tag, const char *format, ...)
{
    (void)tag;
    if (strstr(format, "focus light stopped")) ++stop_warnings;
}

uint32_t qm_now_ms(void) { return fake_now; }

int32_t qm_post_delayed_action(qm_work_t *work, void (*fn)(void *),
                               void *arg, uint32_t delay)
{
    ++post_calls;
    assert(delay == 100U && fn);
    if (post_failure) return post_failure;
    assert(!pending_work || pending_work == work); /* App-lifetime handle. */
    pending_work = work;
    work->fn = fn; work->arg = arg; work->delay = delay; work->active = 1;
    return QM_EOK;
}

int32_t qm_cancel_delayed_action(qm_work_t *work)
{
    work->active = 0;
    return QM_EOK;
}

static void tick(uint32_t elapsed)
{
    assert(pending_work && pending_work->active);
    void (*fn)(void *) = pending_work->fn;
    void *arg = pending_work->arg;
    pending_work->active = 0; /* Real work dispatch consumes before callback. */
    fake_now += elapsed;
    fn(arg);
}

static iot_control_light_rgb_focus_status_t focus_status(void)
{
    iot_control_light_rgb_focus_status_t status;
    assert(iot_control_light_rgb_focus_get_status(&status) == QM_EOK);
    return status;
}

static void baseline(void)
{
    initialize();
    assert(set_number(2, 47) == QM_EOK);
    assert(set_number(3, 0x1234EF) == QM_EOK);
    assert(set_on(0) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
}

static void test_loop(void)
{
    baseline();
    int reports = report_calls;
    assert(iot_control_light_rgb_focus_begin(1) == QM_EOK);
    expect_state(1, 1, 0x00FF00);
    assert(report_calls == reports + 1);
    int previous = 1;
    for (int i = 1; i <= 30; ++i) {
        tick(100);
        iot_control_light_rgb_state_t s = snapshot();
        assert(s.on && s.color == 0x00FF00);
        assert(s.brightness >= 1 && s.brightness <= 25);
        if (i <= 15) assert(s.brightness >= previous);
        else assert(s.brightness <= previous);
        previous = s.brightness;
        assert_physical(&s);
        if (i == 15) assert(s.brightness == 25);
        assert(focus_status().active);
    }
    expect_state(1, 1, 0x00FF00);
    assert(report_calls == reports + 1); /* Never report each frame. */
    assert(iot_control_light_rgb_focus_end(1) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
    assert(report_calls == reports + 2);
    assert(!pending_work->active && !focus_status().active);
    puts("PASS: 100ms steps, 3s green cycle, capped physical output and exact restoration");
}

static void test_owners(void)
{
    baseline();
    assert(iot_control_light_rgb_focus_begin(0) == -QM_EINVAL);
    assert(iot_control_light_rgb_focus_end(0) == -QM_EINVAL);
    assert(iot_control_light_rgb_focus_get_status(NULL) == -QM_EINVAL);
    assert(iot_control_light_rgb_focus_begin(7) == QM_EOK);
    tick(500);
    int calls = driver_calls, posts = post_calls, reports = report_calls;
    assert(iot_control_light_rgb_focus_begin(7) == QM_EOK);
    assert(calls == driver_calls && posts == post_calls && reports == report_calls);
    assert(iot_control_light_rgb_focus_begin(8) == QM_EOK);
    assert(iot_control_light_rgb_focus_end(7) == -QM_EBUSY);
    assert(focus_status().active && focus_status().owner == 8);
    assert(iot_control_light_rgb_focus_end(8) == QM_EOK);
    expect_state(0, 47, 0x1234EF); /* Never capture an intermediate breathing frame. */
    calls = driver_calls; reports = report_calls;
    assert(iot_control_light_rgb_focus_end(8) == QM_EOK);
    assert(calls == driver_calls && reports == report_calls);
    assert(set_number(3, 0xFF2200) == QM_EOK);
    assert(set_number(2, 66) == QM_EOK);
    assert(set_on(1) == QM_EOK);
    assert(iot_control_light_rgb_focus_begin(8) == QM_EOK); /* Resume same owner. */
    tick(500);
    assert(iot_control_light_rgb_focus_end(8) == QM_EOK);
    expect_state(1, 66, 0xFF2200); /* Resume captures its new baseline. */
    puts("PASS: begin/end idempotence, owner handover and pause/resume baseline");
}

static void test_manual_get(void)
{
    initialize();
    assert(iot_control_light_rgb_focus_begin(3) == QM_EOK);
    tick(1500);
    int calls = driver_calls, reports = report_calls;
    assert(set_on(0) == -QM_EBUSY);
    assert(set_number(2, 90) == -QM_EBUSY);
    assert(set_number(3, 0xFF0000) == -QM_EBUSY);
    assert(driver_calls == calls && reports == report_calls);
    expect_state(1, 25, 0x00FF00);
    qm_spec_property_operation_t *requests = qm_spec_property_operation_creat();
    qm_spec_property_operation_t *responses = qm_spec_property_operation_creat();
    for (int piid = 1; piid <= 3; ++piid) {
        qm_spec_property_t *p = qm_spec_property_creat();
        assert(qm_spec_property_pack_xiid(p, 4, piid) == QM_EOK);
        assert(qm_spec_property_add(requests, p) == QM_EOK);
    }
    assert(iot_control_on_get(requests, responses) == QM_EOK);
    iot_control_light_rgb_state_t got = operation_state(responses);
    assert(got.on && got.brightness == 25 && got.color == 0x00FF00);
    assert_physical(&got);
    assert(qm_spec_property_operation_delete(requests) == QM_EOK);
    assert(qm_spec_property_operation_delete(responses) == QM_EOK);
    assert(iot_control_light_rgb_focus_end(3) == QM_EOK);
    assert(set_number(2, 90) == QM_EOK);
    expect_state(1, 90, 0x00FF00);
    puts("PASS: manual 4.x writes return busy; GET is the last successful physical frame");
}

static void test_failures(void)
{
    baseline();
    post_failure = -76;
    assert(iot_control_light_rgb_focus_begin(1) == -76);
    assert(!focus_status().active && focus_status().last_error == -76);
    expect_state(0, 47, 0x1234EF);
    post_failure = 0;
    hardware_failure = -77;
    assert(iot_control_light_rgb_focus_begin(2) == -77);
    assert(!focus_status().active && focus_status().restore_pending);
    expect_unknown();
    assert(set_number(2, 50) == -QM_EBUSY);
    assert(iot_control_light_rgb_focus_end(2) == -77);
    hardware_failure = 0;
    assert(iot_control_light_rgb_focus_end(2) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
    assert(iot_control_light_rgb_focus_begin(3) == QM_EOK);
    post_failure = -78;
    tick(100);
    assert(!focus_status().active && focus_status().last_error == -78);
    expect_state(0, 47, 0x1234EF);
    post_failure = 0;
    assert(iot_control_light_rgb_focus_begin(4) == QM_EOK);
    hardware_failure = -79;
    tick(100);
    assert(!focus_status().active && focus_status().last_error == -79);
    expect_unknown();
    hardware_failure = 0;
    assert(iot_control_light_rgb_focus_end(4) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
    report_failure = -80;
    assert(iot_control_light_rgb_focus_begin(5) == QM_EOK);
    assert(focus_status().active);
    tick(100);
    assert(iot_control_light_rgb_focus_end(5) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
    report_failure = 0;
    have_cloud = 0; /* A disconnected/offline device still runs its local effect. */
    assert(iot_control_light_rgb_focus_begin(6) == QM_EOK);
    tick(1500);
    expect_state(1, 25, 0x00FF00);
    assert(iot_control_light_rgb_focus_end(6) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
    assert(live_allocations == 0);
    puts("PASS: begin/tick/restore/post faults are visible, report/cloud loss does not fake rollback");
}

static void test_late(void)
{
    baseline();
    assert(iot_control_light_rgb_focus_begin(1) == QM_EOK);
    void (*stale)(void *) = pending_work->fn;
    void *old_arg = pending_work->arg;
    /* A callback already dequeued before end is still rejected by generation. */
    assert(iot_control_light_rgb_focus_end(1) == QM_EOK);
    int calls = driver_calls, posts = post_calls;
    stale(old_arg);
    assert(driver_calls == calls && post_calls == posts);
    expect_state(0, 47, 0x1234EF);
    assert(iot_control_light_rgb_focus_begin(2) == QM_EOK);
    tick(500);
    calls = driver_calls; posts = post_calls;
    stale(old_arg);
    assert(calls == driver_calls && posts == post_calls);
    assert(focus_status().active && focus_status().owner == 2);
    assert(iot_control_light_rgb_focus_end(1) == -QM_EBUSY);
    assert(iot_control_light_rgb_focus_end(2) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
    puts("PASS: late queued frames cannot overwrite restore or affect a newer owner");
}

static void test_stall(void)
{
    baseline();
    assert(iot_control_light_rgb_focus_begin(1) == QM_EOK);
    void (*stale)(void *) = pending_work->fn;
    void *arg = pending_work->arg;
    pending_work->active = 0; /* qm_work timer failed to enqueue after post succeeded. */
    fake_now += 1000;
    assert(focus_status().active);
    fake_now += 1;
    iot_control_light_rgb_focus_status_t s = focus_status();
    assert(!s.active && !s.restore_pending && s.last_error == -QM_EIO);
    expect_state(0, 47, 0x1234EF);
    int calls = driver_calls, reports = report_calls, warnings = stop_warnings;
    assert(warnings == 1);
    fake_now += 5000;
    assert(!focus_status().active);
    stale(arg);
    assert(calls == driver_calls && reports == report_calls && warnings == stop_warnings);
    assert(iot_control_light_rgb_focus_end(1) == QM_EOK);
    assert(calls == driver_calls && reports == report_calls);
    puts("PASS: lost delayed-work delivery restores once and leaves a queryable error");
}

static void (*concurrent_frame)(void *);
static void *concurrent_arg;
static int end_started;
static int end_done;

static void *frame_thread(void *unused)
{
    (void)unused;
    concurrent_frame(concurrent_arg);
    return NULL;
}

static void *end_thread(void *unused)
{
    (void)unused;
    __atomic_store_n(&end_started, 1, __ATOMIC_SEQ_CST);
    assert(iot_control_light_rgb_focus_end(1) == QM_EOK);
    __atomic_store_n(&end_done, 1, __ATOMIC_SEQ_CST);
    return NULL;
}

static void test_concurrent_exit(void)
{
    baseline();
    assert(iot_control_light_rgb_focus_begin(1) == QM_EOK);
    concurrent_frame = pending_work->fn;
    concurrent_arg = pending_work->arg;
    pending_work->active = 0;
    fake_now = 100;
    __atomic_store_n(&focus_hold_driver, 1, __ATOMIC_SEQ_CST);
    pthread_t frame, end;
    assert(pthread_create(&frame, NULL, frame_thread, NULL) == 0);
    while (!__atomic_load_n(&focus_driver_entered, __ATOMIC_SEQ_CST)) usleep(100);
    assert(pthread_create(&end, NULL, end_thread, NULL) == 0);
    while (!__atomic_load_n(&end_started, __ATOMIC_SEQ_CST)) usleep(100);
    usleep(1000);
    assert(!__atomic_load_n(&end_done, __ATOMIC_SEQ_CST));
    __atomic_store_n(&focus_hold_driver, 0, __ATOMIC_SEQ_CST);
    assert(pthread_join(frame, NULL) == 0);
    assert(pthread_join(end, NULL) == 0);
    expect_state(0, 47, 0x1234EF);
    assert(!pending_work->active);
    int calls = driver_calls;
    concurrent_frame(concurrent_arg);
    assert(calls == driver_calls);
    puts("PASS: concurrent exit serializes after in-flight frame and cancels its repost");
}

static void test_clock(void)
{
    baseline();
    fake_now = UINT32_MAX - 500U;
    assert(iot_control_light_rgb_focus_begin(1) == QM_EOK);
    tick(1500); /* Unsigned monotonic clock wrapping. */
    expect_state(1, 25, 0x00FF00);
    tick(1500);
    expect_state(1, 1, 0x00FF00);
    tick(1600); /* Scheduler delay uses elapsed time, not number of callbacks. */
    expect_state(1, 23, 0x00FF00);
    assert(focus_status().active);
    assert(iot_control_light_rgb_focus_end(1) == QM_EOK);
    expect_state(0, 47, 0x1234EF);
    puts("PASS: cycle follows monotonic elapsed time across delayed frames and clock wrap");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "loop")) test_loop();
    else if (!strcmp(argv[1], "owners")) test_owners();
    else if (!strcmp(argv[1], "manual-get")) test_manual_get();
    else if (!strcmp(argv[1], "failures")) test_failures();
    else if (!strcmp(argv[1], "late")) test_late();
    else if (!strcmp(argv[1], "stall")) test_stall();
    else if (!strcmp(argv[1], "concurrent-exit")) test_concurrent_exit();
    else if (!strcmp(argv[1], "clock")) test_clock();
    else assert(!"unknown case");
    assert(live_allocations == 0);
    return 0;
}
'''

CASES = ("loop", "owners", "manual-get", "failures", "late", "stall", "concurrent-exit", "clock")


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
    firmware = root / "src/firmware"
    controls = firmware / "apps/main/lj_components/iot_control"
    spec = firmware / "qumios-dev/components/qm_spec"
    with tempfile.TemporaryDirectory(prefix="focus-breathing-") as folder:
        temp = Path(folder)
        header = legacy["QM_HEADER"].replace(
            "#define QM_LOGW(...) ((void)0)",
            "void rgb_focus_warning(const char *, const char *, ...);\n"
            "#define QM_LOGW(...) rgb_focus_warning(__VA_ARGS__)")
        (temp / "qm.h").write_text(header)
        for name in ("qm_types.h", "qm_log.h"):
            (temp / name).write_text('#include "qm.h"\n')
        for name in ("qm_spec_api.h", "qm_spec_core.h", "qm_spec_log.h"):
            shutil.copyfile(spec / "include" / name, temp / name)
        shutil.copyfile(firmware / "apps/main/peripheral/bsp_ws2812.h", temp / "bsp_ws2812.h")
        for name, key in (("ljyd_cloud.h", "CLOUD_HEADER"), ("qm_cli.h", "CLI_HEADER"),
                          ("qm_work.h", "WORK_HEADER")):
            (temp / name).write_text(legacy[key])
        (temp / "qm_iot_api.h").write_text('#include "ljyd_cloud.h"\n')
        fixture = legacy["TEST_SOURCE"].replace("int main(int argc, char **argv)",
                                              "int rgb_legacy_main(int argc, char **argv)")
        fixture = fixture.replace("static int hardware_apply(",
            "static int focus_hold_driver;\nstatic int focus_driver_entered;\n"
            "static int hardware_apply(")
        fixture = fixture.replace("if (add_delay) usleep(100);",
            "if (add_delay) usleep(100);\n"
            "    if (__atomic_load_n(&focus_hold_driver, __ATOMIC_SEQ_CST)) {\n"
            "        __atomic_store_n(&focus_driver_entered, 1, __ATOMIC_SEQ_CST);\n"
            "        while (__atomic_load_n(&focus_hold_driver, __ATOMIC_SEQ_CST)) usleep(100);\n"
            "    }")
        (temp / "focus.c").write_text(fixture + SOURCE)
        command = compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-DRGB_HOST_FOCUS_TEST", "-I", str(temp), "-I", str(controls),
            "-I", str(controls / "ctrl"), str(controls / "iot_control.c"),
            str(controls / "ctrl/iot_control_light_rgb.c"),
            str(controls / "ctrl/iot_control_light_rgb_cli.c"),
            str(spec / "src/qm_spec_api.c"), str(temp / "focus.c"),
            "-o", str(temp / "focus")]
        if args.sanitize:
            command[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(command, check=True)
        for name in args.case or CASES:
            subprocess.run([str(temp / "focus"), name], check=True, timeout=20)


if __name__ == "__main__":
    main()
