#include "prodtst_special.h"
#include "qm_errno.h"
#include "qm_types.h"
#include "prodtst_hal_uart.h"
#include "qm_utils_string.h"
#include "qm_kernel.h"


void *prodtst_spcl_establish(void *param);
int prodtst_spcl_destroy(void *handle);
int prodtst_spcl_send(void *handle, char *buf, int size);
int prodtst_spcl_recv(void *handle, char *buf, int size, int timeout);

static prodtst_special_t  g_prodtst_special[] = {

    {"NFC", prodtst_spcl_establish, prodtst_spcl_destroy, prodtst_spcl_send, prodtst_spcl_recv},

};

#define PRODTST_SPECIAL_NUM    sizeof(g_prodtst_special)/sizeof(g_prodtst_special[0])


prodtst_special_t *prodtst_special_get(char *action)
{
    int i = 0;
    for(i = 0; i < PRODTST_SPECIAL_NUM; i++){
        if(strlen(action) == strlen(g_prodtst_special[i].action) && 0 == strcmp(action, g_prodtst_special[i].action)){
            return &g_prodtst_special[i];
        }
    }
    
    return NULL;
}

typedef struct{
    int uart_channel; 
}spcl_handle_t;

static spcl_handle_t g_spcl_handle = {0};
#define PRODTST_NFC_UART_CHANNEL   PRODTST_UART_NUM_7


void *prodtst_spcl_establish(void *param)
{
    static int init = 0;
    qm_err_t ret = QM_EOK;
    g_spcl_handle.uart_channel = PRODTST_NFC_UART_CHANNEL;
    if(!init){
        init = 1;
        ret = prodtst_uart_init(PRODTST_NFC_UART_CHANNEL, 9600);
    }

    return (void*)&g_spcl_handle;
}

int prodtst_spcl_destroy(void *handle)
{
    spcl_handle_t *spcl_handle = NULL;
    if(handle == NULL){
        return -QM_EINVAL;
    }

    return QM_EOK;
}

int prodtst_spcl_send(void *handle, char *buf, int size)
{
    int hex_len = size/2; 
    uint8_t *hex = NULL;
    qm_err_t ret = QM_EOK;
    spcl_handle_t *spcl_handle = NULL;

    if(handle == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    spcl_handle = (spcl_handle_t*)handle;

    hex = (uint8_t*)qm_malloc(hex_len);
    if(hex == NULL){
        return -QM_ENOMEM;
    }
    
    hex_str_to_nums(buf, size, hex);
    ret = prodtst_uart_write(spcl_handle->uart_channel, hex, hex_len);
    qm_free(hex);
    return ret;
}

int prodtst_spcl_recv(void *handle, char *buf, int size, int timeout)
{
    int recv_len = 0;
    int hex_len = size/2; 
    uint8_t *hex = NULL;
    qm_err_t ret = QM_EOK;
    spcl_handle_t *spcl_handle = NULL;

    hex = (uint8_t*)qm_malloc(hex_len);
    if(hex == NULL){
        return -QM_ENOMEM;
    }
    spcl_handle = (spcl_handle_t*)handle;

    recv_len = prodtst_uart_read(spcl_handle->uart_channel, hex, hex_len, timeout);
    if(recv_len > 0){
        hex_to_strs(hex, recv_len, buf);
        ret = recv_len*2;
    }
    qm_free(hex);
    return ret;
}


