#ifndef _BEEP_H_
#define _BEEP_H_



void beep_init(void);
void beep_on(void);
void beep_off(void);
void beep_work(int on_time, int off_time, int repeat);
void beep_work_times(int on_time, int off_time, int count);

#endif

