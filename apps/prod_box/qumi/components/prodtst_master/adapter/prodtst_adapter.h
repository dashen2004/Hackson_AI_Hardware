#ifndef __PRODTST_ADAPTER_H__
#define __PRODTST_ADAPTER_H__

#include "qm_types.h"


typedef struct{

    int (*init)(void *param);
    int (*deinit)(void);
    int (*channel_get)(void **handle);
    int (*establish)(void *handle, void *param);
    int (*destroy)(void *handle);
    int (*send)(void *handle, char *buf, int size);
    int (*recv)(void *handle, char *buf, int size, int timeout);
}prodtst_adapter_t;
 

extern prodtst_adapter_t wifi_prodtst_adapter;


int prodtst_wifi_ap_event_handler(uint32_t ip);


#endif

