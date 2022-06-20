#ifndef __NTPDATE_H__
#define __NTPDATE_H__

#include "qm_types.h"

#ifndef CONFIG_QM_TIME_TASK_SIZE  
#define CONFIG_QM_TIME_TASK_SIZE                 4*1024    
#endif

#ifndef CONFIG_QM_TIME_TASK_PRIO  
#define CONFIG_QM_TIME_TASK_PRIO                 21    
#endif

#ifndef CONFIG_QM_TIME_SYNC_TIMEOUT  
#define CONFIG_QM_TIME_SYNC_TIMEOUT              6*3600   
#endif

typedef struct 
{
    int8_t timezone;
    uint8_t year;  //from 2000
    uint8_t month;
    uint8_t day;
    uint8_t wday; // 1->monday, 7->sunday
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
}qm_time_info_t;

//get utc time informatinon
int qm_time_info_get(qm_time_info_t* info);
//get utc timestamp
int qm_timestamp_get(uint32_t *timestamp);
//start notify time synchronization from ntp server
int qm_time_sync_start(void);
//stop notify time synchronization from ntp server
int qm_time_sync_stop(void);
//time service initialization
int qm_time_init(void);

#endif