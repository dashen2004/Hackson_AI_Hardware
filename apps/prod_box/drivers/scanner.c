
#include "scanner.h"
#include "uart_ext.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qm_log.h"


#define scanner_log(format, ...)  custom_log("scanner", format, ##__VA_ARGS__)

void scanner_thread(void *arg)
{

    uint32_t recv = 0;
    uint8_t recvbuf[256] = {0};

    uart_ext_config(UART_EXT_NUM_7, 9600, 256, 0);

    while(1)
    {
        recv = uart_ext_read_bytes(UART_EXT_NUM_7, recvbuf, sizeof(recvbuf), 1000);
        if(recv){
            //scanner_log("scan result: %d, %.*s\n", recv, recv, recvbuf);
        }

    }
}


int scanner_read(uint8_t* buf, uint32_t length, uint32_t timeout)
{
    return uart_ext_read_bytes(UART_EXT_NUM_7, buf, length, timeout);
}

static uint8_t recvbuf[256] = {0};

 void scanner_init(void)
 {

     uart_ext_config(UART_EXT_NUM_7, 9600, 256, 256);
     //xTaskCreate(scanner_thread, "scanner", 2048, (void *)0, 10, NULL);

 }