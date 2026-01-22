#include "qm.h"
#include "qm_ai_api.h"
#include "qm_ai_inner_rtc_api.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_ai_license.h"
#include "qm_ai_volume.h"
#include "qm_ai_voice_control.h"

#define AI_TAG "AI"

#define CONFIG_QM_AI_AUDIO_DRIVER_USER_GX8006   0
#define CONFIG_QM_AI_AUDIO_DRIVER_GX8008C       1

#if CONFIG_QM_AI_AUDIO_DRIVER_USER_GX8006
#include "gx8006_driver.h"
extern qm_ai_driver_param_t audio_driver_gx8006;
#elif CONFIG_QM_AI_AUDIO_DRIVER_GX8008C
// #include "gx8008c_driver.h"
extern qm_ai_driver_param_t QM_AI_AUDIO_DRIVER_GX8008C_HANDLE;
#elif CONFIG_QM_AI_DRIVER_CUSTOM
qm_ai_driver_param_t audio_driver_custom;
#endif

#define AI_INNER_EVENT_ID                       (0xFF22)
#define AI_INNER_SUB_EVENT_ID_AGENT_JOINED      (0x0001)
#define AI_INNER_SUB_EVENT_ID_AGENT_ABORT       (0x0002)

#define VOLUME_DEFAULT    (70)
#define VOLUME_MAX        (100)
#define VOLUME_MIN        (1)
#define VOLUME_DELTA      (15)

typedef struct qm_ai_t
{
    uint32_t handle_id;
    char version[16];

    void *service_handle;

    bool ai_start_flag;
    bool link_license_ready;
    bool audio_driver_ready;

    qm_ai_driver_param_t *driver_handle;
    bool audio_timeout_flag;

    bool mic_mute_flag;
    bool player_mute_flag;
    uint32_t player_volume;

    qm_ai_rtc_cfg_t rtc_cfg;

    qm_ai_event_handler_t event_cb;

    qm_task_t audio_task;
};

static void ai_task(void *arg);
static void ai_recv_audio_data(void *data, int len, void *user_data);
static void ai_corpus_request(qm_ai_handle_t handle, uint8_t param);

static void ai_event_callback(qm_input_event_t *input_event, void *arg);
static void ai_stream_event_callback(qm_ai_stream_event_t *event, void *user_data);
static void ai_link_event_callback(qm_ai_link_event_type_e event, void *user_data);

static void ai_inner_event_callback(qm_input_event_t *input_event, void *arg)
{
    if (input_event->arg == NULL) {
        return;
    }

    qm_ai_handle_t handle = (qm_ai_handle_t)(input_event->arg);

    QM_LOGI(AI_TAG, "inner event recv: %04x", input_event->sub_event);

    switch (input_event->sub_event)
    {
        case AI_INNER_SUB_EVENT_ID_AGENT_JOINED:
            ai_corpus_request(handle, QM_AI_VOICE_CONTROL_PARAM_CORPUS_OPEN);
        break;

        // case CONFIG_AI_SUB_INNER_EVENT_WAKE_UP:
        //     if (quit_flag) {
        //         qm_ai_rtc_join();
        //         quit_flag = 0;
        //     }
        // break;

        case AI_INNER_SUB_EVENT_ID_AGENT_ABORT:
            qm_ai_rtc_clear_buffer_data();
        break;

        default:
        break;
    }
}

qm_ai_handle_t qm_ai_open(qm_ai_comm_type_t comm_type, void *arg)
{
    qm_ai_handle_t ai_handle = (qm_ai_handle_t)qm_malloc(sizeof(struct qm_ai_t));

    if (ai_handle == NULL || arg == NULL) {
        return NULL;
    }

    memset(ai_handle, 0, sizeof(struct qm_ai_t));

    qm_event_register(QM_EVENT_QM_AI, ai_event_callback, (void*)ai_handle);
    qm_event_register(AI_INNER_EVENT_ID, ai_inner_event_callback, (void*)ai_handle);

    return ai_handle;
}

int32_t qm_ai_set_service_handle_option(qm_ai_handle_t handle, void *service_handle)
{
    if (service_handle == NULL) {
        return QM_EEMPTY;
    }

    handle->service_handle = service_handle;

    // register and start license module
    qm_ai_license_config_t *config = malloc(sizeof(qm_ai_license_config_t));
    config->type = QM_LICENSE_RTC_QM_IOT;
    config->transport_handle = handle->service_handle;
    config->ai_handle = handle;

    qm_ai_license_init(config);
    qm_ai_license_start();

    qm_ai_volume_init(config);
    qm_ai_volume_start();

    qm_ai_voice_control_init(config);
    qm_ai_voice_control_start();

    return QM_EOK;
}

int32_t qm_ai_set_event_register_option(qm_ai_handle_t handle, qm_ai_event_handler_t handler, void *arg)
{
    if (handler == NULL) {
        QM_LOGE(AI_TAG, "Register event fail, handler is null");
        return QM_EEMPTY;
    }

    handle->event_cb = handler;

    return QM_EOK;
}

int32_t qm_ai_start(qm_ai_handle_t handle)
{
    if (handle == NULL) {
        QM_LOGE(AI_TAG, "SDK not inited, please check and try again");
        return QM_EINVAL;
    }

    if (!handle->link_license_ready) {
        QM_LOGE(AI_TAG, "License not ready, please check and try again");
        return QM_EINVAL;
    }

    if (!handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "Audio driver not inited, please check and try again");
        return QM_EINVAL;
    }

    handle->rtc_cfg.audio_cb = ai_recv_audio_data;
    handle->rtc_cfg.stream_cb = ai_stream_event_callback;
    handle->rtc_cfg.event_cb = ai_link_event_callback;
    handle->rtc_cfg.user_data = handle;

    if (qm_ai_rtc_init(handle->rtc_cfg) != QM_EOK) {
        goto __exit;
    }

    if (qm_ai_rtc_connect() != QM_EOK) {
        goto __exit;
    }

    if (qm_ai_rtc_join() != QM_EOK) {
        goto __exit;
    }

    if (handle->driver_handle != NULL) {
        if (handle->driver_handle->read) {
            qm_task_new_to_core(&handle->audio_task, "audio", ai_task, handle, 10 * 1024, 6, 0);
        }
    }

    handle->ai_start_flag = 1;

    return QM_EOK;

__exit:
    QM_LOGE(AI_TAG, "Fail to start ai, please check and try again");

    return QM_ERROR;
}

int32_t qm_ai_stop(qm_ai_handle_t handle)
{
    handle->ai_start_flag = 0;

    qm_ai_rtc_leave();
    qm_ai_rtc_disconnect();
    qm_ai_rtc_deinit();

    return QM_EOK;
}

int32_t qm_ai_close(qm_ai_handle_t *handle)
{
    qm_free(handle);

    return QM_EOK;
}

int32_t qm_ai_abort_request(qm_ai_handle_t handle, int32_t intr_type)
{
    qm_ai_voice_control_cmd(QM_AI_VOICE_CONTROL_REQ_ABORT, intr_type);

    qm_event_ext_post(AI_INNER_EVENT_ID, AI_INNER_SUB_EVENT_ID_AGENT_ABORT, NULL, 0, handle);

    return QM_EOK;
}

static void ai_corpus_request(qm_ai_handle_t handle, uint8_t param)
{
    qm_ai_voice_control_cmd(QM_AI_VOICE_CONTROL_REQ_CORPUS, param);
}

/*
    audio driver config functions
*/
static bool ai_check_driver_is_null(qm_ai_driver_param_t *driver)
{
    if (driver->open || driver->close || driver->write || driver->read || driver->ctrl) {
        return 0;
    }

    return 1;
}

static bool ai_check_driver_is_valid(qm_ai_driver_param_t *driver)
{
    if (driver->open && driver->close && driver->write && driver->ctrl) {
        return 1;
    }

    return 0;
}

static void ai_driver_read_cb(qm_ai_driver_type_t type, const void *data, size_t len) 
{
    qm_ai_rtc_send_audio_data(data, len);
}

int32_t qm_ai_set_audio_option(qm_ai_handle_t handle, qm_ai_audio_options_t *options)
{
    if (options == NULL) {
        QM_LOGE(AI_TAG, "Error, options is null.");
        return QM_EINVAL;
    }

    handle->driver_handle = (qm_ai_driver_param_t *)qm_malloc(sizeof(qm_ai_driver_param_t));
    memset(handle->driver_handle, 0, sizeof(qm_ai_driver_param_t));

    if (ai_check_driver_is_null(&options->audio_driver)) {
        #if CONFIG_QM_AI_AUDIO_DRIVER_GX8006
            handle->driver_handle = &audio_driver_gx8006;
        #elif CONFIG_QM_AI_AUDIO_DRIVER_GX8008C
            handle->driver_handle = &QM_AI_AUDIO_DRIVER_GX8008C_HANDLE;
        #endif
    } else {
        if (!ai_check_driver_is_valid(&options->audio_driver)) {
            QM_LOGE(AI_TAG, "custom audio driver is invalid");
            return QM_ERROR;
        }

        memcpy(handle->driver_handle, &options->audio_driver, sizeof(qm_ai_driver_param_t));
    }

    // start driver
    if (!ai_check_driver_is_valid(handle->driver_handle)) {
        QM_LOGE(AI_TAG, "audio driver is invalid");
        return QM_ERROR;
    }

    if (handle->driver_handle->read == NULL) {
        handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_READ_CALLBACK, ai_driver_read_cb);
    }

    handle->driver_handle->open();

    handle->audio_driver_ready = 1;

    QM_LOGI(AI_TAG, "Audio driver is ready");

    // rtc config
    handle->rtc_cfg.audio_enable = 1;
    handle->rtc_cfg.data_type = options->data_type;
    handle->rtc_cfg.codec_type = options->codec_type;
    handle->rtc_cfg.sample_rate = options->sample_rate;
    handle->rtc_cfg.channel_num = options->channel_num;
    handle->rtc_cfg.duration = options->duration;

    return QM_EOK;
}

/*
    audio driver control functions
*/
int32_t qm_ai_audio_set_mic_mute(qm_ai_handle_t handle, bool_t mute)
{
    handle->mic_mute_flag = mute;

    return QM_EOK;
}

bool_t qm_ai_audio_get_mic_mute_state(qm_ai_handle_t handle)
{
    return handle->mic_mute_flag;
}

int32_t qm_ai_audio_set_player_mute(qm_ai_handle_t handle, bool_t mute)
{
    handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_SET_MUTE, &mute);
    handle->player_mute_flag = mute;

    return QM_EOK;
}

bool_t qm_ai_audio_get_player_mute_state(qm_ai_handle_t handle)
{
    return handle->player_mute_flag;
}

int32_t qm_ai_audio_set_local_volume(qm_ai_handle_t handle, uint32_t volume)
{
    if (volume < VOLUME_MIN || volume > VOLUME_MAX) {
        volume = VOLUME_DEFAULT;
    }

    handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_SET_VOLUME, &volume);
    handle->player_volume = volume;

    return QM_EOK;
}

int32_t qm_ai_audio_set_local_volume_up(qm_ai_handle_t handle)
{
    uint32_t volume = qm_ai_audio_get_local_volume(handle);
    volume += VOLUME_DELTA;

    if (volume > VOLUME_MAX) {
        volume = VOLUME_MAX;
    }

    handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_SET_VOLUME, &volume);
    handle->player_volume = volume;

    return QM_EOK;
}

int32_t qm_ai_audio_set_local_volume_down(qm_ai_handle_t handle)
{
    uint32_t volume = qm_ai_audio_get_local_volume(handle);
    volume -= VOLUME_DELTA;

    if (volume < VOLUME_MIN || volume > VOLUME_MAX) {
        volume = VOLUME_MIN;
    }

    handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_SET_VOLUME, &volume);
    handle->player_volume = volume;

    return QM_EOK;
}

uint32_t qm_ai_audio_get_local_volume(qm_ai_handle_t handle)
{
    return handle->player_volume;
}

/*
    inner functions
*/
static void ai_task(void *arg)
{
    qm_ai_handle_t handle = (qm_ai_handle_t)arg;

    static int16_t audio_buf[640];

    int ret;

    while (1) {
        if (handle->audio_timeout_flag) {
            qm_msleep(100);
            continue;
        }

        ret = handle->driver_handle->read((void *)audio_buf, sizeof(audio_buf), 0xFFFF);
        if (ret == QM_EOK) {
            qm_ai_rtc_send_audio_data(audio_buf, sizeof(audio_buf));
        }

        if (!handle->ai_start_flag) {
            QM_LOGI(AI_TAG, "ai func stop, quit ai task");
            break;
        }
    }

    qm_task_exit(&handle->audio_task);
}

static void ai_recv_audio_data(void *data, int len, void *user_data)
{
    if (user_data == NULL) {
        return;
    }

    qm_ai_handle_t handle = (qm_ai_handle_t)user_data;
    handle->driver_handle->write(data, len);
}

/*
    callback
*/
static void ai_event_callback(qm_input_event_t *input_event, void *arg)
{
    if (input_event->event != QM_EVENT_QM_AI){
        return;
    }

    if (input_event->arg == NULL) {
        return;
    }

    int event_type = input_event->sub_event;
    
    qm_ai_handle_t handle = (qm_ai_handle_t)(input_event->arg);

    QM_LOGI(AI_TAG, "recv event: %04x", event_type);

    switch (event_type)
    {
        case QM_AI_EVENT_CLOUD_LICENSE:
            if (input_event->value != NULL) {
                char *license = (char*)input_event->value;
                handle->rtc_cfg.license = (char*)qm_malloc(strlen(license) + 1);
                memcpy(handle->rtc_cfg.license, license, strlen(license) + 1);

                handle->link_license_ready = 1;

                qm_ai_license_stop();
                qm_ai_license_deinit();
            }
        break;

        case QM_AI_EVENT_VOLUME_CTRL:
            if (input_event->value != NULL) {
                uint32_t volume = (uint32_t)input_event->value;

                qm_ai_audio_set_local_volume(handle, volume);
            }
        break;

        case QM_AI_EVENT_VOLUME_ADJUST:
            if (input_event->value != NULL) {
                int cmd = (int)input_event->value;
                switch (cmd) {
                    case 1: qm_ai_audio_set_local_volume_up(handle);    break;
                    case 2: qm_ai_audio_set_local_volume_down(handle);  break;
                    case 3: qm_ai_audio_set_local_volume(handle, VOLUME_MAX);   break;
                    case 4: qm_ai_audio_set_local_volume(handle, VOLUME_MIN);   break;
                    default:    break;
                }
            }
        break;

        case QM_AI_EVENT_MUTE_CTRL:
            if (input_event->value != NULL) {
                bool_t mute = (bool_t)input_event->value;

                qm_ai_audio_set_player_mute(handle, mute);
            }
        break;

        default:
        break;
    }

    if (handle->event_cb) {
        qm_ai_event_t event = {0};
        event.type = (qm_ai_event_type_t)event_type;
        handle->event_cb(handle, &event, NULL);
    }
}

static void ai_stream_event_callback(qm_ai_stream_event_t *event, void *user_data)
{
    if (user_data == NULL) {
        return;
    }

    qm_ai_handle_t ai_handle = (qm_ai_handle_t)user_data;
    qm_ai_event_t event_param = {0};
    int32_t text_len = 0;

    switch (event->type)
    {
        case QM_AI_STREAM_TYPE_ASR:
            text_len = strlen(event->data.stt.text) + 1;

            event_param.type = QM_AI_EVENT_AGENT_ASR;
            event_param.data.asr.text = (char*)qm_malloc(text_len);
            memcpy(event_param.data.asr.text, event->data.stt.text, text_len);
        break;

        case QM_AI_STREAM_TYPE_TTS:
            text_len = event->data.tts.text_len;

            event_param.type = QM_AI_EVENT_AGENT_TTS;
            event_param.data.tts.text_len = event->data.tts.text_len;
            event_param.data.tts.duration_ms = event->data.tts.duration_ms;
            event_param.data.tts.text = (char*)qm_malloc(text_len);
            memcpy(event_param.data.tts.text, event->data.tts.text, text_len);
        break;

        case QM_AI_STREAM_TYPE_AGENT_STATE:
            text_len = strlen(event->data.agent_state.text) + 1;

            event_param.type = QM_AI_EVENT_AGENT_STATE;
            event_param.data.agent_state.state = event->data.agent_state.state;
            event_param.data.agent_state.text = (char*)qm_malloc(text_len);
            memcpy(event_param.data.agent_state.text, event->data.agent_state.text, text_len);
        break;

        case QM_AI_STREAM_TYPE_INTERRUPT:
            event_param.type = QM_AI_EVENT_AGENT_ABORT;
            event_param.data.agent_abort.turn_id = event->data.interrupt.turn_id;
        break;

        default:
        break;
    }

    if (ai_handle->event_cb) {
        ai_handle->event_cb(ai_handle, &event_param, NULL);
    }
}

static void ai_link_event_callback(qm_ai_link_event_type_e event, void *user_data)
{
    if (user_data == NULL) {
        return;
    }

    qm_ai_handle_t ai_handle = (qm_ai_handle_t)user_data;

    switch (event) {
        case QM_AI_LINK_EVENT_TYPE_AGENT_JOINED:
            qm_event_ext_post(AI_INNER_EVENT_ID, AI_INNER_SUB_EVENT_ID_AGENT_JOINED, NULL, 0, ai_handle);
        break;

        default:
        break;
    }
}
