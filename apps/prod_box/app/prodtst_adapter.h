#ifndef __PRODTST_ADAPTER_H__
#define __PRODTST_ADAPTER_H__

#include "qm_types.h"

#define CONFIG_QM_PRODTST_TCP_PORT    8888

typedef struct{

    int (*init)(void *param);
    int (*deinit)(void);
    int (*establish)(void *param, void **handle);
    int (*destroy)(void *handle);
    int (*send)(void *handle, char *buf, int size);
    int (*recv)(void *handle, char *buf, int size, int timeout);
}prodtst_adapter_t;
 

int prodtst_fw_ver(void *rsp, char **data);
int prodtst_fw_name(void *rsp, char **data);
int wifi_sta_mac(void *rsp, char **data);
int wifi_ap_mac(void *rsp, char **data);
int prodtst_hw_config(void *rsp, char **data);



int prodtst_wifi_ap_event_handler(uint32_t ip);

#endif

