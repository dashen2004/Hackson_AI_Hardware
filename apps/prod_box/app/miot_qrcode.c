#include "drivers.h"
#include "qm_kernel.h"
#include "qm_at.h"
#include "prodtst_hal_uart.h"
#include "qm_log.h"
#include "qm_errno.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_TAG "miot_qrcode"

#define MIOT_QRCODE_UART_NUM  PRODTST_UART_NUM_1 

void miot_qrcode_task(void *arg)
{
    int ret = 0;
    int argc = 0;
    char buf[256] = {0};
    char code[33] = {0};
    qm_at_client_t *at_client = NULL;
    qm_at_response_t *resp = NULL;
    #define MIOT_QRCODE_MAX_LEN  33

    const char *req_expr = "%*[^\"]\"%[^\"]\"";

    qm_at_client_param_t at_param = {0};

    uint8_t entery_prodtst_cmd[] = {0xA5, 0xA5, 0x03, 0x61, 0x00, 0x01, 0x03, 0xB2};

    prodtst_uart_init(MIOT_QRCODE_UART_NUM, 9600);

    at_param.recv_buf_len = 128;
    at_param.send = prodtst_uart_write;
    at_param.recv = prodtst_uart_read;

    while(1){

        ret = scanner_read((uint8_t*)buf, MIOT_QRCODE_MAX_LEN, 1000);
        if(ret != MIOT_QRCODE_MAX_LEN){
            continue;
        }
        buf[ret] = '\0';
        QM_LOGI(LOG_TAG, "qrcode: %s", buf);

        prodtst_uart_write(MIOT_QRCODE_UART_NUM, entery_prodtst_cmd, sizeof(entery_prodtst_cmd));

        at_client = qm_at_client_init(&at_param);
        if(at_client == NULL){
            continue;
        }
        resp = qm_at_client_create_resp(64, 0, 2000);
        if(resp == NULL){
            continue;
        }

        ret = qm_at_client_exec_cmd(resp, "AT+MIOT_QRCODE=\"%s\"", buf);
        if(ret == -QM_ETIMEOUT){
            QM_LOGE(LOG_TAG, "wait response timeout");
            beep_work_times(1500, 200, 1);
            continue;
        }else if(ret == -QM_ERROR){
            beep_work_times(1500, 200, 1);
            QM_LOGE(LOG_TAG, "response failed");
            continue;
        }

        qm_at_client_exec_cmd(resp, "AT+MIOT_QRCODE?");
        argc = qm_at_client_resp_parse_line_args(resp, 1, req_expr, code);
        if (argc != 1){
            beep_work_times(1500, 200, 1);
            QM_LOGE(LOG_TAG, "at response parse failed");
            continue;
        }
        QM_LOGI(LOG_TAG, "reponse code :%s", code);

        if(strcmp(buf, code) == 0){
            beep_work_times(500, 500, 2);
            QM_LOGI(LOG_TAG, "qrcode config success");
        }else{
            beep_work_times(1500, 200, 1);
            QM_LOGE(LOG_TAG, "qrcode config failed");
        }

        qm_at_client_deinit(at_client);
    }
}

void miot_qrcode_init(void)
{
    xTaskCreate(miot_qrcode_task, "miot qrcode", 2048, (void *)0, 10, NULL);

}