#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "uart_ext.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"

#define SERIAL_EXT_A_ISR_IO 36
#define SERIAL_EXT_A_INPUT_PIN_SEL (1ULL << SERIAL_EXT_A_ISR_IO)

#define SERIAL_EXT_B_ISR_IO 39
#define SERIAL_EXT_B_INPUT_PIN_SEL (1ULL << SERIAL_EXT_B_ISR_IO)

#define SERIAL_EXT_A_TX_IO 33
#define SERIAL_EXT_A_RX_IO 32

#define SERIAL_EXT_B_TX_IO 5
#define SERIAL_EXT_B_RX_IO 17

#define SERIAL_EXT_RST_IO 15

#define SERIAL_EXT_RX_BUF_SIZE 1024
#define SERIAL_EXT_TX_BUF_SIZE 1024

#define uart_delay_ms(ms) vTaskDelay(ms / portTICK_RATE_MS)
#define uart_rst_set() gpio_set_level(SERIAL_EXT_RST_IO, 1)
#define uart_rst_reset() gpio_set_level(SERIAL_EXT_RST_IO, 0)

#define WK_CHAN_A UART_NUM_1
#define WK_CHAN_B UART_NUM_2

#define UART_EXT_BAUDRATE_MAX  2000000

static xQueueHandle uart_ext_tx_queue = NULL;
#define UART_EXT_TX_QUEUE_SIZE  (UART_EXT_NUM_MAX*10)

static xQueueHandle uart_ext_rx_queue = NULL;
#define UART_EXT_RX_QUEUE_SIZE  (UART_EXT_NUM_MAX*10)

typedef struct{

    uint8_t chan;
    uint8_t port;

    int rx_size;
    RingbufHandle_t rx_ringbuf;
    int tx_size;
    RingbufHandle_t tx_ringbuf;
    
}uart_ext_dev_t;


static uart_ext_dev_t g_uart_ext_dev[UART_EXT_NUM_MAX] = {

    {WK_CHAN_A, 1, 0, NULL, 0, NULL},
    {WK_CHAN_A, 2, 0, NULL, 0, NULL},
    {WK_CHAN_A, 3, 0, NULL, 0, NULL},
    {WK_CHAN_A, 4, 0, NULL, 0, NULL},

    {WK_CHAN_B, 1, 0, NULL, 0, NULL},
    {WK_CHAN_B, 2, 0, NULL, 0, NULL},
    {WK_CHAN_B, 3, 0, NULL, 0, NULL},
    {WK_CHAN_B, 4, 0, NULL, 0, NULL},

};


static int uart_send_bytes(uint8_t uart_num, uint8_t *data, int size)
{
    return uart_write_bytes(uart_num, (const char *)data, size);
}

static int uart_rec_bytes(uint8_t uart_num, uint8_t *data, int size)
{
    return uart_read_bytes(uart_num, data, size, 500 / portTICK_RATE_MS);
}

/****************WkWriteGReg***********************************/
//函数功能：写全局寄存器函数（前提是该寄存器可写，
//某些寄存器如果你写1，可能会自动置1，具体见数据手册)
//参数：
//      greg:为全局寄存器的地址
//      dat:为写入寄存器的数据
//*************************************************************/
void wk_write_greg(uint8_t uart_num, uint8_t greg, uint8_t dat)
{
    uint8_t cmd;
    cmd = 0 | greg;
    uart_send_bytes(uart_num, &cmd, 1); //写指令，对于指令的构成见数据手册
    uart_send_bytes(uart_num, &dat, 1); //写数据
}
/****************WkReadGReg***********************************/
//函数功能：读全局寄存器
//参数：
//      greg:为全局寄存器的地址
//      rec:返回的寄存器值
//************************************************************/
uint8_t wk_read_greg(uint8_t uart_num, uint8_t greg)
{
    uint8_t cmd = 0, rec = 0;
    cmd = 0x40 | greg;
    uart_send_bytes(uart_num, &cmd, 1); //写指令，对于指令的构成见数据手册
    uart_rec_bytes(uart_num, &rec, 1);  //接收返回的寄存器值
    return rec;
}

/**************WkWriteSReg***********************************/
//函数功能:
//参数：port:为子串口
//      sreg:为子串口寄存器
//      dat:为写入寄存器的数据
//注意：在子串口被打通的情况下，向FDAT写入的数据会通过TX引脚输出
//***********************************************************/
void wk_write_sreg(uint8_t uart_num, uint8_t port, uint8_t sreg, uint8_t dat)
{
    uint8_t cmd;
    cmd = 0x0 | ((port - 1) << 4) | sreg;
    uart_send_bytes(uart_num, &cmd, 1); //写指令，对于指令的构成见数据手册
    uart_send_bytes(uart_num, &dat, 1); //写数据
}

/*****************WkReadSReg***********************************/
//函数功能：读子串口寄存器
//参数：port为子串口端口号
//      sreg:为子串口寄存器地址
//      rec:返回的寄存器值
//************************ ***********************************/
uint8_t wk_read_sreg(uint8_t uart_num, uint8_t port, uint8_t sreg)
{
    uint8_t cmd = 0, rec = 0;
    cmd = 0x40 | ((port - 1) << 4) | sreg;
    uart_send_bytes(uart_num, &cmd, 1); //写指令，对于指令的构成见数据手册
    uart_rec_bytes(uart_num, &rec, 1);  //接收返回的寄存器值
    return rec;
}
/**************WkWriteSFifo***********************************/
//函数功能:向子串口fifo写入需要发送的数据
//参数：port:为子串口
//      *dat:写入数据
//      num：为写入数据的个数，单次不超过16个字节
//注意：通过该方式写入的数据，被直接写入子串口的缓存FIFO，然后被发送
//************************************************************/
void wk_write_sfifo(uint8_t uart_num, uint8_t port, uint8_t *dat, uint8_t num)
{
    uint8_t cmd = 0;
    cmd = 0x80 | ((port - 1) << 4) | (num - 1);
    uart_send_bytes(uart_num, &cmd, 1);  //写指令,对于指令构成见数据手册
    uart_send_bytes(uart_num, dat, num); //写数据
}

/**************************************WkReadSFifo***********************************/
//函数功能:从子串口的fifo中读出接收到的数据
//参数：port:为子串口
//      *rec：接收到的数据
//      num：读出的数据个数。
//注意：通过该方式读出子串口缓存中的数据。单次不能超过16个。
//***************************************************************************************/
void wk_read_sfifo(uint8_t uart_num, uint8_t port, uint8_t *rec, uint8_t num)
{
    uint8_t cmd = 0;
    cmd = 0xc0 | ((port - 1) << 4) | (num - 1);
    uart_send_bytes(uart_num, &cmd, 1);
    uart_rec_bytes(uart_num, rec, num);
}

/**********************Wk_BaudAdaptive*******************************************/
//函数功能:主串口波特率匹配
/*********************************************************************************/
void wk_baud_adaptive(void)
{
    uint8_t cmd = 0x55;
    /*初始化复位引脚为高电平*/
    uart_rst_set();
    uart_delay_ms(10);
    /*拉低RST引脚10ms，复位WK芯片*/
    uart_rst_reset(); /*拉低*/
    uart_delay_ms(10);
    /*拉高RST引脚，完成复位，wk进入正常工作状态*/
    uart_rst_set();
    uart_delay_ms(20);
    /*复位完成以后，发送0x55，实现主串口波特率匹配*/
    uart_send_bytes(WK_CHAN_A, &cmd, 1);
    uart_send_bytes(WK_CHAN_B, &cmd, 1);
    uart_delay_ms(100);
}

/******************************WkInit*******************************************/
//函数功能：初始化子串口F
/*********************************************************************************/
void wk_init(uint8_t uart_num, uint8_t port)
{
    uint8_t gena = 0, grst = 0, gier = 0, sier = 0, scr = 0;
    //使能子串口时钟
    gena = wk_read_greg(uart_num, WK2XXX_GENA);
    printf("gena = 0x%x.\n", gena);
    gena = gena | (1 << (port - 1));
    wk_write_greg(uart_num, WK2XXX_GENA, gena);
    //软件复位子串口
    grst = wk_read_greg(uart_num, WK2XXX_GRST);
    printf("grst = 0x%x.\n", grst);
    grst = grst | (1 << (port - 1));
    wk_write_greg(uart_num, WK2XXX_GRST, grst);
    //使能子串口总中断
    gier = wk_read_greg(uart_num, WK2XXX_GIER);
    printf("gier = 0x%x.\n", gier);
    gier = gier | (1 << (port - 1));
    wk_write_greg(uart_num, WK2XXX_GIER, gier);
    //使能子串口接收触点中断和超时中断
    sier = wk_read_sreg(uart_num, port, WK2XXX_SIER);
    printf("sier = 0x%x.\n", sier);
    sier |= WK2XXX_RFTRIG_IEN | WK2XXX_RXOUT_IEN;
    wk_write_sreg(uart_num, port, WK2XXX_SIER, sier);
    //初始化FIFO和设置固定中断触点
    wk_write_sreg(uart_num, port, WK2XXX_FCR, 0XFF);
    //设置任意中断触点，如果下面的设置有效，那么上面FCR寄存器中断的固定中断触点将失效
    wk_write_sreg(uart_num, port, WK2XXX_SPAGE, 1);   //切换到page1
    wk_write_sreg(uart_num, port, WK2XXX_RFTL, 0X40); //设置接收触点为64个字节
    wk_write_sreg(uart_num, port, WK2XXX_TFTL, 0X10); //设置发送触点为16个字节
    wk_write_sreg(uart_num, port, WK2XXX_SPAGE, 0);   //切换到page0
    //使能子串口的发送和接收使能
    scr = wk_read_sreg(uart_num, port, WK2XXX_SCR);
    printf("scr = 0x%x.\n", scr);
    scr |= WK2XXX_TXEN | WK2XXX_RXEN;
    wk_write_sreg(uart_num, port, WK2XXX_SCR, scr);
}

/******************************Wk_DeInit*******************************************/
//函数功能：初始化子串口
/*********************************************************************************/
void wk_deinit(uint8_t uart_num, uint8_t port)
{
    uint8_t gena, grst, gier;
    //关闭子串口总时钟
    gena = wk_read_greg(uart_num, WK2XXX_GENA);
    gena = gena & (~(1 << (port - 1)));
    wk_write_greg(uart_num, WK2XXX_GENA, gena);
    //使能子串口总中断
    gier = wk_read_greg(uart_num, WK2XXX_GIER);
    gier = gier & (~(1 << (port - 1)));
    wk_write_greg(uart_num, WK2XXX_GIER, gier);
    //软件复位子串口
    grst = wk_read_greg(uart_num, WK2XXX_GRST);
    grst = grst | (1 << (port - 1));
    wk_write_greg(uart_num, WK2XXX_GRST, grst);
}

/**************************Wk_SetBaud*******************************************************/
//函数功能：设置子串口波特率函数、此函数中波特率的匹配值是根据11.0592Mhz下的外部晶振计算的
// port:子串口号
// baud:波特率大小.波特率表示方式，
/**************************Wk2114SetBaud*******************************************************/
uint8_t wk_setbaud(uint8_t uart_num, uint8_t port, uint32_t baudrate)
{
    uint32_t temp, freq;
    uint8_t scr;
    uint8_t baud1, baud0, pres;
    freq = 24000000; /*芯片外部时钟频率*/
    if (freq >= (baudrate * 16))
    {
        temp = (freq) / (baudrate * 16);
        temp = temp - 1;
        baud1 = (uint8_t)((temp >> 8) & 0xff);
        baud0 = (uint8_t)(temp & 0xff);
        temp = (((freq % (baudrate * 16)) * 100) / (baudrate));
        pres = (temp + 100 / 2) / 100;
        printf("Wk_SetBaud---freq:%d, baudrate:%d\n", freq, baudrate);
        printf("Wk_SetBaud---baud1:%x, baud0:%x, pres:%x\n", baud1, baud0, pres);
        //关掉子串口收发使能
        scr = wk_read_sreg(uart_num, port, WK2XXX_SCR);
        wk_write_sreg(uart_num, port, WK2XXX_SCR, 0);
        //设置波特率相关寄存器
        wk_write_sreg(uart_num, port, WK2XXX_SPAGE, 1); //切换到page1
        wk_write_sreg(uart_num, port, WK2XXX_BAUD1, baud1);
        wk_write_sreg(uart_num, port, WK2XXX_BAUD0, baud0);
        wk_write_sreg(uart_num, port, WK2XXX_PRES, pres);
        wk_write_sreg(uart_num, port, WK2XXX_SPAGE, 0); //切换到page0
        //使能子串口收发使能
        wk_write_sreg(uart_num, port, WK2XXX_SCR, scr);
        return 0;
    }
    else
    {
        printf("Wk_SetBaud error！！！！\n");
        return 1;
    }
}

/**************************WK_RS485*******************************************************/
//函数功能:设置子串口RS485的收发转换函数，使用RTS引脚控制485电平转换芯片的收发
// port:子串口号
//
//注意：只有WK2168/WK2204支持该功能
/**************************WK_RS485*******************************************************/
void wk_rs485(uint8_t uart_num, uint8_t port)
{
    wk_write_sreg(uart_num, port, WK2XXX_RS485, 0x02); //
    wk_write_sreg(uart_num, port, WK2XXX_SPAGE, 1);    //切换到page1
    wk_write_sreg(uart_num, port, WK2XXX_RTSDLY, 0X10);
    wk_write_sreg(uart_num, port, WK2XXX_SPAGE, 0); //切换到page0
}

/**************************WK_TxLen*******************************************/
//函数功能:获取子串口发送FIFO剩余空间长度
// port:端口号
// 返回值：发送FIFO剩余空间长度
/**************************WK_Len********************************************/
int wk_txlen(uint8_t uart_num, uint8_t port)
{
    uint8_t fsr, tfcnt;
    int len = 0;
    fsr = wk_read_sreg(uart_num, port, WK2XXX_FSR);
    tfcnt = wk_read_sreg(uart_num, port, WK2XXX_TFCNT);
    if (fsr & WK2XXX_TFULL)
    {
        len = 0;
    }
    else
    {
        len = 256 - tfcnt;
    }
    return len;
}

/**************************WK_TxChars*******************************************/
//函数功能:通过子串口发送固定长度数据
// port:端口号
// len:单次发送长度不超过256
//
/**************************WK_TxChars********************************************/
void wk_txchars(uint8_t uart_num, uint8_t port, int len, uint8_t *sendbuf)
{

    int slen;
    while (len) //发送len长度的数据。
    {
        slen = len >= 16 ? 16 : len;
        len = len - slen;
        wk_write_sfifo(uart_num, port, sendbuf, slen); //通过fifo方式发送数据
        sendbuf = sendbuf + slen;
    }
}

/**************************WK_RxChars*******************************************/
//函数功能:读取子串口fifo中的数据
// port:端口号
// recbuf:接收到的数据
// 返回值：接收数据的长度
/**************************WK_RxChars********************************************/
int wk_rxchars(uint8_t uart_num, uint8_t port, uint8_t *recbuf)
{
    uint8_t fsr, rfcnt, rfcnt2, sifr;
    int offset = 0;
    int len = 0, num = 0;
    sifr = wk_read_sreg(uart_num, port, WK2XXX_SIFR);
    if ((sifr & WK2XXX_RFTRIG_INT) || (sifr & WK2XXX_RXOVT_INT)) //有接收中断和接收超时中断
    {
        fsr = wk_read_sreg(uart_num, port, WK2XXX_FSR);
        rfcnt = wk_read_sreg(uart_num, port, WK2XXX_RFCNT);
        rfcnt2 = wk_read_sreg(uart_num, port, WK2XXX_RFCNT);
        /*判断fifo中数据个数*/
        if (fsr & WK2XXX_RDAT) //判断是否为空，0为空，1不为空
        {
            if (!(rfcnt2 >= rfcnt))
            {
                rfcnt = rfcnt2;
            }
            len = (rfcnt == 0) ? 256 : rfcnt;
        }
        num = len;
        /*读取接收fifo中的数据*/
        while (len)
        {
            if (len >= 16)
            {
                wk_read_sfifo(uart_num, port, recbuf + offset, 16);
                offset = offset + 16;
                len = len - 16;
            }
            else
            {
                wk_read_sfifo(uart_num, port, recbuf + offset, len);
                len = 0;
            }
        }
    }
    else
    {
        num = 0;
    }
    return num;
}

static void IRAM_ATTR serial_ext_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(uart_ext_rx_queue, &gpio_num, NULL);
}

static void _gpio_init(void)
{
    gpio_config_t io_conf;

    // interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = SERIAL_EXT_A_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = SERIAL_EXT_B_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_set_intr_type(SERIAL_EXT_A_ISR_IO, GPIO_INTR_NEGEDGE);

    gpio_set_intr_type(SERIAL_EXT_B_ISR_IO, GPIO_INTR_NEGEDGE);

    // install gpio isr service
    gpio_install_isr_service(0);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(SERIAL_EXT_A_ISR_IO, serial_ext_isr_handler, (void *)SERIAL_EXT_A_ISR_IO);
    gpio_isr_handler_add(SERIAL_EXT_B_ISR_IO, serial_ext_isr_handler, (void *)SERIAL_EXT_B_ISR_IO);

    gpio_pad_select_gpio(SERIAL_EXT_RST_IO);
    gpio_pad_pullup(SERIAL_EXT_RST_IO);
    gpio_set_direction(SERIAL_EXT_RST_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(SERIAL_EXT_RST_IO, 1);
}

static void wk2xxx_init(void)
{
    uint8_t dat_a = 0, dat_b = 0;

    uart_config_t uart_config = {
        .baud_rate = 1500000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, SERIAL_EXT_A_TX_IO, SERIAL_EXT_A_RX_IO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, SERIAL_EXT_RX_BUF_SIZE * 2, SERIAL_EXT_TX_BUF_SIZE, 0, NULL, 0);

    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, SERIAL_EXT_B_TX_IO, SERIAL_EXT_B_RX_IO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, SERIAL_EXT_RX_BUF_SIZE * 2, SERIAL_EXT_TX_BUF_SIZE, 0, NULL, 0);


    wk_baud_adaptive();

    uart_delay_ms(100);
    /*读写GNEA，测试主接口通信是否成功*/
    dat_a = wk_read_greg(WK_CHAN_A, WK2XXX_GENA);
    printf("gena_a = 0x%x.\n", dat_a);

    dat_b = wk_read_greg(WK_CHAN_B, WK2XXX_GENA);
    printf("gena_b = 0x%x.\n", dat_b);

}

static uint8_t wk_chan_get(uint32_t io_num)
{
    if (SERIAL_EXT_A_ISR_IO == io_num){
        return WK_CHAN_A;
    }else if (SERIAL_EXT_B_ISR_IO == io_num){
        return WK_CHAN_B;
    }
    return 0;
}


int uart_ext_config(uart_ext_port_t uart_num, uint32_t baudrate, int rx_buffer_size, int tx_buffer_size)
{ 
    if(uart_num >= UART_EXT_NUM_MAX){
        return -1;
    }

    if(baudrate > UART_EXT_BAUDRATE_MAX){
        return -1;
    }

    wk_init(g_uart_ext_dev[uart_num].chan, g_uart_ext_dev[uart_num].port);
    wk_setbaud(g_uart_ext_dev[uart_num].chan, g_uart_ext_dev[uart_num].port, baudrate);

    if(g_uart_ext_dev[uart_num].rx_ringbuf){
        vRingbufferDelete(g_uart_ext_dev[uart_num].rx_ringbuf);
        g_uart_ext_dev[uart_num].rx_ringbuf = NULL;
    }

    if(g_uart_ext_dev[uart_num].tx_ringbuf){
        vRingbufferDelete(g_uart_ext_dev[uart_num].tx_ringbuf);
        g_uart_ext_dev[uart_num].rx_ringbuf = NULL;
    }

    g_uart_ext_dev[uart_num].rx_size = rx_buffer_size;
    g_uart_ext_dev[uart_num].tx_size = tx_buffer_size;

    g_uart_ext_dev[uart_num].rx_ringbuf = xRingbufferCreate(rx_buffer_size, RINGBUF_TYPE_BYTEBUF);
    g_uart_ext_dev[uart_num].tx_ringbuf = xRingbufferCreate(tx_buffer_size, RINGBUF_TYPE_BYTEBUF);

    return 0;
}


static uart_ext_port_t uart_ext_port_get(uint8_t chan, uint8_t port)
{
    uint8_t uart_num = 0;
    for(uart_num = 0; uart_num< UART_EXT_NUM_MAX; uart_num++){
        if(g_uart_ext_dev[uart_num].chan == chan && g_uart_ext_dev[uart_num].port == port){
            return uart_num;
        }
    }
    return UART_EXT_NUM_MAX;
}

static int uart_ext_ringbuf_send(uart_ext_port_t uart_num, const uint8_t* src, uint32_t size, TickType_t ticks_to_wait)
{
    BaseType_t ret;

    if(uart_num >= UART_EXT_NUM_MAX){
        return -1;
    }

    if(!src || size == 0){
        return -1;
    }

    if(!g_uart_ext_dev[uart_num].rx_ringbuf){
        return -1;
    }

    ret = xRingbufferSend(g_uart_ext_dev[uart_num].rx_ringbuf, src, size, ticks_to_wait);
    if(ret != pdTRUE){
        return -1;
    }

    return 0;
}

int uart_ext_fifo_clear(uart_ext_port_t uart_num)
{
    if(uart_num >= UART_EXT_NUM_MAX){
        return -1;
    }

    if(!g_uart_ext_dev[uart_num].rx_ringbuf){
        return -1;
    }

    vRingbufferDelete(g_uart_ext_dev[uart_num].tx_ringbuf);
    vRingbufferDelete(g_uart_ext_dev[uart_num].rx_ringbuf);

    g_uart_ext_dev[uart_num].rx_ringbuf = xRingbufferCreate(g_uart_ext_dev[uart_num].rx_size, RINGBUF_TYPE_BYTEBUF);
    g_uart_ext_dev[uart_num].tx_ringbuf = xRingbufferCreate(g_uart_ext_dev[uart_num].tx_size, RINGBUF_TYPE_BYTEBUF);

    return 0;
}

int uart_ext_write_bytes(uart_ext_port_t uart_num, const uint8_t* buf, uint32_t length)
{
    BaseType_t ret;

    if(uart_num >= UART_EXT_NUM_MAX){
        return -1;
    }

    if(!buf || length == 0){
        return -1;
    }

    if(!g_uart_ext_dev[uart_num].tx_ringbuf){
        return -1;
    }

    ret = xRingbufferSend(g_uart_ext_dev[uart_num].tx_ringbuf, buf, length, (portTickType)portMAX_DELAY);
    if(ret != pdTRUE){
        return -1;
    }

    xQueueSend(uart_ext_tx_queue, &uart_num, 0);
 
    return length;
}

int uart_ext_read_bytes(uart_ext_port_t uart_num, uint8_t* buf, uint32_t length, uint32_t timeout)
{
    uint32_t recv = 0;
    uint8_t *pdata = NULL;

    if(uart_num >= UART_EXT_NUM_MAX){
        return -1;
    }

    if(!buf || length == 0){
        return -1;
    }

    if(!g_uart_ext_dev[uart_num].rx_ringbuf){
        return -1;
    }

    pdata = xRingbufferReceiveUpTo(g_uart_ext_dev[uart_num].rx_ringbuf, &recv, timeout/portTICK_RATE_MS, length);
    if(!pdata){
        return 0;
    }
    
    memcpy(buf, pdata, recv);

    vRingbufferReturnItem(g_uart_ext_dev[uart_num].rx_ringbuf, (void *)pdata);

    return recv;
}

void uart_ext_tx_thread(void *arg)
{
    uint32_t uart_num = 0;
    uint32_t recv = 0;
    uint8_t *pdata = NULL;

    while(1){

        if (!xQueueReceive(uart_ext_tx_queue, &uart_num, portMAX_DELAY)){
            continue;
        }

        if(uart_num >= UART_EXT_NUM_MAX){
            continue;
        }

        while(1){

            pdata = xRingbufferReceiveUpTo(g_uart_ext_dev[uart_num].tx_ringbuf, &recv, 0, 256);
            if(!pdata || recv == 0){
                break;
            }
            wk_txchars(g_uart_ext_dev[uart_num].chan, g_uart_ext_dev[uart_num].port, recv, pdata);

            vRingbufferReturnItem(g_uart_ext_dev[uart_num].tx_ringbuf, (void *)pdata);
        }

    }
       
}


static void uart_ext_rx_thread(void *arg)
{
    uint8_t chan = 0;
    uint8_t gifr = 0;
    uint32_t rxlen_1 = 0, rxlen_2 = 0, rxlen_3 = 0, rxlen_4 = 0;
    uart_ext_port_t uart_num;

    uint8_t recvbuf[256] = {0};
    uint32_t io_num = 0;

    #if 0

    wk_init(WK_CHAN_A, 1);
    wk_init(WK_CHAN_A, 2);
    wk_init(WK_CHAN_A, 3);
    wk_init(WK_CHAN_A, 4);

    wk_init(WK_CHAN_B, 1);
    wk_init(WK_CHAN_B, 2);
    wk_init(WK_CHAN_B, 3);
    wk_init(WK_CHAN_B, 4);

    wk_setbaud(WK_CHAN_A, 1, 115200);
    wk_setbaud(WK_CHAN_A, 2, 115200);
    wk_setbaud(WK_CHAN_A, 3, 115200);
    wk_setbaud(WK_CHAN_A, 4, 115200);

    wk_setbaud(WK_CHAN_B, 1, 115200);
    wk_setbaud(WK_CHAN_B, 2, 115200);
    wk_setbaud(WK_CHAN_B, 3, 115200);
    wk_setbaud(WK_CHAN_B, 4, 115200);
    #endif

    while (1)
    {

        if (!xQueueReceive(uart_ext_rx_queue, &io_num, portMAX_DELAY)){
            continue;
        }
       
        gpio_intr_disable(io_num);

        chan = wk_chan_get(io_num);
        gifr = wk_read_greg(chan, WK2XXX_GIFR);

        do{
            if (gifr & WK2XXX_UT1INT) {                        
                rxlen_1 = wk_rxchars(chan, 1, recvbuf); 
                uart_num = uart_ext_port_get(chan, 1); 
                uart_ext_ringbuf_send(uart_num, recvbuf, rxlen_1, 0);
            }
            if (gifr & WK2XXX_UT2INT) {
                rxlen_2 = wk_rxchars(chan, 2, recvbuf); 
                uart_num = uart_ext_port_get(chan, 2); 
                uart_ext_ringbuf_send(uart_num, recvbuf, rxlen_2, 0); 
            }
            if (gifr & WK2XXX_UT3INT) {    
                rxlen_3 = wk_rxchars(chan, 3, recvbuf); 
                uart_num = uart_ext_port_get(chan, 3); 
                uart_ext_ringbuf_send(uart_num, recvbuf, rxlen_3, 0);
            }
            if (gifr & WK2XXX_UT4INT) {  
                rxlen_4 = wk_rxchars(chan, 4, recvbuf); 
                uart_num = uart_ext_port_get(chan, 4); 
                uart_ext_ringbuf_send(uart_num, recvbuf, rxlen_4, 0);
            }
            gifr = wk_read_greg(chan, WK2XXX_GIFR);
           
        } while (gifr & 0x0f);

        gpio_intr_enable(io_num);
        
    }
}


void uart_ext_test_thread(void *arg)
{
    uint32_t recv = 0;
    uint8_t recvbuf[512] = {0};
    uart_delay_ms(1000);

    uart_ext_config(UART_EXT_NUM_0, 115200, 1024, 1024);

    while(1)
    {

        recv = uart_ext_read_bytes(UART_EXT_NUM_0, recvbuf, sizeof(recvbuf), 1000);
        if(recv){
            printf("recv: %d, %.*s\n", recv, recv, recvbuf);
            uart_ext_write_bytes(UART_EXT_NUM_0, recvbuf, recv);
        }

    }

}

void uart_ext_init(void)
{

    uart_ext_tx_queue = xQueueCreate(UART_EXT_TX_QUEUE_SIZE, sizeof(uint32_t));
    uart_ext_rx_queue = xQueueCreate(UART_EXT_RX_QUEUE_SIZE, sizeof(uint32_t));

    _gpio_init();
    wk2xxx_init();

    xTaskCreate(uart_ext_rx_thread, "uart ext rx", 2048, (void *)0, 20, NULL);
    xTaskCreate(uart_ext_tx_thread, "uart ext tx", 2048, (void *)0, 19, NULL);

    //xTaskCreate(uart_ext_test_thread, "uart ext test", 2048, (void *)0, 10, NULL);
}
 

