#include "led.h"
#include "gpio_ext.h"
#include "qm.h"

#define LED_COLOUR_NUM  3

typedef struct{
    uint32_t gpio_pin;
}led_gpio_t;

typedef struct{
    int colour_support;
    led_gpio_t gpio[LED_COLOUR_NUM];
}led_info_t;

static qm_timer_t led_timer;


#define LED_BLINK_FAST_MS   100
#define LED_BLINK_SLOW_MS   500

typedef struct{
    uint8_t active;
    uint8_t status;
    uint8_t count;
    uint8_t colour;
}led_ctrl_t;


static led_info_t g_led_info[LED_TYPE_MAX] = {

    [LED_TYPE_PRODTST_CHANNEL_1] = {
        .colour_support = 1, 
        .gpio[LED_COLOUR_GREEN] = { .gpio_pin = GPIO_EXT_OC_Pin_0},
        .gpio[LED_COLOUR_RED] = {.gpio_pin = GPIO_EXT_OC_Pin_8}
    },

    [LED_TYPE_PRODTST_CHANNEL_2] = {
        .colour_support = 1, 
        .gpio[LED_COLOUR_GREEN] = { .gpio_pin = GPIO_EXT_OC_Pin_1},
        .gpio[LED_COLOUR_RED] = {.gpio_pin = GPIO_EXT_OC_Pin_9}
    },

    [LED_TYPE_PRODTST_CHANNEL_3] = {
        .colour_support = 1, 
        .gpio[LED_COLOUR_GREEN] = { .gpio_pin = GPIO_EXT_OC_Pin_2},
        .gpio[LED_COLOUR_RED] = {.gpio_pin = GPIO_EXT_OC_Pin_10}
    },

    [LED_TYPE_PRODTST_CHANNEL_4] = {
        .colour_support = 1, 
        .gpio[LED_COLOUR_GREEN] = { .gpio_pin = GPIO_EXT_OC_Pin_3},
        .gpio[LED_COLOUR_RED] = {.gpio_pin = GPIO_EXT_OC_Pin_11}
    },

    [LED_TYPE_PRODTST_CHANNEL_5] = {
        .colour_support = 1, 
        .gpio[LED_COLOUR_GREEN] = { .gpio_pin = GPIO_EXT_OC_Pin_4},
        .gpio[LED_COLOUR_RED] = {.gpio_pin = GPIO_EXT_OC_Pin_12}
    },

    [LED_TYPE_PRODTST_CHANNEL_6] = {
        .colour_support = 1, 
        .gpio[LED_COLOUR_GREEN] = { .gpio_pin = GPIO_EXT_OC_Pin_5},
        .gpio[LED_COLOUR_RED] = {.gpio_pin = GPIO_EXT_OC_Pin_13}
    },

    [LED_TYPE_PRODTST_READY] = {
        .colour_support = 0, 
        .gpio[LED_COLOUR_NONE] = {.gpio_pin = GPIO_EXT_OC_Pin_6}
    },

    [LED_TYPE_NETWORK] = {
        .colour_support = 0, 
        .gpio[LED_COLOUR_NONE] = {.gpio_pin = GPIO_EXT_OC_Pin_7}
    },
};

static led_ctrl_t led_ctrl[LED_TYPE_MAX] = {0};
static int is_led_active(void);

void led_on(led_type_t type, led_colour_t colour)
{
    if(type >= LED_TYPE_MAX){
        return;
    }

    if(colour >= LED_COLOUR_MAX){
        return;
    }

    led_ctrl[type].active = 0;
    led_ctrl[type].status = 1;
    
    if(!is_led_active()){
        qm_timer_stop(&led_timer);
    }

    if(g_led_info[type].colour_support){
        if(led_ctrl[type].colour){
            gpio_ext_set(g_led_info[type].gpio[led_ctrl[type].colour].gpio_pin, 0);
        }
        gpio_ext_set(g_led_info[type].gpio[colour].gpio_pin, 1);
    }else{
        gpio_ext_set(g_led_info[type].gpio[LED_COLOUR_NONE].gpio_pin, 1);
    }

    led_ctrl[type].colour = colour;

}

void led_off(led_type_t type)
{
    if(type >= LED_TYPE_MAX){
        return;
    }

    led_ctrl[type].active = 0;
    led_ctrl[type].status = 0;

    if(!is_led_active()){
        qm_timer_stop(&led_timer);
    }

    if(g_led_info[type].colour_support){
        gpio_ext_set(g_led_info[type].gpio[led_ctrl[type].colour].gpio_pin, 0);
    }else{
        gpio_ext_set(g_led_info[type].gpio[LED_COLOUR_NONE].gpio_pin, 0);
    }

    led_ctrl[type].colour = LED_COLOUR_NONE;
}

static void led_timeout(qm_timer_t *timer, void *arg)
{
    int i = 0;
    int change = 0;
    for(i = 0; i < LED_TYPE_MAX; i++){
        if(led_ctrl[i].active == 0){
            continue;
        }

        if(led_ctrl[i].count != 0){
            --led_ctrl[i].count;
            if(led_ctrl[i].count == 0){
                led_ctrl[i].count = 5;
                change = 1;
            }
        }else{
            change = 1;
        }

        if(!change){
            continue;
        }

        if(led_ctrl[i].status == 1){
            led_ctrl[i].status = 0;
            gpio_ext_set(g_led_info[i].gpio[led_ctrl[i].colour].gpio_pin, 0);
        }else{
            led_ctrl[i].status = 1;
            gpio_ext_set(g_led_info[i].gpio[led_ctrl[i].colour].gpio_pin, 1);
        }
    }
}

static int is_led_active(void)
{
    int i = 0;
    for(i = 0; i < LED_TYPE_MAX; i++){
        if(led_ctrl[i].active){
            return 1;
        }
    }
    return 0;
}

void led_blink(led_type_t type, led_colour_t colour, led_blink_t blink)
{
    qm_err_t ret = QM_EOK;

    if(type >= LED_TYPE_MAX){
        return;
    }
    if(colour >= LED_COLOUR_MAX){
        return;
    }

    if(blink == LED_BLINK_FAST){
        led_ctrl[type].count = 0;
    }else if(blink == LED_BLINK_SLOW){
        led_ctrl[type].count = 5;
    }

    if(!is_led_active()){
        ret = qm_timer_start(&led_timer);
        if(ret != QM_EOK){
            return;
        }
    }

    if(g_led_info[type].colour_support){
        if(led_ctrl[type].colour){
            gpio_ext_set(g_led_info[type].gpio[led_ctrl[type].colour].gpio_pin, 0);
        }
        gpio_ext_set(g_led_info[type].gpio[colour].gpio_pin, 1);
    }else{
        gpio_ext_set(g_led_info[type].gpio[LED_COLOUR_NONE].gpio_pin, 1);
    }

    led_ctrl[type].active = 1;
    led_ctrl[type].status = 1;
    led_ctrl[type].colour = colour;
}


void led_all_off(void)
{
    int i = 0;

    for(i = 0; i < LED_TYPE_MAX; i++){
        led_off(i);
    }
}

void led_init(void)
{
    qm_err_t ret = QM_EOK;
    memset(&led_ctrl, 0, sizeof(led_ctrl_t));
    led_all_off();
    ret = qm_timer_new(&led_timer, led_timeout, NULL, LED_BLINK_FAST_MS, 1);
    if(ret != QM_EOK){
        return;
    }
}