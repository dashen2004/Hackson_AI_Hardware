#ifndef __PRODTST_CORE_H__
#define __PRODTST_CORE_H__

#include "qm_at.h"
#include "qm_kernel.h"
#include "prodtst_adapter.h"


#ifndef CONFIG_QM_PRODTST_TASK_PRIO  
#define CONFIG_QM_PRODTST_TASK_PRIO           19    
#endif

#ifndef CONFIG_QM_PRODTST_TASK_SIZE  
#define CONFIG_QM_PRODTST_TASK_SIZE           5*1024    
#endif

#ifndef CONFIG_QM_PRODTST_OBJ_NUM
#define CONFIG_QM_PRODTST_OBJ_NUM  6
#endif

#ifndef CONFIG_QM_PRODTST_CMD_NUM
#define CONFIG_QM_PRODTST_CMD_NUM  10
#endif

#ifndef CONFIG_QM_PRODTST_NOTIFY_QUEUE_NUM
#define CONFIG_QM_PRODTST_NOTIFY_QUEUE_NUM  10
#endif

#ifndef CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT
#define CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT   2000
#endif


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
    char *name;
    char *cmd;
    int timeout;
    int recv_size;
    int (*parser)(void *rsp, char **data);
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

typedef struct{
    char uuid[16];
    prodtst_state_t state;
    qm_task_t task;
    qm_queue_t notify;
    void *param;
    void *channel_handle;
    qm_at_client_t *at_client;
    prodtst_data_t data[CONFIG_QM_PRODTST_CMD_NUM];
}prodtst_obj_t;


typedef struct{
    int start;
    qm_task_t task;
    qm_queue_t notify;
    prodtst_testsuite_t *testsuite;
    prodtst_obj_t *prodtst_obj[CONFIG_QM_PRODTST_OBJ_NUM];
}prodtst_context_t; 


typedef struct{
    char *platform;
    char *module_name;
    char *module_type;
}prodtst_info_t;


typedef struct{
    unsigned char uuid[16];
    int event;
    int type;
    void *data;
    int len;
}prodtst_notify_t;



int prodtst_init(void);

int prodtst_start(prodtst_info_t *info);
int prodtst_stop(void);


int prodtst_device_notify(void *param);
int prodtst_cloud_notify(unsigned char *uuid, int event, int type, void *data, int len);

int prodtst_device_num_get(void);

#endif

