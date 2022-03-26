#include "drivers.h"

void drivers_init(void)
{
    gpio_ext_init();

    key_init();

    beep_init();

    led_init();

    //beep_work(500, 500, 1);

    uart_ext_init();
}