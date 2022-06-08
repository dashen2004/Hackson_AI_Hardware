#include "drivers.h"

void drivers_init(void)
{
    gpio_ext_init();

    key_init();

    beep_init();

    led_init();

    uart_ext_init();

    scanner_init();
}