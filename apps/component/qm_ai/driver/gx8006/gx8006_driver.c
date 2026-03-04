#include "gx8006_driver.h"
#include "nc_audio.h"
#include "qm_uart.h"
#include "qm_gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define SERIAL_RECV_LEN  (512)

struct nc_player_handle_t {
    nc_audio_spk_play_header_t spk_play;
    uint8_t recv_buff[SERIAL_RECV_LEN];
};

static struct nc_player_handle_t nc_player_handle= {0};

static gx8006_info_t gx8006_info = {
    .inited      = 0,
    .mic_send_en = 0,
    .mic_read_cb = NULL,
    .event_cb    = NULL,
    .user_data   = NULL,
};

static const char *TAG = "gx8006";

qm_task_t  uart_read_task;
static QueueHandle_t uart_queue;

static qm_uart_dev_t uart_dev = {
    .config = {
        .baud_rate = 115200,
        .data_bits = QM_UART_DATA_8_BITS,
        .parity    = QM_UART_PARITY_NONE,
        .pins.tx_pin = 40,
        .pins.rx_pin = 39,
        .stop_bits = QM_UART_STOP_BITS_1 ,
        .flow_ctrl = QM_UART_FLOW_CTRL_NONE,
    },
    .port = 1,
};

static qm_gpio_dev_t nc_int_io = {
    .port = 38,
    .config = {
        .mode = QM_GPIO_MODE_INPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    },
};

static int nc_audio_event_cb(nc_audio_event_type_t event_type, nc_audio_event_param_t *p_event)
{
    nc_audio_mic_data_t *mic_data = &p_event->mic_data;

    switch (event_type)
    {
        case NC_AUDIO_EVENT_TYPE_MIC_DATA_GET:
            if (gx8006_info.mic_read_cb && gx8006_info.mic_send_en) {
                gx8006_info.mic_read_cb(QM_AI_DRIVER_TYPE_AUDIO, mic_data->payload, mic_data->payload_len);
            }
        break;

        case NC_AUDIO_EVENT_TYPE_WAKEUP:
            qm_ai_driver_event_t event = {0};
            event.wakeup.handle = gx8006_info.user_data;

            if (gx8006_info.event_cb) {
                gx8006_info.event_cb(QM_AI_DRIVER_TYPE_AUDIO, QM_AI_DRIVER_EVENT_WAKEUP, &event);
            }

            gx8006_info.mic_send_en = 1;
        break;

        case NC_AUDIO_EVENT_TYPE_SPK_REPLY:
            if (p_event->spk_state.status == NC_AUDIO_SPK_REPLY_TYPE_START) {
                gx8006_info.mic_send_en = 0;
            } else {
                gx8006_info.mic_send_en = 1;
            }
        break;

        default:
        break;
    }
    
    return QM_EOK;
}

int serial_send(uint8_t *data, int len)
{
    if(data == NULL || len == 0){
        return -QM_EINVAL;
    }

    int32_t ret = 0;

    ret = qm_uart_write(&uart_dev, data, len, 0);
    uart_wait_tx_done(uart_dev.port, QM_WAIT_FOREVER);

    return ret;
}

void uart_read(void *pvParameters) 
{
    while (1) {
        int len = uart_read_bytes(uart_dev.port, nc_player_handle.recv_buff, SERIAL_RECV_LEN, pdMS_TO_TICKS(20));

        if (len > 0) {
            nc_audio_recv_push(nc_player_handle.recv_buff, len);
        }
    }
}

static int gx8006_open(void) 
{
    nc_audio_param_t param = {
        .rate = 16000,
        .bits = 16,
        .channel = 1,
        .spk_gain = 10,
        .mic_gain = 26,
        .vad_timeout = 10,
        .fmt_type = 2,
        .send = serial_send,
        .notify = nc_audio_event_cb,
    };

    qm_uart_init(&uart_dev);
    qm_task_new_to_core(&uart_read_task, "uart read", uart_read, (void *)NULL, 2*1024, 5, 0);
    qm_gpio_init(&nc_int_io);

    nc_audio_init(&param);

    nc_audio_cfg(NC_AUDIO_CFG_TYPE_SLIENT_TIMEOUT, 0xFF);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_MAX_RECORD_TIME, 0xFF);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_VAD_ONOFF, 0);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_WAKEUP_ONOFF, QM_FALSE);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_TIMEOUT_ONTIFY_ONOFF, QM_TRUE);

    nc_audio_cfg(NC_AUDIO_CFG_TYPE_MIC_ONOFF, 1);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_WAKEUP_ONOFF, 1);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_WAKEUP_ONTIFY_ONOFF, 1);

    nc_player_handle.spk_play.msg_id = 0;
    nc_player_handle.spk_play.type = NC_AUDIO_SPK_PLAY_TYPE_START;
    nc_play_spk_direct_play(&nc_player_handle.spk_play, NULL, 0);

    gx8006_info.inited = 1;

    return QM_EOK;
}

static int gx8006_close(void) 
{
    gx8006_info.inited = 0;
    return QM_EOK;
}

static int gx8006_write(void *data, int len) 
{
    int res = QM_EOK;

    if ((gx8006_info.inited == 0) || (data == NULL) || (len <= 0)) {
        return -QM_EINVAL;
    }

    while(qm_gpio_get_level(&nc_int_io) == 0) {
        qm_msleep(10);
    }

    nc_player_handle.spk_play.msg_id++;
    nc_player_handle.spk_play.type = NC_AUDIO_SPK_PLAY_TYPE_DATA;
    res = nc_play_spk_direct_play(&nc_player_handle.spk_play, data, len);
    if (res != QM_EOK) {
        QM_LOGD(TAG, "audio data push fail");
    }

    return res;
}

static int gx8006_ctrl(qm_ai_driver_ctrl_t cmd, void *param, void *user_data)
{
    if (gx8006_info.inited == 0) {
        return -QM_EINVAL;
    }

    gx8006_info.user_data = user_data;

    switch (cmd) {
        case QM_AI_DRIVER_CTRL_NONE:
            break;
        case QM_AI_DRIVER_CTRL_READ_CALLBACK:
            gx8006_info.mic_read_cb = (qm_ai_driver_read_handler_t)param;
            break;
        case QM_AI_DRIVER_CTRL_EVENT_CALLBACK:
            gx8006_info.event_cb = (qm_ai_driver_event_handler_t)param;
            break;
        case QM_AI_DRIVER_CTRL_SET_MUTE:
            // nc_audio_set_mute(*(bool *)param);
            break;
        case QM_AI_DRIVER_CTRL_GET_MUTE:
            break;
        case QM_AI_DRIVER_CTRL_SET_VOLUME:
            // nc_audio_set_volume(*(uint32_t *)param);
            break;
        case QM_AI_DRIVER_CTRL_GET_VOLUME:
            break;
        case QM_AI_DRIVER_CTRL_ENABLE_PA:
            break;
        default:
            break;
    }

    return QM_EOK;
}

qm_ai_driver_param_t QM_AI_AUDIO_DRIVER_GX8006_HANDLE = {
    .open  = gx8006_open,
    .close = gx8006_close,
    .write = gx8006_write,
    .read  = NULL,
    .ctrl  = gx8006_ctrl,
};
