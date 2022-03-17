#include "prodtst_special.h"
#include "qm_errno.h"
#include "qm_kernel.h"
#include "prodtst_hal_uart.h"
#include "qm_utils_string.h"

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
    int hex_len = len/2; 
    uint8_t *hex = NULL;
    qm_err_t ret = QM_EOK;

    if(data == NULL || len == 0){
        return -QM_EINVAL;
    }

    hex = (uint8_t*)qm_malloc(hex_len);
    if(hex == NULL){
        return -QM_ENOMEM;
    }

    hex_str_to_nums(data, len, hex);

    qm_mutex_lock(&nfc_mutex, QM_WAIT_FOREVER);
    
    prodtst_uart_write(PRODTST_NFC_UART_CHANNEL, hex, len);


#if 0
    ret = prodtst_uart_read(PRODTST_NFC_UART_CHANNEL, );
    


    prodtst_uart_wtite();


    prodtst_uart_read();


#endif


    qm_mutex_unlock(&nfc_mutex);

    return QM_EOK;
}