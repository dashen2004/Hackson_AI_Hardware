#include "qm.h"
#include "qm_work.h"
#include "qm_gpio.h"

#include "bsp_button.h"
#include "bsp_audio.h"

#include "audio_main.h"

#include "multi_button.h"

typedef struct 
{
    qm_timer_t      button_timer;

    qm_gpio_dev_t   wakeup_dev;
    qm_gpio_dev_t   mute_dev;

    multi_button_t  wakeup_button_set;
    multi_button_t  mute_button_set;

}bsp_button_handle_t;

static bsp_button_handle_t g_button_handle = 
{
    .wakeup_dev = {
        .port = 10,
        .config = {
            .mode = QM_GPIO_MODE_INPUT,
            .pull_en = QM_GPIO_PULLUP_ONLY,
        },
    },    
    
    .mute_dev = {
        .port = 18,
        .config = {
            .mode = QM_GPIO_MODE_INPUT,
            .pull_en = QM_GPIO_PULLUP_ONLY,
        },
    }
};

static void button_isr_handler(void *arg)
{
    bsp_button_handle_t *button_handle = (bsp_button_handle_t *)arg;
    if(button_handle == NULL){
        return ;
    }
    qm_timer_start_from_isr(&button_handle->button_timer);
}

static void button_ticks_timeout(qm_timer_t *timer, void *arg)
{
    multi_button_ticks();
}

static uint8_t wakeup_pin_level_get(void)
{
    return qm_gpio_get_level(&g_button_handle.wakeup_dev);
}

static uint8_t mute_pin_level_get(void)
{
    return qm_gpio_get_level(&g_button_handle.mute_dev);
}

static void wakeup_button_press_down_handler(void *btn)
{   
    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_BUTTON_WAKEUP_ID, NULL, 0);
}

static void wakeup_button_press_hold_handler(void *btn)
{   
    QM_LOGD("1", "ready_button_press_hold_handler");
    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_BUTTON_WAKEUP_HOLD_ID, NULL, 0);
}

static void mute_button_press_down_handler(void *btn)
{   
    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_BUTTON_MUTE_ID, NULL, 0);
}

static void mute_button_press_hold_handler(void *btn)
{   
    QM_LOGD("1", "mute_button_press_hold_handler");
    qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_BUTTON_MUTE_HOLD_ID, NULL, 0);
}

qm_err_t bsp_button_init(void)
{
    qm_timer_new(&g_button_handle.button_timer, button_ticks_timeout, NULL, CONFIG_MULTI_BUTTON_TICKS_INTERVAL, 1);

    qm_gpio_init(&g_button_handle.wakeup_dev);
    qm_gpio_enable_irq(&g_button_handle.wakeup_dev, QM_GPIO_INTR_NEGEDGE, button_isr_handler, (void *)&g_button_handle);

    qm_gpio_init(&g_button_handle.mute_dev);
    qm_gpio_enable_irq(&g_button_handle.mute_dev, QM_GPIO_INTR_NEGEDGE, button_isr_handler, (void *)&g_button_handle);

    multi_button_init(&g_button_handle.wakeup_button_set, wakeup_pin_level_get, 0);
    multi_button_attach(&g_button_handle.wakeup_button_set, PRESS_EVENT_DOWN, wakeup_button_press_down_handler);
    multi_button_attach(&g_button_handle.wakeup_button_set, PRESS_EVENT_LONG_START, wakeup_button_press_hold_handler);

    multi_button_init(&g_button_handle.mute_button_set, mute_pin_level_get, 0);
    multi_button_attach(&g_button_handle.mute_button_set, PRESS_EVENT_DOWN, mute_button_press_down_handler);
    multi_button_attach(&g_button_handle.mute_button_set, PRESS_EVENT_LONG_START, mute_button_press_hold_handler);

    multi_button_start(&g_button_handle.wakeup_button_set);
    multi_button_start(&g_button_handle.mute_button_set);

    return QM_EOK;
}

qm_err_t bsp_button_stop(void)
{
    multi_button_stop(&g_button_handle.wakeup_button_set);
    multi_button_stop(&g_button_handle.mute_button_set);

    return QM_EOK;
}

qm_err_t bsp_button_start(void)
{
    multi_button_start(&g_button_handle.wakeup_button_set);
    multi_button_start(&g_button_handle.mute_button_set);

    return QM_EOK;
}