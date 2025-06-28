#include <malloc.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "qm.h"
#include "qm_kernel.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "qm_at.h"
#include "qm_uart.h"
#include "qm_tlv.h"
#include "qm_kv.h"
#include "qm_gpio.h"
#include "qm_wifi.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_ble_gap.h"

#include "serial_common.h"
#include "serial_protocol.h"
#include "serial_platform.h"

#include "audio_prodtst.h"
#include "audio_main.h"

static qm_at_result_t at_set_fac_mode(void *channel, const char *args);
static qm_at_result_t at_get_mem_free(void *channel);
static qm_at_result_t at_get_fw_name(void *channel);
static qm_at_result_t at_get_ble_mac(void *channel);
static qm_at_result_t at_set_io_wr(void *channel, const char *args);
static qm_at_result_t at_set_voice_scan(void *channel, const char *args);
static qm_at_result_t at_set_voice(void *channel, const char *args);
static qm_at_result_t at_set_rf_w(void *channel, const char *args);

typedef struct prodtst_wifi_scan {
    qm_task_t task;
    qm_timer_t timer;
    int8_t result;
    char name[CONFIG_QM_WIFI_SSID_MAX_LEN + 1];
} prodtst_wifi_scan_t;

typedef struct prodtst_voice {
    int id;
    int count;
    int interval;
    int wait;
} prodtst_voice_t;

typedef struct prodtst_voice_scan {
    qm_timer_t timer;
    int channel;
    int wait_time;
    int8_t result;
} prodtst_voice_scan_t;


static uint8_t prodtst_hub_map[] = {10, 9};
static qm_at_cmd_t at_fac_cmd[] = {
    {"AT+FAC", NULL, NULL, NULL, at_set_fac_mode, NULL},
    {"AT+MEM", NULL, NULL, at_get_mem_free, NULL, NULL},
};
static qm_at_cmd_t at_cmd[] = {   
    {"AT+FAC", NULL, NULL, NULL, at_set_fac_mode, NULL},
	{"AT+FW_NAME", NULL, NULL, at_get_fw_name, NULL, NULL},
	{"AT+BLE_MAC", NULL, NULL, at_get_ble_mac, NULL, NULL},
	{"AT+IO_WR", NULL, NULL, NULL, at_set_io_wr, NULL},
	{"AT+VOICE_SCAN", NULL, NULL, NULL, at_set_voice_scan, NULL},
	{"AT+VOICE", NULL, NULL, NULL, at_set_voice, NULL},
	{"AT+RF_W", NULL, NULL, NULL, at_set_rf_w, NULL},
};
static qm_uart_dev_t at_uart = {
    .config = {
        .baud_rate = 115200,
        .data_bits = QM_UART_DATA_8_BITS,
        .parity    = QM_UART_PARITY_NONE,
        .pins.tx_pin = 43,
        .pins.rx_pin = 44,
        .stop_bits = QM_UART_STOP_BITS_1 ,
    },
    .port = 0,
};

static prodtst_wifi_scan_t g_ws;
static prodtst_voice_scan_t g_vcs;
static int g_prodtst_config_type = 0;
static void *g_channel = NULL;

static int prodtst_config_set(int type)
{
    int flag = type;
    qm_kv_set(FLASH_SECTOR_KEY, &flag, 4, 1);
    qm_printf("prodtst_config_set = %d\r\n", flag);
    g_prodtst_config_type = flag;
    return 0;
}

int prodtst_config_get(void)
{
    int type = 0;
    int size = 4;
    qm_kv_get(FLASH_SECTOR_KEY, &type, &size);
    return type;
}

static qm_at_result_t at_get_mem_free(void *channel) 
{
	qm_at_server_printfln(channel, "+MEM:%ld", qm_free_mem_get());
	return QM_AT_RESULT_OK;
}

static qm_at_result_t at_set_fac_mode(void *channel, const char *args)
{
    int value = 0;

    if(channel == NULL || args == NULL){
        return QM_AT_RESULT_FAILE;
    }

    args = args + 1;
    sscanf(args, "%d", &value);
    if(value != 0 && value != 1) {
        return QM_AT_RESULT_FAILE;
    }
    prodtst_config_set(value);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    qm_reboot();
	return QM_AT_RESULT_OK;
}

static qm_at_result_t at_get_fw_name(void *channel)
{
	qm_at_server_printfln(channel, "+FW_NAME:\"%s\"", CONFIG_AUDIO_LIGHT_HW_VERSION);
	return QM_AT_RESULT_OK;
}

static qm_at_result_t at_get_ble_mac(void *channel)
{
    uint8_t mac[6] = {0};
    char buff[20] = {0};
    qm_ble_gap_get_local_addr(mac);
    qm_snprintf(buff, 20, "%02x:%02x:%02x:%02x:%02x:%02x", 
            mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);    
    qm_at_server_printfln(channel, "+BLE_MAC:\"%s\"", buff);
	return QM_AT_RESULT_OK;
}

static qm_at_result_t at_set_io_wr(void *channel, const char *args)
{
    char *temp_buff;
    int io;
    int value;
    qm_gpio_dev_t gpio_dev;
    
    if(channel == NULL || args == NULL){
        return QM_AT_RESULT_FAILE;
    }
    
    args = args + 1;
    temp_buff = strtok((char *)args, ",");
    gpio_dev.config.mode = QM_GPIO_MODE_OUTPUT;
    gpio_dev.config.pull_en = QM_GPIO_PULLDOWN_ONLY;

    while (temp_buff) {
        if(sscanf(temp_buff, "%d", &io) != 1) {
            return QM_AT_RESULT_FAILE;
        }
        temp_buff = strtok(NULL, ",");
        if(sscanf(temp_buff, "%d", &value) != 1) {
            return QM_AT_RESULT_FAILE;
        }
        temp_buff = strtok(NULL, ",");
        if(io != 1 && io != 2) {
            return QM_AT_RESULT_FAILE;
        }
        gpio_dev.port = prodtst_hub_map[(io-1)];
        qm_gpio_deinit(&gpio_dev);
        qm_gpio_init(&gpio_dev);
        qm_gpio_set_level(&gpio_dev, value);
    }
	return QM_AT_RESULT_OK;
}

qm_err_t prodtst_rec_engine_cb(audio_rec_evt_t *event, void *user_data) 
{
    switch (event->type) {
    case AUDIO_REC_WAKEUP_START:
        if(g_vcs.result != 1) {
            break;
        }
        qm_timer_stop(&g_vcs.timer);
        qm_timer_free(&g_vcs.timer);
        qm_at_server_printfln(g_channel, "+VOICE_SCAN:%d,3", g_vcs.channel);
        qm_at_server_printfln(g_channel, "OK");
        g_vcs.result = 0;
        break;
    default:
        break;
    }
    return ESP_OK;
}

void voice_scan_result_cb( qm_timer_t *timer, void *arg)
{
    g_vcs.result = 2;
    qm_at_server_printfln(g_channel, "ERROR");
}

static qm_at_result_t at_set_voice_scan(void *channel, const char *args)
{
    if(channel == NULL || args == NULL){
        return QM_AT_RESULT_FAILE;
    }
    g_channel = channel;

    args = args + 1;
    if(sscanf(args, "%d,%d", &g_vcs.channel, &g_vcs.wait_time) != 2) {
        return QM_AT_RESULT_CHECK_FAILE;
    }
    if(g_vcs.channel < 0 || g_vcs.wait_time <= 0) {
        return QM_AT_RESULT_PARSE_FAILE;
    }
    if(g_vcs.result == 1) {
        qm_timer_stop(&g_vcs.timer);
        qm_timer_free(&g_vcs.timer);
    } else {
        g_vcs.result = 1;
    }
    qm_timer_new(&g_vcs.timer, voice_scan_result_cb, NULL, g_vcs.wait_time, 0);
    qm_timer_start(&g_vcs.timer);

    return QM_AT_RESULT_NULL;
}

extern void qm_bsp_offline_voice_play(uint8_t index);
extern uint8_t qm_bsp_voice_is_idle(void);
static qm_at_result_t at_set_voice(void *channel, const char *args)
{
    int voice_channel;
    prodtst_voice_t voice;
    
    if(channel == NULL || args == NULL){
        return QM_AT_RESULT_FAILE;
    }
    
    args = args + 1;
    if(sscanf(args, "%d,%d,%d,%d,%d", &voice_channel, 
              &voice.id, &voice.count, &voice.interval, &voice.wait) != 5) {
        return QM_AT_RESULT_CHECK_FAILE;
    }
    if(voice.wait < 0 || voice.interval < 0 || voice.count < 0 || voice.id < 0) {
        return QM_AT_RESULT_PARSE_FAILE;
    }
    
    if(voice.wait) {
        vTaskDelay(voice.wait / portTICK_PERIOD_MS);
    }
    for(uint8_t i = 0; i < voice.count; i++) {
        while(qm_bsp_voice_is_idle() != 1) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        if(voice.interval) {
            vTaskDelay(voice.interval / portTICK_PERIOD_MS);
        }
        qm_bsp_offline_voice_play((uint8_t)voice.id);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
	return QM_AT_RESULT_OK;
}

static void qm_event_handler(qm_input_event_t *input_event, void *arg)
{
    qm_wifi_event_info_t *event_info = (qm_wifi_event_info_t*)input_event->value;
    switch(input_event->sub_event){
        case QM_WIFI_EVENT_SCAN_DONE: {
            QM_LOGD(LOG_TAG, "wifi scan done, status: %d, ap num: %d", event_info->scan_done.status, event_info->scan_done.ap_num);
            qm_timer_stop(&g_ws.timer);
            qm_timer_free(&g_ws.timer);
            if(event_info->scan_done.status != 0 || g_ws.result == 2) {
                break;
            }
            g_ws.result = 0;
            for(uint8_t i = 0; i < event_info->scan_done.ap_num; i++) {
                if(strcmp((char *)event_info->scan_done.ap_record[i].ssid, g_ws.name) == 0) {
                    qm_at_server_printfln(g_channel, "+RF_W:%d", event_info->scan_done.ap_record[i].rssi);
                    qm_at_server_printfln(g_channel, "OK");
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

void wifi_scan_result_cb( qm_timer_t *timer, void *arg)
{
    g_ws.result = 2;
    qm_wifi_scan_stop();
    qm_at_server_printfln(g_channel, "ERROR");
}

static void wifi_scan_task(void *param)
{
    qm_wifi_scan_start(NULL);
    qm_timer_new(&g_ws.timer, wifi_scan_result_cb, NULL, WIFI_SCAN_TIMEOUT_MS, 0);
    qm_timer_start(&g_ws.timer);
    vTaskDelete(NULL);
}

static qm_at_result_t at_set_rf_w(void *channel, const char *args)
{
    char *temp_buff;
    int len = 0;
    if(channel == NULL || args == NULL){
        return QM_AT_RESULT_FAILE;
    }
    if(g_ws.result == 1) {
        qm_timer_stop(&g_ws.timer); 
        qm_wifi_scan_stop();
        qm_timer_free(&g_ws.timer);
    } else {
        g_ws.result = 1;
    }
    g_channel = channel;
    args = args + 1;
    temp_buff = strtok((char *)args, "\"");
    len = strlen(temp_buff);
    if(len == 0) {
        return QM_AT_RESULT_CHECK_FAILE;
    }
    memset(g_ws.name, 0, (len + 1));
    memcpy(g_ws.name, temp_buff, len);
    qm_task_new(&g_ws.task, "wscan_task", wifi_scan_task, NULL, 4 * 1024, 0);
	return QM_AT_RESULT_NULL;
}


static int audio_at_read(void *handle, char *buf, int size, int timeout)
{
    return qm_uart_read(&at_uart, buf, (uint32_t)size, (uint32_t)timeout);
}

static int audio_at_write(void *handle, char *buf, int size)
{
    return qm_uart_write(&at_uart, buf, (uint32_t)size, 0);
}

int audio_prodtst_init(void)
{
    qm_at_server_t *at_handle = NULL;
    qm_at_server_param_t param = {0};

    qm_uart_init(&at_uart);    

    param.recv = audio_at_read;
    param.send = audio_at_write;
    param.channel_type = 0;
    param.recv_buf_len = 128;

    at_handle = qm_at_server_init(&param);

    if(prodtst_config_get() == 1) {
        prodtst_config_set(0);
        qm_at_register_commands(at_handle, at_cmd, QM_ARRAY_SIZE(at_cmd));
        qm_event_register(QM_EVENT_WIFI, qm_event_handler, NULL);
        qm_wifi_init();
        qm_wifi_set_mode(QM_WIFI_MODE_STA);
        qm_wifi_start();
        return 1;
    } else {
        qm_at_register_commands(at_handle, at_fac_cmd, QM_ARRAY_SIZE(at_fac_cmd));
        return 0;
    }
}