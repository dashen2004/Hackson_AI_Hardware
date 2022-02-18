#include "prodtst_cmd.h"
#include "qm_at.h"
#include "qm_errno.h"
#include "qm_log.h"

#define LOG_TAG "special cmd"


static int prodtst_nfc(void *rsp, prodtst_data_t **data, int *num);

prodtst_cmd_t prodtst_hw_wifi_specific_cmd[] = {
    {"nfc",  "AT+NFC=",  CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, PRODTST_NFC_MAX_LEN, prodtst_nfc},
};


static int prodtst_nfc(void *rsp, prodtst_data_t **data, int *num)
{

    
    return QM_EOK;
}