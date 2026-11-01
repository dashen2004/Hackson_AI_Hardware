#!/usr/bin/env python3
"""Exercise actual wake callback/worker C against blocking cross-task queues.

The host adapter models audio_event -> qm_agent_ctl -> audio STOP synchronous
round trips with pthread queues, not immediate-return STOP/interrupt mocks.
The pre-fix synchronous callback must reproduce that wait cycle. This is a host
scheduling regression, not an ESP32/ASR, physical audio or long-duration test.
"""
import argparse
import importlib.util
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

PREAMBLE = r"""
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define QM_TRUE 1
#define QM_FALSE 0
#define QM_EOK 0
#define QM_ENOMEM 6
#define QM_EIO 8
#define QM_EINVAL 10
#define QM_EINIT 11
#define QM_EBUSY 7
#define QM_ETIMEOUT 2
#define QM_NO_WAIT 0
#define QM_ENOSYS 38
#define QM_WAIT_FOREVER (-1)
#define CONFIG_LJYD_PLAYBACK_SUPPORT 1
#define QM_AGENT_EVENT_AGENT_ABORTED 1
#define QM_AGENT_EVENT_SOURCE_TRANSPORT 2
#define QM_LOGI(...) ((void)0)
#define QM_LOGW(...) ((void)0)
#define QM_LOGE(...) ((void)0)
typedef int qm_bool;
typedef pthread_mutex_t qm_mutex_t;
typedef struct { pthread_mutex_t mutex; pthread_cond_t cond; int count, valid, initialized; } qm_sem_t;
typedef struct { pthread_t thread; } qm_task_t;
typedef struct { int unused; } test_agent_t;
typedef test_agent_t *qm_agent_handle_t;
typedef struct { int id, source, code; } qm_agent_event_t;
static test_agent_t agent;
static qm_agent_handle_t s_agent = &agent;
static uint8_t s_cloud_connected = 1, s_agent_started = 1;
static qm_task_t s_wakeup_task;
static qm_sem_t s_wakeup_sem, s_tts_complete_sem;
static uint8_t s_wakeup_worker_ready, s_wakeup_pending;
static qm_mutex_t s_upload_policy_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t s_upload_policy_lock_ready = 1, s_resource_mode_active;
static uint8_t s_tts_complete_sem_ready;
static volatile uint8_t s_waiting_tts_complete, s_tts_wait_aborted;
static volatile uint8_t s_tts_text_seen, s_tts_speaking_seen, s_tts_playback_started;
static volatile uint8_t s_tts_playback_failed, s_tts_silent_seen, s_tts_input_complete_posted;
static volatile int s_tts_playback_error;
static pthread_mutex_t hook = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static pthread_t audio_thread_id, ctl_thread_id;
static int callback_returns, callback_requests, wake_signals, worker_waits;
static int stop_requested, stop_completed, ctl_requested, ctl_completed, ctl_entered;
static int hold_ctl, interrupts, starts, start_result, upload_enabled, enables, disables;
static int resource_playing, pings, fail_sem_new, invalid_sem, fail_task_new, fail_signal;
static int sem_news, sem_frees, task_news, reopen_events;
static int hold_enable, enable_entered, allow_enable, claim_lock_attempted;
static int claim_result, resume_result;
static _Thread_local int thread_role;
static int audio_types[2048], audio_ids[2048], head, tail, count;
enum { AUDIO_WAKE = 1, AUDIO_STOP, AUDIO_PING };
static void on_wakeup(void);
static int agent_app_set_resource_mode(qm_bool active);
static void notify(void) { pthread_cond_broadcast(&changed); }
static int qm_mutex_lock(qm_mutex_t *m, int timeout) {
    (void)timeout;
    if (m == &s_upload_policy_lock && thread_role == 2) {
        pthread_mutex_lock(&hook); claim_lock_attempted = 1; notify(); pthread_mutex_unlock(&hook);
    }
    return pthread_mutex_lock(m);
}
static int qm_mutex_unlock(qm_mutex_t *m) { return pthread_mutex_unlock(m); }
static void qm_msleep(unsigned ms) {
    struct timespec t = { (time_t)(ms / 1000), (long)(ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}
static int qm_sem_new(qm_sem_t *s, int n) {
    ++sem_news;
    if (fail_sem_new) return -QM_ENOMEM;
    assert(pthread_mutex_init(&s->mutex, NULL) == 0);
    assert(pthread_cond_init(&s->cond, NULL) == 0);
    s->count = n; s->initialized = 1; s->valid = !invalid_sem;
    return QM_EOK;
}
static int qm_sem_is_valid(qm_sem_t *s) { return s->valid; }
static int qm_sem_free(qm_sem_t *s) {
    ++sem_frees;
    if (s->initialized) {
        pthread_cond_destroy(&s->cond); pthread_mutex_destroy(&s->mutex);
    }
    s->initialized = s->valid = 0; return QM_EOK;
}
static int qm_sem_signal(qm_sem_t *s) {
    if (s == &s_wakeup_sem) {
        pthread_mutex_lock(&hook);
        if (fail_signal) { fail_signal = 0; pthread_mutex_unlock(&hook); return -QM_EIO; }
        ++wake_signals; notify(); pthread_mutex_unlock(&hook);
    }
    assert(s->valid);
    pthread_mutex_lock(&s->mutex); ++s->count;
    pthread_cond_signal(&s->cond); pthread_mutex_unlock(&s->mutex);
    return QM_EOK;
}
static int qm_sem_wait(qm_sem_t *s, unsigned timeout) {
    (void)timeout;
    if (s == &s_wakeup_sem) {
        pthread_mutex_lock(&hook); ++worker_waits; notify(); pthread_mutex_unlock(&hook);
    }
    assert(s->valid);
    pthread_mutex_lock(&s->mutex);
    if (!s->count && timeout == QM_NO_WAIT) {
        pthread_mutex_unlock(&s->mutex); return -QM_ETIMEOUT;
    }
    while (!s->count) pthread_cond_wait(&s->cond, &s->mutex);
    --s->count; pthread_mutex_unlock(&s->mutex); return QM_EOK;
}
typedef struct { void (*fn)(void *); void *arg; } task_start_t;
static void *task_entry(void *arg) {
    task_start_t s = *(task_start_t *)arg; free(arg); s.fn(s.arg); return NULL;
}
static int qm_task_new(qm_task_t *task, const char *name, void (*fn)(void *), void *arg,
                       int stack, int priority) {
    assert(!strcmp(name, "agent_wakeup") && stack >= 4096 && priority > 0);
    ++task_news;
    if (fail_task_new) return -QM_ENOMEM;
    task_start_t *s = malloc(sizeof(*s)); assert(s); s->fn = fn; s->arg = arg;
    assert(pthread_create(&task->thread, NULL, task_entry, s) == 0);
    assert(pthread_detach(task->thread) == 0); return QM_EOK;
}
static void enqueue_locked(int type, int id) {
    assert(count < 2048); audio_types[tail] = type; audio_ids[tail] = id;
    tail = (tail + 1) % 2048; ++count; notify();
}
static void *audio_event_task(void *arg) {
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&hook);
        while (!count) pthread_cond_wait(&changed, &hook);
        int type = audio_types[head], id = audio_ids[head];
        head = (head + 1) % 2048; --count;
        pthread_mutex_unlock(&hook);
        if (type == AUDIO_WAKE) {
            on_wakeup();
            pthread_mutex_lock(&hook); ++callback_returns; notify(); pthread_mutex_unlock(&hook);
        } else {
            pthread_mutex_lock(&hook);
            if (type == AUDIO_STOP) { assert(id == stop_completed + 1); stop_completed = id; }
            else { assert(type == AUDIO_PING); ++pings; }
            notify(); pthread_mutex_unlock(&hook);
        }
    }
    return NULL;
}
static void synchronous_audio_stop(void) {
    pthread_mutex_lock(&hook);
    int id = ++stop_requested; enqueue_locked(AUDIO_STOP, id);
    while (stop_completed < id) pthread_cond_wait(&changed, &hook);
    pthread_mutex_unlock(&hook);
}
static void *agent_ctl_task(void *arg) {
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&hook);
        while (ctl_requested == ctl_completed) pthread_cond_wait(&changed, &hook);
        ++ctl_entered; notify();
        while (hold_ctl) pthread_cond_wait(&changed, &hook);
        pthread_mutex_unlock(&hook);
        /* Model actual interrupt -> abort_stream -> release_operation -> STOP:
         * this call cannot finish until the original audio event queue runs. */
        synchronous_audio_stop();
        pthread_mutex_lock(&hook); ++ctl_completed; notify(); pthread_mutex_unlock(&hook);
    }
    return NULL;
}
static int qm_agent_interrupt(qm_agent_handle_t h) {
    assert(h == s_agent);
    pthread_mutex_lock(&hook); ++interrupts;
    int id = ++ctl_requested; notify();
    while (ctl_completed < id) pthread_cond_wait(&changed, &hook);
    pthread_mutex_unlock(&hook); return -QM_ENOSYS;
}
static int playback_stop(void) {
    if (resource_playing) {
        synchronous_audio_stop(); resource_playing = 0;
        assert(agent_app_set_resource_mode(QM_FALSE) == QM_EOK);
    }
    return QM_EOK;
}
static int voice_audio_ready(void) { return 1; }
static int qm_agent_start(qm_agent_handle_t h) { assert(h == s_agent); ++starts; return start_result; }
static int qm_agent_get_state(qm_agent_handle_t h) { (void)h; return 1; }
static const char *life_name(int state) { (void)state; return "running"; }
static void qm_agent_set_upload_enabled(qm_agent_handle_t h, qm_bool enabled) {
    assert(h == s_agent);
    pthread_mutex_lock(&hook);
    if (enabled && hold_enable) {
        enable_entered = 1; notify();
        while (!allow_enable) pthread_cond_wait(&changed, &hook);
    }
    upload_enabled = enabled;
    if (enabled) ++enables; else ++disables;
    notify(); pthread_mutex_unlock(&hook);
}
static void *qm_malloc(size_t n) { return malloc(n); }
#define qm_memset memset
static int qm_agent_control_post_event(qm_agent_handle_t h, qm_agent_event_t *e) {
    assert(h == s_agent && e); free(e); ++reopen_events; return QM_EOK;
}
"""
TESTS = r"""
static struct timespec deadline(void) {
    struct timespec t; assert(clock_gettime(CLOCK_REALTIME, &t) == 0); t.tv_sec += 3; return t;
}
static void wait_complete(int callbacks, int controls, int waits) {
    struct timespec t = deadline();
    pthread_mutex_lock(&hook);
    while (callback_returns < callbacks || ctl_completed < controls || worker_waits < waits ||
           __atomic_load_n(&s_wakeup_pending, __ATOMIC_ACQUIRE)) {
        assert(pthread_cond_timedwait(&changed, &hook, &t) != ETIMEDOUT);
    }
    pthread_mutex_unlock(&hook);
}
static void start_contexts(void) {
    assert(pthread_create(&audio_thread_id, NULL, audio_event_task, NULL) == 0);
    assert(pthread_detach(audio_thread_id) == 0);
    assert(pthread_create(&ctl_thread_id, NULL, agent_ctl_task, NULL) == 0);
    assert(pthread_detach(ctl_thread_id) == 0);
    assert(qm_sem_new(&s_tts_complete_sem, 0) == QM_EOK); s_tts_complete_sem_ready = 1;
}
static int emit_wake(void) {
    pthread_mutex_lock(&hook); int id = ++callback_requests;
    enqueue_locked(AUDIO_WAKE, 0); pthread_mutex_unlock(&hook); return id;
}
static void setup_worker(void) { start_contexts(); assert(wakeup_worker_init() == QM_EOK); }
static void test_cycle(void) {
    setup_worker();
    assert(!pthread_equal(s_wakeup_task.thread, audio_thread_id));
    assert(!pthread_equal(s_wakeup_task.thread, ctl_thread_id));
    int id = emit_wake(); wait_complete(id, 1, 2);
    assert(stop_completed == 1 && interrupts == 1 && upload_enabled && reopen_events == 1);
    puts("PASS: actual WAKE callback returns while worker/ctl/audio STOP complete their blocking round trip");
}
static void test_resource(void) {
    setup_worker(); resource_playing = 1;
    assert(agent_app_set_resource_mode(QM_TRUE) == QM_EOK && !upload_enabled);
    s_waiting_tts_complete = 1;
    int id = emit_wake(); wait_complete(id, 1, 2);
    assert(!resource_playing && !s_resource_mode_active && stop_completed == 2);
    assert(s_tts_wait_aborted && upload_enabled && interrupts == 1);
    puts("PASS: resource STOP and TTS interrupt both complete off audio_event and restore input ownership");
}
static void test_coalesce(void) {
    setup_worker();
    pthread_mutex_lock(&hook); hold_ctl = 1; pthread_mutex_unlock(&hook);
    emit_wake();
    struct timespec t = deadline();
    pthread_mutex_lock(&hook);
    while (!ctl_entered) assert(pthread_cond_timedwait(&changed, &hook, &t) != ETIMEDOUT);
    for (int i = 0; i < 100; ++i) { ++callback_requests; enqueue_locked(AUDIO_WAKE, 0); }
    enqueue_locked(AUDIO_PING, 0);
    while (callback_returns < 101 || pings != 1)
        assert(pthread_cond_timedwait(&changed, &hook, &t) != ETIMEDOUT);
    assert(interrupts == 1 && wake_signals == 1);
    hold_ctl = 0; notify(); pthread_mutex_unlock(&hook);
    wait_complete(101, 1, 2);
    int id = emit_wake(); wait_complete(id, 2, 3);
    assert(wake_signals == 2 && interrupts == 2 && upload_enabled);
    puts("PASS: 100 busy WAKE events coalesce without blocking audio traffic; a later wake is accepted");
}
static void test_later(void) {
    setup_worker();
    pthread_mutex_lock(&hook);
    for (int i = 0; i < 1000; ++i) enqueue_locked(AUDIO_PING, 0);
    struct timespec t = deadline();
    while (pings != 1000) assert(pthread_cond_timedwait(&changed, &hook, &t) != ETIMEDOUT);
    assert(!interrupts); pthread_mutex_unlock(&hook);
    qm_msleep(200);
    for (int i = 1; i <= 3; ++i) { int id = emit_wake(); wait_complete(id, i, i + 1); }
    assert(upload_enabled && interrupts == 3 && stop_completed == 3);
    puts("PASS: the persistent waiting worker accepts later repeated wakes after sustained unrelated audio traffic");
}
static void test_start_offline(void) {
    setup_worker(); s_agent_started = 0;
    int id = emit_wake(); wait_complete(id, 0, 2);
    assert(starts == 1 && s_agent_started && upload_enabled);
    s_cloud_connected = 0; upload_enabled = 0;
    id = emit_wake(); wait_complete(id, 0, 3);
    assert(starts == 1 && !interrupts && !upload_enabled);
    s_cloud_connected = 1; s_agent = NULL;
    id = emit_wake(); wait_complete(id, 0, 4); assert(!upload_enabled);
    s_agent = &agent; s_agent_started = 0; start_result = -QM_EIO;
    id = emit_wake(); wait_complete(id, 0, 5);
    assert(starts == 2 && !s_agent_started && !upload_enabled);
    puts("PASS: first start remains correct and offline/absent/failed-start wakes do not enable upload");
}
static void test_init(void) {
    fail_sem_new = 1; assert(wakeup_worker_init() == -QM_ENOMEM);
    assert(!s_wakeup_worker_ready && !task_news);
    fail_sem_new = 0; invalid_sem = 1;
    assert(wakeup_worker_init() == -QM_ENOMEM && sem_frees == 1);
    invalid_sem = 0; fail_task_new = 1;
    assert(wakeup_worker_init() == -QM_ENOMEM && sem_frees == 2);
    fail_task_new = 0; assert(wakeup_worker_init() == QM_EOK);
    int created = sem_news, tasks = task_news;
    assert(wakeup_worker_init() == QM_EOK && sem_news == created && task_news == tasks);
    assert(s_wakeup_worker_ready);
    puts("PASS: worker resource failures clean up and initialization is idempotent before callback registration");
}
static void test_signal_failure(void) {
    setup_worker(); fail_signal = 1;
    int id = emit_wake(); wait_complete(id, 0, 1);
    assert(!interrupts && !__atomic_load_n(&s_wakeup_pending, __ATOMIC_ACQUIRE));
    id = emit_wake(); wait_complete(id, 1, 2);
    assert(upload_enabled && wake_signals == 1);
    puts("PASS: failed nonblocking wake post clears pending and a subsequent real callback retries normally");
}
static void test_resume(void) {
    assert(agent_app_resume_input() == QM_EOK && upload_enabled);
    upload_enabled = 0; s_waiting_tts_complete = 1;
    assert(agent_app_resume_input() == -QM_EBUSY && !upload_enabled);
    s_waiting_tts_complete = 0;
    assert(agent_app_set_resource_mode(QM_TRUE) == QM_EOK);
    assert(agent_app_resume_input() == -QM_EBUSY && !upload_enabled);
    assert(agent_app_set_resource_mode(QM_FALSE) == QM_EOK);
    upload_enabled = 0; assert(agent_app_resume_input() == QM_EOK && upload_enabled);
    upload_enabled = 0; s_cloud_connected = 0;
    assert(agent_app_resume_input() == -QM_EINIT && !upload_enabled);
    s_cloud_connected = 1; s_agent = NULL;
    assert(agent_app_resume_input() == -QM_EINIT && !upload_enabled);
    s_agent = &agent; s_agent_started = 0;
    assert(agent_app_resume_input() == -QM_EINIT && !upload_enabled);
    s_agent_started = 1; s_upload_policy_lock_ready = 0;
    assert(agent_app_resume_input() == -QM_EINIT && !upload_enabled);
    puts("PASS: explicit application resume cannot steal input from live TTS/resource or an absent cloud/session");
}
static void *resume_thread(void *arg) {
    (void)arg; resume_result = agent_app_resume_input(); return NULL;
}
static void *claim_thread(void *arg) {
    (void)arg; thread_role = 2; claim_result = begin_tts_wait(); return NULL;
}
static void test_resume_race(void) {
    assert(qm_sem_new(&s_tts_complete_sem, 0) == QM_EOK); s_tts_complete_sem_ready = 1;
    hold_enable = 1;
    pthread_t resume, claim;
    assert(pthread_create(&resume, NULL, resume_thread, NULL) == 0);
    struct timespec t = deadline(); pthread_mutex_lock(&hook);
    while (!enable_entered) assert(pthread_cond_timedwait(&changed, &hook, &t) != ETIMEDOUT);
    pthread_mutex_unlock(&hook);
    assert(pthread_create(&claim, NULL, claim_thread, NULL) == 0);
    pthread_mutex_lock(&hook);
    while (!claim_lock_attempted) assert(pthread_cond_timedwait(&changed, &hook, &t) != ETIMEDOUT);
    allow_enable = 1; notify(); pthread_mutex_unlock(&hook);
    assert(pthread_join(resume, NULL) == 0 && pthread_join(claim, NULL) == 0);
    assert(resume_result == QM_EOK && claim_result == QM_EOK);
    assert(s_waiting_tts_complete && !upload_enabled);
    assert(agent_app_resume_input() == -QM_EBUSY && !upload_enabled);
    assert(begin_tts_wait() == -QM_EBUSY);
    puts("PASS: a new actual TTS claim serializes against a concurrent finally-resume and retains input ownership");
}
static int test_baseline(void) {
    start_contexts(); emit_wake();
    struct timespec t = deadline(); pthread_mutex_lock(&hook);
    while (!ctl_entered || !stop_requested)
        assert(pthread_cond_timedwait(&changed, &hook, &t) != ETIMEDOUT);
    assert(callback_returns == 0 && stop_completed == 0 && ctl_completed == 0);
    pthread_mutex_unlock(&hook);
    puts("REGRESSION: synchronous audio WAKE waits for ctl, whose STOP waits for the same audio event queue");
    return 42;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "cycle")) test_cycle();
    else if (!strcmp(argv[1], "resource")) test_resource();
    else if (!strcmp(argv[1], "coalesce")) test_coalesce();
    else if (!strcmp(argv[1], "later")) test_later();
    else if (!strcmp(argv[1], "start-offline")) test_start_offline();
    else if (!strcmp(argv[1], "init")) test_init();
    else if (!strcmp(argv[1], "signal-failure")) test_signal_failure();
    else if (!strcmp(argv[1], "resume")) test_resume();
    else if (!strcmp(argv[1], "resume-race")) test_resume_race();
    else if (!strcmp(argv[1], "baseline")) return test_baseline();
    else assert(0);
    return 0;
}
"""
CASES = ("cycle", "resource", "coalesce", "later", "start-offline", "init", "signal-failure", "resume", "resume-race")

def main():
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-baseline', action='store_true', help='Require the original synchronous callback layout to reproduce the cycle')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('wakeup_policy', root / 'tools/tests/firmware/test-agent-wakeup.py')
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    app = (root / 'src/firmware/apps/main/agent/agent_app.c').read_text()
    names = ('drain_tts_complete_sem', 'begin_tts_wait', 'abort_tts_wait', 'agent_app_set_resource_mode', 'agent_app_resume_input',
             'agent_app_abort_output', 'resume_wakeup_upload', 'process_wakeup',
             'wakeup_task', 'wakeup_worker_init', 'on_wakeup')
    definitions = [module.function_source(app, name) for name in names]
    if args.check_baseline:
        definitions[-1] = module.function_source(app, 'process_wakeup').replace('process_wakeup(', 'on_wakeup(', 1)
    init = module.function_source(app, 'agent_app_init')
    assert init.index('wakeup_worker_init()') < init.index('voice_set_wakeup_cb(on_wakeup)')
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    if not compiler or not shutil.which(compiler[0]): parser.error('A host C compiler is required')
    with tempfile.TemporaryDirectory(prefix='agent-wakeup-dispatch-') as folder:
        temp = Path(folder); source = temp / 'dispatch.c'; binary = temp / 'dispatch'
        source.write_text(PREAMBLE + '\n'.join(definitions) + TESTS)
        command = compiler + ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-pthread']
        if args.sanitize: command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        command += [str(source), '-o', str(binary)]
        subprocess.run(command, check=True)
        env = os.environ.copy()
        if args.sanitize:
            env['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'; env['UBSAN_OPTIONS'] = 'halt_on_error=1'
        for case in (('baseline',) if args.check_baseline else CASES):
            result = subprocess.run([str(binary), case], text=True, capture_output=True, env=env, timeout=10)
            if args.check_baseline:
                if result.returncode != 42: raise SystemExit(result.stdout + result.stderr + '\nBaseline wait cycle did not reproduce')
                print('EXPECTED FAILURE: ' + result.stdout.strip())
            else:
                if result.returncode: raise SystemExit(result.stdout + result.stderr + f'\nFAILED: {case}, exit={result.returncode}')
                print(result.stdout.strip(), flush=True)

if __name__ == '__main__': main()
