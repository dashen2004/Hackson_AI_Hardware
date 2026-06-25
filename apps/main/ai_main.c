
#include "qm.h"
#include "qm_kv.h"

#include "esp_log.h"
#include "esp_system.h"
#include "qm_iot_api.h"

#include "ai_main.h"
#include "qm_ai_inner_rtc_api.h"
#include "qm_ai_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp_bat.h"
#include "bsp_i2c.h"
#include "bsp_sensor_light.h"
#include "example_video_common.h"
#include "bsp_sensor_tof.h"
#include "bsp_lcd.h"
#include "bsp_lcd_touch.h"
#include "bsp_camera.h"
#include "app_lvgl.h"
#include "xjiang_cloud.h"

#include "alarm_manager.h"
#include "qm_mem_log.h"

#define LOG_TAG "log"

#if APP_HEADLESS_CAMERA_OSS
#define HEADLESS_CLOUD_DELAY_MS  2000

static void headless_cloud_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(HEADLESS_CLOUD_DELAY_MS));
    ESP_LOGI(LOG_TAG, "headless: start cloud");
    xjiang_cloud_init();
    vTaskDelete(NULL);
}
#endif

#define DEAFALUT_REPLY_WAV              ("/spiffs/dingding.wav")
#define NETCONFIG_WAV                   ("/spiffs/1.wav")
#define CONN_CLOUD_WAV                  ("/spiffs/2.wav")
#define CONN_ERROR_WAV                  ("/spiffs/3.wav")
#define XJIANG_TEST_WAV                 ("/spiffs/4.wav")

static qm_ai_handle_t ai_handle = NULL;

qm_ai_handle_t ai_main_get_handle(void)
{
    return ai_handle;
}

static void qm_example_play_data(qm_ai_handle_t handle)
{
    // FILE *fd = fopen(XJIANG_TEST_WAV, "r");

    // uint8_t data[256] = {0};
    // int len = 0;

    // while ((len = fread(data, 1, sizeof(data), fd)) > 0) {
    //     qm_ai_audio_player_data(handle, data, len);
    // }
}

static void qm_ai_event_cb(qm_ai_handle_t handle, qm_ai_event_t *event, void *user_data)
{
    switch (event->type)
    {
        case QM_AI_EVENT_CLOUD_LICENSE:
            QM_LOGI("1", "Cloud license success!");
            // qm_example_play_data(ai_handle);
            // qm_ai_start(ai_handle);
        break;

        case QM_AI_EVENT_AGENT_STATE:
            QM_LOGI("1", "agent state: %s", event->data.agent_state.text);
        break;

        case QM_AI_EVENT_AGENT_ASR:
            QM_LOGI("1", "User says: %s", event->data.asr.text);
        break;

        case QM_AI_EVENT_AGENT_TTS:
            QM_LOGI("1", "Assistant says[%lu]: %.*s", event->data.tts.duration_ms, event->data.tts.text_len, event->data.tts.text);
        break;

        case QM_AI_EVENT_AGENT_ABORT:
            QM_LOGI("1", "Agent abort %d", event->data.agent_abort.turn_id);
        break;

        default:
        break;
    }
}

void qm_example_ai_init(void *handle)
{
    ai_handle = qm_ai_open(QM_AI_COMM_TYPE_RTC, NULL);

    qm_ai_set_service_handle_option(ai_handle, handle);

    qm_ai_driver_param_t driver = {0};

    qm_ai_audio_options_t audio_op = {
        .channel_num = 1,
        .codec_type = QM_AI_AUDIO_CODEC_DISABLED,
        .data_type = QM_AI_AUDIO_DATA_TYPE_OPUS,
        .duration = 40,
        .sample_rate = 16000,
        .audio_driver = driver,
        .wakeup_timeout = 0
    };

    qm_ai_set_audio_option(ai_handle, &audio_op);
    qm_ai_set_event_register_option(ai_handle, qm_ai_event_cb, NULL);

    // qm_ai_audio_set_player_mute(ai_handle, 1);
}

void qm_application_start(void)
{
    QM_LOGD("1", "Ver:%s", CONFIG_CT_EYE_PLUSH_VERSION);
    qm_mem_log_print("app_start");

    bsp_bat_init();
    qm_mem_log_print("bsp_bat");
#if !APP_HEADLESS_CAMERA_OSS
    bsp_i2c_adopt_bus(example_video_get_i2c_bus_handle());
    qm_mem_log_print("bsp_i2c");
    bsp_sensor_light_init();
    qm_mem_log_print("bsp_sensor_light");
#endif
#if APP_HEADLESS_CAMERA_OSS
    xTaskCreate(headless_cloud_task, "cloud_init", 4096, NULL, 5, NULL);
#else
    // bsp_sensor_tof_init();
    bsp_lcd_init();
    qm_mem_log_print("bsp_lcd");
    bsp_lcd_touch_init();
    qm_mem_log_print("bsp_lcd_touch");
    alarm_manager_init();
    qm_mem_log_print("alarm_manager");
    app_lvgl_init();
    qm_mem_log_print("app_lvgl");
#endif
}

int qm_set_reset_flag(void)
{
    return 1;
}
