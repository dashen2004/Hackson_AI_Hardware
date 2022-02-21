#include "prodtst_cmd.h"
#include "qm_at.h"
#include "qm_errno.h"
#include "qm_log.h"

#define LOG_TAG "special cmd"


static int nfc_parse(void *rsp, prodtst_data_t **data, int *num);

prodtst_cmd_t prodtst_hw_wifi_specific_cmd[] = {
    {"nfc",  "AT+NFC",  CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, NULL, nfc_parse},
};


static int nfc_parse(void *rsp, prodtst_data_t **data, int *num)
{
    int ret = 0;
    int data_num = 1;
    char *cfg = NULL;
    prodtst_data_t *prodtst_data = NULL;
    qm_at_response_t *at_resp = (qm_at_response_t*)rsp;
    const char *rsp_expr = "%*[^\"]\"%[^\"]\"";
    char *kw = "+nfc:"

    prodtst_data = prodtst_data_creat(data_num);
    if(prodtst_data == NULL){
        return -QM_ENOMEM;
    }   

    cfg = qm_at_client_resp_get_line_by_kw(at_resp, kw);
    if(cfg == NULL){
        goto __exit;
    }

    ret = prodtst_data_zalloc(prodtst_data, strlen(cfg));
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_at_client_resp_parse_line_args(at_resp, 1, rsp_expr, prodtst_data->data);
    if(ret <= 0){
        ret = -QM_EINVAL;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "nfc data: %s", prodtst_data->data);

    prodtst_name_copy(prodtst_data, PRODTST_NFC_KEY, strlen(PRODTST_NFC_KEY));
    
    *data = prodtst_data; 
    *num = data_num;

    return QM_EOK;

__exit:
    ret = prodtst_data_destroy(prodtst_data, data_num);
    return ret;
}