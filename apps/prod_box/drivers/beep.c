#include "beep.h"
#include "gpio_ext.h"
#include "qm.h"

static qm_timer_t beep_timer;
#define BEEP_ON_TIMEOUT_MS  1000
#define BEEP_GPIO_PIN  GPIO_EXT_OC_Pin_14

#define BEEP_ON    1
#define BEEP_OFF   0

typedef struct{
    int status;
    int on_time;
    int off_time;
    int repeat;
    int count;
}beep_ctrl_t;

static beep_ctrl_t beep_ctrl = {0};

void beep_on(void)
{
    beep_ctrl.status = BEEP_ON;
    qm_timer_stop(&beep_timer);
    gpio_ext_set(BEEP_GPIO_PIN, BEEP_ON);
}

void beep_off(void)
{
    beep_ctrl.status = BEEP_OFF;
    qm_timer_stop(&beep_timer);
    gpio_ext_set(BEEP_GPIO_PIN, BEEP_OFF);
}

void beep_work(int on_time, int off_time, int repeat)
{
    if(on_time == 0 || off_time == 0){
        return;
    }

    qm_timer_stop(&beep_timer);

    beep_ctrl.on_time = on_time;
    beep_ctrl.off_time = off_time;
    beep_ctrl.repeat = repeat;
    beep_ctrl.count = 0;
    beep_ctrl.status = BEEP_ON;
    gpio_ext_set(BEEP_GPIO_PIN, BEEP_ON);

    qm_timer_change(&beep_timer, on_time);
    qm_timer_start(&beep_timer);
}

void beep_work_times(int on_time, int off_time, int count)
{
    if(on_time == 0 || off_time == 0 || count == 0){
        return;
    }

    qm_timer_stop(&beep_timer);

    beep_ctrl.on_time = on_time;
    beep_ctrl.off_time = off_time;
    beep_ctrl.repeat = 0;
    beep_ctrl.count = count;
    beep_ctrl.status = BEEP_ON;
    gpio_ext_set(BEEP_GPIO_PIN, BEEP_ON);

    qm_timer_change(&beep_timer, on_time);
    qm_timer_start(&beep_timer);
}

static void beep_timeout(qm_timer_t *timer, void *arg) 
{
    if(beep_ctrl.status == BEEP_ON){
        beep_ctrl.status = BEEP_OFF;
        gpio_ext_set(BEEP_GPIO_PIN, BEEP_OFF);
        qm_timer_change(&beep_timer, beep_ctrl.off_time);
        qm_timer_start(&beep_timer);
    }else{
        if(beep_ctrl.repeat){
            beep_ctrl.status = BEEP_ON;
            gpio_ext_set(BEEP_GPIO_PIN, BEEP_ON);
            qm_timer_change(&beep_timer, beep_ctrl.on_time);
            qm_timer_start(&beep_timer);   
        }else{
            if(beep_ctrl.count > 0){
                beep_ctrl.count--;
                qm_timer_change(&beep_timer, beep_ctrl.on_time);
                qm_timer_start(&beep_timer);  
            }
        }
    }
}

void beep_init(void)
{
    qm_err_t ret = QM_EOK;
    memset(&beep_ctrl, 0, sizeof(beep_ctrl));
    gpio_ext_set(BEEP_GPIO_PIN, 0);
    ret = qm_timer_new(&beep_timer, beep_timeout, NULL, BEEP_ON_TIMEOUT_MS, 0);
    if(ret != QM_EOK){
        return;
    }
}