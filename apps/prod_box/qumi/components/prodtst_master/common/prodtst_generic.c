#include "prodtst_generic.h"
#include "qm_at.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "qm_kernel.h"

#define LOG_TAG "prodtst cmd"

#define PRODTST_FW_VER_KEY             "fw_ver"
#define PRODTST_FW_VER_MAX_LEN           16
#define PRODTST_FW_NAME_KEY             "fw_name"
#define PRODTST_FW_NAME_MAX_LEN          16
#define PRODTST_WIFI_STA_MAC_KEY         "wifi_sta_mac"
#define PRODTST_WIFI_AP_MAC_KEY         "wifi_ap_mac"
#define PRODTST_MAC_MAX_LEN              24
#define PRODTST_HW_CONFIG_KEY           "hw_config"
#define PRODTST_HW_TOKEN_KEY            "hw_token"
#define PRODTST_HW_CONFIG_MAX_LEN        4096


int prodtst_fw_ver(void *rsp, prodtst_data_t **data, int *num)
{
    char *str = NULL;
    int ret = 0;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = NULL;
    at_resp = (qm_at_response_t*)rsp;

    prodtst_data = (prodtst_data_t*)qm_malloc(sizeof(prodtst_data_t));
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }  

    prodtst_data->data = (char*)qm_malloc(PRODTST_FW_VER_MAX_LEN);
    if(prodtst_data->data == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, PRODTST_FW_VER_MAX_LEN);
    ret = qm_at_client_resp_parse_line_args(at_resp, 1, "%s", prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "fw ver: %s", prodtst_data->data);
    
    prodtst_data->name = (char*)qm_malloc(strlen(PRODTST_FW_VER_KEY)+1);
    if(prodtst_data->name == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, strlen(PRODTST_FW_VER_KEY)+1);
    memcpy(prodtst_data->name, PRODTST_FW_VER_KEY, strlen(PRODTST_FW_VER_KEY));
    
    *data = prodtst_data; 
    *num = 1;

    return QM_EOK;

__exit:
    if(prodtst_data->name){
        qm_free(prodtst_data->name);
    }

    if(prodtst_data->data){
        qm_free(prodtst_data->data);
    }
    
    qm_free(prodtst_data);
    
    return ret;
}

int prodtst_fw_name(void *rsp, prodtst_data_t **data, int *num)
{
    char *str = NULL;
    int ret = 0;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = NULL;
    at_resp = (qm_at_response_t*)rsp;

    prodtst_data = (prodtst_data_t*)qm_malloc(sizeof(prodtst_data_t));
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }  

    prodtst_data->data = (char*)qm_malloc(PRODTST_FW_NAME_MAX_LEN);
    if(prodtst_data->data == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, PRODTST_FW_NAME_MAX_LEN);
    ret = qm_at_client_resp_parse_line_args(at_resp, 1, "%s", prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "fw name: %s", prodtst_data->data);
    
    prodtst_data->name = (char*)qm_malloc(strlen(PRODTST_FW_NAME_KEY)+1);
    if(prodtst_data->name == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, strlen(PRODTST_FW_NAME_KEY)+1);
    memcpy(prodtst_data->name, PRODTST_FW_NAME_KEY, strlen(PRODTST_FW_NAME_KEY));
    
    *data = prodtst_data; 
    *num = 1;

    return QM_EOK;

__exit:
    if(prodtst_data->name){
        qm_free(prodtst_data->name);
    }

    if(prodtst_data->data){
        qm_free(prodtst_data->data);
    }

    qm_free(prodtst_data);

    return ret;
}

int wifi_sta_mac(void *rsp, prodtst_data_t **data, int *num)
{
    char *str = NULL;
    int ret = 0;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = NULL;
    at_resp = (qm_at_response_t*)rsp;

    prodtst_data = (prodtst_data_t*)qm_malloc(sizeof(prodtst_data_t));
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }  

    prodtst_data->data = (char*)qm_malloc(PRODTST_MAC_MAX_LEN);
    if(prodtst_data->data == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, PRODTST_MAC_MAX_LEN);
    ret = qm_at_client_resp_parse_line_args(at_resp, 1, "%s", prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "wifi sta mac: %s", prodtst_data->data);
    
    prodtst_data->name = (char*)qm_malloc(strlen(PRODTST_WIFI_STA_MAC_KEY)+1);
    if(prodtst_data->name == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, strlen(PRODTST_WIFI_STA_MAC_KEY)+1);
    memcpy(prodtst_data->name, PRODTST_WIFI_STA_MAC_KEY, strlen(PRODTST_WIFI_STA_MAC_KEY));
    
    *data = prodtst_data; 
    *num = 1;

    return QM_EOK;

__exit:
    if(prodtst_data->name){
        qm_free(prodtst_data->name);
    }

    if(prodtst_data->data){
        qm_free(prodtst_data->data);
    }

    qm_free(prodtst_data);

    return ret;
}

int wifi_ap_mac(void *rsp, prodtst_data_t **data, int *num)
{
    char *str = NULL;
    int ret = 0;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = NULL;
    at_resp = (qm_at_response_t*)rsp;

    prodtst_data = (prodtst_data_t*)qm_malloc(sizeof(prodtst_data_t));
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }  

    prodtst_data->data = (char*)qm_malloc(PRODTST_MAC_MAX_LEN);
    if(prodtst_data->data == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, PRODTST_MAC_MAX_LEN);
    ret = qm_at_client_resp_parse_line_args(at_resp, 1, "%s", prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "wifi ap mac: %s", prodtst_data->data);
    
    prodtst_data->name = (char*)qm_malloc(strlen(PRODTST_WIFI_AP_MAC_KEY)+1);
    if(prodtst_data->name == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    memset(prodtst_data->data, 0, strlen(PRODTST_WIFI_AP_MAC_KEY)+1);
    memcpy(prodtst_data->name, PRODTST_WIFI_AP_MAC_KEY, strlen(PRODTST_WIFI_AP_MAC_KEY));
    
    *data = prodtst_data; 
    *num = 1;

    return QM_EOK;

__exit:
    if(prodtst_data->name){
        qm_free(prodtst_data->name);
    }

    if(prodtst_data->data){
        qm_free(prodtst_data->data);
    }

    qm_free(prodtst_data);

    return ret;
}

int prodtst_hw_config(void *rsp, prodtst_data_t **data, int *num)
{

    return QM_EOK;
}