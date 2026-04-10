#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define ALARM_LIST_MAX_COUNT    (10)

typedef struct {
    uint8_t hour;
    uint8_t min;

    uint8_t repeat_mask;
    uint8_t enable;    
} alarm_t;

qm_err_t alarm_manager_init(void);
qm_err_t alarm_manager_update(void);
qm_err_t alarm_manager_get_count(int32_t *count);
qm_err_t alarm_manager_get_alarm_list(alarm_t *alarm_list);
qm_err_t alarm_manager_get_alarm(alarm_t *alarm, uint32_t alarm_index);

qm_err_t alarm_manager_add_alarm(alarm_t alarm);
qm_err_t alarm_manager_del_alarm(int32_t alarm_index);
qm_err_t alarm_manager_mod_alarm(alarm_t alarm, int32_t alarm_index);

#ifdef __cplusplus
}
#endif

#endif