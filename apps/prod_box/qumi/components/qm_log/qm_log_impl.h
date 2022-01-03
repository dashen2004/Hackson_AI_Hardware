#ifndef QM_LOG_IMPL_H
#define QM_LOG_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif


extern unsigned int qm_log_level;
static inline unsigned int qm_log_get_level(void)
{
    return qm_log_level;
}

enum log_level_bit {
    QM_LL_V_NONE_BIT = -1,
    QM_LL_V_FATAL_BIT,
    QM_LL_V_ERROR_BIT,
    QM_LL_V_WARN_BIT,
    QM_LL_V_INFO_BIT,
    QM_LL_V_DEBUG_BIT,
    QM_LL_V_MAX_BIT
};

#define QM_LOG_LEVEL qm_log_get_level()

#define QM_LL_V_NONE  0
#define QM_LL_V_ALL   0XFF
#define QM_LL_V_FATAL (1 << QM_LL_V_FATAL_BIT)
#define QM_LL_V_ERROR (1 << QM_LL_V_ERROR_BIT)
#define QM_LL_V_WARN  (1 << QM_LL_V_WARN_BIT)
#define QM_LL_V_INFO  (1 << QM_LL_V_INFO_BIT)
#define QM_LL_V_DEBUG (1 << QM_LL_V_DEBUG_BIT)

/*
 * color def.
 * see http://stackoverflow.com/questions/3585846/color-text-in-terminal-applications-in-unix
 */
#define COL_DEF "\x1B[0m"  /* white */
#define COL_RED "\x1B[31m" /* red */
#define COL_GRE "\x1B[32m" /* green */
#define COL_BLU "\x1B[34m" /* blue */
#define COL_YEL "\x1B[33m" /* yellow */
#define COL_WHE "\x1B[37m" /* white */
#define COL_CYN "\x1B[36m"
#define COL_MAG "\x1B[35m"

extern int qm_printf(const char *fmt, ...);
#ifdef CONFIG_LOGMACRO_DETAILS
#include <qmos/qm_kernel.h>
#define qm_log_print(CON, MOD, COLOR, LVL, FMT, ...) \
    do {                                          \
        if (QM_LOG_LEVEL & CON) {                \
            long long ms = qm_now_ms();          \
            qm_printf(COLOR " [%4d.%03d]<%s> %s [%s#%d] : ", (int)(ms/1000), (int)(ms%1000), LVL, MOD, __FUNCTION__, __LINE__); \
            qm_printf(FMT COL_DEF "\r\n", ##__VA_ARGS__); \
        } \
    } while (0)
#else
#if defined(QM_LOG_SIMPLE)
extern void qm_log_print_simple(unsigned int log_level, const char *fmt, ...);
#define qm_log_print(CON, MOD, COLOR, LVL, FMT, ...) \
    do {                                          \
        qm_log_print_simple(CON, FMT, ##__VA_ARGS__);\
    } while (0)
#else
#define qm_log_print(CON, MOD, COLOR, LVL, FMT, ...) \
    do { \
        if (QM_LOG_LEVEL & CON) { \
            qm_printf("[%06d]<" LVL "> ", (unsigned)qm_now_ms()); \
            qm_printf(FMT, ##__VA_ARGS__); \
            qm_printf("\r\n"); \
        } \
    } while (0)
#endif
#endif

#define void_func(fmt, ...)

#ifndef qm_printf
int qm_printf(const char *fmt, ...);
#else
extern int qm_printf(const char *fmt, ...);
#endif        


#ifdef CONFIG_LOGMACRO_SILENT
#define QM_LOGF_IMPL(mod, fmt, ...) void_func(fmt, ##__VA_ARGS__)
#define QM_LOGE_IMPL(mod, fmt, ...) void_func(fmt, ##__VA_ARGS__)
#define QM_LOGW_IMPL(mod, fmt, ...) void_func(fmt, ##__VA_ARGS__)
#define QM_LOGI_IMPL(mod, fmt, ...) void_func(fmt, ##__VA_ARGS__)
#define QM_LOGD_IMPL(mod, fmt, ...) void_func(fmt, ##__VA_ARGS__)
#else

#define QM_LOGD_IMPL(mod, fmt, ...) \
        qm_log_print(QM_LL_V_DEBUG, mod, COL_WHE, "D", fmt, ##__VA_ARGS__)
#define QM_LOGF_IMPL(mod, fmt, ...) \
        qm_log_print(QM_LL_V_FATAL, mod, COL_RED, "F", fmt, ##__VA_ARGS__)
#define QM_LOGE_IMPL(mod, fmt, ...) \
        qm_log_print(QM_LL_V_ERROR, mod, COL_YEL, "E", fmt, ##__VA_ARGS__)
#define QM_LOGW_IMPL(mod, fmt, ...) \
        qm_log_print(QM_LL_V_WARN, mod, COL_BLU, "W", fmt, ##__VA_ARGS__)
#define QM_LOGI_IMPL(mod, fmt, ...) \
        qm_log_print(QM_LL_V_INFO, mod, COL_GRE, "I", fmt, ##__VA_ARGS__)

#endif /* CONFIG_LOGMACRO_SILENT */

#ifdef __cplusplus
}
#endif

#endif /* QM_LOG_IMPL_H */

