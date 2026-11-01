#!/usr/bin/env python3
"""Run real completion-task C with deterministic queue/audio substitutes.

Checks queue acceptance vs delivery, stale rounds, TTS failure fallback and
user interruption. Does not prove physical speaker output or cloud delivery.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

HEADER = r'''
#include <stdint.h>
#include <stddef.h>
typedef void *qm_queue_t;
typedef void *qm_task_t;
#define QM_EOK 0
#define QM_EBUSY 1
#define QM_ENOSYS 2
#define QM_EINVAL 3
#define QM_EINIT 4
#define QM_EIO 5
#define QM_EINTR 6
#define QM_WAIT_FOREVER 0xffffffffU
void host_log(const char*,const char*,...);
#define QM_LOGI(...) host_log(__VA_ARGS__)
#define QM_LOGW(...) host_log(__VA_ARGS__)
int qm_queue_new(qm_queue_t*,unsigned,unsigned);
int qm_queue_free(qm_queue_t*);
int qm_queue_send(qm_queue_t*,void*,unsigned);
int qm_queue_recv(qm_queue_t*,void*,unsigned*,unsigned);
int qm_task_new(qm_task_t*,const char*,void(*)(void*),void*,int,int);
int pomodoro_is_finished_round(uint32_t);
int pomodoro_notify_finished(uint32_t);
int agent_app_abort_output(void);
int agent_app_user_corpus(const char*);
int agent_app_wait_tts_complete(uint32_t);
int agent_app_resume_input(void);
int boot_sound_start(void);
int iot_control_focus_report_state(void);
'''

HARNESS = r'''
#include "qm.h"
#include "pomodoro_notify.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
void host_log(const char*t,const char*f,...){(void)t;(void)f;}
static jmp_buf out;
static void (*worker)(void*);
static uint32_t queue[2], current=1;
static unsigned count;
static int queue_error, task_error, freed;
static int abort_ret, corpus_ret, wait_ret;
static int aborted, corpus, waited, resumed, chime;
int qm_queue_new(qm_queue_t*q,unsigned n,unsigned size){assert(n==2 && size==4);*q=(void*)1;return queue_error;}
int qm_queue_free(qm_queue_t*q){(void)q;freed++;return 0;}
int qm_task_new(qm_task_t*t,const char*n,void(*f)(void*),void*a,int stack,int p){(void)t;(void)n;(void)a;assert(stack>=4096 && p<10);worker=f;return task_error;}
int qm_queue_send(qm_queue_t*q,void*m,unsigned n){(void)q;assert(n==4);if(count==2)return -QM_EBUSY;queue[count++]=*(uint32_t*)m;return 0;}
int qm_queue_recv(qm_queue_t*q,void*m,unsigned*n,unsigned ms){(void)q;assert(ms==QM_WAIT_FOREVER);if(!count)longjmp(out,1);*(uint32_t*)m=queue[0];queue[0]=queue[1];count--;*n=4;return 0;}
int pomodoro_is_finished_round(uint32_t id){return id==current;}
int agent_app_abort_output(void){aborted++;return abort_ret;}
int agent_app_user_corpus(const char*s){assert(strstr(s,"这一轮专注时间到了"));corpus++;return corpus_ret;}
int agent_app_wait_tts_complete(uint32_t ms){assert(ms==20000);waited++;return wait_ret;}
int agent_app_resume_input(void){resumed++;return 0;}
int boot_sound_start(void){chime++;return 0;}
int iot_control_focus_report_state(void){return 0;}
static void drain(void){if(setjmp(out)==0)worker(NULL);}
static void reset_audio(void){aborted=corpus=waited=resumed=chime=0;abort_ret=corpus_ret=wait_ret=0;}
int main(void){
 assert(pomodoro_notify_finished(1)==-QM_EINIT);
 queue_error=-QM_EIO;assert(pomodoro_notify_init()==-QM_EIO);
 queue_error=0;task_error=-QM_EIO;assert(pomodoro_notify_init()==-QM_EIO && freed==1);
 task_error=0;assert(pomodoro_notify_init()==0);assert(pomodoro_notify_init()==0);
 assert(pomodoro_notify_finished(0)==-QM_EINVAL);
 assert(pomodoro_notify_finished(1)==0 && corpus==0);drain();
 assert(aborted==1 && corpus==1 && waited==1 && resumed==1 && !chime);
 puts("PASS init failures, async queue and successful reminder");
 reset_audio();assert(pomodoro_notify_finished(1)==0);current=2;drain();
 assert(!aborted && !corpus && !chime);puts("PASS stale round skips output");
 current=1;reset_audio();corpus_ret=-QM_EIO;assert(!pomodoro_notify_finished(1));drain();
 assert(corpus==1 && !waited && resumed==1 && chime==1);puts("PASS corpus failure resumes input and queues local cue");
 reset_audio();abort_ret=-QM_ENOSYS;assert(!pomodoro_notify_finished(1));drain();
 assert(corpus==1 && waited==1 && !chime);puts("PASS normalized Agora interrupt continues speech");
 reset_audio();wait_ret=-QM_EINTR;assert(!pomodoro_notify_finished(1));drain();
 assert(waited==1 && resumed==1 && !chime);puts("PASS user interruption does not start fallback audio");
 reset_audio();assert(!pomodoro_notify_finished(1));assert(!pomodoro_notify_finished(1));
 assert(pomodoro_notify_finished(1)==-QM_EBUSY);current=0;drain();assert(!corpus);
 puts("PASS queue backpressure rejects acceptance");return 0;
}
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    source = root / 'src/firmware/apps/main/lj_components/pomodoro'
    with tempfile.TemporaryDirectory(prefix='ticko-focus-notify-') as tmp:
        tmp = Path(tmp)
        for name in ('qm.h', 'pomodoro.h', 'agent_app.h', 'boot_sound.h', 'iot_control_focus.h'):
            (tmp / name).write_text('#pragma once\n' + HEADER)
        (tmp / 'pomodoro_notify.h').write_text((source / 'pomodoro_notify.h').read_text())
        # Local quoted headers must resolve to substitutes, not live app headers.
        (tmp / 'notify.c').write_text((source / 'pomodoro_notify.c').read_text())
        (tmp / 'test.c').write_text(HARNESS)
        command = ['clang', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(tmp),
                   str(tmp / 'notify.c'), str(tmp / 'test.c'), '-o', str(tmp / 'test')]
        if args.sanitize:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        subprocess.run(command, check=True)
        subprocess.run([str(tmp / 'test')], check=True, timeout=20)

if __name__ == '__main__':
    main()
