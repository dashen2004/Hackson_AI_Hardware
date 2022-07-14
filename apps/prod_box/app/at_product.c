#include "qm_at.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define AT_PRODUCT_TXD  (GPIO_NUM_4)
#define AT_PRODUCT_RXD  (GPIO_NUM_5)
#define AT_PRODUCT_RTS  (UART_PIN_NO_CHANGE)
#define AT_PRODUCT_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

static void *product_uart_channel = NULL;
static char product_uart_buf[BUF_SIZE];

static int product_uart_send(void *handle, char *buf, int len);
static int product_uart_recv(void *handle, char *buf, int len, int timeout);

static qm_at_result_t at_exec(void *channel);

static qm_at_cmd_t prodtst_at_cmd[] = {
    {"ATS", NULL, NULL, NULL, NULL, at_exec},
};

#define PRODTST_AT_CMD_NUM (sizeof(prodtst_at_cmd) / sizeof(prodtst_at_cmd[0]))


static qm_at_server_param_t at_server_param = {
    1,
    AT_CHANNEL_UART,
    product_uart_buf,
    BUF_SIZE,
    NULL,
    product_uart_send,
    product_uart_recv,
};


static qm_at_result_t at_exec(void *channel)
{
    return QM_AT_RESULT_OK;
}


static int product_uart_send(void *handle, char *data, int len)
{
    return uart_write_bytes(UART_NUM_1, data, len);
}

static int product_uart_recv(void *handle, char *data, int len, int timeout)
{
    return uart_read_bytes(UART_NUM_1, (uint8_t*)data, len, timeout / portTICK_RATE_MS);
}

void at_server_product_init(void)
{

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, AT_PRODUCT_TXD, AT_PRODUCT_RXD, AT_PRODUCT_RTS, AT_PRODUCT_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    product_uart_channel = qm_at_server_init(&at_server_param);

    qm_at_register_commands(product_uart_channel, prodtst_at_cmd, PRODTST_AT_CMD_NUM);

} 


static qm_at_client_t *product_at_client = NULL;

static qm_at_client_param_t at_client_param = {
    AT_CHANNEL_UART,
    product_uart_buf,
    BUF_SIZE,
    NULL,
    product_uart_send,
    product_uart_recv,
};


static void at_client_product_task(void *arg)
{
    qm_at_response_t *resp = NULL;
    int test = 0;

    vTaskDelay(3000/portTICK_PERIOD_MS);

    resp = qm_at_client_create_resp(64, 0, 5000);

    qm_at_client_exec_cmd(resp, "AT+TEST?");

    qm_at_client_resp_parse_line_args(resp, 1,"%*[^=]:%d", &test);

    printf("test value :%d\r\n", test);

    qm_at_client_delete_resp(resp);

    vTaskDelete(NULL);
}


void at_client_product_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, AT_PRODUCT_TXD, AT_PRODUCT_RXD, AT_PRODUCT_RTS, AT_PRODUCT_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    product_at_client = qm_at_client_init(&at_client_param);

    xTaskCreate(at_client_product_task, "at client product", 4096, (void *)0, 5, NULL);

}