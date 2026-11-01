#!/usr/bin/env python3
"""Run actual application wake/abort and core upload-gate C functions on a host.

Only surrounding device/transport/OS calls are stubbed. This does not use a
board, credentials, cloud ASR, or audio samples, and does not prove live speech.
With deferred wake enabled, these policy tests call the real worker processor;
test-agent-wakeup-dispatch.py separately exercises callback/task scheduling.
"""

import argparse
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile


def function_source(source, name):
    """Keep the real definition, including comments/strings within its body."""
    match = re.search(r"(?m)^(?:static\s+)?(?:void|int)\s+" + re.escape(name)
                      + r"\s*\([^;]*?\)\s*\{", source)
    if match is None:
        raise ValueError(f"C function not found: {name}")
    index = match.end() - 1
    depth = 0
    state = "code"
    while index < len(source):
        char = source[index]
        pair = source[index:index + 2]
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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define QM_TRUE 1
#define QM_FALSE 0
#define QM_EOK 0
#define QM_EINVAL 22
#define QM_EINIT 5
#define QM_EIO 5
#define QM_EBUSY 16
#define QM_ENOSYS 38
#define QM_WAIT_FOREVER (-1)
#define CONFIG_LJYD_PLAYBACK_SUPPORT 1
#define QM_LOGI(...) ((void)0)
#define QM_LOGW(...) ((void)0)
#define QM_LOGE(...) ((void)0)
#define QM_LOGD(...) ((void)0)
typedef int qm_bool;
typedef int qm_sem_t;
typedef pthread_mutex_t qm_mutex_t;
typedef int qm_event_base_t;
enum { QM_AGENT_DIALOG_STATE_NONE, QM_AGENT_DIALOG_STATE_SILENT,
       QM_AGENT_DIALOG_STATE_LISTENING, QM_AGENT_DIALOG_STATE_THINKING,
       QM_AGENT_DIALOG_STATE_SPEAKING };
typedef int qm_agent_dialog_state_t;
enum { QM_AGENT_CAPTURE_POLICY_KEEP = 1 };
enum { QM_AGENT_EVENT_STATE_CHANGED = 1, QM_AGENT_EVENT_CONNECTED,
       QM_AGENT_EVENT_DISCONNECTED, QM_AGENT_EVENT_ASR, QM_AGENT_EVENT_TTS,
       QM_AGENT_EVENT_AGENT_TEXT, QM_AGENT_EVENT_AGENT_STATE,
       QM_AGENT_EVENT_TTS_PLAYBACK_STARTED, QM_AGENT_EVENT_TTS_PLAYBACK_FAILED,
       QM_AGENT_EVENT_TTS_PLAYBACK_COMPLETE, QM_AGENT_EVENT_ERROR,
       QM_AGENT_EVENT_AGENT_ABORTED };
typedef int qm_agent_event_id_t;
#define QM_AGENT_EVENT_BASE 7
#define QM_AGENT_EVENT_SOURCE_TRANSPORT 2
typedef struct { int id, source, code; } qm_agent_event_t;
typedef struct {
    int code;
    unsigned generation;
    union {
        struct { int old_state, new_state; } state;
        struct { int final; char text[32]; } text;
        struct { int state; } agent_state;
    } data;
} qm_agent_event_notice_t;
typedef struct {
    int runtime_valid, user_upload_enabled, transport_ready;
    qm_mutex_t lock;
    int capture_closing, dialog_state, playback_started, upload_enabled;
} qm_agent_audio_engine_t;
typedef struct {
    qm_agent_audio_engine_t audio;
    struct { int transport_capture_policy; } config;
} test_agent_t;
typedef test_agent_t *qm_agent_handle_t;
typedef struct { const void *data; unsigned size; } qm_audio_data_t;
static qm_agent_handle_t s_agent;
static uint8_t s_cloud_connected, s_agent_started;
static uint8_t s_resource_mode_active;
static qm_mutex_t s_upload_policy_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t s_upload_policy_lock_ready = 1;
static qm_sem_t s_tts_complete_sem;
static uint8_t s_tts_complete_sem_ready;
static volatile uint8_t s_waiting_tts_complete, s_tts_wait_aborted;
static volatile uint8_t s_tts_text_seen, s_tts_speaking_seen;
static volatile uint8_t s_tts_playback_started, s_tts_playback_failed;
static volatile int s_tts_playback_error;
static volatile uint8_t s_tts_silent_seen;
static int start_result, interrupt_result, start_calls, interrupt_calls;
static int transport_packets, scheduled, sem_signals;
static int upload_enables, upload_disables;
static pthread_mutex_t hook_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t hook_cond = PTHREAD_COND_INITIALIZER;
static int race_active, callback_reached, wake_lock_attempted, callback_release;
static _Thread_local int thread_role;
static int qm_mutex_lock(qm_mutex_t *lock, int timeout) {
    (void)timeout;
    if (lock == &s_upload_policy_lock && thread_role == 2 && race_active) {
        pthread_mutex_lock(&hook_lock);
        wake_lock_attempted = 1;
        pthread_cond_broadcast(&hook_cond);
        pthread_mutex_unlock(&hook_lock);
    }
    return pthread_mutex_lock(lock);
}
static int qm_mutex_unlock(qm_mutex_t *lock) { return pthread_mutex_unlock(lock); }
static void schedule_audio(qm_agent_handle_t h, qm_bool retry) {
    (void)h; (void)retry; ++scheduled;
}
static int qm_agent_transport_send_audio(qm_agent_handle_t h,
                                         const void *data, unsigned size) {
    assert(h == s_agent && data && size); ++transport_packets; return 0;
}
static int playback_stop(void) { return -1; }
static int voice_audio_ready(void) { return 1; }
static int qm_agent_start(qm_agent_handle_t h) {
    assert(h == s_agent); ++start_calls; return start_result;
}
static int qm_agent_interrupt(qm_agent_handle_t h) {
    assert(h == s_agent); ++interrupt_calls; return interrupt_result;
}
static int qm_sem_signal(qm_sem_t *sem) {
    (void)sem; ++sem_signals; return 0;
}
static void *qm_malloc(size_t size) { return malloc(size); }
#define qm_memset memset
static int qm_agent_control_post_event(qm_agent_handle_t h,
                                         qm_agent_event_t *event) {
    assert(h == s_agent && event); free(event); return 0;
}
static void finish_custom_tts_input_if_ready(void) {}
/* Count public intent changes while executing the actual core setter. */
static void test_set_upload(qm_agent_handle_t h, qm_bool enabled);
"""


TESTS = r"""
static test_agent_t agent;
static int agent_lock_initialized;
static void reset_test(void) {
    if (agent_lock_initialized) pthread_mutex_destroy(&agent.audio.lock);
    memset(&agent, 0, sizeof(agent));
    assert(pthread_mutex_init(&agent.audio.lock, NULL) == 0);
    agent_lock_initialized = 1;
    agent.audio.runtime_valid = 1;
    agent.audio.dialog_state = QM_AGENT_DIALOG_STATE_SILENT;
    agent.config.transport_capture_policy = QM_AGENT_CAPTURE_POLICY_KEEP;
    s_agent = &agent; s_cloud_connected = 1; s_agent_started = 0;
    s_upload_policy_lock_ready = 1;
    s_tts_complete_sem_ready = 1; s_waiting_tts_complete = 0;
    s_resource_mode_active = 0;
    s_tts_wait_aborted = 0; s_tts_text_seen = 0;
    s_tts_speaking_seen = 0; s_tts_playback_started = 0;
    s_tts_playback_failed = 0; s_tts_playback_error = 0;
    s_tts_silent_seen = 0; start_result = 0; interrupt_result = -QM_ENOSYS;
    start_calls = interrupt_calls = transport_packets = scheduled = 0;
    sem_signals = upload_enables = upload_disables = 0;
}
static void packet(void) {
    uint8_t byte = 0;
    qm_audio_data_t packet = { &byte, 1 };
    on_capture_packet(&packet, s_agent);
}
static void state_event(int state) {
    qm_agent_event_notice_t event = {0};
    event.data.agent_state.state = state;
    qm_agent_audio_engine_set_dialog_state(s_agent, state);
    on_agent_event(NULL, QM_AGENT_EVENT_BASE, QM_AGENT_EVENT_AGENT_STATE,
                   &event, sizeof(event));
}
static void *callback_thread(void *arg) {
    (void)arg; thread_role = 1;
    state_event(QM_AGENT_DIALOG_STATE_SPEAKING);
    return NULL;
}
static void *wakeup_thread(void *arg) {
    (void)arg; thread_role = 2;
    on_wakeup();
    return NULL;
}
int main(void) {
    reset_test();
    on_wakeup();
    assert(start_calls == 1 && s_agent_started && upload_enables == 1);
    assert(agent.audio.user_upload_enabled && !agent.audio.upload_enabled);
    packet(); assert(transport_packets == 0);
    qm_agent_audio_engine_set_transport_ready(s_agent, QM_TRUE);
    packet(); assert(transport_packets == 1);
    on_wakeup(); on_wakeup();
    assert(start_calls == 1 && interrupt_calls == 2);
    assert(upload_enables == 3 && upload_disables == 2);
    packet(); assert(transport_packets == 2);
    puts("PASS: first/repeated wake restores real core upload admission after transport joins");

    assert(agent_app_set_resource_mode(QM_TRUE) == 0);
    packet(); assert(transport_packets == 2);
    assert(!agent.audio.user_upload_enabled);
    assert(agent_app_set_resource_mode(QM_FALSE) == 0);
    packet(); assert(transport_packets == 3);
    (void)agent_app_abort_output();
    state_event(QM_AGENT_DIALOG_STATE_SILENT);
    packet(); assert(transport_packets == 3);
    assert(!agent.audio.user_upload_enabled);
    puts("PASS: resource ownership and non-wake abort remain closed across dialog transitions");

    reset_test(); s_cloud_connected = 0;
    on_wakeup();
    assert(!start_calls && !interrupt_calls && !upload_enables);
    reset_test(); s_agent = NULL;
    on_wakeup(); assert(!start_calls && !upload_enables);
    reset_test(); start_result = -QM_EINIT;
    on_wakeup();
    assert(start_calls == 1 && !s_agent_started && !upload_enables);
    assert(!agent.audio.user_upload_enabled);
    puts("PASS: offline, absent agent and failed start do not open upload");

    reset_test(); s_agent_started = 1;
    s_waiting_tts_complete = 1;
    on_wakeup();
    assert(s_tts_wait_aborted && sem_signals == 1);
    assert(s_waiting_tts_complete); /* Cleanup still belongs to its waiter. */
    qm_agent_audio_engine_set_transport_ready(s_agent, QM_TRUE);
    state_event(QM_AGENT_DIALOG_STATE_SPEAKING);
    state_event(QM_AGENT_DIALOG_STATE_SILENT);
    packet(); assert(transport_packets == 1);
    assert(agent.audio.user_upload_enabled);
    puts("PASS: cancelled custom TTS cannot re-close upload via delayed state events");

    reset_test(); s_waiting_tts_complete = 1;
    on_wakeup();
    assert(start_calls == 1 && s_tts_wait_aborted && sem_signals == 1);
    assert(s_waiting_tts_complete && agent.audio.user_upload_enabled);
    qm_agent_audio_engine_set_transport_ready(s_agent, QM_TRUE);
    state_event(QM_AGENT_DIALOG_STATE_SILENT);
    packet(); assert(transport_packets == 1);
    puts("PASS: first wake cancels old custom wait before restoring upload");

    reset_test(); s_agent_started = 1;
    qm_agent_audio_engine_set_transport_ready(s_agent, QM_TRUE);
    qm_agent_set_upload_enabled(s_agent, QM_TRUE);
    s_waiting_tts_complete = 1;
    state_event(QM_AGENT_DIALOG_STATE_SPEAKING);
    packet(); assert(transport_packets == 0);
    assert(!agent.audio.user_upload_enabled && !s_tts_wait_aborted);
    puts("PASS: a live, uncancelled custom TTS transaction still suppresses upload");

    reset_test(); s_agent_started = 1; s_waiting_tts_complete = 1;
    qm_agent_audio_engine_set_transport_ready(s_agent, QM_TRUE);
    qm_agent_set_upload_enabled(s_agent, QM_TRUE);
    race_active = 1; callback_reached = wake_lock_attempted = callback_release = 0;
    pthread_t callback, wake;
    assert(pthread_create(&callback, NULL, callback_thread, NULL) == 0);
    pthread_mutex_lock(&hook_lock);
    while (!callback_reached) pthread_cond_wait(&hook_cond, &hook_lock);
    pthread_mutex_unlock(&hook_lock);
    assert(pthread_create(&wake, NULL, wakeup_thread, NULL) == 0);
    pthread_mutex_lock(&hook_lock);
    while (!wake_lock_attempted) pthread_cond_wait(&hook_cond, &hook_lock);
    callback_release = 1;
    pthread_cond_broadcast(&hook_cond);
    pthread_mutex_unlock(&hook_lock);
    assert(pthread_join(callback, NULL) == 0);
    assert(pthread_join(wake, NULL) == 0);
    race_active = 0;
    packet(); assert(transport_packets == 1 && agent.audio.user_upload_enabled);
    puts("PASS: paused old dialog guard/setter cannot overwrite concurrent wake restoration");
    return 0;
}
"""

BASELINE_MAIN = r"""
int main(void) {
    reset_test(); s_agent_started = 1;
    qm_agent_audio_engine_set_transport_ready(s_agent, QM_TRUE);
    qm_agent_set_upload_enabled(s_agent, QM_TRUE);
    packet(); assert(transport_packets == 1);
    on_wakeup();
    packet();
    if (transport_packets != 2 && !agent.audio.user_upload_enabled) {
        puts("REGRESSION: repeated wake closed persistent upload and rejected the next microphone packet");
        return 42;
    }
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-baseline", action="store_true",
                        help="Verify that the SDK HEAD reproduces the repeated-wake upload regression")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    sdk = root / "src/firmware"
    if args.check_baseline:
        app = subprocess.check_output(
            ["git", "show", "HEAD:apps/main/agent/agent_app.c"],
            cwd=sdk, text=True,
        )
    else:
        app = (sdk / "apps/main/agent/agent_app.c").read_text()
    core = (root / "src/firmware/qumios-dev/components/qm_agent/src/audio/"
            "qm_agent_audio_engine.c").read_text()
    core_names = ["refresh_upload_locked", "qm_agent_set_upload_enabled",
                  "qm_agent_audio_engine_set_transport_ready",
                  "qm_agent_audio_engine_set_dialog_state", "on_capture_packet"]
    app_names = ["abort_tts_wait", "agent_app_set_resource_mode",
                 "agent_app_abort_output", "resume_wakeup_upload",
                 "on_wakeup", "on_agent_event"]
    # These seven tests exercise the unchanged policy processor; async callback
    # and real cross-task waits are exercised by test-agent-wakeup-dispatch.py.
    has_worker = not args.check_baseline and "static void process_wakeup(" in app
    if has_worker:
        app_names[app_names.index("on_wakeup")] = "process_wakeup"
    if args.check_baseline:
        app_names.remove("resume_wakeup_upload")
    # Wrap the actual setter for call accounting; its body remains unmodified.
    core_code = "\n\n".join(function_source(core, name) for name in core_names)
    wrapped = r"""
static void test_set_upload(qm_agent_handle_t h, qm_bool enabled) {
    if (race_active && thread_role == 1 && !enabled) {
        pthread_mutex_lock(&hook_lock);
        callback_reached = 1;
        pthread_cond_broadcast(&hook_cond);
        while (!callback_release) pthread_cond_wait(&hook_cond, &hook_lock);
        pthread_mutex_unlock(&hook_lock);
    }
    if (enabled) ++upload_enables; else ++upload_disables;
    qm_agent_set_upload_enabled(h, enabled);
}
#define qm_agent_set_upload_enabled test_set_upload
"""
    app_code = "\n\n".join(function_source(app, name) for name in app_names)
    if has_worker:
        app_code += "\n#define on_wakeup process_wakeup\n"
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not compiler or not shutil.which(compiler[0]):
        raise SystemExit("A host C compiler is required (set CC if necessary)")
    with tempfile.TemporaryDirectory(prefix="agent-wakeup-test-") as directory:
        folder = Path(directory)
        source = folder / "test_wakeup.c"
        test_code = TESTS
        if args.check_baseline:
            test_code = TESTS[:TESTS.index("int main(void)")] + BASELINE_MAIN
        source.write_text(PREAMBLE + core_code + wrapped + app_code + test_code)
        binary = folder / ("test-wakeup.exe" if os.name == "nt" else "test-wakeup")
        subprocess.run(compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror",
                                  "-Wno-unused-function", "-pthread",
                                  str(source), "-o", str(binary)], check=True)
        if args.check_baseline:
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            if result.returncode != 42:
                raise SystemExit("SDK HEAD did not reproduce the expected upload regression")
            print("EXPECTED FAILURE (SDK HEAD): " + result.stdout.strip())
        else:
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
