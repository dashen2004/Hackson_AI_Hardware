#include "serial_platform.h"
#include "qm_types.h"
#include "qm_kernel.h"
#include "qm_errno.h"
#include "qm_uart.h"
#include "ntpdate.h"

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
    qm_err_t ret = QM_EOK;
    qm_mutex_t *qm_mutex = NULL;
    qm_mutex= qm_malloc(sizeof(qm_mutex_t));
    if(qm_mutex == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    ret = qm_mutex_new(qm_mutex);
    if(ret != QM_EOK){
        goto __exit;
    }
    
    return (void*)qm_mutex;
    
__exit:
    if(qm_mutex){
        qm_free(qm_mutex);
        qm_mutex = NULL;
    }
    return NULL;
}

/**
 * @brief Destroy the specified mutex object, it will free related resource.
 *
 * @param[in] mutex @n The specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */

int serial_mutex_deinit( void *mutex )
{
    qm_mutex_t *qm_mutex = (qm_mutex_t*)mutex;
    if(mutex == NULL){
        return -QM_EINVAL;
        
    }
    qm_mutex_free(qm_mutex);
    return QM_EOK;
}

/**
 * @brief Waits until the specified mutex is in the signaled state.
 *
 * @param[in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
int serial_mutex_lock(void *mutex )
{
    qm_mutex_t *qm_mutex = (qm_mutex_t*)mutex;
    if(mutex == NULL){
        return -QM_EINVAL;
    }
    return qm_mutex_lock(qm_mutex, QM_WAIT_FOREVER);
}

/**
 * @brief Releases ownership of the specified mutex object..
 *
 * @param[in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
int serial_mutex_unlock(void *mutex )
{
    qm_mutex_t *qm_mutex = (qm_mutex_t*)mutex;
    if(mutex == NULL){
        return -QM_EINVAL;
    }
    return qm_mutex_unlock(qm_mutex);
}


void *serial_sem_init(uint8_t count)
{
    qm_err_t ret = QM_EOK;
    qm_sem_t *qm_sem = NULL;
    qm_sem = qm_malloc(sizeof(qm_sem_t));
    if(qm_sem == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    ret = qm_sem_new(qm_sem, count);
    if(ret != QM_EOK){
        goto __exit;
    }
    
    return (void*)qm_sem;
    
__exit:
    if(qm_sem){
        qm_free(qm_sem);
        qm_sem = NULL;
    }
    return NULL;
}
/**
 * @brief Signals a semaphore
 * @param sem the semaphore to signal */
int serial_sem_signal(void *sem)
{
    qm_sem_t *qm_sem = (qm_sem_t*)sem;
    if(qm_sem == NULL){
        return -QM_EINVAL;
    }
    return qm_sem_signal(qm_sem);
}
/**
 * @brief Wait for a semaphore for the specified timeout
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return time (in milliseconds) waited for the semaphore
 *         or mbus_ARCH_TIMEOUT on timeout */
int serial_sem_wait(void *sem, uint32_t timeout)
{
    qm_sem_t *qm_sem = (qm_sem_t*)sem;
    if(qm_sem == NULL){
        return -QM_EINVAL;
    }
    if(timeout == 0){
        return qm_sem_wait(qm_sem, QM_WAIT_FOREVER);
    }else{
        return qm_sem_wait(qm_sem, timeout);
    }
}
/**
 * @brief Delete a semaphore
 * @param sem semaphore to delete */
int serial_sem_deinit(void *sem)
{
    qm_mutex_t *qm_sem = (qm_mutex_t*)qm_sem;
    if(qm_sem == NULL){
        return -QM_EINVAL;
        
    }
    return qm_sem_free(qm_sem);
}
#endif

void *serial_uart_init(void)
{
    qm_uart_dev_t uart = {
        .config = {
            .baud_rate = 9600,
            .data_bits = QM_UART_DATA_8_BITS,
            .parity    = QM_UART_PARITY_NONE,
            .pins.tx_pin = 3,
            .pins.rx_pin = 4,
            .stop_bits = QM_UART_STOP_BITS_1 ,
        },
        .port = 1,
    };
    
    qm_uart_dev_t *uart_dev = qm_malloc(sizeof(qm_uart_dev_t));
    if(uart_dev == NULL){
        return NULL;
    }
    memcpy(uart_dev, &uart, sizeof(qm_uart_dev_t));
    qm_uart_init(uart_dev);
    return (void*)uart_dev;
}

/**
 * @brief Increases the count of the specified semaphore object by 1.
 *
 * @param[in] A pointer to a buffer containing the data to be transmitted.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @return None.
 * @see None.
 * @note None.
 */
int serial_send(void *handle, uint8_t* buffer, uint32_t length)
{
    qm_uart_dev_t *uart_dev = (qm_uart_dev_t*)handle;
    if(uart_dev == NULL){
        return -QM_EINVAL;
    }
    return qm_uart_write(uart_dev, buffer, length, 0);
}

/**
 * @brief Increases the count of the specified semaphore object by 1.
 *
 * @param[in] buffer @n A pointer to a buffer to receive incoming data.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @param[in] timeout_ms @n timeout interval in millisecond.
 * @return 0 or length.
 * @see None.
 * @note None.
 */
int serial_recv(void *handle, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
{
    int ret = 0; 
    qm_uart_dev_t *uart_dev = (qm_uart_dev_t*)handle;
		
    if(uart_dev == NULL){
        return -QM_EINVAL;
    }
    ret = qm_uart_read(uart_dev, buffer, length, timeout_ms);
    if(ret > 0){
        return ret;
    }else{
        return 0;
    }
}


int serial_get_time(serial_time_info_t *time_info)
{
    qm_err_t ret = QM_EOK;
    qm_time_info_t info = {0};
    if(time_info == NULL){
        return -QM_EINVAL;
    }

    info.timezone = time_info->timezone;

    ret = qm_time_info_get(&info);
    if(ret != QM_EOK){
        return ret;
    }

    time_info->year = info.year;
    time_info->month = info.month;
    time_info->day = info.day;
    time_info->hour = info.hour;
    time_info->minute = info.minute;
    time_info->second = info.second;
    time_info->wday = info.wday;
    
    return 0;
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
    return (uint32_t)qm_now_ms();
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
   qm_msleep(ms);
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
    return qm_malloc(size);
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
    qm_free(ptr);
}

#ifndef SERIAL_QM_PRINTF_SUPPORT
int serial_printf(const char *fmt, ...)
{
    return 0;
}
#endif
