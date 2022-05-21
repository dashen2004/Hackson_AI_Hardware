#include "ntpdate.h"
#include "qm_kernel.h"
#include "qm_errno.h"
#include "qm_utils_epochtime.h"
#include "time.h"
#include "qm_log.h"

#define LOG_TAG  "ntpdate"

static qm_task_t time_task = {0};
static qm_sem_t time_sem = {0};
static qm_timer_t timer = {0};
static qm_mutex_t time_lock = {0};
static uint32_t utc_timestamp = 0;
static uint32_t notify_time = 0;

//get utc time informatinon
int qm_time_info_get(qm_time_info_t* info)
{
    qm_err_t ret = QM_EOK;
    uint32_t timestamp = 0; 
    struct tm *tm;
    time_t time = 0;

    if(info == NULL){
        return -QM_EINVAL;
    }

    if(info->timezone > 12 || info->timezone < -12){
        return -QM_EINVAL;
    }

    ret = qm_timestamp_get(&timestamp);
    if(ret != QM_EOK){
        return ret;
    }
    timestamp += info->timezone*3600;
    time = (time_t)timestamp;
    tm = localtime(&time);

    info->year   = (uint8_t)tm->tm_year-100;
    info->month  = (uint8_t)tm->tm_mon+1;
    info->day    = (uint8_t)tm->tm_mday;
    info->hour   = (uint8_t)tm->tm_hour;
    info->minute = (uint8_t)tm->tm_min;
    info->second = (uint8_t)tm->tm_sec;
    if(tm->tm_wday == 0){
        info->wday = 7;
    }else{
        info->wday = (uint8_t)tm->tm_wday;
    }

    return QM_EOK;
}

//get utc timestamp
int qm_timestamp_get(uint32_t *timestamp)
{
    uint32_t m_timestamp = 0;
    if(timestamp == NULL){
        return -QM_EINVAL;
    }
    if(!time_lock.hdl){
        return -QM_ERROR;
    }
    qm_mutex_lock(&time_lock, QM_WAIT_FOREVER);
    m_timestamp = utc_timestamp;
    qm_mutex_unlock(&time_lock);

    if(m_timestamp == 0){
        return -QM_ERROR;
    }

    *timestamp = m_timestamp;

    return QM_EOK;
}

//notify time aynchronization form ntp server
int qm_time_aync_notify(void)
{
    if(time_task.hdl == NULL){
        return -QM_ERROR;
    }
    if(utc_timestamp == 0){
        return qm_sem_signal(&time_sem);
    }else{
        return QM_EOK;
    }
}

static void qm_time_task(void *arg)
{
    qm_err_t ret = QM_EOK;
    struct timeval_t time = {0};

    while(1){
        ret = qm_sem_wait(&time_sem, QM_WAIT_FOREVER);
        if(ret != QM_EOK){
            continue;
        }
        ret = qm_utils_get_epoch_time_from_ntp(&time);
        if(ret != QM_EOK){
            continue;
        }
        qm_mutex_lock(&time_lock, QM_WAIT_FOREVER);
        utc_timestamp = time.tv_sec;
        qm_mutex_unlock(&time_lock);
        QM_LOGD(LOG_TAG, "utc timestamp: %u", utc_timestamp);
        qm_timer_stop(&timer);
        qm_timer_start(&timer);
    }

}

static void qm_timer_callback( qm_timer_t *timer, void *arg)
{
    utc_timestamp++;
    notify_time++;
    if(notify_time >= CONFIG_QM_TIME_AYNC_TIMEOUT){
        notify_time = 0;
        qm_sem_signal(&time_sem);
    }
}
//time service initialization
int qm_time_init(void)
{
    qm_err_t ret = QM_EOK;
    ret = qm_sem_new(&time_sem, 1);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_timer_new(&timer, qm_timer_callback, NULL, 1000, 1);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_mutex_new(&time_lock);
    if(ret != QM_EOK){
        goto __exit;
    }
    
    ret = qm_task_new(&time_task, "time", qm_time_task, NULL, CONFIG_QM_TIME_TASK_SIZE, CONFIG_QM_TIME_TASK_PRIO);
    if(ret != QM_EOK){
        goto __exit;
    }

    return QM_EOK;

__exit:

    if(time_sem.hdl){
        qm_sem_free(&time_sem);
    }
    if(timer.hdl){
        qm_timer_free(&timer);
    }
    if(time_lock.hdl){
        qm_mutex_free(&time_lock);
    }
    
    return ret;
}