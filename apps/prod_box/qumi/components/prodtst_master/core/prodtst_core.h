#ifndef __PRODTST_CORE_H__
#define __PRODTST_CORE_H__

#include "qm_at.h"
#include "qm_kernel.h"
#include "prodtst_adapter.h"
#include "prodtst_config.h"



#define PRODTST_DATA_TYPE_MAX_LEN      (8)
#define PRODTST_PLATFORM_MAX_LEN       (8) 
#define PRODTST_DEV_TYPE_MAX_LEN       (8) 
#define PRODTST_MOUDLE_NAME_MAX_LEN    (8) 
#define PRODTST_MOUDLE_TYPE_MAX_LEN    (8) 


typedef enum{
    MODULE_TYPE_WIFI = 1,
    MODULE_TYPE_WIFI_BLE,
    MODULE_TYPE_BLE,
    MODULE_TYPE_BLE_MESH,
}module_type_t;

enum{
    PLATFORM_TYPE_ALI = 1,
    PLATFORM_TYPE_MIOT,
    PLATFORM_TYPE_HARMONY,
    PLATFORM_TYPE_TECENT,
    PLATFORM_TYPE_XJIOT,
};

typedef enum{
    PRODTST_STATE_NONE,
    PRODTST_STATE_SETUP,
    PRODTST_STATE_GENERIC,
    PRODTST_STATE_SPECIFIC,
    PRODTST_STATE_SPECIAL,
}prodtst_state_t;

typedef enum{
    PRODTST_EVENT_NONE,
    PRODTST_EVENT_CONFIG,
    PRODTST_EVENT_SET,
    PRODTST_EVENT_GET,
    PRODTST_EVENT_SPECIAL,
    PRODTST_EVENT_FINISH,
    PRODTST_EVENT_DEVICE_FOUND,
}prodtst_event_t;

typedef enum{
    PRODTST_DATA_TYPE_NONE,
    PRODTST_DATA_TYPE_SERIAL,
    PRODTST_DATA_TYPE_ALI,
    PRODTST_DATA_TYPE_NFC,
}prodtst_data_type_t;

typedef struct{
    char *name;
    char *data;
}prodtst_data_t;

typedef struct{
    char *platform;
    char *module_name;
    char *module_type;
}prodtst_info_t;

typedef struct{
    char ip[16];
    uint16_t port;
}prodtst_ip_info_t;

typedef struct{
    char platform[PRODTST_PLATFORM_MAX_LEN+1];
    char dev_type[PRODTST_DEV_TYPE_MAX_LEN+1];
    char module_name[PRODTST_MOUDLE_NAME_MAX_LEN+1];
    char module_type[PRODTST_MOUDLE_TYPE_MAX_LEN+1];
}prodtst_config_info_t;

typedef struct{
    int type;
    uint8_t uuid[16];
    uint8_t *data;
    int len;
}prodtst_data_info_t;

typedef struct{
    uint8_t uuid[16];
    int result;
}prodtst_finish_info_t;

typedef struct{
    void *param;
}prodtst_device_info_t;

typedef struct{
    int event;
    union{
        prodtst_ip_info_t ip_info;
        prodtst_config_info_t config_info;
        prodtst_data_info_t data_info;
        prodtst_finish_info_t finish_info;
        prodtst_device_info_t device_info;
    }dat;
}prodtst_notify_t;

typedef struct{
    char *name;
    char *cmd;
    int timeout;
    int recv_size;
    int (*parser)(void *rsp, prodtst_data_t **data, int num);
}prodtst_cmd_t;

typedef struct{
    char *platform;
    char *module_name;
    char *module_type;
    int recv_size;
    int obj_num;
    prodtst_cmd_t *generic_cmd;
    int generic_cmd_num;
    prodtst_cmd_t *specific_cmd;
    int specific_cmd_num;
    prodtst_adapter_t *adapter;
}prodtst_testsuite_t;

typedef struct {
    prodtst_data_t *data[CONFIG_QM_PRODTST_DATA_TABLE_NUM];    
    uint16_t num[CONFIG_QM_PRODTST_DATA_TABLE_NUM];        
}prodtst_data_table_t;

typedef struct{
    char uuid[16];
    prodtst_state_t state;
    qm_task_t task;
    qm_queue_t notify;
    qm_mutex_t lock;
    void *channel_handle;
    qm_at_client_t *at_client;
    prodtst_data_table_t data_tbl;
}prodtst_obj_t;


typedef struct{
    int start;
    qm_task_t task;
    qm_queue_t notify;
    prodtst_testsuite_t *testsuite;
    prodtst_config_info_t config_info;
    prodtst_obj_t *prodtst_obj[CONFIG_QM_PRODTST_OBJ_NUM];
}prodtst_context_t; 



int prodtst_init(void);

int prodtst_start(prodtst_info_t *info);
int prodtst_stop(void);


/********prodtst notify********/
int prodtst_found_notify(void *param);
int prodtst_ip_notify(char *ip);
int prodtst_data_notify(int event, char* type, char *uuid, uint8_t *data, int len);
int prodtst_config_notify(char *platform, char *module_name, char *module_type);
int prodtst_finish_notify(char *uuid, int result);
/*****************************/

int prodtst_device_num_get(void);

#endif

