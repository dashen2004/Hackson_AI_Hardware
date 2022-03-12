#include "drivers.h"

void drivers_init(void)
{
    gpio_ext_init();

    beep_init();

    //beep_work(500, 500, 1);

    uart_ext_init();
}