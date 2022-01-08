#include "qm_work.h"
#include "qm_workqueue.h"
#include "qm_errno.h"


int32_t qm_post_delayed_action(void **handle, void (*fn)(void *), void *arg, int ms)
{
    qm_err_t ret = QM_EOK;
    qm_work_t *work = NULL;

    if(fn == NULL){
        return -QM_EINVAL;
    }

    work = (qm_work_t*)qm_malloc(sizeof(qm_work_t));
    if(work == NULL){
        ret = -QM_EFULL;
        goto __exit;
    }

    ret = qm_work_init(work, fn, arg, ms);
    if(ret != QM_EOK){
        goto __exit;
    }

    work->dyn = 1;

    ret = qm_work_on_sched(work);
    if(ret != QM_EOK){
        goto __exit;
    }

    *handle = (void*)work; 

    return QM_EOK;

__exit:
    if(work){
        qm_free(work);
    }
    return ret;
}

int32_t qm_cancel_delayed_action(void *handle)
{
    qm_err_t ret = QM_EOK;
    qm_work_t *work = (qm_work_t*)handle;
    if(work == NULL){
        return -QM_EINVAL;
    }
    ret = qm_work_on_cancel(work);
    if(ret != QM_EOK){
        return ret;
    }
    if(work->dly > 0){
        qm_timer_free(&work->timer);
    }
    qm_free(work);
    return ret;
}