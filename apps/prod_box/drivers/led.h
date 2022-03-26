#ifndef _LED_H_
#define _LED_H_

typedef enum{
    LED_TYPE_PRODTST_CHANNEL_1 = 0,
    LED_TYPE_PRODTST_CHANNEL_2,
    LED_TYPE_PRODTST_CHANNEL_3,
    LED_TYPE_PRODTST_CHANNEL_4,
    LED_TYPE_PRODTST_CHANNEL_5,
    LED_TYPE_PRODTST_CHANNEL_6,
    LED_TYPE_PRODTST_READY,
    LED_TYPE_NETWORK,
    LED_TYPE_MAX,
}led_type_t;

typedef enum{
    LED_BLINK_FAST,
    LED_BLINK_SLOW,
    LED_BLINK_MAX,
}led_blink_t;


void led_on(led_type_t type);
void led_off(led_type_t type);
void led_blink(led_type_t type, led_blink_t blink);
void led_init(void);
#endif