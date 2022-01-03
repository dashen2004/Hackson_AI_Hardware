#include "qm_uart.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

int32_t qm_uart_init(qm_uart_dev_t *uart)
{
    uart_config_t uart_config = {0};
    uint32_t uart_buf_size = 1024;

    uart_config.baud_rate = uart->config.baud_rate;
    uart_config.data_bits = uart->config.data_bits;
    uart_config.parity = uart->config.parity;
    uart_config.stop_bits = uart->config.stop_bits;
    uart_config.flow_ctrl = uart->config.flow_ctrl;
    
    uart_param_config(uart->port, &uart_config);
    if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_NONE){
        uart_set_pin(uart->port, uart->config.pins.tx_pin, uart->config.pins.rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }else if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_RTS){
        uart_set_pin(uart->port, uart->config.pins.tx_pin, uart->config.pins.rx_pin, uart->config.pins.rtx_pin, UART_PIN_NO_CHANGE);
    }else if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_CTS){
        uart_set_pin(uart->port, uart->config.pins.tx_pin, uart->config.pins.rx_pin, UART_PIN_NO_CHANGE, uart->config.pins.ctx_pin);
    }else if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_CTS_RTS){
        uart_set_pin(uart->port, uart->config.pins.tx_pin, uart->config.pins.rx_pin, uart->config.pins.rtx_pin, uart->config.pins.rtx_pin);
    }

    uart_driver_install(uart->port, uart_buf_size * 2, uart_buf_size, 0, NULL, 0);

    return 0;

}


int32_t qm_uart_write(qm_uart_dev_t *uart, const void *data, uint32_t size, uint32_t timeout)
{
    return uart_write_bytes(uart->port, data, size);
}


int32_t qm_uart_read(qm_uart_dev_t *uart, void *data, uint32_t expect_size, uint32_t timeout)
{
    return uart_read_bytes(uart->port, data, expect_size, timeout);
}
