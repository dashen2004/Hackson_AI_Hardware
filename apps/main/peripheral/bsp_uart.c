#include "bsp_uart.h"

#include "qm_log.h"
#include "qm_uart.h"
#include "qm_work.h"
#include "vb701n.h"
#include "driver/gpio.h"

#define LOG_TAG "uart"

#define BSP_UART_PORT    1
#define BSP_UART_TX_PIN  40
#define BSP_UART_RX_PIN  39
#define BSP_UART_BAUD    2000000

static qm_uart_dev_t g_uart = {
    .config = {
        .baud_rate = BSP_UART_BAUD,
        .data_bits = QM_UART_DATA_8_BITS,
        .parity    = QM_UART_PARITY_NONE,
        .stop_bits = QM_UART_STOP_BITS_1,
        .flow_ctrl = QM_UART_FLOW_CTRL_NONE,
        .pins.tx_pin = BSP_UART_TX_PIN,
        .pins.rx_pin = BSP_UART_RX_PIN,
    },
    .port = BSP_UART_PORT,
};

qm_err_t bsp_uart_init(void)
{
    qm_err_t ret;

    QM_LOGD(LOG_TAG, "port %u TX=%u RX=%u baud=%u",
            (unsigned)BSP_UART_PORT, (unsigned)BSP_UART_TX_PIN,
            (unsigned)BSP_UART_RX_PIN, (unsigned)BSP_UART_BAUD);
    ret = qm_work_on_init();
    if (ret != QM_EOK) {
        QM_LOGD(LOG_TAG, "qm_work_on_init fail %d", ret);
        return ret;
    }
    ret = qm_uart_init(&g_uart);
    if (ret != QM_EOK) {
        QM_LOGD(LOG_TAG, "qm_uart_init fail %d", ret);
        return ret;
    }
    gpio_set_pull_mode(BSP_UART_RX_PIN, GPIO_PULLUP_ONLY);
    QM_LOGD(LOG_TAG, "rx pin %u level %d",
            (unsigned)BSP_UART_RX_PIN, gpio_get_level(BSP_UART_RX_PIN));
    return vb701n_bind(&g_uart);
}
