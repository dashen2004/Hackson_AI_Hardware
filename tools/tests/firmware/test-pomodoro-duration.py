#!/usr/bin/env python3
"""Compile real qm_pomodoro.c with deterministic host timer and pthread mutexes.

Exercises duration overrides and existing component lifecycle; it does not
validate MCU wall-clock accuracy, FreeRTOS scheduling, network, speech or LEDs.
A stopped timer's late callback is tested while its owner remains alive; calling
an external stale handle after destroy returns is not part of the API contract.
"""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

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
SOURCE = r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qm_kernel.h"
#include "qm_pomodoro.h"

static qm_timer_t *timer;
static int start_failure;
static atomic_int live_allocations;
static atomic_int timer_frees;
static pthread_mutex_t gate_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cond = PTHREAD_COND_INITIALIZER;
static int block_start_event, callback_entered, release_callback, destroy_waiting;
static qm_pomodoro_event_data_t events[128];
static int event_count;

void *qm_calloc(size_t n, size_t size) { void *p=calloc(n,size); if(p)atomic_fetch_add(&live_allocations,1); return p; }
void qm_free(void *p) { if(p)atomic_fetch_sub(&live_allocations,1); free(p); }
int qm_mutex_new(qm_mutex_t *p) { return pthread_mutex_init(p,NULL); }
int qm_mutex_lock(qm_mutex_t *p,int timeout) { (void)timeout; return pthread_mutex_lock(p); }
int qm_mutex_unlock(qm_mutex_t *p) { return pthread_mutex_unlock(p); }
int qm_mutex_free(qm_mutex_t *p) { return pthread_mutex_destroy(p); }
int qm_timer_new(qm_timer_t *p,void(*cb)(qm_timer_t*,void*),void *arg,int period,int periodic) {
    assert(periodic==1); p->cb=cb;p->arg=arg;p->period=period;p->active=0;timer=p;return QM_EOK;
}
int qm_timer_start(qm_timer_t *p) { if(start_failure){start_failure=0;return -QM_EIO;}p->active=1;return QM_EOK; }
int qm_timer_stop(qm_timer_t *p) { p->active=0;return QM_EOK; }
int qm_timer_free(qm_timer_t *p) { assert(p==timer);timer=NULL;atomic_fetch_add(&timer_frees,1);return QM_EOK; }
int qm_msleep(int ms) {
    struct timespec pause={0,1000000};(void)ms;
    pthread_mutex_lock(&gate_lock);destroy_waiting=1;pthread_cond_broadcast(&gate_cond);pthread_mutex_unlock(&gate_lock);
    nanosleep(&pause,NULL);return QM_EOK;
}
static void event_cb(qm_pomodoro_t p,const qm_pomodoro_event_data_t *e) {
    (void)p;pthread_mutex_lock(&gate_lock);assert(event_count<128);events[event_count++]=*e;
    if(block_start_event && e->event==QM_POMODORO_EVENT_STATE_CHANGED && e->status.state==QM_POMODORO_STATE_FOCUS) {
        callback_entered=1;pthread_cond_broadcast(&gate_cond);
        while(!release_callback)pthread_cond_wait(&gate_cond,&gate_lock);
    }
    pthread_mutex_unlock(&gate_lock);
}
static qm_pomodoro_t make_timer(uint32_t target) {
    qm_pomodoro_config_t c={0};c.focus_sec=5;c.short_break_sec=1;c.long_break_sec=2;
    c.long_break_interval=4;c.focus_target=target;c.tick_ms=1000;c.event_cb=event_cb;
    qm_pomodoro_t p=qm_pomodoro_create(&c);assert(p);return p;
}
static qm_pomodoro_status_t status(qm_pomodoro_t p) { qm_pomodoro_status_t s;assert(qm_pomodoro_get_status(p,&s)==0);return s; }
static void tick(unsigned n) { assert(timer);while(n--){assert(timer->active);timer->cb(timer,timer->arg);} }
static void late_tick(void) { assert(timer);timer->cb(timer,timer->arg); }
static int count_event(qm_pomodoro_event_t kind) { int n=0;for(int i=0;i<event_count;i++)if(events[i].event==kind)n++;return n; }
static void finish(qm_pomodoro_t p) { qm_pomodoro_destroy(p);assert(!timer);assert(atomic_load(&live_allocations)==0); }
static void defaults(void) {
    qm_pomodoro_t p=make_timer(1);assert(qm_pomodoro_start_for(p,7)==0);assert(status(p).duration_sec==7);
    tick(2);assert(status(p).remaining_sec==5);assert(qm_pomodoro_stop(p)==0);
    assert(qm_pomodoro_start_for(p,3)==0);assert(status(p).duration_sec==3);assert(qm_pomodoro_stop(p)==0);
    assert(qm_pomodoro_start(p)==0);assert(status(p).duration_sec==5);finish(p);
}
static void busy(void) {
    qm_pomodoro_t p=make_timer(1);assert(qm_pomodoro_start_for(p,7)==0);tick(2);
    assert(qm_pomodoro_start_for(p,20)==-QM_EBUSY);assert(qm_pomodoro_start(p)==-QM_EBUSY);
    assert(status(p).remaining_sec==5);assert(status(p).duration_sec==7);
    assert(qm_pomodoro_pause(p)==0);assert(qm_pomodoro_start_for(p,9)==-QM_EBUSY);late_tick();
    assert(status(p).paused && status(p).remaining_sec==5);assert(qm_pomodoro_resume(p)==0);
    tick(1);assert(status(p).remaining_sec==4);assert(qm_pomodoro_stop(p)==0);
    assert(qm_pomodoro_start_for(p,4)==0);assert(status(p).duration_sec==4);finish(p);
}
static void natural(void) {
    qm_pomodoro_t p=make_timer(1);assert(qm_pomodoro_start_for(p,2)==0);tick(2);
    qm_pomodoro_status_t s=status(p);assert(s.state==QM_POMODORO_STATE_FINISHED && s.completed_focus==1 && s.remaining_sec==0);
    assert(!timer->active);assert(count_event(QM_POMODORO_EVENT_PHASE_DONE)==1);assert(count_event(QM_POMODORO_EVENT_FINISHED)==1);
    int count=event_count;late_tick();assert(event_count==count);assert(status(p).state==QM_POMODORO_STATE_FINISHED);
    assert(qm_pomodoro_start_for(p,9)==0);assert(status(p).duration_sec==9 && status(p).completed_focus==0);finish(p);
}
static void rounds(void) {
    qm_pomodoro_t p=make_timer(2);assert(qm_pomodoro_start_for(p,2)==0);tick(2);
    assert(status(p).state==QM_POMODORO_STATE_SHORT_BREAK);tick(1);
    assert(status(p).state==QM_POMODORO_STATE_FOCUS && status(p).duration_sec==2);tick(2);
    assert(status(p).state==QM_POMODORO_STATE_FINISHED && status(p).completed_focus==2);
    assert(qm_pomodoro_start(p)==0);assert(status(p).duration_sec==5);finish(p);
}
static void boundaries(void) {
    qm_pomodoro_t p=make_timer(1);uint32_t max=UINT32_MAX/1000u;
    assert(qm_pomodoro_start_for(NULL,1)==-QM_EINVAL);assert(qm_pomodoro_start_for(p,0)==-QM_EINVAL);
    assert(qm_pomodoro_start_for(p,max+1)==-QM_EINVAL);assert(qm_pomodoro_start_for(p,UINT32_MAX)==-QM_EINVAL);
    assert(status(p).state==QM_POMODORO_STATE_IDLE);assert(event_count==0);
    assert(qm_pomodoro_start_for(p,max)==0);assert(status(p).duration_sec==max && status(p).remaining_sec==max);
    assert(qm_pomodoro_stop(p)==0);assert(qm_pomodoro_start_for(p,1)==0);tick(1);assert(status(p).state==QM_POMODORO_STATE_FINISHED);finish(p);
}
static void start_failure_rollback(void) {
    qm_pomodoro_t p=make_timer(1);assert(qm_pomodoro_start_for(p,1)==0);tick(1);
    int count=event_count;start_failure=1;assert(qm_pomodoro_start_for(p,13)==-QM_EIO);
    qm_pomodoro_status_t s=status(p);assert(s.state==QM_POMODORO_STATE_FINISHED && s.completed_focus==1 && s.duration_sec==0);
    assert(event_count==count);assert(qm_pomodoro_start(p)==0);assert(status(p).duration_sec==5);finish(p);
}
static void stopped_late_tick(void) {
    qm_pomodoro_t p=make_timer(1);assert(qm_pomodoro_start_for(p,3)==0);tick(1);assert(qm_pomodoro_stop(p)==0);
    int count=event_count;late_tick();assert(event_count==count && status(p).state==QM_POMODORO_STATE_IDLE);
    assert(qm_pomodoro_start_for(p,8)==0);assert(status(p).remaining_sec==8);finish(p);
}
static void *start_thread(void *arg) { assert(qm_pomodoro_start_for(arg,7)==0);return NULL; }
static void *destroy_thread(void *arg) { qm_pomodoro_destroy(arg);return NULL; }
static void wait_callback(void) { pthread_mutex_lock(&gate_lock);while(!callback_entered)pthread_cond_wait(&gate_cond,&gate_lock);pthread_mutex_unlock(&gate_lock); }
static void release_event(void) { pthread_mutex_lock(&gate_lock);release_callback=1;pthread_cond_broadcast(&gate_cond);pthread_mutex_unlock(&gate_lock); }
static void concurrent_stop(void) {
    qm_pomodoro_t p=make_timer(1);pthread_t worker;block_start_event=1;assert(pthread_create(&worker,NULL,start_thread,p)==0);wait_callback();
    assert(qm_pomodoro_stop(p)==0);assert(status(p).state==QM_POMODORO_STATE_IDLE);
    release_event();assert(pthread_join(worker,NULL)==0);assert(status(p).state==QM_POMODORO_STATE_IDLE);
    block_start_event=0;assert(qm_pomodoro_start_for(p,4)==0);assert(status(p).duration_sec==4);finish(p);
}
static void concurrent_destroy(void) {
    qm_pomodoro_t p=make_timer(1);pthread_t starter,destroyer;block_start_event=1;
    assert(pthread_create(&starter,NULL,start_thread,p)==0);wait_callback();assert(pthread_create(&destroyer,NULL,destroy_thread,p)==0);
    pthread_mutex_lock(&gate_lock);while(!destroy_waiting)pthread_cond_wait(&gate_cond,&gate_lock);pthread_mutex_unlock(&gate_lock);
    assert(atomic_load(&timer_frees)==0);assert(qm_pomodoro_start_for(p,6)==-QM_EBUSY);
    release_event();assert(pthread_join(starter,NULL)==0);assert(pthread_join(destroyer,NULL)==0);
    assert(atomic_load(&timer_frees)==1 && atomic_load(&live_allocations)==0 && !timer);
}
int main(int argc,char **argv) {
    assert(argc==2);
    if(!strcmp(argv[1],"defaults"))defaults();else if(!strcmp(argv[1],"busy"))busy();
    else if(!strcmp(argv[1],"natural"))natural();else if(!strcmp(argv[1],"rounds"))rounds();
    else if(!strcmp(argv[1],"boundaries"))boundaries();else if(!strcmp(argv[1],"rollback"))start_failure_rollback();
    else if(!strcmp(argv[1],"late"))stopped_late_tick();else if(!strcmp(argv[1],"stop_race"))concurrent_stop();
    else if(!strcmp(argv[1],"destroy_race"))concurrent_destroy();else return 2;
    printf("PASS %s\n",argv[1]);return 0;
}
'''
CASES = ["defaults", "busy", "natural", "rounds", "boundaries", "rollback", "late", "stop_race", "destroy_race"]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--case", action="append", choices=CASES)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[3]
    sdk = repo / "src/firmware"
    core = sdk / "qumios-dev/components/qm_pomodoro"
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not shutil.which(compiler[0]):
        raise SystemExit("C compiler unavailable")
    with tempfile.TemporaryDirectory(prefix="ticko-pomodoro-duration-") as folder:
        root = Path(folder)
        (root / "qm_types.h").write_text(TYPES)
        (root / "qm_kernel.h").write_text(KERNEL)
        shutil.copy2(sdk / "qumios-dev/include/common/qm_errno.h", root / "qm_errno.h")
        (root / "test.c").write_text(SOURCE)
        binary = root / "test-duration"
        command = compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread", "-I", str(root), "-I", str(core / "include")]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        command += [str(core / "src/qm_pomodoro.c"), str(root / "test.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        env = os.environ.copy()
        if args.sanitize:
            env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
            env["UBSAN_OPTIONS"] = "halt_on_error=1"
        for case in args.case or CASES:
            subprocess.run([str(binary), case], check=True, env=env, timeout=20)

if __name__ == "__main__":
    main()
