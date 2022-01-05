#include <qmos/qm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/timers.h>
#include "esp_system.h"
#include <sys/time.h>

#define ms2tick(ms) ((ms)/portTICK_PERIOD_MS)

void qm_reboot(void)
{
    esp_restart();
}

const char *qm_version_get(void)
{
    return "V1.0.0";
}

int qm_task_new(const char *name, void (*fn)(void *), void *arg,
                 int stack_size)
{
    TaskHandle_t xHandle = NULL;
	stack_size /= sizeof(StackType_t);
	xTaskCreate( fn, name, stack_size, arg, tskIDLE_PRIORITY+5, &xHandle );

    return xHandle ? 0 : -1;
}

int qm_task_new_ext(qm_task_t *task, const char *name, void (*fn)(void *), void *arg,
                     int stack_size, int prio)
{
    return qm_task_new(name, fn, arg, stack_size);
}

void qm_task_exit(int code)
{
    vTaskDelete(NULL);
}

int qm_mutex_new(qm_mutex_t *mutex)
{
    SemaphoreHandle_t mux = xSemaphoreCreateMutex();
    mutex->hdl = mux;
    return mux != NULL ? 0 : -1;
}

void qm_mutex_free(qm_mutex_t *mutex)
{
    vSemaphoreDelete(mutex->hdl);
}

int qm_mutex_lock(qm_mutex_t *mutex, unsigned int ms)
{
    if (!mutex) {
        return -1;
    }
    xSemaphoreTake(mutex->hdl, ms == QM_WAIT_FOREVER ? portMAX_DELAY : ms2tick(ms));

    return 0;
}

int qm_mutex_unlock(qm_mutex_t *mutex)
{
    if (!mutex) {
        return -1;
    }
    xSemaphoreGive(mutex->hdl);

    return 0;
}

int qm_mutex_is_valid(qm_mutex_t *mutex)
{
    return mutex->hdl != NULL;
}

int qm_sem_new(qm_sem_t *sem, int count)
{
    SemaphoreHandle_t s = xSemaphoreCreateCounting(1, count);
    sem->hdl = s;
    return 0;
}

void qm_sem_free(qm_sem_t *sem)
{
    if (sem == NULL) {
        return;
    }

    vSemaphoreDelete(sem->hdl);
}

int qm_sem_wait(qm_sem_t *sem, unsigned int ms)
{
    if (sem == NULL) {
        return -1;
    }

    int ret = xSemaphoreTake(sem->hdl, ms == QM_WAIT_FOREVER ? portMAX_DELAY : ms2tick(ms));
    return ret == pdPASS ? 0 : -1;
}

int qm_sem_signal(qm_sem_t *sem)
{
    if (sem == NULL) {
        return;
    }

    xSemaphoreGive(sem->hdl);
    return 0;
}

int qm_sem_is_valid(qm_sem_t *sem)
{
    return sem && sem->hdl != NULL;
}

int qm_event_new(qm_event_t *event, unsigned int flags)
{


}

int qm_event_free(qm_event_t *event)
{



}


int qm_event_get(qm_event_t *event, unsigned int flags, unsigned char opt, unsigned int *actl_flags, unsigned int timeout)
{

}

int qm_event_set(qm_event_t *event, unsigned int flags, unsigned char opt)
{



}

int qm_queue_new(qm_queue_t *queue, void *buf, unsigned int size, int max_msg)
{
    xQueueHandle q;
    q = xQueueCreate(size / max_msg, max_msg);
    queue->hdl = q;
    return 0;
}

void qm_queue_free(qm_queue_t *queue)
{
    vQueueDelete(queue->hdl);
}

int qm_queue_send(qm_queue_t *queue, void *msg, unsigned int size)
{
    return xQueueSend(queue->hdl, msg, portMAX_DELAY) == pdPASS ? 0 : -1;
}

int qm_queue_recv(qm_queue_t *queue, unsigned int ms, void *msg,
                   unsigned int *size)
{
    return xQueueReceive(queue->hdl, msg, ms == QM_WAIT_FOREVER ? portMAX_DELAY : ms2tick(ms))
           == pdPASS ? 0 : -1;
}

int qm_queue_is_valid(qm_queue_t *queue)
{
    return queue && queue->hdl != NULL;
}

#define QM_TIMER_MAX_NUM  10

typedef struct 
{
    qm_timer_t *timer;
    qm_time_callback_t cb;
    void *arg;
    int is_used;
}qm_timer_info_t;


static qm_timer_info_t qm_timer_info[QM_TIMER_MAX_NUM] = {0};


static int qm_timer_unused_get(void)
{
    int i = 0;
    for(i = 0; i < QM_TIMER_MAX_NUM; i++){
        if(!qm_timer_info[i].is_used){
            return i;
        }
    }
    return -1;
}

static void timer_callback(TimerHandle_t timer)
{
    int index = 0;
    if(timer == NULL){
        return;
    }
    index = ( int32_t ) pvTimerGetTimerID( timer );
    if(qm_timer_info[index].cb){
        qm_timer_info[index].cb(qm_timer_info[index].timer, qm_timer_info[index].arg);
    }
}

int qm_timer_new(qm_timer_t *timer, qm_time_callback_t cb,
                  void *arg, int ms, int repeat)
{
    int index = 0;

    index = qm_timer_unused_get();
    if(index < 0){
        return -1;
    }

    qm_timer_info[index].timer = xTimerCreate(NULL, ms/portTICK_PERIOD_MS, repeat, (void*)index, timer_callback);
    if(qm_timer_info[index].timer == NULL){
        return -1;
    }

    timer->hdl = qm_timer_info[index].timer;

    qm_timer_info[index].is_used = 1; 
    qm_timer_info[index].cb = cb;
    qm_timer_info[index].arg = arg;

    return 0;
}

int qm_timer_free(qm_timer_t *timer)
{
    int index = 0;
    if(timer == NULL){
        return -1;
    }

    index = ( int32_t ) pvTimerGetTimerID( timer->hdl );
    qm_timer_info[index].is_used = 0; 

    xTimerDelete(timer->hdl, 0);

    return 0;
}

int qm_timer_start(qm_timer_t *timer)
{
    if(timer == NULL){
        return -1;
    }

    xTimerStart(timer->hdl, 0);

    return 0;
}

int qm_timer_stop(qm_timer_t *timer)
{
    if(timer == NULL){
        return -1;
    }

    xTimerStop(timer->hdl, 0);
    return 0;
}

int qm_timer_change(qm_timer_t *timer, int ms)
{
    if(timer == NULL){
        return -1;
    };

    xTimerChangePeriod(timer->hdl, ms/portTICK_PERIOD_MS, 0);

    return  0;
}

int qm_workqueue_create(qm_workqueue_t *workqueue, int pri, int stack_size)
{



}

int qm_work_init(qm_work_t *work, void (*fn)(void *), void *arg, int dly)
{


}

int qm_work_destroy(qm_work_t *work)
{


}

int qm_work_run(qm_workqueue_t *workqueue, qm_work_t *work)
{


}

int qm_work_sched(qm_work_t *work)
{


}

int qm_work_cancel(qm_work_t *work)
{



}

void *qm_zalloc(unsigned int size)
{
    return calloc(size, 1);
}

void *qm_malloc(unsigned int size)
{
    return malloc(size);
}

void *qm_realloc(void *mem, unsigned int size)
{
    return realloc(mem, size);
}

void qm_free(void *mem)
{
    free(mem);
}

static struct timeval sys_start_time;


long long qm_now_ms(void)
{
    struct timeval tv;
    long long ms;
    gettimeofday(&tv, NULL);
    timersub(&tv, &sys_start_time, &tv);
    ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    return ms;
}

void qm_msleep(int ms)
{
    return vTaskDelay(ms/portTICK_PERIOD_MS);
}

void qm_init(void)
{
    gettimeofday(&sys_start_time, NULL);
}


