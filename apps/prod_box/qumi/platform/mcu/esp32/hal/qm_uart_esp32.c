#include "qm_uart.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

int32_t qm_uart_init(qm_uart_dev_t *uart)
{
    uart_config_t uart_config = {0};
    uint32_t uart_buf_size = 1024;

    uart_config.baud_rate = uart->config.baud_rate;

    if(uart->config.data_bits == QM_UART_DATA_8_BITS){
        uart_config.data_bits = UART_DATA_8_BITS;
    }else if(uart->config.data_bits == QM_UART_DATA_7_BITS){
        uart_config.data_bits = UART_DATA_7_BITS;
    }else if(uart->config.data_bits == QM_UART_DATA_6_BITS){
        uart_config.data_bits = UART_DATA_6_BITS;
    }else if(uart->config.data_bits == QM_UART_DATA_5_BITS){
        uart_config.data_bits = UART_DATA_5_BITS;
    }

    if(uart->config.parity == QM_UART_PARITY_NONE){
        uart_config.parity = UART_PARITY_DISABLE;
    }else if(uart->config.parity == QM_UART_PARITY_EVEN){
        uart_config.parity = UART_PARITY_EVEN;
    }else if(uart->config.parity == QM_UART_PARITY_ODD){
        uart_config.parity = UART_PARITY_ODD;
    }

    if(uart->config.stop_bits == QM_UART_STOP_BITS_1){
        uart_config.stop_bits = UART_STOP_BITS_1;
    }else if(uart->config.stop_bits == QM_UART_STOP_BITS_1_5){
        uart_config.stop_bits = UART_STOP_BITS_1_5;
    }else if(uart->config.stop_bits == QM_UART_STOP_BITS_2){
        uart_config.stop_bits = UART_STOP_BITS_2;
    }

    if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_NONE){
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    }else if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_RTS){
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_RTS;
    }else if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_CTS){
         uart_config.flow_ctrl = UART_HW_FLOWCTRL_CTS;
    }else if(uart->config.flow_ctrl == QM_UART_FLOW_CTRL_CTS_RTS){
         uart_config.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    }
    
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
    return uart_read_bytes(uart->port, data, expect_size, timeout/portTICK_RATE_MS);
}
