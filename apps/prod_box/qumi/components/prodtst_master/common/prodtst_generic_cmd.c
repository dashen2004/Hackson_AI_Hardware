#include "prodtst_cmd.h"
#include "qm_at.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "qm_kernel.h"
#include "prodtst_config.h"
#include "prodtst_core.h"

#define LOG_TAG "prodtst cmd"


static int fw_ver_parse(void *rsp, prodtst_data_t **data, int *num);
static int fw_name_parse(void *rsp, prodtst_data_t **data, int *num);
static int sta_mac_parse(void *rsp, prodtst_data_t **data, int *num);
static int ap_mac_parse(void *rsp, prodtst_data_t **data, int *num);
static int ap_rssi_parse(void *rsp, prodtst_data_t **data, int *num);


prodtst_cmd_t prodtst_wifi_generic_cmd[] = {
    {PRODTST_FW_VER_KEY,        "AT+FW_VER?",         CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, NULL,              fw_ver_parse      },
    {PRODTST_FW_NAME_KEY,       "AT+FW_NAME?",        CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, NULL,              fw_name_parse     },
    {"wifi_sta_mac",            "AT+MAC_STA?",        CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, NULL,              sta_mac_parse     },
    {"wifi_ap_mac",             "AT+MAC_AP?",         CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, NULL,              ap_mac_parse      },
    {"ap_rssi",                 "AT+RF_W",            CONFIG_QM_PRODTST_CMD_ACK_RSSI_TIMEOUT,    ap_rssi_cmd_creat, ap_rssi_parse     },
};


static int fw_ver_parse(void *rsp, prodtst_data_t **data, int *num)
{
    int ret = 0;
    int data_num = 1;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = (qm_at_response_t*)rsp;
    const char *rsp_expr = "%*[^\"]\"%[^\"]\"";

    prodtst_data = prodtst_data_creat(data_num);
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }  

    ret = prodtst_data_zalloc(prodtst_data, PRODTST_FW_VER_MAX_LEN);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_at_client_resp_parse_line_args(at_resp, 1, rsp_expr, prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "fw ver: %s", prodtst_data->data);
    
    prodtst_name_copy(prodtst_data, PRODTST_FW_VER_KEY, strlen(PRODTST_FW_VER_KEY));
    
    *data = prodtst_data; 
    *num = data_num;

    return QM_EOK;

__exit:
    ret = prodtst_data_destroy(prodtst_data, data_num);
    return ret;
}

static int fw_name_parse(void *rsp, prodtst_data_t **data, int *num)
{
    int ret = 0;
    int data_num = 1;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = (qm_at_response_t*)rsp;
    const char *rsp_expr = "%*[^\"]\"%[^\"]\"";

    prodtst_data = prodtst_data_creat(data_num);
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }   

    ret = prodtst_data_zalloc(prodtst_data, PRODTST_FW_NAME_MAX_LEN);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_at_client_resp_parse_line_args(at_resp, 1, rsp_expr, prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "fw name: %s", prodtst_data->data);

    prodtst_name_copy(prodtst_data, PRODTST_FW_NAME_KEY, strlen(PRODTST_FW_NAME_KEY));

    *data = prodtst_data; 
    *num = data_num;

    return QM_EOK;

__exit:
    ret = prodtst_data_destroy(prodtst_data, data_num);
    return ret;
}

static int wifi_sta_mac(void *rsp, prodtst_data_t **data, int *num)
{
    int ret = 0;
    int data_num = 1;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = (qm_at_response_t*)rsp;
    const char *rsp_expr = "%*[^\"]\"%[^\"]\"";

    prodtst_data = prodtst_data_creat(data_num);
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }   

    ret = prodtst_data_zalloc(prodtst_data, PRODTST_MAC_MAX_LEN);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_at_client_resp_parse_line_args(at_resp, 1, rsp_expr, prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "wifi sta mac: %s", prodtst_data->data);

    prodtst_name_copy(prodtst_data, PRODTST_WIFI_STA_MAC_KEY, strlen(PRODTST_WIFI_STA_MAC_KEY));
    
    *data = prodtst_data; 
    *num = data_num;

    return QM_EOK;

__exit:
    ret = prodtst_data_destroy(prodtst_data, data_num);
    return ret;
}

static int wifi_ap_mac(void *rsp, prodtst_data_t **data, int *num)
{
    int ret = 0;
    int data_num = 1;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = (qm_at_response_t*)rsp;
    const char *rsp_expr = "%*[^\"]\"%[^\"]\"";

    prodtst_data = prodtst_data_creat(data_num);
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }   

    ret = prodtst_data_zalloc(prodtst_data, PRODTST_MAC_MAX_LEN);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_at_client_resp_parse_line_args(at_resp, 1, rsp_expr, prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "wifi ap mac: %s", prodtst_data->data);

    prodtst_name_copy(prodtst_data, PRODTST_WIFI_STA_MAC_KEY, strlen(PRODTST_WIFI_STA_MAC_KEY));
    
    *data = prodtst_data; 
    *num = data_num;

    return QM_EOK;

__exit:
    ret = prodtst_data_destroy(prodtst_data, data_num);
    return ret;
}

static int ap_rssi_cmd_creat(char *cmd, char *param, char **out)
{
    char *cmd_str = (char*)qm_malloc(PRODTST_AP_RSSI_CMD_MAX_LEN+1);
    if(cmd_str == NULL){
        return -QM_ENOMEM;
    }

    memset(cmd_str, 0, PRODTST_AP_RSSI_CMD_MAX_LEN+1);

    snprintf(cmd_str, "%s=\"param\"", cmd, CONFIG_QM_PRODTST_AP_SSID);
    *out = cmd_str;

    return QM_EOK;
}

static int ap_rssi_parser(void *rsp, prodtst_data_t **data, int *num)
{
    int ret = 0;
    int data_num = 1;
    #define AP_RSSI_DATA_MAX_LEN 8
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = (qm_at_response_t*)rsp;
    const char *rsp_expr = "%*[^=]=%s";

    prodtst_data = prodtst_data_creat(data_num);
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }   

    ret = prodtst_data_zalloc(prodtst_data, PRODTST_AP_RSSI_MAX_LEN);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_at_client_resp_parse_line_args(at_resp, 1, rsp_expr, prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "ap rssi: %s", prodtst_data->data);

    prodtst_name_copy(prodtst_data, PRODTST_AP_RSSI_KEY, strlen(PRODTST_AP_RSSI_KEY));
    
    *data = prodtst_data; 
    *num = data_num;

    return QM_EOK;

__exit:
    ret = prodtst_data_destroy(prodtst_data, data_num);
    return ret;
}