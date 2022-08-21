#include <qmos/qm_types.h>
#include <qmos/qm_kernel.h>
#include <qm_log_impl.h>
#include <qm_log.h>

#define CONFIG_QM_PRINTF   0

unsigned int qm_log_level = QM_LL_V_DEBUG | QM_LL_V_INFO | QM_LL_V_WARN | QM_LL_V_ERROR | QM_LL_V_FATAL;

static qm_mutex_t log_mutex = {0};

#if !CONFIG_QM_PRINTF
__attribute__((weak)) int qm_printf(const char * fmt, ...)
{
    va_list args;
    int ret;
    if(!log_mutex.hdl){
        qm_mutex_new(&log_mutex);
    }
    ret = qm_mutex_lock(&log_mutex, QM_WAIT_FOREVER);
    if (ret == 0)
    {
        va_start(args, fmt);
        ret = vprintf(fmt, args);
        va_end(args);
    }

    qm_mutex_unlock(&log_mutex);

    return ret;
}
#endif

#if defined(QM_LOG_SIMPLE)
void qm_log_print_simple(unsigned int log_level, const char *fmt, ...)
{
    va_list args;
    int ret;
    char level_prt;

    if ((qm_log_level & log_level) == 0)
    {
        return;
    }

    switch (log_level)
    {
    case QM_LL_V_ALL:
        level_prt = 'A';
        break;
    case QM_LL_V_FATAL:
        level_prt = 'F';
        break;
    case QM_LL_V_ERROR:
        level_prt = 'E';
        break;
    case QM_LL_V_WARN:
        level_prt = 'W';
        break;
    case QM_LL_V_INFO:
        level_prt = 'I';
        break;
    case QM_LL_V_DEBUG:
        level_prt = 'D';
        break;
    default:
        level_prt = ' ';
    }

    if(!log_mutex.hdl){
        qm_mutex_new(&log_mutex);
    }
    
    ret = qm_mutex_lock(&log_mutex, QM_WAIT_FOREVER);
    if (ret != 0)
    {
        return;
    }

    printf("[%06d]<%c> ", (unsigned)qm_now_ms(), level_prt);
    va_start(args, fmt);
    ret = vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
    printf("\r\n");

    qm_mutex_unlock(&log_mutex);
}
#endif


void qm_set_log_level(qm_log_level_t log_level)
{
    unsigned int value = 0;

    switch (log_level)
    {
    case QM_LL_NONE:
        value |= QM_LL_V_NONE;
        break;
    case QM_LL_DEBUG:
        value |= QM_LL_V_DEBUG;
    case QM_LL_INFO:
        value |= QM_LL_V_INFO;
    case QM_LL_WARN:
        value |= QM_LL_V_WARN;
    case QM_LL_ERROR:
        value |= QM_LL_V_ERROR;
    case QM_LL_FATAL:
        value |= QM_LL_V_FATAL;
        break;
    default:
        break;
    }

    qm_log_level = value;
}

