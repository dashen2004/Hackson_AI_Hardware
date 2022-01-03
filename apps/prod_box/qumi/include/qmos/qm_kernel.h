#ifndef QM_KERNEL_H
#define QM_KERNEL_H


#ifdef __cplusplus
extern "C" {
#endif

#define QM_WAIT_FOREVER    0xffffffffu
#define QM_NO_WAIT         0x0
#define QM_DEFAULT_APP_PRI 32

typedef struct {
    void *hdl;
} qm_hdl_t;

typedef qm_hdl_t qm_task_t;
typedef qm_hdl_t qm_mutex_t;
typedef qm_hdl_t qm_sem_t;
typedef qm_hdl_t qm_queue_t;
typedef qm_hdl_t qm_timer_t;


/**
 * Defines the prototype to which timer callback functions must conform.
 */
typedef void (*qm_timer_cb_t)( qm_timer_t *timer, void *arg);


/**
 * Reboot QMOS.
 */
void qm_reboot(void);

/**
 * Get kernel version.
 *
 * @return  SYSINFO_KERNEL_VERSION.
 */
const char *qm_version_get(void);
/**
 * Create a task.
 *
 * @param[in]  task        handle.
 * @param[in]  name        task name.
 * @param[in]  fn          task function.
 * @param[in]  arg         argument of the function..
 * @param[in]  stack_buf   stack-buf: if stack_buf==NULL, provided by kernel.
 * @param[in]  stack_size  stack-size in bytes.
 * @param[in]  prio        priority value, the max is RHINO_CONFIG_USER_PRI_MAX(default 60).
 *
 * @return  0: success.
 */
int qm_task_new(qm_task_t *task, const char *name, void (*fn)(void *), void *arg, int stack_size, int prio);
/**
 * Exit a task.
 *
 * @param[in]  task  handle.
 * 
 * @return  0: success.
 */
int qm_task_exit(qm_task_t *task);

/**
 * Alloc a mutex.
 *
 * @param[in]  mutex  pointer of mutex object, mutex object must be alloced,
 *                    hdl pointer in qm_mutex_t will refer a kernel obj internally.
 *
 * @return  0: success.
 */
int qm_mutex_new(qm_mutex_t *mutex);

/**
 * Free a mutex.
 *
 * @param[in]  mutex  mutex object, mem refered by hdl pointer in qm_mutex_t will
 *                    be freed internally.
 * @return  0: success.
 */
int qm_mutex_free(qm_mutex_t *mutex);

/**
 * Lock a mutex.
 *
 * @param[in]  mutex    mutex object, it contains kernel obj pointer which qm_mutex_new alloced.
 * @param[in]  timeout  waiting until timeout in milliseconds.
 *
 * @return  0: success.
 */
int qm_mutex_lock(qm_mutex_t *mutex, unsigned int timeout);

/**
 * Unlock a mutex.
 *
 * @param[in]  mutex  mutex object, it contains kernel obj pointer which oc_mutex_new alloced.
 *
 * @return  0: success.
 */
int qm_mutex_unlock(qm_mutex_t *mutex);

/**
 * This function will check if mutex is valid.
 *
 * @param[in]  mutex  pointer to the mutex.
 *
 * @return  0: valid, -1: invalid.
 */
int qm_mutex_is_valid(qm_mutex_t *mutex);

/**
 * Alloc a semaphore.
 *
 * @param[out]  sem    pointer of semaphore object, semaphore object must be alloced,
 *                     hdl pointer in qm_sem_t will refer a kernel obj internally.
 * @param[in]   count  initial semaphore counter.
 *
 * @return  0:success.
 */
int qm_sem_new(qm_sem_t *sem, int count);

/**
 * Destroy a semaphore.
 *
 * @param[in]  sem  pointer of semaphore object, mem refered by hdl pointer
 *                  in qm_sem_t will be freed internally.
 * @return  0: success.
 */
int qm_sem_free(qm_sem_t *sem);

/**
 * Acquire a semaphore.
 *
 * @param[in]  sem      semaphore object, it contains kernel obj pointer which qm_sem_new alloced.
 * @param[in]  timeout  waiting until timeout in milliseconds.
 *
 * @return  0: success.
 */
int qm_sem_wait(qm_sem_t *sem, unsigned int timeout);

/**
 * Release a semaphore.
 *
 * @param[in]  sem  semaphore object, it contains kernel obj pointer which qm_sem_new alloced.
 * 
 * @return  0: success.
 */
int qm_sem_signal(qm_sem_t *sem);

/**
 * This function will check if semaphore is valid.
 *
 * @param[in]  sem  pointer to the semaphore.
 * 
 * @return  0: valid, -1: invalid.
 */
int qm_sem_is_valid(qm_sem_t *sem);

/**
 * This function will create a queue.
 *
 * @param[in]  queue    pointer to the queue(the space is provided by user).
 * @param[in]  queue_len      queue length
 * @param[in]  queue_size     Size of each message in the message queue
 * 
 * @return  0: success.
 */

int qm_queue_new(qm_queue_t *queue, unsigned int queue_len, unsigned int queue_size);

/**
 * This function will delete a queue.
 *
 * @param[in]  queue  pointer to the queue.
 * @return  0: success.
 */
int qm_queue_free(qm_queue_t *queue);

/**
 * This function will send a msg to the front of a queue.
 *
 * @param[in]  queue  pointer to the queue.
 * @param[in]  msg    msg to send.
 * @param[in]  size   size of the msg.
 *
 * @return  0: success.
 */
int qm_queue_send(qm_queue_t *queue, void *msg, unsigned int size);

/**
 * This function will receive msg from a queue.
 *
 * @param[in]   queue  pointer to the queue.
 * @param[out]  msg    buf to save msg.
 * @param[out]  size   size of the msg.
 * @param[in]   ms     ms to wait before receive.
 *
 * @return  0: success.
 */
int qm_queue_recv(qm_queue_t *queue, void *msg, unsigned int *size, unsigned int ms);

/**
 * This function will check if queue is valid.
 *
 * @param[in]  queue  pointer to the queue.
 *
 * @return  0: valid, -1: invalid.
 */
int qm_queue_is_valid(qm_queue_t *queue);

/**
 * This function will create a timer and run auto.
 *
 * @param[in]  timer   pointer to the timer.
 * @param[in]  cb      callbak of the timer.
 * @param[in]  arg     the argument of the callback.
 * @param[in]  ms      ms of the normal timer triger.
 * @param[in]  repeat  repeat or not when the timer is created.
 *
 * @return  0: success.
 */
int qm_timer_new(qm_timer_t *timer, qm_timer_cb_t cb,
                  void *arg, int ms, int repeat);

/**
 * This function will create a timer.
 *
 * @param[in]  timer   pointer to the timer.
 * @param[in]  cb      callbak of the timer.
 * @param[in]  arg     the argument of the callback.
 * @param[in]  ms      ms of the normal timer triger.
 * @param[in]  repeat  repeat or not when the timer is created.
 * @param[in]  auto_run  run auto or not when the timer is created.
 *
 * @return  0: success.
 */
int qm_timer_new_ext(qm_timer_t *timer, qm_timer_cb_t cb,
                  void *arg, int ms, int repeat, unsigned char auto_run);

/**
 * This function will delete a timer.
 *
 * @param[in]  timer  pointer to a timer.
 *  
 * @return  0: success.
 */
int qm_timer_free(qm_timer_t *timer);

/**
 * This function will start a timer.
 *
 * @param[in]  timer  pointer to the timer.
 *
 * @return  0: success.
 */
int qm_timer_start(qm_timer_t *timer);

/**
 * This function will stop a timer.
 *
 * @param[in]  timer  pointer to the timer.
 *
 * @return  0: success.
 */
int qm_timer_stop(qm_timer_t *timer);

/**
 * This function will change attributes of a timer.
 *
 * @param[in]  timer  pointer to the timer.
 * @param[in]  ms     ms of the timer triger.
 *
 * @return  0: success.
 */
int qm_timer_change(qm_timer_t *timer, int ms);

/**
 * Realloc memory.
 *
 * @param[in]  mem   current memory address point.
 * @param[in]  size  new size of the mem to remalloc.
 *
 * @return  NULL: error.
 */
void *qm_realloc(void *mem, unsigned int size);

/**
 * Alloc memory.
 *
 * @param[in]  size  size of the mem to malloc.
 *
 * @return  NULL: error.
 */
void *qm_malloc(unsigned int size);

/**
 * Alloc memory and clear to zero.
 *
 * @param[in]  size  size of the mem to malloc.
 *
 * @return  NULL: error.
 */
void *qm_zalloc(unsigned int size);

/**
 * Free memory.
 *
 * @param[in]  ptr  address point of the mem.
 */
void qm_free(void *mem);

/**
 * Get current time in mini seconds.
 *
 * @return  elapsed time in mini seconds from system starting.
 */
long long qm_now_ms(void);

/**
 * Msleep.
 *
 * @param[in]  ms  sleep time in milliseconds.
 * 
 * @return  0: success.
 */
int qm_msleep(int ms);

/**
 * Initialize system
 */
void qm_init(void);


#ifdef __cplusplus
}
#endif

#endif /* QM_KERNEL_H */

