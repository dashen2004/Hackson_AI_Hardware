#include "qm.h"
#include "qm_work.h"
#include "qm_kv.h"

#include "bsp_audio.h"

#include "audio_main.h"

#include "qm_utils_timer.h"

#include "qm_iot_config.h"

#include "qm_gpio.h"
#include "qm_uart.h"
#include "qm_audio.h"
#include "nc_player.h"
#include "nc_audio.h"
#include "qm_agora.h"
#include "xjiang_cloud.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG "nc_player"

#define VOLUME_SET  ("volume")

#define SERIAL_RECV_LEN  (512)
#define OPUS_OFFSET (44)
#define PLAY_DATA_DIFF  (0)  //打印时间差

#if PLAY_DATA_DIFF
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
uint32_t last = 0;
#endif

struct nc_player_handle_t {
    int wakeup_enable;
    int mute;
    int volume;
    qm_task_t   recv_task;
    nc_audio_code_t spk_code;
    nc_audio_spk_play_header_t spk_play;
    nc_player_event_t event_cb;
    nc_audio_event_t notify;
    qm_work_t ind_time;
    void *user_data;
    void *opus_handle;
    uint8_t pcm_play_idle;
    uint8_t agent_play_idle;
    bsp_pcm_id_t play_pcm_id;
    int file_offset;
    qm_agent_state_t state;
    bsp_pcm_file_t *pcm_file;
    qm_mutex_t lock;
    uint8_t recv_buff[SERIAL_RECV_LEN];
    char version[CONFIG_NC_AUDIO_RECV_VER_MAX_LEN + 1];
};

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
    .queue = &uart_queue,
    .qlen = 20,
};

static qm_gpio_dev_t nc_int_io = {
    .port = 38,
    .config = {
        .mode = QM_GPIO_MODE_INPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    },
};

static int qm_play_get(uint8_t **data, int *len);

static qm_play_data_t play_data = {
    .play_get = qm_play_get
};

static struct nc_player_handle_t nc_player_handle= {0};

int seial_send(uint8_t *data, int len)
{
    int32_t ret = 0;

    if(data == NULL || len == 0){
        return -QM_EINVAL;
    }

    // QM_HEX_LOGD("1", "send ", data, len);
    // QM_LOGI("1", "send data len:%d", len);
    ret = qm_uart_write(&uart_dev, data, len, 0);
    uart_wait_tx_done(uart_dev.port, portMAX_DELAY);

    return ret;
}

static void audio_play_start(void)
{
    if(nc_player_handle.spk_play.type == NC_AUDIO_SPK_PLAY_TYPE_START){
        return ;
    }
    QM_LOGD("1", "audio_play_start");
    nc_player_handle.spk_play.msg_id = 0;
    nc_player_handle.spk_play.type = NC_AUDIO_SPK_PLAY_TYPE_START;

    nc_play_spk_direct_play(&nc_player_handle.spk_play, NULL, 0);
    qm_msleep(CONFIG_AUDIO_FMT_DURATION_MS);
}

static void audio_play_data(uint8_t *payload, int payload_len)
{
    if(nc_player_handle.spk_play.type == NC_AUDIO_SPK_PLAY_TYPE_STOP){
        return ;
    }
    nc_player_handle.spk_play.msg_id++;
    nc_player_handle.spk_play.type = NC_AUDIO_SPK_PLAY_TYPE_DATA;
    nc_play_spk_direct_play(&nc_player_handle.spk_play, payload, payload_len);
}

static void audio_play_stop(void)
{
    if(nc_player_handle.spk_play.type == NC_AUDIO_SPK_PLAY_TYPE_STOP){
        return ;
    }
    QM_LOGD("1", "audio_play_stop");
    nc_player_handle.spk_play.msg_id = 0;
    nc_player_handle.spk_play.type = NC_AUDIO_SPK_PLAY_TYPE_STOP;

    nc_play_spk_direct_play(&nc_player_handle.spk_play, NULL, 0);
}

static int qm_play_get(uint8_t **data, int *len)
{
    qm_mutex_lock(&nc_player_handle.lock, QM_WAIT_FOREVER);
    if(nc_player_handle.file_offset >= nc_player_handle.pcm_file->file_len){
        *data = NULL;
        *len = 0;
    } else {
        *data = &nc_player_handle.pcm_file->file[nc_player_handle.file_offset];
        *len = 80;
    }
    nc_player_handle.file_offset += 80;
    qm_mutex_unlock(&nc_player_handle.lock);
    return QM_EOK;
}

static void agent_set_ind_timeout(void *arg)
{
    qm_mutex_lock(&nc_player_handle.lock, QM_WAIT_FOREVER);
    if(qm_audio_state_get() == QM_AUDIO_STATE_SPEAK && nc_player_handle.pcm_play_idle == 0) {
        qm_audio_stop();
    }
    nc_player_handle.agent_play_idle = 0;
    qm_mutex_unlock(&nc_player_handle.lock);
}

void qm_agora_set_agent_state(qm_agent_state_t state, const char *text)
{
    if(AGENT_STATE_MAX <= state && state <= AGENT_STATE_NONE) {
        return;
    }
    nc_player_handle.state = state;
    qm_mutex_lock(&nc_player_handle.lock, QM_WAIT_FOREVER);
    if(nc_player_handle.pcm_play_idle == 1 || nc_player_handle.play_pcm_id == BSP_PCM_ID_SLEEP) {
        qm_mutex_unlock(&nc_player_handle.lock);
        return;
    }
    qm_mutex_unlock(&nc_player_handle.lock);
    qm_cancel_delayed_action(&nc_player_handle.ind_time);
    switch (state)
    {
        case AGENT_STATE_SILENT:
            qm_post_delayed_action(&nc_player_handle.ind_time, agent_set_ind_timeout, NULL, 2000);
            break;
        case AGENT_STATE_SPEAKING:
        case AGENT_STATE_THINKING:
            qm_mutex_lock(&nc_player_handle.lock, QM_WAIT_FOREVER);
            nc_player_handle.agent_play_idle = 1;
            if(qm_audio_state_get() == QM_AUDIO_STATE_SLIENT && nc_player_handle.pcm_play_idle == 0) {
                audio_play_start();
                qm_audio_play(QM_AUDIO_TYPE_AGENT);
            }
            qm_mutex_unlock(&nc_player_handle.lock);
            break;
        default:
            break;
    }
}

void nc_audio_play_bin(bsp_pcm_id_t pcm_id)
{
    QM_LOGD(TAG, "%s:%d", __func__, pcm_id);

    qm_mutex_lock(&nc_player_handle.lock, QM_WAIT_FOREVER);
    if(nc_player_handle.play_pcm_id == BSP_PCM_ID_SLEEP) {
        qm_mutex_unlock(&nc_player_handle.lock);
        return;
    } else if(pcm_id == BSP_PCM_ID_POWER_ON) {
        audio_play_start();
    }
    nc_player_handle.play_pcm_id = pcm_id;
    nc_player_handle.pcm_play_idle = 1;
    nc_player_handle.file_offset = OPUS_OFFSET;
    nc_player_handle.pcm_file = &bsp_pcm_file[nc_player_handle.play_pcm_id];
    qm_mutex_unlock(&nc_player_handle.lock);

    if(qm_audio_state_get() != QM_AUDIO_STATE_LOCAL_SPEAK) {
        qm_audio_play_with_data(QM_AUDIO_TYPE_LOCAL, &play_data);
    }
}

uint8_t nc_audio_play_is_idle(void)
{
    if(nc_player_handle.pcm_play_idle == 0 && nc_player_handle.agent_play_idle == 0) {
        return 1;
    }
    return 0;
}

char *nc_audio_get_version(void)
{
    if(nc_player_handle.version[0] == '\0'){
        return NULL;
    }

    return &nc_player_handle.version[0];
}

qm_err_t nc_audio_set_volume(int volume)
{
    nc_player_handle.volume = volume;
    nc_audio_set_spk_volume((uint8_t)volume);
    qm_kv_set(VOLUME_SET, &nc_player_handle.volume, sizeof(int), 1);
    return QM_EOK;
}

int nc_audio_get_volume(void)
{
    int len = sizeof(int);
    if(nc_player_handle.volume == -1) {
        if(qm_kv_get(VOLUME_SET, &nc_player_handle.volume, &len) != QM_EOK){
            nc_player_handle.volume = CONFIG_AUDIO_FMT_SPK_GAIN;
        }
    }
    return nc_player_handle.volume;
}

int32_t nc_audio_set_spk_on_off(bool onoff)
{
    if(onoff) {
        return nc_audio_set_spk_volume(nc_player_handle.volume);
    } else {
        return nc_audio_set_spk_volume(0);
    }
}

int32_t nc_audio_set_mic_on_off(bool onoff) 
{
    return nc_audio_cfg(NC_AUDIO_CFG_TYPE_MIC_ONOFF, onoff);
}

int32_t nc_audio_set_wakeup_on_off(bool onoff) 
{
    return nc_audio_cfg(NC_AUDIO_CFG_TYPE_WAKEUP_ONOFF, onoff);
}

int32_t nc_audio_set_wakeup_state_on_off(bool onoff) 
{
    if(onoff) {
        return nc_audio_cfg(NC_AUDIO_CFG_TYPE_WAKEUP_ONTIFY_ONOFF, QM_TRUE);
    } else {
        return nc_audio_cfg(NC_AUDIO_CFG_TYPE_TIMEOUT_ONTIFY_ONOFF, QM_TRUE);
    }
}

int32_t nc_audio_set_mute(bool mute)
{
    nc_player_handle.mute = mute;
    return nc_audio_set_spk_on_off(!mute);
}

bool nc_audio_get_mute(void)
{
    return nc_player_handle.mute;
}

static int qm_audio_play_data(qm_audio_type_t type, uint8_t *data, int len, int *timeout)
{
    while(qm_gpio_get_level(&nc_int_io) == 0) {
        qm_msleep(10);
    }
#if PLAY_DATA_DIFF
    QM_LOGD("1", "diff:%d", (xTaskGetTickCount() - last));
    last = xTaskGetTickCount();
#endif
    // QM_LOGD(TAG, "play data:");
    // if(nc_player_handle.spk_play.msg_id < 3) {
    //     *timeout = 0;
    // } else {
    //     *timeout = CONFIG_AUDIO_FMT_DURATION_MS;
    // }
    audio_play_data(data, len);
    return QM_EOK;
}

static int qm_audio_player_event_cb(qm_audio_event_t event, void *param)
{
    QM_LOGD(TAG, "%s:%d", __func__, event);
    switch (event)
    {
        case QM_AUDIO_EVENT_PLAY_START:
        break;
        case QM_AUDIO_EVENT_PLAY_STOP:
        break;
        case QM_AUDIO_EVENT_PLAY_LOCAL_STOP:
        break;
        case QM_AUDIO_EVENT_PLAY_MUTE:
        break;
        default:
        break;
    }
    return QM_EOK;
}

static int nc_audio_event_cb(nc_audio_event_type_t event_type, nc_audio_event_param_t *p_event)
{
    nc_audio_code_t *spk_code = &p_event->spk_play_ack;

    switch (event_type)
    {
        case NC_AUDIO_EVENT_TYPE_MIC_DATA_GET:
            if(nc_player_handle.notify) {
                nc_player_handle.notify(event_type, p_event);
            }
        break;
        case NC_AUDIO_EVENT_TYPE_SPK_REPLY:
            QM_LOGD("1", "spk state: %d", p_event->spk_state.status);
            switch (p_event->spk_state.status)
            {
                case NC_AUDIO_SPK_REPLY_TYPE_START:             // 真正播放开始事件
                break;
                case NC_AUDIO_SPK_REPLY_TYPE_EMPTY:             // 真正播放完毕事件
                qm_mutex_lock(&nc_player_handle.lock, QM_WAIT_FOREVER);
                if(nc_player_handle.pcm_play_idle == 1) {
                    nc_player_handle.pcm_play_idle = 0;
                    if(nc_player_handle.event_cb) {
                        nc_player_handle.event_cb(nc_player_handle.play_pcm_id, QM_AUDIO_EVENT_PLAY_LOCAL_STOP, nc_player_handle.user_data);
                    }
                    if(nc_player_handle.agent_play_idle == 1) {
                        qm_audio_play(QM_AUDIO_TYPE_AGENT);
                    }
                }
                qm_mutex_unlock(&nc_player_handle.lock);
                break;
                case NC_AUDIO_SPK_REPLY_TYPE_STOP:
                break;
                default:
                break;
            }
        break;
        case NC_AUDIO_EVENT_TYPE_SPK_PLAY_ACK:
            // QM_LOGD("1", "play ack:%ld,%d", spk_code->msg_id, spk_code->code);   
            nc_player_handle.spk_code.code = spk_code->code;
            nc_player_handle.spk_code.msg_id = spk_code->msg_id;
        break;
        case NC_AUDIO_EVENT_TYPE_WAKEUP:
            QM_LOGD("1", "wakeup");
            qm_post_delayed_action(&nc_player_handle.ind_time, agent_set_ind_timeout, NULL, 0);
            if(nc_player_handle.notify) {
                nc_player_handle.notify(event_type, p_event);
            }
        break;
        case NC_AUDIO_EVENT_TYPE_SLEEP:
            QM_LOGD("1", "Sleep");
        break;
        case NC_AUDIO_EVENT_TYPE_GET_MCU_VER:
            QM_LOGD("1", "audio version:%s", p_event->mcu_ver.str_ver); 
            memcpy(&nc_player_handle.version[0], p_event->mcu_ver.str_ver, CONFIG_NC_AUDIO_RECV_VER_MAX_LEN);
        break;
        default:
            break;
    }
    
    return QM_EOK;
}

void nc_player_event_regesiter(nc_player_event_t event_cb)
{
    nc_player_handle.event_cb = event_cb;
}

void uart_read(void *pvParameters) {
    uart_event_t event;
    
    for(;;) {
        // 等待UART事件
        if(xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch(event.type) {
                case UART_DATA:
                    memset(nc_player_handle.recv_buff, 0, SERIAL_RECV_LEN);
                    int len = uart_read_bytes(uart_dev.port, nc_player_handle.recv_buff, event.size, portMAX_DELAY);
                    if (len){
                        // QM_HEX_LOGD("1", "recv ", nc_player_handle.recv_buff, len);
                        nc_audio_recv_push(nc_player_handle.recv_buff, len);
                    }
                    break;
                case UART_FIFO_OVF:
                    QM_LOGD("1", "hw fifo overflow\n");
                    uart_flush_input(uart_dev.port);
                    xQueueReset(uart_queue);
                    break;
                    
                case UART_BUFFER_FULL:
                    QM_LOGD("1", "ring buffer full\n");
                    uart_flush_input(uart_dev.port);
                    xQueueReset(uart_queue);
                    break;
                    
                default:
                    QM_LOGD("1", "uart event type: %d\n", event.type);
                    break;
            }
        }
    }
}

void nc_player_init(nc_player_param_t player_param)
{
    nc_player_handle.volume = -1;
    nc_audio_param_t param = {
        .rate = CONFIG_AUDIO_FMT_RATE,
        .bits = CONFIG_AUDIO_FMT_BITS,
        .channel = CONFIG_AUDIO_FMT_CHANNEL,
        .spk_gain = 50,
        .mic_gain = CONFIG_AUDIO_FMT_MIC_GAIN,
        .vad_tiemout = CONFIG_AUDIO_FMT_VAD_TIMEOUT,

        .fmt_type = NC_AUDIO_FMT_TYPE_OPUS,
        .send = seial_send,
        .notify = nc_audio_event_cb,
    };
    qm_uart_init(&uart_dev);
    qm_task_new_to_core(&uart_read_task, "uart read", uart_read, (void *)NULL, 4096, 5, 0);
    qm_gpio_init(&nc_int_io);

    nc_player_handle.event_cb = player_param.event_cb;
    nc_player_handle.notify = player_param.notify;
    nc_player_handle.user_data = player_param.user_data;
    nc_player_handle.play_pcm_id = BSP_PCM_ID_MAX;
    
    if (qm_mutex_new(&nc_player_handle.lock) != QM_EOK) {
        return;
    }
    nc_audio_init(&param);

    nc_audio_cfg(NC_AUDIO_CFG_TYPE_SLIENT_TIMEOUT, (CONFIG_AUDIO_WAKEUP_TIMEOUT_S  + CONFIG_AUDIO_READY_SLEEP_TIMEOUT_S));
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_MAX_RECORD_TIME, CONFIG_AUDIO_RECORED_TIMEOUT / 1000);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_VAD_ONOFF, CONFIG_AUDIO_FMT_VAD_TIMEOUT);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_WAKEUP_ONOFF, QM_FALSE);
    nc_audio_cfg(NC_AUDIO_CFG_TYPE_TIMEOUT_ONTIFY_ONOFF, QM_TRUE);

    nc_player_handle.spk_play.type = NC_AUDIO_SPK_PLAY_TYPE_STOP;

    qm_audio_param_t audio_param = {
        .audio_init = NULL,
        .audio_deinit = NULL,
        .audio_play = qm_audio_play_data,
        .audio_mute = NULL,
        .audio_eventcb = qm_audio_player_event_cb,
        .arg = NULL,
    };

    qm_audio_init(&audio_param);
    qm_audio_stop();
}
