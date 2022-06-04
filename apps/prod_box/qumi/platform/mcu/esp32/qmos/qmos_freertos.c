#include <qmos/qm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>
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

int qm_task_new(qm_task_t *task, const char *name, void (*fn)(void *), void *arg,
                     int stack_size, int prio)
{
    TaskHandle_t xHandle = NULL;
	stack_size /= sizeof(StackType_t);
	xTaskCreate( fn, name, stack_size, arg, prio, &xHandle );

    task->hdl = xHandle;

    return xHandle ? 0 : -1;
}

int qm_task_exit(qm_task_t *task)
{
    if(task == NULL){
        vTaskDelete(NULL);
    }else{
        vTaskDelete(task->hdl);
    }
    return 0;
}

int qm_mutex_new(qm_mutex_t *mutex)
{
    SemaphoreHandle_t mux = xSemaphoreCreateMutex();
    mutex->hdl = mux;
    return mux != NULL ? 0 : -1;
}

int qm_mutex_free(qm_mutex_t *mutex)
{
    vSemaphoreDelete(mutex->hdl);
    return 0;
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
    SemaphoreHandle_t s = xSemaphoreCreateCounting(100, count);
    sem->hdl = s;
    return 0;
}

int qm_sem_free(qm_sem_t *sem)
{
    if (sem == NULL) {
        return -1;
    }

    vSemaphoreDelete(sem->hdl);
    return 0;
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
        return -1;
    }

    xSemaphoreGive(sem->hdl);
    return 0;
}

int qm_sem_is_valid(qm_sem_t *sem)
{
    return sem && sem->hdl != NULL;
}

int qm_event_new(qm_event_t *event)
{
    EventGroupHandle_t event_group = NULL;
    event_group = xEventGroupCreate();
    if(event_group == NULL){
        return -1;
    }

    event->hdl = (void*)event_group;
    return 0;
}

int qm_event_free(qm_event_t *event)
{
    EventGroupHandle_t event_group = (EventGroupHandle_t*)event;
    vEventGroupDelete(event_group);
    return 0;
}

int qm_event_get(qm_event_t *event, unsigned int wait_event_bits, unsigned char opt, unsigned int *actl_event_bits, unsigned int timeout)
{
    EventBits_t event_bits = 0;

    EventGroupHandle_t event_group = (EventGroupHandle_t*)event;
    if(opt == QM_EVENT_OR){
        event_bits = xEventGroupWaitBits(event_group, (EventBits_t)wait_event_bits, 0, 0, timeout/portTICK_PERIOD_MS);

    }else if(opt == QM_EVENT_OR_CLEAR){
        event_bits = xEventGroupWaitBits(event_group, (EventBits_t)wait_event_bits, 1, 0, timeout/portTICK_PERIOD_MS);

    }else if(opt == QM_EVENT_AND){
        event_bits = xEventGroupWaitBits(event_group, (EventBits_t)wait_event_bits, 0, 1, timeout/portTICK_PERIOD_MS);
        
    }else if(opt == QM_EVENT_AND_CLEAR){
        event_bits = xEventGroupWaitBits(event_group, (EventBits_t)wait_event_bits, 1, 1, timeout/portTICK_PERIOD_MS);
    }else{
        return -1;
    }

    *actl_event_bits = event_bits;

    return 0;
}

int qm_event_set(qm_event_t *event, unsigned int event_bits)
{
    EventGroupHandle_t event_group = (EventGroupHandle_t*)event;
    xEventGroupSetBits( event_group, (EventBits_t)event_bits );
    return 0;
}

int qm_queue_new(qm_queue_t *queue, unsigned int queue_len, unsigned int queue_size)
{
    xQueueHandle q;
    q = xQueueCreate(queue_len, queue_size);
    if(q == NULL){
        return -1;
    }
    queue->hdl = q;
    return 0;
}

int qm_queue_free(qm_queue_t *queue)
{
    vQueueDelete(queue->hdl);
    return 0;
}

int qm_queue_send(qm_queue_t *queue, void *msg, unsigned int size)
{
    return xQueueSend(queue->hdl, msg, portMAX_DELAY) == pdPASS ? 0 : -1;
}

int qm_queue_recv(qm_queue_t *queue, void *msg, unsigned int *size, unsigned int ms)
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
    qm_timer_cb_t cb;
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

int qm_timer_new(qm_timer_t *timer, qm_timer_cb_t cb,
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
    if(timer == NULL || timer->hdl == NULL){
        return -1;
    }

    index = ( int32_t ) pvTimerGetTimerID( timer->hdl );
    qm_timer_info[index].is_used = 0; 

    xTimerDelete(timer->hdl, 0);

    return 0;
}

int qm_timer_start(qm_timer_t *timer)
{
    if(timer == NULL || timer->hdl == NULL){
        return -1;
    }

    xTimerStart(timer->hdl, 0);

    return 0;
}

int qm_timer_stop(qm_timer_t *timer)
{
    if(timer == NULL || timer->hdl == NULL){
        return -1;
    }

    xTimerStop(timer->hdl, 0);
    return 0;
}

int qm_timer_change(qm_timer_t *timer, int ms)
{
    if(timer == NULL || timer->hdl == NULL){
        return -1;
    };

    xTimerChangePeriod(timer->hdl, ms/portTICK_PERIOD_MS, 0);

    return  0;
}

#if !CONFIG_QM_MTRACE

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

#endif

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

int qm_msleep(int ms)
{
    vTaskDelay(ms/portTICK_PERIOD_MS);
    return 0;
}

void qm_init(void)
{
    gettimeofday(&sys_start_time, NULL);
}


