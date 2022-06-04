#include "qm_iot_import.h"
#include <qmos/qm.h>
#include "qm_kvmgr.h"

void *hal_mutex_create(void)
{
    qm_mutex_t *mutex = hal_malloc(sizeof(qm_mutex_t));
    if(!mutex){
        return NULL;
    }
    
    if(qm_mutex_new(mutex) == 0){
        return (void*)mutex;
    }else{
        return NULL;
    }
}

void hal_mutex_destory(_IN_ void *mutex)
{
    if(!mutex){
        return;
    }
    qm_mutex_free((qm_mutex_t*)mutex);
    hal_free(mutex);
}

void hal_mutex_lock(_IN_ void *mutex)
{
    if(!mutex){
        return;
    }
    qm_mutex_lock((qm_mutex_t*)mutex, QM_WAIT_FOREVER);   
} 

void hal_mutex_unlock(_IN_ void *mutex)
{
    if(!mutex){
        return;
    }
    qm_mutex_unlock((qm_mutex_t*)mutex);    
}


void hal_free(void *ptr)
{
    qm_free(ptr);
}


void *hal_malloc(int size)
{
    return qm_malloc(size);
}

uint32_t hal_uptime_ms(void)
{
    return (uint32_t)qm_now_ms();
}

void hal_sleep_ms(_IN_ uint32_t ms)
{
    qm_msleep(ms);
}

static uint32_t next;  
  
/* RAND_MAX assumed to be 32767 */  
static uint32_t hal_rand(void) {  
    next = next * 1103515245 + 12345;  
    return((uint32_t)(next/65536) % 32768);  
}  
  
void hal_srand(uint32_t seed)
{  
    next = seed;  
}  

void hal_srandom(uint32_t seed)
{
    hal_srand(seed);
}

uint32_t hal_random_get(uint32_t region)
{
    return (region > 0) ? (hal_rand() % region) : 0;
}

int hal_snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int hal_vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}

void hal_printf(_IN_ const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
}

int hal_kv_set(const char *key, const void *val, int len, int sync)
{
    int   ret = 0;
    ret = qm_kv_set(key, val, len, sync);
    return ret;
}

int hal_kv_get(const char *key, void *buffer, int *buffer_len)
{
    int   ret = 0;
    ret = qm_kv_get(key, buffer, buffer_len);
    return ret;
}

int hal_kv_del(const char *key)
{
    int   ret = 0;
    ret = qm_kv_del(key);
    return ret;
}


