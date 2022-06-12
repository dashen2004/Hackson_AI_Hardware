#include "drivers.h"
#include "qm_kernel.h"
#include "qm_at.h"
#include "prodtst_hal_uart.h"
#include "qm_log.h"
#include "qm_errno.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_TAG "miot_qrcode"

#define MIOT_QRCODE_UART_NUM  PRODTST_UART_NUM_3 


static int product_uart_send(void *handle, char *data, int len)
{
    return uart_ext_write_bytes(MIOT_QRCODE_UART_NUM, (uint8_t*)data, len);
}

static int product_uart_recv(void *handle, char *data, int len, int timeout)
{
    return uart_ext_read_bytes(MIOT_QRCODE_UART_NUM, (uint8_t*)data, len, timeout / portTICK_RATE_MS);
}

void miot_qrcode_task(void *arg)
{
    int ret = 0;
    int argc = 0;
    char buf[256] = {0};
    char req_code[33] = {0};
    char code[33] = {0};
    char *key = NULL;
    int need_len = 0;
    qm_at_client_t *at_client = NULL;
    qm_at_response_t *resp = NULL;
    #define MIOT_QRCODE_MAX_LEN  256

    const char *req_expr = "%*[^\"]\"%[^\"]\"";

    qm_at_client_param_t at_param = {0};

    uint8_t entery_prodtst_cmd[] = {0xA5, 0xA5, 0x03, 0x61, 0x00, 0x01, 0x03, 0xB2};

    uart_ext_config(MIOT_QRCODE_UART_NUM, 9600, 256, 256);

    at_param.recv_buf_len = 128;
    at_param.send = product_uart_send;
    at_param.recv = product_uart_recv;

    while(1){

        ret = scanner_read((uint8_t*)buf, MIOT_QRCODE_MAX_LEN, 300);
        if(ret == 0){
            continue;
        }

        key = strstr(buf, "OOB=");
        if(!key){
            continue;
        }
        
        need_len = 33-(ret-(key-buf)-4);

        printf("need: %d\r\n", need_len);

        if(need_len > 0){
            ret = scanner_read((uint8_t*)buf+ret, need_len, 100);
            if(ret != need_len){
                continue;
            }          
        }

        QM_LOGI(LOG_TAG, "scan data: %s", buf);
        memcpy(req_code, key+4, 32);
        QM_LOGI(LOG_TAG, "scan qrcode: %s", req_code);

        prodtst_uart_write(MIOT_QRCODE_UART_NUM, entery_prodtst_cmd, sizeof(entery_prodtst_cmd));
        qm_msleep(2000);

        uart_ext_fifo_clear(MIOT_QRCODE_UART_NUM);

        at_client = qm_at_client_init(&at_param);
        if(at_client == NULL){
            continue;
        }

        resp = qm_at_client_create_resp(64, 0, 2000);
        if(resp == NULL){
            continue;
        }

        ret = qm_at_client_exec_cmd(resp, "AT+MIOT_QRCODE=\"%s\"", req_code);
        if(ret == -QM_ETIMEOUT){
            QM_LOGE(LOG_TAG, "wait response timeout");
            beep_work_times(1500, 200, 1);
            goto __exit;
        }else if(ret == -QM_ERROR){
            beep_work_times(1500, 200, 1);
            QM_LOGE(LOG_TAG, "response failed");
            goto __exit;
        }

        qm_at_client_exec_cmd(resp, "AT+MIOT_QRCODE?");
        argc = qm_at_client_resp_parse_line_args(resp, 1, req_expr, code);
        if (argc != 1){
            beep_work_times(1500, 200, 1);
            QM_LOGE(LOG_TAG, "at response parse failed");
            goto __exit;
        }
        QM_LOGI(LOG_TAG, "reponse code: %s", code);

        if(strncmp(req_code, code, 32) == 0){
            beep_work_times(500, 500, 2);
            QM_LOGI(LOG_TAG, "qrcode config success");
        }else{
            beep_work_times(1500, 200, 1);
            QM_LOGE(LOG_TAG, "qrcode config failed");
        } 
__exit:
        qm_at_client_deinit(at_client);
        memset(buf, 0, MIOT_QRCODE_MAX_LEN);

    }
}

void miot_qrcode_init(void)
{
    xTaskCreate(miot_qrcode_task, "miot qrcode", 4096, (void *)0, 10, NULL);

}