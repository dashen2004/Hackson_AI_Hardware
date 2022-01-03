#include "serial_platform.h"
#include "qm_types.h"
#include "hi_time.h"
#include "hi_task.h"
#include "hi_mux.h"
#include "hi_sem.h"
#include "hi_uart.h"
#include "hi_mem.h"
#include "hi_config.h"

typedef struct{
    hi_u32 mux_id;
    
}hi_mutex_t;
/**
 * @brief Create a mutex.
 *
 * @return Mutex handle.
 * @see None.
 * @note None.
 */
#ifdef SERIAL_RTOS_SUPPORT
void *serial_mutex_init(void)
{
    int ret = 0;
    hi_mutex_t* hi_mutex = hi_malloc(HI_MOD_ID_APP_COMMON, sizeof(hi_mutex_t));
    if(hi_mutex == NULL){
        return NULL;
    }
    
    ret = hi_mux_create(&hi_mutex->mux_id);
    if (ret != 0) {
        printf("sem creat err\r\n");
        return NULL;
    }else{
        return hi_mutex;
    }  
}

/**
 * @brief Destroy the specified mutex object, it will free related resource.
 *
 * @param[in] mutex @n The specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */

void serial_mutex_deinit( void *mutex )
{
    hi_mutex_t* hi_mutex;
    if(mutex){

        hi_mutex = (hi_mutex_t*)mutex;
        hi_mux_delete(hi_mutex->mux_id);
        hi_free(HI_MOD_ID_APP_COMMON, mutex);
    }
}

/**
 * @brief Waits until the specified mutex is in the signaled state.
 *
 * @param[in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void serial_mutex_lock(void *mutex )
{
    hi_mutex_t* hi_mutex;
    if(mutex == NULL){

        return;
    }

    hi_mutex = (hi_mutex_t*)mutex;
    
    hi_mux_pend(hi_mutex->mux_id, HI_SYS_WAIT_FOREVER);
}

/**
 * @brief Releases ownership of the specified mutex object..
 *
 * @param[in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void serial_mutex_unlock(void *mutex )
{
    hi_mutex_t* hi_mutex;
    if(mutex == NULL){

        return;
    }

    hi_mutex = (hi_mutex_t*)mutex;
    
    hi_mux_post(hi_mutex->mux_id);
}


typedef struct{
    hi_u32 sem_id;
    
}hi_sem_t;

void *serial_sem_init(uint8_t count)
{
    int ret = 0;
    hi_sem_t* hi_sem = hi_malloc(HI_MOD_ID_APP_COMMON, sizeof(hi_sem_t));
    if(hi_sem == NULL){
        return NULL;
    }
    
    ret = hi_sem_create(&hi_sem->sem_id, count);
    if (ret != 0) {
        printf("sem creat err\r\n");
        return NULL;
    }else{
        return hi_sem;
    }  
}
/**
 * @brief Signals a semaphore
 * @param sem the semaphore to signal */
void serial_sem_signal(void *sem)
{
    hi_sem_t* hi_sem;
    if(sem == NULL){

        return;
    }

    hi_sem = (hi_sem_t*)sem;
    
    hi_sem_signal(hi_sem->sem_id);
}
/**
 * @brief Wait for a semaphore for the specified timeout
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return time (in milliseconds) waited for the semaphore
 *         or mbus_ARCH_TIMEOUT on timeout */
uint8_t serial_sem_wait(void *sem, uint32_t timeout)
{
    int ret = 0;
    hi_sem_t* hi_sem;
    if(sem == NULL){

        return 0;
    }
    
    hi_sem = (hi_sem_t*)sem;

    if(timeout == 0){
        hi_sem_wait(hi_sem->sem_id, HI_SYS_WAIT_FOREVER);
    }else{
        hi_sem_wait(hi_sem->sem_id, timeout);
    }
    
    return 0;
}
/**
 * @brief Delete a semaphore
 * @param sem semaphore to delete */
void serial_sem_deinit(void *sem)
{
    hi_sem_t* hi_sem;
    if(sem){

        hi_sem = (hi_sem_t*)sem;
        hi_sem_delete(hi_sem->sem_id);
        hi_free(HI_MOD_ID_APP_COMMON, sem);
    }
}
#endif

/**
 * @brief Increases the count of the specified semaphore object by 1.
 *
 * @param[in] A pointer to a buffer containing the data to be transmitted.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @return None.
 * @see None.
 * @note None.
 */
void serial_send(uint8_t* buffer, uint32_t length)
{
    hi_uart_write(HI_UART_IDX_1, buffer, length);
}

/**
 * @brief Increases the count of the specified semaphore object by 1.
 *
 * @param[in] buffer @n A pointer to a buffer to receive incoming data.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @param[in] timeout_ms @n timeout interval in millisecond.
 * @return None.
 * @see None.
 * @note None.
 */
uint32_t serial_recv(uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
{
    int len = 0 ;
    len = hi_uart_read_timeout(HI_UART_IDX_1, buffer, length, timeout_ms);
    if(len < 0){
        return 0;
    }else{
        return len;
    }
}



/**
 * @brief Retrieves the number of milliseconds that have elapsed since the system was boot.
 *
 * @param None.
 * @return the number of milliseconds.
 * @see None.
 * @note None.
 */

uint32_t serial_get_timems( void )
{
    return hi_get_milli_seconds();
}


/**
 * @brief sleep thread itself.
 *
 * @param[in] ms @n the time interval for which execution is to be suspended, in milliseconds.
 * @return None.
 * @see None.
 * @note None.
 */

#ifdef SERIAL_RTOS_SUPPORT
void serial_msleep(uint32_t ms)
{
    if(ms){
        hi_sleep(ms);
    }
}
#endif

/**
 * @brief Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 *
 * @param[in] size @n specify block size in bytes.
 * @return A pointer to the beginning of the block.
 * @see None.
 * @note Block value is indeterminate.
 */
void *serial_malloc(uint32_t size)
{
    return hi_malloc(HI_MOD_ID_APP_COMMON, size);
}

/**
 * @brief Deallocate memory block
 *
 * @param[in] ptr @n Pointer to a memory block previously allocated with platform_malloc.
 * @return None.
 * @see None.
 * @note None.
 */
void serial_free(void *ptr)
{
    hi_free(HI_MOD_ID_APP_COMMON, ptr);
}


int serial_printf(const char *fmt, ...)
{
    va_list ap;
    int ret = 0;
    va_start(ap, fmt);
    ret = vprintf(fmt, ap);
    va_end(ap);
    return ret;

}
