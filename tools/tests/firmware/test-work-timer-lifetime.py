#!/usr/bin/env python3
"""Run real qm_work C against a deterministic preempting host event adapter.

The complete SDK source and its public qm_work.h are compiled without rewriting
function bodies. Only mutex, event queue and OS timer APIs are host adapters.
No board, credentials, firmware build or external Python dependency is needed.
"""

import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
SDK = ROOT / "src/firmware"
SOURCE = "qumios-dev/components/qm_event/src/qm_work.c"
HEADER = "qumios-dev/components/qm_event/include/qm_work.h"

CONFIG = """\
#define CONFIG_QM_WORK_SUPPORT 1
#define CONFIG_QM_OS_SUPPORT 1
#define CONFIG_QM_OS_TIMER_SUPPORT 1
#define CONFIG_QM_WORKQUEUE_NO_TASK 0
"""

QM_H = r"""
#ifndef HOST_QM_H
#define HOST_QM_H
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "qm_config.h"
#define QM_UNUSED_ARG(x) ((void)(x))
#define QM_WAIT_FOREVER UINT32_MAX
#define QM_NO_WAIT 0
#define QM_EOK 0
#define QM_EINVAL 22
#define QM_EINIT 19
#define QM_ENOMEM 12
#define QM_ENOSYS 38
typedef struct { void *hdl; } qm_timer_t;
typedef void (*qm_timer_cb_t)(qm_timer_t *, void *);
typedef struct { int locked; } qm_mutex_t;
int qm_mutex_new(qm_mutex_t *);
int qm_mutex_lock(qm_mutex_t *, uint32_t);
int qm_mutex_unlock(qm_mutex_t *);
int qm_timer_new(qm_timer_t *, qm_timer_cb_t, void *, int, int);
int qm_timer_start(qm_timer_t *);
int qm_timer_stop(qm_timer_t *);
int qm_timer_free(qm_timer_t *);
#endif
"""

EVENT_H = r"""
#include "qm.h"
typedef const char *qm_event_base_t;
typedef void (*qm_event_handler_t)(void *, qm_event_base_t, int32_t, void *, size_t);
#define QM_EVENT_DEFINE_BASE(id) qm_event_base_t const id = #id
int32_t qm_event_default_create(void);
int32_t qm_event_handler_register(qm_event_base_t, int32_t, qm_event_handler_t, void *);
int32_t qm_event_post(qm_event_base_t, int32_t, const void *, size_t, uint32_t);
"""

HARNESS = r"""
#include <stdio.h>
#include "qm_work_under_test.c"

typedef struct {
    qm_timer_t *owner;
    qm_timer_cb_t cb;
    void *arg;
    int live, started;
} host_timer_t;
typedef struct { qm_work_t work; int expected; } host_node_t;
static host_timer_t timers[512];
static int timer_count, live_timers, stop_count, free_count;
static int callback_count, callback_value, node_frees, immediate, post_failure;
static qm_event_handler_t handler;
static void *handler_arg;
static struct {
    qm_event_base_t base;
    int32_t id;
    size_t size;
    unsigned char data[128];
} events[32];
static int queue_len;

int qm_mutex_new(qm_mutex_t *lock) { lock->locked=0; return QM_EOK; }
int qm_mutex_lock(qm_mutex_t *lock, uint32_t timeout) {
    (void)timeout; assert(!lock->locked); lock->locked=1; return QM_EOK;
}
int qm_mutex_unlock(qm_mutex_t *lock) {
    assert(lock->locked); lock->locked=0; return QM_EOK;
}
int qm_timer_new(qm_timer_t *timer, qm_timer_cb_t cb, void *arg, int ms, int repeat) {
    assert(ms>0 && !repeat && timer_count<512 && !timer->hdl);
    host_timer_t *entry=&timers[timer_count++];
    *entry=(host_timer_t){.owner=timer,.cb=cb,.arg=arg,.live=1};
    timer->hdl=entry; live_timers++; return QM_EOK;
}
int qm_timer_start(qm_timer_t *timer) {
    host_timer_t *entry=timer->hdl;
    assert(entry && entry->live); entry->started=1; return QM_EOK;
}
int qm_timer_stop(qm_timer_t *timer) {
    host_timer_t *entry=timer->hdl;
    assert(entry && entry->live); entry->started=0; stop_count++; return QM_EOK;
}
int qm_timer_free(qm_timer_t *timer) {
    host_timer_t *entry=timer->hdl;
    assert(entry && entry->live); entry->live=0; timer->hdl=NULL;
    free_count++; live_timers--; return QM_EOK;
}
int32_t qm_event_default_create(void) { return QM_EOK; }
int32_t qm_event_handler_register(qm_event_base_t base, int32_t id,
                                qm_event_handler_t fn, void *arg) {
    (void)base; (void)id; handler=fn; handler_arg=arg; return QM_EOK;
}
int32_t qm_event_post(qm_event_base_t base, int32_t id, const void *data,
                     size_t size, uint32_t timeout) {
    (void)timeout; assert(handler && size<=sizeof(events[0].data));
    if(post_failure) return -QM_ENOMEM;
    if(immediate) {
        /* Model a higher-priority event task running before post returns. */
        unsigned char copy[128]; memcpy(copy,data,size);
        handler(handler_arg,base,id,copy,size);
    } else {
        assert(queue_len<32);
        events[queue_len].base=base; events[queue_len].id=id;
        events[queue_len].size=size; memcpy(events[queue_len].data,data,size);
        queue_len++;
    }
    return QM_EOK;
}
static void drain(void) {
    while(queue_len) {
        /* Keep event data valid when its callback reposts. */
        unsigned char data[128];
        qm_event_base_t base=events[0].base;
        int32_t id=events[0].id; size_t size=events[0].size;
        memcpy(data,events[0].data,size);
        memmove(events,events+1,(size_t)(--queue_len)*sizeof(events[0]));
        handler(handler_arg,base,id,data,size);
    }
}
static void fire(host_timer_t *entry) {
    assert(entry && entry->live && entry->started);
    /* The callback may free the entire owner; do not touch it afterwards. */
    qm_timer_cb_t cb=entry->cb; qm_timer_t *timer=entry->owner; void *arg=entry->arg;
    cb(timer,arg);
}
static void self_free(void *arg) {
    host_node_t *node=arg;
    assert(!node->work.active && !node->work.dly && !node->work.timer.hdl);
    callback_count++; callback_value=node->expected; node_frees++;
    free(node);
}
static host_node_t *node_new(int value) {
    host_node_t *node=calloc(1,sizeof(*node)); assert(node); node->expected=value;
    return node;
}
static void begin(void) {
    assert(!live_timers && !queue_len && !g_work_lock.locked);
    callback_count=callback_value=node_frees=stop_count=free_count=0;
    immediate=post_failure=0;
}
static void test_immediate(void) {
    begin(); immediate=1;
    for(int i=0;i<64;i++) {
        host_node_t *node=node_new(i);
        assert(qm_post_delayed_action(&node->work,self_free,node,20)==QM_EOK);
        fire(node->work.timer.hdl);
    }
    assert(callback_count==64 && node_frees==64 && !live_timers);
    assert(stop_count==64 && free_count==64);
    puts("PASS timer success: immediate preemption frees owner safely (64 rounds)");
}
static void test_deferred(void) {
    begin(); host_node_t *node=node_new(27);
    assert(qm_post_delayed_action(&node->work,self_free,node,20)==QM_EOK);
    fire(node->work.timer.hdl);
    assert(!callback_count && queue_len==1 && node->work.active);
    drain(); assert(callback_count==1 && callback_value==27 && !live_timers);
    assert(stop_count==1 && free_count==1);
    puts("PASS timer success: deferred dispatch releases timer exactly once");
}
static void test_failure(void) {
    begin(); host_node_t *node=node_new(1);
    assert(qm_post_delayed_action(&node->work,self_free,node,20)==QM_EOK);
    post_failure=1; fire(node->work.timer.hdl);
    assert(!callback_count && !queue_len && !live_timers);
    assert(!node->work.active && !node->work.dly && !node->work.timer.hdl);
    assert(stop_count==1 && free_count==1);
    assert(qm_cancel_delayed_action(&node->work)==QM_EOK);
    assert(free_count==1); free(node);
    puts("PASS failed event post: cancels and releases timer once, owner retained");
}
static void test_cancel(void) {
    begin(); host_node_t *node=node_new(1);
    assert(qm_post_delayed_action(&node->work,self_free,node,20)==QM_EOK);
    assert(qm_cancel_delayed_action(&node->work)==QM_EOK);
    /* A late OS callback sees inactive state and must not enqueue anything. */
    qm_work_delayed_action_timer_cb(NULL,&node->work);
    assert(!callback_count && !queue_len && !live_timers && free_count==1);
    free(node);
    node=node_new(2);
    assert(qm_post_delayed_action(&node->work,self_free,node,20)==QM_EOK);
    fire(node->work.timer.hdl); assert(queue_len==1);
    assert(qm_cancel_delayed_action(&node->work)==QM_EOK);
    drain(); assert(!callback_count && !live_timers && free_count==2);
    free(node);
    puts("PASS cancel before timer/after queued event: no callback or double release");
}
static void test_stale_repost(void) {
    begin(); host_node_t *node=node_new(1);
    assert(qm_post_delayed_action(&node->work,self_free,node,20)==QM_EOK);
    fire(node->work.timer.hdl); assert(queue_len==1);
    uint32_t old_seq=node->work.seq;
    node->expected=73;
    assert(qm_post_delayed_action(&node->work,self_free,node,30)==QM_EOK);
    assert(node->work.seq!=old_seq);
    drain(); assert(!callback_count && node->work.active && live_timers==1);
    fire(node->work.timer.hdl); drain();
    assert(callback_count==1 && callback_value==73 && !live_timers && free_count==2);
    puts("PASS stale sequence after repost: old event leaves replacement intact");
}
static void test_zero_delay(void) {
    begin(); immediate=1; host_node_t *node=node_new(44);
    assert(qm_post_delayed_action(&node->work,self_free,node,0)==QM_EOK);
    assert(callback_count==1 && callback_value==44 && !free_count && !live_timers);
    puts("PASS zero-delay immediate self-free: no timer and no post-return owner access");
}
int main(int argc, char **argv) {
    test_immediate();
    if(argc>1 && strcmp(argv[1],"immediate")==0) return 0;
    test_deferred(); test_failure(); test_cancel(); test_stale_repost(); test_zero_delay();
    assert(!live_timers && !queue_len && !g_work_lock.locked);
    return 0;
}
"""


def run(source, sanitized, immediate_only=False):
    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise SystemExit("C compiler not found")
    with tempfile.TemporaryDirectory(prefix="ticko-work-lifetime-") as folder:
        work = Path(folder)
        for name, content in {
            "qm_config.h": CONFIG, "qm.h": QM_H, "qm_event.h": EVENT_H,
            "qm_kernel.h": '#include "qm.h"\n',
            "qm_types.h": '#include "qm.h"\n',
            "qm_errno.h": '#include "qm.h"\n',
            "qm_work.h": (SDK / HEADER).read_text(),
            "qm_work_under_test.c": source, "runner.c": HARNESS,
        }.items():
            (work / name).write_text(content)
        command = [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
                   "-I", str(work), str(work / "runner.c"), "-o", str(work / "runner")]
        if sanitized:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(command, check=True, capture_output=True, text=True)
        leak_check = "0" if platform.system() == "Darwin" else "1"
        env = dict(os.environ, ASAN_OPTIONS=f"detect_leaks={leak_check}:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        return subprocess.run([str(work / "runner")] + (["immediate"] if immediate_only else []),
                              env=env, capture_output=True, text=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true", help="enable ASan and UBSan")
    parser.add_argument("--prove-original", action="store_true",
                        help="prove HEAD's original timer callback UAF with ASan; do not run current suite")
    args = parser.parse_args()
    if args.prove_original:
        source = subprocess.run(["git", "show", f"HEAD:{SOURCE}"], cwd=SDK,
                                check=True, capture_output=True, text=True).stdout
        result = run(source, True, immediate_only=True)
        if result.returncode == 0 or "heap-use-after-free" not in result.stderr:
            raise SystemExit("Original source did not reproduce the expected ASan heap-use-after-free\n"
                             + result.stdout + result.stderr)
        print("PASS original HEAD: ASan deterministically detects heap-use-after-free after immediate self-free")
        print(next(line for line in result.stderr.splitlines() if "ERROR: AddressSanitizer" in line))
        return
    result = run((SDK / SOURCE).read_text(), args.sanitize)
    print(result.stdout, end="")
    if result.returncode:
        raise SystemExit(result.stderr or f"C regression exited {result.returncode}")
    print("PASS complete real-C work lifetime regression" + (" (ASan + UBSan)" if args.sanitize else ""))


if __name__ == "__main__":
    main()
