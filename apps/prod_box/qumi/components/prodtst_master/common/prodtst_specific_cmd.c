#include "prodtst_cmd.h"
#include "qm_at.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "prodtst_core.h"
#include "prodtst_config.h"

#define LOG_TAG "specific cmd"

static int prodtst_hw_token(void *rsp, prodtst_data_t **data, int *num);
static int prodtst_hw_config(void *rsp, prodtst_data_t **data, int *num);

static prodtst_cmd_t prodtst_hw_wifi_specific_cmd[] = {
    {"fw_ver",  "AT+HW_CFG?",  CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, 32, PRODTST_HW_CFG_MAX_LEN},
    {"fw_name", "AT+FW_TOKEN?", CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, 32, PRODTST_HW_TOKEN_MAX_LEN},
};


static int prodtst_hw_token(void *rsp, prodtst_data_t **data, int *num)
{

    return QM_EOK;
}


static int prodtst_hw_config(void *rsp, prodtst_data_t **data, int *num)
{


    return QM_EOK;
}