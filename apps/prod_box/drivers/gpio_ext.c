#include "driver/gpio.h"
#include "gpio_ext.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "rom/ets_sys.h"


#define I2C_MASTER_SCL_IO          14               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO          13               /*!< gpio number for I2C master data  */

#define GPIO_EXT_ISR_IO            16               /*!< gpio number for interrupt  */
#define GPIO_EXT_INPUT_PIN_SEL     (1ULL<<GPIO_EXT_ISR_IO)

#define CH423_SDA_SET   gpio_set_level(I2C_MASTER_SDA_IO, 1)
#define CH423_SDA_CLR   gpio_set_level(I2C_MASTER_SDA_IO, 0)

#define CH423_SCL_SET   gpio_set_level(I2C_MASTER_SCL_IO, 1)
#define CH423_SCL_CLR   gpio_set_level(I2C_MASTER_SCL_IO, 0)

#define CH423_SDA_IN    gpio_get_level(I2C_MASTER_SDA_IO)

#define gpio_ext_delay_us(us)     ets_delay_us(us)

static portMUX_TYPE gpio_ext_spinlock = portMUX_INITIALIZER_UNLOCKED;
#define GPIO_EXT_ENTER_CRITICAL()        portENTER_CRITICAL(&gpio_ext_spinlock)
#define GPIO_EXT_CRITICAL()              portEXIT_CRITICAL(&gpio_ext_spinlock)


static uint8_t gpio_ext_low_oc = 0;
static uint8_t gpio_ext_high_oc = 0;
static uint8_t gpio_ext_io = 0;

static xQueueHandle gpio_ext_queue = NULL;

static void IRAM_ATTR gpio_ext_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_ext_queue, &gpio_num, NULL);
}

static void _gpio_init(void)
{
    gpio_config_t io_conf;

    gpio_pad_select_gpio(I2C_MASTER_SCL_IO);
    gpio_pad_select_gpio(I2C_MASTER_SDA_IO);

    gpio_pad_pullup(I2C_MASTER_SCL_IO);
    gpio_pad_pullup(I2C_MASTER_SDA_IO);
    
    gpio_set_direction(I2C_MASTER_SCL_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(I2C_MASTER_SDA_IO, GPIO_MODE_INPUT_OUTPUT);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_EXT_INPUT_PIN_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_EXT_ISR_IO, GPIO_INTR_ANYEDGE);
    

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_EXT_ISR_IO, gpio_ext_isr_handler, (void*) GPIO_EXT_ISR_IO);

}

static void gpio_ext_isr_thread(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_ext_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}


void ch423_i2c_start( void )    // 操作起始
{
    CH423_SDA_SET;    // 发送起始条件的数据信号
    CH423_SCL_SET;
    gpio_ext_delay_us(4);
    CH423_SDA_CLR;    //发送起始信号
    gpio_ext_delay_us(4);   
    CH423_SCL_CLR;    //钳住I2C总线，准备发送或接收数据 
}

void ch423_i2c_stop( void )    // 操作结束
{
    CH423_SDA_CLR;
    CH423_SCL_CLR; //STOP:when CLK is high DATA change form low to high
    gpio_ext_delay_us(4);
    CH423_SDA_SET;
    CH423_SCL_SET;//发送I2C总线结束信号
    gpio_ext_delay_us(4);
}

void ch423_i2c_ack(void)
{
	CH423_SCL_CLR;
	CH423_SDA_CLR;
	gpio_ext_delay_us(2);
	CH423_SCL_SET;
	gpio_ext_delay_us(2);
	CH423_SCL_CLR;
}

void ch423_i2c_nack(void)
{
	CH423_SCL_CLR;
	CH423_SDA_SET;
	gpio_ext_delay_us(2);
    CH423_SCL_SET;
	gpio_ext_delay_us(2);
	CH423_SCL_CLR;
}

void ch423_i2c_write_byte( unsigned char dat )    // 写一个字节数据
{
    unsigned char i;
    CH423_SCL_CLR;
    for( i = 0; i < 8; i++ )    // 输出8位数据
    {
        if( (dat&0x80) >> 7 ) { CH423_SDA_SET; }
        else { CH423_SDA_CLR; }
        dat<<=1;
        gpio_ext_delay_us(100);
        CH423_SCL_SET;
        gpio_ext_delay_us(100);
        CH423_SCL_CLR;
        gpio_ext_delay_us(100);
    }
}


unsigned char ch423_i2c_read_byte( unsigned char ack )    // 读一个字节数据
{
    unsigned char dat,i;
    dat = 0;
    for( i = 0; i < 8; i++ )    // 输入8位数据
    {
        CH423_SCL_CLR;
        gpio_ext_delay_us(100);
        CH423_SCL_SET;
        gpio_ext_delay_us(100);
        dat<<=1;
        if(CH423_SDA_IN) dat++;    // 输入1位
        gpio_ext_delay_us(100);
    }
    if (!ack){
        ch423_i2c_nack();//发送nACK
    }else{
        ch423_i2c_ack(); //发送ACK   
    }
	dat &= 0x7f;	

    return dat;
}


void ch423_write( unsigned char cmd, unsigned char data)    
{
    GPIO_EXT_ENTER_CRITICAL();

    ch423_i2c_start();    // 启动总线
    ch423_i2c_write_byte( cmd );
    ch423_i2c_ack();

    ch423_i2c_write_byte( data );    // 发送数据
    ch423_i2c_ack();
    ch423_i2c_stop();    // 结束总线  

    GPIO_EXT_CRITICAL();
}


unsigned char ch423_read(void)    // 读取数据
{
    unsigned char din;
    ch423_i2c_start();    // 启动总线
    ch423_i2c_write_byte( CH423_RD_IO_CMD );    // 此值为0x4D
    ch423_i2c_ack();
    din = ch423_i2c_read_byte(1);    // 读取数据
    ch423_i2c_stop();    // 结束总线
    return( din );
}


void gpio_ext_set(uint32_t gpio_pin, uint8_t value)
{
    uint8_t cmd = 0;
    uint8_t gpio_dat = 0;
    uint8_t data = 0;
    uint8_t low_gpio = (uint8_t)gpio_pin;
    uint8_t high_gpio = (uint8_t)(gpio_pin>>8);

    if(low_gpio){
        cmd = CH423_OC_L_CMD;
        if(value){
            gpio_ext_low_oc |= low_gpio;
        }else{
            gpio_ext_low_oc &= ~low_gpio;
        }
        data = gpio_ext_low_oc;
        ch423_write(cmd, data);
    }
    if(high_gpio){
        cmd = CH423_OC_H_CMD;
        if(value){
            gpio_ext_high_oc |= high_gpio;
        }else{
            gpio_ext_high_oc &= ~high_gpio;
        }
        data = gpio_ext_high_oc;

        ch423_write(cmd, data);
    }
    
}

static void gpio_ext_thread(void *arg)
{
    ch423_output_t output;
    output.v = 0x0;

    printf("gpio ext init\r\n");

    ch423_write(CH423_SYS_CMD, BIT_X_INT|BIT_IO_OE);     

    vTaskDelay(100 / portTICK_PERIOD_MS);

    while(1){

        gpio_ext_set(GPIO_EXT_OC_Pin_6|GPIO_EXT_OC_Pin_7|GPIO_EXT_OC_Pin_8|GPIO_EXT_OC_Pin_9|GPIO_EXT_OC_Pin_10|GPIO_EXT_OC_Pin_11|GPIO_EXT_OC_Pin_12|GPIO_EXT_OC_Pin_13, 1);   
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_ext_set(GPIO_EXT_OC_Pin_6|GPIO_EXT_OC_Pin_7|GPIO_EXT_OC_Pin_8|GPIO_EXT_OC_Pin_9|GPIO_EXT_OC_Pin_10|GPIO_EXT_OC_Pin_11|GPIO_EXT_OC_Pin_12|GPIO_EXT_OC_Pin_13, 0);     
        vTaskDelay(300 / portTICK_PERIOD_MS);

    }

    vTaskDelete(NULL);
}


void gpio_ext_init(void)
{
   
    _gpio_init();

    //create a queue to handle gpio event from isr
    gpio_ext_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_ext_isr_thread, "gpio isr", 2048, NULL, 10, NULL);

    //xTaskCreate(gpio_ext_thread, "gpio ext", 2048, (void* ) 0, 10, NULL);

}