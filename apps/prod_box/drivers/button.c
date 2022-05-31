#include "button.h"
#include "multi_button.h"
#include "driver/gpio.h"
 
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "prodtst_core.h"


#define GPIO_SET_BUTTON_IO                 35             
#define GPIO_SET_BUTTON_INPUT_PIN_SEL     (1ULL<<GPIO_SET_BUTTON_IO)

static QueueHandle_t gpio_button_queue = NULL;
static TimerHandle_t button_timer = NULL;
static bool is_button_timer_started = false;

static struct Button button_set;

uint8_t read_button_GPIO(void)
{
	return gpio_get_level(GPIO_SET_BUTTON_IO);
}


static void IRAM_ATTR gpio_set_key_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    //xQueueSendFromISR(gpio_button_queue, &gpio_num, NULL);
    if(is_button_timer_started){
        return;
    }
    xTimerStartFromISR(button_timer, 0);
    is_button_timer_started = true;
}


static void button_isr_thread(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_button_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

static void button_ticks_timeout(void *arg)
{
    button_ticks();
}


void BTN1_PRESS_DOWN_Handler(void* btn)
{
	//do something...

     //printf("press down\n");
}

void BTN1_PRESS_UP_Handler(void* btn)
{
	//do something...
    //printf("press up\n");

    prodtst_next_round_notify();
    
    xTimerStop(button_timer, 0);
    is_button_timer_started = false;
    button_set.state = 0;
}

void BTN1_PRESS_REPEAT_Handler(void* btn)
{
	//do something...
    //printf("press repeat\n");
}

void BTN1_SINGLE_Click_Handler(void* btn)
{
	//do something...
    ////printf("single click\n");
}

void BTN1_DOUBLE_Click_Handler(void* btn)
{
	//do something...
    //printf("double click\n");
}

void BTN1_LONG_PRESS_START_Handler(void* btn)
{
	//do something...
    //printf("long press start\n");
}

void BTN1_LONG_PRESS_HOLD_Handler(void* btn)
{
	//do something...
    //printf("long press hold\n");
}


void key_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    io_conf.pin_bit_mask = GPIO_SET_BUTTON_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    
    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_SET_BUTTON_IO, gpio_set_key_isr_handler, (void*) GPIO_SET_BUTTON_IO);

    gpio_button_queue = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(button_isr_thread, "key", 2048, (void* ) 0, 10, NULL);

    button_timer = xTimerCreate("button", TICKS_INTERVAL/portTICK_PERIOD_MS, pdTRUE, NULL, button_ticks_timeout);

    button_init(&button_set, read_button_GPIO, 0);

    button_attach(&button_set, PRESS_DOWN,       BTN1_PRESS_DOWN_Handler);
	button_attach(&button_set, PRESS_UP,         BTN1_PRESS_UP_Handler);
    button_attach(&button_set, PRESS_REPEAT,     BTN1_PRESS_REPEAT_Handler);
    button_attach(&button_set, SINGLE_CLICK,     BTN1_SINGLE_Click_Handler);
    button_attach(&button_set, DOUBLE_CLICK,     BTN1_DOUBLE_Click_Handler);
    button_attach(&button_set, LONG_PRESS_START, BTN1_LONG_PRESS_START_Handler);
    button_attach(&button_set, LONG_PRESS_HOLD,  BTN1_LONG_PRESS_HOLD_Handler);

    button_start(&button_set);

    //xTimerStart(button_timer, 0);
    

}

