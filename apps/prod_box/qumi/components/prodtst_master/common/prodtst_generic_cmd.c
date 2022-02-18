#include "prodtst_cmd.h"
#include "qm_at.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "qm_kernel.h"
#include "prodtst_config.h"
#include "prodtst_core.h"

#define LOG_TAG "prodtst cmd"


static int fw_ver_parse(void *rsp, prodtst_data_t **data, int *num);
static int fw_ver_creat(char *in_data, char **out);
static int fw_name_parse(void *rsp, prodtst_data_t **data, int *num);
static int wifi_sta_mac(void *rsp, prodtst_data_t **data, int *num);
static int wifi_ap_mac(void *rsp, prodtst_data_t **data, int *num);
static int prodtst_hw_config(void *rsp, prodtst_data_t **data, int *num);


prodtst_cmd_t prodtst_wifi_generic_cmd[] = {
    {PRODTST_FW_VER_KEY,        "AT+FW_VER?",                          CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, PRODTST_FW_VER_MAX_LEN,  prodtst_fw_ver},
    {"fw_name",                 "AT+FW_NAME?",                         CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, PRODTST_FW_NAME_MAX_LEN, prodtst_fw_name},
    {"wifi_sta_mac",            "AT+MAC_STA?",                         CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, PRODTST_MAC_MAX_LEN,     wifi_sta_mac},
    {"wifi_ap_mac",             "AT+MAC_AP?",                          CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, PRODTST_MAC_MAX_LEN,     wifi_ap_mac},
    {"ap_rssi",                 "AT+RF_W="CONFIG_QM_PRODTST_AP_SSID"", CONFIG_QM_PRODTST_CMD_ACK_RSSI_TIMEOUT,    0,                       wifi_ssid_rssi},
};


static int prodtst_fw_ver(void *rsp, prodtst_data_t **data, int *num)
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

static int prodtst_fw_name(void *rsp, prodtst_data_t **data, int *num)
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

static int wifi_sta_mac(void *rsp, prodtst_data_t **data, int *num)
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

static int wifi_ap_mac(void *rsp, prodtst_data_t **data, int *num)
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

static int wifi_ssid_rssi(void *rsp, prodtst_data_t **data, int *num)
{

    


    
}