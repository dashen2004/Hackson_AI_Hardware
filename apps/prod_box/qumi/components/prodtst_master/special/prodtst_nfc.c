#include "prodtst_special.h"
#include "qm_errno.h"
#include "qm_kernel.h"
#include "prodtst_hal_uart.h"

static qm_mutex_t nfc_mutex = {0};

#define PRODTST_NFC_UART_CHANNEL   PRODTST_UART_NUM_7

int prodtst_nfc_init(void *param)
{
    qm_err_t ret = QM_EOK;
    static int is_init = 0;
    if(!is_init){
        is_init = 1;
        qm_mutex_new(&nfc_mutex);
        ret = prodtst_uart_init(PRODTST_NFC_UART_CHANNEL, 9600);
    }
    return ret;
    
}


int prodtst_nfc_deinit(void *param)
{
    return QM_EOK;
}

int prodtst_nfc_process(char *data, int len)
{

    qm_mutex_lock(&nfc_mutex, QM_WAIT_FOREVER);

    


    qm_mutex_unlock(&nfc_mutex);

    return QM_EOK;
}