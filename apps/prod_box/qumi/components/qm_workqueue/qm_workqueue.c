#include "qm_workqueue.h"
#include "qm_errno.h"

typedef struct{
    void *arg;
    work_fn_t fn;
    void *work;
}work_info_t;


static qm_workqueue_t g_qm_workqueue = {0};

static void worker_task(void *arg)
{
    qm_err_t ret = 0;
    uint8_t exist = 0;
    qm_work_t *work = NULL;
    qm_workqueue_t *wq = (qm_workqueue_t*)arg;

    while(1){

        ret = qm_queue_recv(&wq->queue, &work, sizeof(void*), QM_WAIT_FOREVER);
        if(ret != QM_EOK){
            continue;
        }
        qm_mutex_lock(&wq->lock, QM_WAIT_FOREVER);
        wq->work_cur = work;
        work->exist = 0;
        qm_mutex_unlock(&wq->lock);
        /* do work */
        work->fn(work->arg);
        /* clean current work */
        wq->work_cur = NULL;
    }

}

int32_t qm_workqueue_create(qm_workqueue_t *workqueue, int pri, int stack_size)
{
    qm_err_t ret = 0;
    if(workqueue == NULL || stack_size == 0){
        return -QM_EINVAL;
    }

    ret = qm_mutex_new(&workqueue->lock);
    if(ret != QM_EOK){
        return ret;
    }

    ret = qm_queue_new(&workqueue->queue, CONFIG_QM_WORKER_NUM, sizeof(void*));
    if(ret != QM_EOK){
        return ret;
    }

    ret = qm_task_new(&workqueue->worker, "worker", worker_task, (void*)workqueue, stack_size, pri);
    if(ret != QM_EOK){
        return ret;
    }

    workqueue->work_cur = NULL;
    
    return QM_EOK;
}

static void work_timer_cb(qm_timer_t *timer, void *arg)
{
    uint8_t exist = 0;
    qm_work_t *work_cur = NULL;
    qm_work_t *work = (qm_work_t*)arg;
    qm_workqueue_t *wq = (qm_workqueue_t*)work->wq;

    qm_mutex_lock(&wq->lock, QM_WAIT_FOREVER);
    exist = work->exist;
    work_cur = wq->work_cur;;
    qm_mutex_unlock(&wq->lock);
    
    if(work_cur == work){
        return;
    }
    if(exist == 1){
        return;
    }

    work->exist = 1;

    qm_queue_send(&wq->queue, &work, sizeof(void*));
}

int32_t qm_work_init(qm_work_t *work, void (*fn)(void *), void *arg, int dly)
{
    if(work == NULL || fn == NULL){
        return -QM_EINVAL;
    }
    work->fn  = fn;
    work->arg = arg;
    work->dly = dly;
    work->exist = 0;
    work->wq = NULL;

    return QM_EOK;
}

int32_t qm_work_sched(qm_workqueue_t *workqueue, qm_work_t *work)
{
    qm_err_t ret = 0;
    uint8_t exist = 0;
    qm_work_t *work_cur = NULL;

    if(workqueue == NULL || work == NULL){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&workqueue->lock, QM_WAIT_FOREVER);
    work_cur = workqueue->work_cur;
    exist = work->exist;
    qm_mutex_unlock(&workqueue->lock);
    
    if(work->dly == 0){
        if(work_cur == work){
            return -QM_EBUSY;
        }
        if(work->exist == 1){
            return -QM_EBUSY;
        }

        work->wq = (void*)workqueue;
        work->exist = 1;

        ret = qm_queue_send(&workqueue->queue, &work, sizeof(void*));
        if(ret != QM_EOK){
            return ret;
        }

    }else{
        work->wq = (void*)workqueue;
        ret = qm_timer_new(&work->timer, work_timer_cb, (void*)work, work->dly, 0);
        if(ret != QM_EOK){
            return ret;
        }
    }

    return QM_EOK;
}

int32_t qm_work_cancel(qm_work_t *work)
{
    uint8_t exist = 0;
    qm_work_t *work_cur = NULL;
    qm_workqueue_t *wq = (qm_workqueue_t*)work->wq;

    if(work == NULL){
        return -QM_EINVAL;
    }

    if(wq == NULL){
        if (work->dly > 0) {
            qm_timer_stop(&work->timer);
        }
        return QM_EOK;
    }

    qm_mutex_lock(&wq->lock, QM_WAIT_FOREVER);
    work_cur = wq->work_cur;
    exist = work->exist;
    qm_mutex_unlock(&wq->lock);

    if (work->exist == 1) {
        return -QM_EBUSY;
    }
    
    work->wq = NULL;
    return QM_EOK;
}


int32_t qm_work_on_sched(qm_work_t *work)
{
    return qm_work_sched(&g_qm_workqueue, work);
}


int32_t qm_work_on_cancel(qm_work_t *work)
{
    return qm_work_cancel(work);
}


int32_t workqueue_on_init(void)
{
    return qm_workqueue_create(&g_qm_workqueue, CONFIG_QM_DEFAULT_WORKQUEUE_TASK_PRIO, CONFIG_QM_DEFAULT_WORKQUEUE_TASK_SIZE);
}