#ifndef QM_MEM_LOG_H
#define QM_MEM_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

void qm_mem_log_init(void);
void qm_mem_log_print(const char *stage);

#ifdef __cplusplus
}
#endif

#endif
