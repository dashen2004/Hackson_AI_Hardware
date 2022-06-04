#include "prodtst_core.h"
#include "drivers.h"

void prodtst_notify_callback(int channel, int prodtst_status)
{

    switch(prodtst_status){

        case PRODTST_STATUS_START:
            led_blink(LED_TYPE_PRODTST_CHANNEL_1, LED_COLOUR_GREEN, LED_BLINK_SLOW);

        break;

        case PRODTST_STATUS_SUCCESS:
            printf("PRODTST_STATUS_SUCCESS\r\n");
            led_on(LED_TYPE_PRODTST_CHANNEL_1, LED_COLOUR_GREEN);
            beep_off();

        break;

        case PRODTST_STATUS_FAIL:
            printf("PRODTST_STATUS_FAIL\r\n");
            led_on(LED_TYPE_PRODTST_CHANNEL_1, LED_COLOUR_RED);
            beep_off();
        break;

        case PRODTST_STATUS_NOT_READY:
            led_off(LED_TYPE_PRODTST_READY);

        break;

        case PRODTST_STATUS_CONFIG_READY:
            beep_work(500, 500, 0);
            led_blink(LED_TYPE_PRODTST_READY,LED_COLOUR_NONE, LED_BLINK_SLOW);

        break;

        case PRODTST_STATUS_READY:
            beep_work(1000, 500, 0);
            led_on(LED_TYPE_PRODTST_READY, LED_COLOUR_NONE);
            led_off(LED_TYPE_PRODTST_CHANNEL_1);

        break;

        case PRODTST_STATUS_GENERIC_START:


        break;

        case PRODTST_STATUS_GENERIC_SUCCESS:


        break;

        case PRODTST_STATUS_FUN_START:


        break;
        
        case PRODTST_STATUS_FUN_SUCCESS:


        break;

        case PRODTST_STATUS_SPECIAL_START:
            beep_work(500, 500, 1);

        break;

        case PRODTST_STATUS_SPECIAL_SUCCESS:
            beep_off();

        break;

        default:
            break;
    }

}

#include "mtrace.h"

void prodtst_app_init(void)
{
    prodtst_init(prodtst_notify_callback);
}
