#include "qm_ai_driver.h"
#include "esp_system.h"
#include "i2s_stream.h"
#include "es8311.h"
#include "raw_stream.h"
#include "qm_audio_processor.h"

#define LOG_TAG "audio driver"

#define CONFIG_PCM_RXBUF_SIZE   (4096)

// MICARRAY设备结构体
typedef struct {
    player_pipeline_handle_t player_pipeline;
    recorder_pipeline_handle_t recorder_pipeline;
    uint8_t is_open;
    qm_mutex_t mutex;
} bsp_audio_t;

static int32_t pcm_rxbuf[CONFIG_PCM_RXBUF_SIZE];    // pcm静态缓冲区

static bsp_audio_t audio_handle;

/**
 * @brief 打开音频设备
 * 
 * @details 
 * 
 */
static int bsp_audio_open(void)
{
    esp_err_t ret = ESP_OK;
    // 初始化pipeline
    audio_handle.player_pipeline = player_pipeline_open();
    audio_handle.recorder_pipeline = recorder_pipeline_open();

    qm_mutex_new(&audio_handle.mutex);
    // 启动pipeline
    ret = player_pipeline_run(audio_handle.player_pipeline);
    if (ret != ESP_OK) {
        goto err;
    }

    ret = recorder_pipeline_run(audio_handle.recorder_pipeline);
        if (ret != ESP_OK) {
            player_pipeline_stop(audio_handle.player_pipeline);
        goto err;
    }
    // 设置默认音量
    es8311_codec_set_voice_volume(70);
    audio_handle.is_open = 1;

    return QM_EOK;
err:
    qm_mutex_free(&audio_handle.mutex);
    player_pipeline_close(audio_handle.player_pipeline);
    recorder_pipeline_close(audio_handle.recorder_pipeline);
    QM_LOGE(LOG_TAG, "audio pipeline init failed");

    return QM_EINIT;
}

/**
 * @brief 关闭音频设备
 * 
 * @details 
 * 
 */
static int bsp_audio_close(void)
{   
    if (!audio_handle.is_open) {
        return QM_EINVAL;
    }
    qm_mutex_free(&audio_handle.mutex);
    player_pipeline_stop(audio_handle.player_pipeline);
    recorder_pipeline_stop(audio_handle.recorder_pipeline);

    player_pipeline_close(audio_handle.player_pipeline);
    recorder_pipeline_close(audio_handle.recorder_pipeline);
    audio_handle.is_open = 0;

    return QM_EOK;
}

/**
 * @brief 发送音频数据
 * @param data pcm音频数组
 * @param len  pcm音频长度
 * @details 
 * 
 */
static int bsp_audio_write(void *data, size_t len)
{
    if (data == NULL || !audio_handle.is_open) {
        return QM_EINVAL;
    }
    qm_mutex_lock(&audio_handle.mutex, QM_WAIT_FOREVER);
    raw_stream_write(player_pipeline_get_raw_write(audio_handle.player_pipeline), (char *)data, len);
    qm_mutex_unlock(&audio_handle.mutex);

    return QM_EOK;
}

/**
 * @brief 读取音频数据
 * @param data pcm音频数组
 * @param len  pcm音频长度
 * @details GX8008C采用32bit/双声道输出，右声道为空，只取左声道并截取16位
 * 
 */
static int bsp_audio_read(void *data, size_t len, uint32_t timeout)
{
    int ret;
    int rx_len = len * 4;                           // 左声道+右声道一共需要接收的长度
    int32_t left_chan = 0;                          // 左声道PCM数据
    if (data == NULL || !audio_handle.is_open) {
        return QM_EINVAL;
    }

    qm_mutex_lock(&audio_handle.mutex, QM_WAIT_FOREVER);
    ret = recorder_pipeline_read(audio_handle.recorder_pipeline, (char *)pcm_rxbuf, rx_len);
    // if (ret != AEL_IO_OK) {
    //     qm_mutex_unlock(&audio_handle.mutex);
    //     return QM_ERROR;
    // }
    
    // 取出左声道并截取高16位
    for (int i = 0; i < len/2; i++) {
        left_chan = pcm_rxbuf[i * 2];
        *((int16_t *)data + i) = left_chan >> 16; 
    }
    qm_mutex_unlock(&audio_handle.mutex);
    
    memset(pcm_rxbuf, 0, sizeof(pcm_rxbuf));

    return QM_EOK;
}

/**
 * @brief 音频设备控制
 * @param cmd  命令类型
 * @param param  命令参数
 * @details 
 * 
 */
static int bsp_audio_ctrl(qm_ai_driver_ctrl_t cmd, void *param)
{
    if (!audio_handle.is_open) {
        return QM_EINVAL;
    }
    int volume = 0;
    int mute_state = 0;
    switch (cmd) {
        // 设置消音状态
        case QM_AI_DRIVER_CTRL_SET_MUTE:
            mute_state = *(int *)param;
            if (mute_state)
                es8311_set_voice_mute(true);
            else
                es8311_set_voice_mute(false);
        break;

        // 获取消音状态
        case QM_AI_DRIVER_CTRL_GET_MUTE:
            es8311_get_voice_mute(&mute_state);
            memcpy(param, &mute_state, sizeof(int));
        break;

        // 设置音量
        case QM_AI_DRIVER_CTRL_SET_VOLUME:
            volume = *(int *)param;
            es8311_codec_set_voice_volume(volume);
        break;

        // 获取音量
        case QM_AI_DRIVER_CTRL_GET_VOLUME:
            es8311_codec_get_voice_volume(&volume);
            memcpy(param, &volume, sizeof(int));
        break;

        // 使能PA状态
        case QM_AI_DRIVER_CTRL_ENABLE_PA:
            int pa_state = *(int *)param;
            if (pa_state) {
                es8311_pa_power(true);
            } else {
                es8311_pa_power(false);
            }
        break;

        default:
        break;
    }
    
    return QM_EOK;
}

// MICARRAY驱动句柄
qm_ai_driver_param_t QM_AI_AUDIO_DRIVER_GX8008C_HANDLE = {
    .open = bsp_audio_open,
    .close = bsp_audio_close,
    .write = bsp_audio_write,
    .read = bsp_audio_read,
    .ctrl = bsp_audio_ctrl
};

// #define CONFIG_USE_MICARRAY_DEMO
// e.g.
#ifdef CONFIG_USE_MICARRAY_DEMO

static int16_t audio_buf[640];

static qm_task_t audio_demo_task;

static void audio_demo_task_entry(void *arg)
{
    while(1) {
        if (QM_AI_AUDIO_DRIVER_GX8008C_HANDLE.read((void *)audio_buf, sizeof(audio_buf), 0xFFFF) == QM_EOK) {
            QM_AI_AUDIO_DRIVER_GX8008C_HANDLE.write((void *)audio_buf, sizeof(audio_buf));
        }
    }
}

void qm_application_start(void)
{
    QM_AI_AUDIO_DRIVER_GX8008C_HANDLE.open();
    qm_msleep(1000);
    qm_task_new(&audio_demo_task, "audio loop demo", audio_demo_task_entry, NULL, 1024*10, 5);
}


#endif
