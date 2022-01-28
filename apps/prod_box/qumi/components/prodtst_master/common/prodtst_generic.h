#ifndef __PRODTST_GENERIC_H__
#define __PRODTST_GENERIC_H__

#include "prodtst_core.h" 

int prodtst_fw_ver(void *rsp, prodtst_data_t **data, int *num);
int prodtst_fw_name(void *rsp, prodtst_data_t **data, int *num);
int wifi_sta_mac(void *rsp, prodtst_data_t **data, int *num);
int wifi_ap_mac(void *rsp, prodtst_data_t **data, int *num);
int prodtst_hw_config(void *rsp, prodtst_data_t **data, int *num);


#endif

