#include "ntpdate.h"
#include "qm_kernel.h"
#include "qm_errno.h"
#include "qm_utils_epochtime.h"
#include "time.h"
#include "qm_log.h"

#define LOG_TAG  "ntpdate"

#define TIMESTAMP_2000  943891200  /* 1970~2000 in seconds */

static qm_task_t time_task = {0};
static qm_timer_t timer = {0};
static qm_mutex_t time_lock = {0};
static uint32_t utc_timestamp = 0;
static uint32_t notify_time = 0;
static int need_sync = 0;
static int is_time_init = 0;
static int is_time_sync = 0;
static int is_time_sync_start = 0;

static void qm_time_task(void *arg);

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

    if(m_timestamp <= TIMESTAMP_2000){
        return -QM_ERROR;
    }

    *timestamp = m_timestamp;

    return QM_EOK;
}

//start notify time synchronization form ntp server
int qm_time_sync_start(void)
{
    qm_err_t ret = QM_EOK;

    if(!is_time_init){
        return -QM_ERROR;
    }

    if(is_time_sync && !need_sync){
        return QM_EOK;
    }

    qm_mutex_lock(&time_lock, QM_WAIT_FOREVER);
    is_time_sync_start = 1;
    qm_mutex_unlock(&time_lock);

    ret = qm_task_new(&time_task, "time", qm_time_task, NULL, CONFIG_QM_TIME_TASK_SIZE, CONFIG_QM_TIME_TASK_PRIO);
    if(ret != QM_EOK){
        return ret;
    }
    
    need_sync = 0;

    return QM_EOK;
}

static int time_sync_start(void)
{
    qm_err_t ret = QM_EOK;

    ret = qm_task_new(&time_task, "time", qm_time_task, NULL, CONFIG_QM_TIME_TASK_SIZE, CONFIG_QM_TIME_TASK_PRIO);
    if(ret != QM_EOK){
        return ret;
    }
    return QM_EOK;
}

//stop notify time synchronization form ntp server
int qm_time_sync_stop(void)
{
    qm_err_t ret = QM_EOK;

    if(!is_time_init){
        return -QM_ERROR;
    }

    qm_mutex_lock(&time_lock, QM_WAIT_FOREVER);
    is_time_sync_start = 0;
    qm_mutex_unlock(&time_lock);

    return QM_EOK;
}

static void qm_time_task(void *arg)
{
    qm_err_t ret = QM_EOK;
    struct timeval_t time = {0};

    while(1){
        ret = qm_utils_get_epoch_time_from_ntp(&time);
        if(ret != QM_EOK){
            continue;
        }
        is_time_sync = 1;
        qm_timer_stop(&timer);
        qm_mutex_lock(&time_lock, QM_WAIT_FOREVER);
        utc_timestamp = time.tv_sec;
        qm_mutex_unlock(&time_lock);
        QM_LOGD(LOG_TAG, "utc timestamp: %u", utc_timestamp);
        qm_timer_start(&timer);
        qm_task_exit(&time_task);
    }

}

static void qm_timer_callback( qm_timer_t *timer, void *arg)
{
    utc_timestamp++;
    if(!need_sync){
        notify_time++;
    }
    if(notify_time >= CONFIG_QM_TIME_SYNC_TIMEOUT){
        notify_time = 0;
        if(is_time_sync_start){
            need_sync = 0;
            time_sync_start();
        }else{
            need_sync = 1;
        }
    }
}
//time service initialization
int qm_time_init(void)
{
    qm_err_t ret = QM_EOK;

    if(is_time_init){
        return QM_EOK;
    }

    ret = qm_timer_new(&timer, qm_timer_callback, NULL, 1000, 1);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_mutex_new(&time_lock);
    if(ret != QM_EOK){
        goto __exit;
    }

    qm_timer_start(&timer);

    is_time_init = 1;

    return QM_EOK;

__exit:

    if(timer.hdl){
        qm_timer_free(&timer);
    }
    if(time_lock.hdl){
        qm_mutex_free(&time_lock);
    }
    
    return ret;
}