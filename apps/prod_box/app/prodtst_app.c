#include "prodtst_core.h"
#include "drivers.h"

void prodtst_notify_callback(int channel, int prodtst_status)
{

    switch(prodtst_status){

        case PRODTST_STATUS_START:
            led_blink(LED_TYPE_PRODTST_CHANNEL_1, LED_BLINK_SLOW);

        break;

        case PRODTST_STATUS_SUCCESS:
            led_on(LED_TYPE_PRODTST_CHANNEL_1);

        break;

        case PRODTST_STATUS_FAIL:
            led_on(LED_TYPE_PRODTST_CHANNEL_2);
            led_off(LED_TYPE_PRODTST_CHANNEL_1);

        break;

        case PRODTST_STATUS_READY:
            led_on(LED_TYPE_PRODTST_READY);

        break;

        case PRODTST_STATUS_NOT_READY:
            led_off(LED_TYPE_PRODTST_READY);

        break;

        case PRODTST_STATUS_NEXT_ROUND:
            led_off(LED_TYPE_PRODTST_CHANNEL_1);
            led_off(LED_TYPE_PRODTST_CHANNEL_2);
            beep_off();

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


void prodtst_app_init(void)
{
    prodtst_init(prodtst_notify_callback);
}
