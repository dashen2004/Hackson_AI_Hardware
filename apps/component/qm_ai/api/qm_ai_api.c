#include "qm.h"
#include "qm_ai_api.h"
#include "qm_ai_inner_rtc_api.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_ai_license.h"
#include "qm_ai_volume.h"
#include "qm_ai_voice_control.h"
#include "qm_ai_sch_audio.h"
#include "qm_task_psram.h"

#define AI_TAG "AI"

#define CONFIG_QM_AI_AUDIO_DRIVER_GX8006    1
#define CONFIG_QM_AI_AUDIO_DRIVER_GX8008C   0

#if CONFIG_QM_AI_AUDIO_DRIVER_GX8006
#include "gx8006_driver.h"
extern qm_ai_driver_param_t QM_AI_AUDIO_DRIVER_GX8006_HANDLE;
#elif CONFIG_QM_AI_AUDIO_DRIVER_GX8008C
// #include "gx8008c_driver.h"
extern qm_ai_driver_param_t QM_AI_AUDIO_DRIVER_GX8008C_HANDLE;
#elif CONFIG_QM_AI_DRIVER_CUSTOM
qm_ai_driver_param_t audio_driver_custom;
#endif

#define AI_INNER_EVENT_ID                       (0xFF22)
#define AI_INNER_SUB_EVENT_ID_AGENT_JOINED      (0x0001)
#define AI_INNER_SUB_EVENT_ID_AGENT_ABORT       (0x0002)
#define AI_INNER_SUB_EVENT_ID_AGENT_STATE       (0x0003)
#define AI_INNER_SUB_EVENT_ID_LINK_ERROR        (0x0004)
#define AI_INNER_SUB_EVENT_ID_WAKE_UP           (0x0005)

#define AI_AUDIO_TASK_QUIT_MAX_WAIT_TIME        (5000)

#define VOLUME_CMD_VOL_UP   (1)
#define VOLUME_CMD_VOL_DOWN (2)
#define VOLUME_CMD_VOL_MAX  (3)
#define VOLUME_CMD_VOL_MIN  (4)
#define VOLUME_DEFAULT      (70)
#define VOLUME_MAX          (100)
#define VOLUME_MIN          (10)
#define VOLUME_DELTA        (15)

typedef struct qm_ai_t
{
    uint32_t handle_id;
    char version[16];

    void *service_handle;

    bool_t ai_start_flag;
    bool_t link_license_ready;
    bool_t audio_driver_ready;

    qm_ai_driver_param_t *driver_handle;
    bool_t audio_timeout_flag;

    bool_t mic_upload_flag;
    bool_t mic_mute_flag;
    bool_t player_mute_flag;
    uint32_t player_volume;

    qm_ai_rtc_cfg_t rtc_cfg;

    qm_ai_event_handler_t event_cb;

    qm_task_t audio_task;
    qm_sem_t audio_task_quit_sem;
};

static void ai_task(void *arg);
static void ai_recv_audio_data(void *data, int len, void *user_data);
static void ai_corpus_request(qm_ai_handle_t handle, uint8_t param);
static void ai_abort_request(qm_ai_handle_t handle, int32_t intr_type);

static void ai_inner_event_callback(qm_input_event_t *input_event, void *arg);
static void ai_inner_stream_event_callback(qm_ai_stream_event_t *event, void *user_data);
static void ai_inner_link_event_callback(qm_ai_link_event_type_e event, void *user_data);

static int qm_audio_play_data_cb(qm_audio_type_t type, uint8_t *data, int len, void *user_data);

static void ai_event_callback(qm_input_event_t *input_event, void *arg)
{
    if (input_event->event != QM_EVENT_QM_AI){
        return;
    }

    if (input_event->arg == NULL) {
        return;
    }

    qm_ai_handle_t handle = (qm_ai_handle_t)(input_event->arg);

    qm_ai_event_t event = {0};
    event.type = (qm_ai_event_type_t)input_event->sub_event;

    // QM_LOGD(AI_TAG, "recv event: %04x", input_event->sub_event);

    switch (input_event->sub_event)
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
                int volume = (int)input_event->value;

                qm_ai_audio_set_local_volume(handle, volume);
            }
        break;

        case QM_AI_EVENT_VOLUME_ADJUST:
            if (input_event->value != NULL) {
                int cmd = (int)input_event->value;

                switch (cmd) {
                    case VOLUME_CMD_VOL_UP:     qm_ai_audio_set_local_volume_up(handle);    break;
                    case VOLUME_CMD_VOL_DOWN:   qm_ai_audio_set_local_volume_down(handle);  break;
                    case VOLUME_CMD_VOL_MAX:    qm_ai_audio_set_local_volume(handle, VOLUME_MAX);   break;
                    case VOLUME_CMD_VOL_MIN:    qm_ai_audio_set_local_volume(handle, VOLUME_MIN);   break;
                    default:    break;
                }
            }
        break;

        case QM_AI_EVENT_MUTE_CTRL:
            int mute = (int)input_event->value;

            qm_ai_audio_set_player_mute(handle, mute);
        break;

        case QM_AI_EVENT_AGENT_LLM:
            if (input_event->value != NULL) {
                char *emotion = (char*)qm_malloc(input_event->size);
                memcpy(emotion, input_event->value, input_event->size);

                QM_LOGI(AI_TAG, "Emotion: %s", emotion);

                event.data.agent_llm.emotion = (char*)qm_malloc(input_event->size);
                memcpy(event.data.agent_llm.emotion, emotion, input_event->size);

                qm_free(input_event->value);
                qm_free(emotion);
                input_event->value = NULL;
                emotion = NULL;
            }
        break;

        default:
        break;
    }

    if (handle->event_cb) {
        handle->event_cb(handle, &event, NULL);
    }
}

qm_ai_handle_t qm_ai_open(qm_ai_comm_type_t comm_type, void *arg)
{
    qm_ai_handle_t ai_handle = (qm_ai_handle_t)qm_malloc(sizeof(struct qm_ai_t));

    if (ai_handle == NULL) {
        QM_LOGE(AI_TAG, "Fail to open sdk");
        return NULL;
    }

    memset(ai_handle, 0, sizeof(struct qm_ai_t));

    qm_event_register(QM_EVENT_QM_AI, ai_event_callback, (void*)ai_handle);
    qm_event_register(AI_INNER_EVENT_ID, ai_inner_event_callback, (void*)ai_handle);

    return ai_handle;
}

int32_t qm_ai_set_service_handle_option(qm_ai_handle_t handle, void *service_handle)
{
    if (handle == NULL || service_handle == NULL) {
        QM_LOGE(AI_TAG, "sdk handle or service handle is null, please check");
        return QM_EEMPTY;
    }

    handle->service_handle = service_handle;

    // register and start license module
    qm_ai_license_config_t *config = (qm_ai_license_config_t*)qm_malloc(sizeof(qm_ai_license_config_t));
    config->type = QM_LICENSE_RTC_QM_IOT;
    config->transport_handle = handle->service_handle;
    config->ai_handle = handle;

    qm_ai_license_init(config);
    qm_ai_license_start();

    qm_ai_volume_init(config);
    qm_ai_volume_start();

    qm_ai_voice_control_init(config);
    qm_ai_voice_control_start();

    qm_free(config);
    config = NULL;

    return QM_EOK;
}

int32_t qm_ai_set_event_register_option(qm_ai_handle_t handle, qm_ai_event_handler_t handler, void *arg)
{
    if (handle == NULL || handler == NULL) {
        QM_LOGE(AI_TAG, "sdk handle or event handler is null, please check");
        return QM_EEMPTY;
    }

    handle->event_cb = handler;

    return QM_EOK;
}

int32_t qm_ai_start(qm_ai_handle_t handle)
{
    if (handle == NULL) {
        QM_LOGE(AI_TAG, "SDK not inited, please check and try again");
        return QM_EEMPTY;
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
    handle->rtc_cfg.stream_cb = ai_inner_stream_event_callback;
    handle->rtc_cfg.event_cb = ai_inner_link_event_callback;
    handle->rtc_cfg.user_data = handle;

    if (qm_ai_rtc_init(handle->rtc_cfg) != QM_EOK) {
        goto __exit;
    }

    // qm_free(handle->rtc_cfg.license);
    // handle->rtc_cfg.license = NULL;

    if (qm_ai_rtc_connect() != QM_EOK) {
        goto __exit;
    }

    if (qm_ai_rtc_join() != QM_EOK) {
        goto __exit;
    }

    handle->ai_start_flag = 1;

    if (handle->driver_handle->read) {
        qm_task_new_psram_to_core(&handle->audio_task, "ai_read_task", ai_task, handle, 3 * 1024, 6, 1);
    }

    return QM_EOK;

__exit:
    QM_LOGE(AI_TAG, "Fail to start ai, please check and try again");

    return QM_ERROR;
}

int32_t qm_ai_stop(qm_ai_handle_t handle)
{
    if (handle == NULL) {
        QM_LOGE(AI_TAG, "sdk handle is null, please check");
        return QM_EEMPTY;
    }

    qm_sem_new(&handle->audio_task_quit_sem, 0);

    handle->ai_start_flag = 0;

    // clear audio and stream data
    qm_audio_mute();
    qm_ai_rtc_clear_buffer_data();

    // quit link
    qm_ai_rtc_leave();
    qm_ai_rtc_disconnect();
    qm_ai_rtc_deinit();

    qm_sem_wait(&handle->audio_task_quit_sem, AI_AUDIO_TASK_QUIT_MAX_WAIT_TIME);

    return QM_EOK;
}

int32_t qm_ai_close(qm_ai_handle_t *handle)
{
    if (handle == NULL || *handle == NULL) {
        QM_LOGE(AI_TAG, "error when close sdk, please check param");
        return QM_EEMPTY;
    }

    qm_ai_handle_t ctx = *handle;

    if (ctx->ai_start_flag == 1) {
        qm_ai_stop(ctx);
    }

    qm_free(ctx);

    *handle = NULL;

    return QM_EOK;
}

int32_t qm_ai_abort_request(qm_ai_handle_t handle, int32_t intr_type)
{
    if (handle == NULL) {
        QM_LOGE(AI_TAG, "sdk handle is null, please check");
        return QM_EEMPTY;
    }

    qm_ai_voice_control_cmd(QM_AI_VOICE_CONTROL_REQ_ABORT, intr_type);

    qm_event_ext_post(AI_INNER_EVENT_ID, AI_INNER_SUB_EVENT_ID_AGENT_ABORT, NULL, 0, handle);

    return QM_EOK;
}

static void ai_abort_request(qm_ai_handle_t handle, int32_t intr_type)
{
    if (handle == NULL) {
        QM_LOGE(AI_TAG, "sdk handle is null, please check");
        return;
    }

    qm_ai_voice_control_cmd(QM_AI_VOICE_CONTROL_REQ_ABORT, intr_type);
}

static void ai_corpus_request(qm_ai_handle_t handle, uint8_t param)
{
    if (handle == NULL) {
        QM_LOGE(AI_TAG, "sdk handle is null, please check");
        return;
    }

    qm_ai_voice_control_cmd(QM_AI_VOICE_CONTROL_REQ_CORPUS, param);
}

/*
    audio driver config functions
*/
static bool_t ai_check_driver_is_null(qm_ai_driver_param_t *driver)
{
    if (driver->open || driver->close || driver->write || driver->read || driver->ctrl) {
        return 0;
    }

    return 1;
}

static bool_t ai_check_driver_is_valid(qm_ai_driver_param_t *driver)
{
    if (driver->open && driver->close && driver->write && driver->ctrl) {
        return 1;
    }

    return 0;
}

static void ai_driver_read_cb(qm_ai_driver_type_t type, const void *data, int len) 
{
    qm_ai_rtc_send_audio_data(data, len);
}

static void ai_driver_event_cb(qm_ai_driver_type_t type, qm_ai_driver_event_id_t event_id, qm_ai_driver_event_t *param)
{
    if (event_id == QM_AI_DRIVER_EVENT_WAKEUP) {
        qm_event_ext_post(AI_INNER_EVENT_ID, AI_INNER_SUB_EVENT_ID_WAKE_UP, NULL, 0, (qm_ai_handle_t)param->wakeup.handle);
    }
}

int32_t qm_ai_set_audio_option(qm_ai_handle_t handle, qm_ai_audio_options_t *options)
{
    if (handle == NULL || options == NULL) {
        QM_LOGE(AI_TAG, "sdk handle or audio option is null, please check");
        return QM_EEMPTY;
    }

    handle->driver_handle = (qm_ai_driver_param_t *)qm_malloc(sizeof(qm_ai_driver_param_t));
    memset(handle->driver_handle, 0, sizeof(qm_ai_driver_param_t));

    if (ai_check_driver_is_null(&options->audio_driver)) {
        #if CONFIG_QM_AI_AUDIO_DRIVER_GX8006
            handle->driver_handle = &QM_AI_AUDIO_DRIVER_GX8006_HANDLE;
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

    handle->driver_handle->open();

    handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_EVENT_CALLBACK, (void*)ai_driver_event_cb, (void*)handle);

    if (handle->driver_handle->read == NULL) {
        handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_READ_CALLBACK, (void*)ai_driver_read_cb, (void*)handle);
    }

    handle->audio_driver_ready = 1;

    QM_LOGI(AI_TAG, "Audio driver is ready");

    // rtc config
    handle->rtc_cfg.audio_enable = 1;
    handle->rtc_cfg.data_type = options->data_type;
    handle->rtc_cfg.codec_type = options->codec_type;
    handle->rtc_cfg.sample_rate = options->sample_rate;
    handle->rtc_cfg.channel_num = options->channel_num;
    handle->rtc_cfg.duration = options->duration;

    // qm_audio config
    qm_audio_param_t audio_param = {
        .audio_init = NULL,
        .audio_deinit = NULL,
        .audio_play = qm_audio_play_data_cb,
        .audio_mute = NULL,
        .audio_eventcb = NULL,
        .arg = handle,
    };
    qm_audio_init(&audio_param);

    return QM_EOK;
}

/*
    audio driver control functions
*/
int32_t qm_ai_audio_set_mic_mute(qm_ai_handle_t handle, bool_t mute)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    handle->mic_mute_flag = mute;

    return QM_EOK;
}

bool_t qm_ai_audio_get_mic_mute_state(qm_ai_handle_t handle)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    return handle->mic_mute_flag;
}

int32_t qm_ai_audio_set_player_mute(qm_ai_handle_t handle, bool_t mute)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_SET_MUTE, (void*)mute, (void*)handle);
    handle->player_mute_flag = mute;

    return QM_EOK;
}

bool_t qm_ai_audio_get_player_mute_state(qm_ai_handle_t handle)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    return handle->player_mute_flag;
}

int32_t qm_ai_audio_set_local_volume(qm_ai_handle_t handle, uint32_t volume)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    qm_ai_audio_set_player_mute(handle, 0);

    if (volume < VOLUME_MIN || volume > VOLUME_MAX) {
        volume = VOLUME_DEFAULT;
    }

    handle->driver_handle->ctrl(QM_AI_DRIVER_CTRL_SET_VOLUME, (void*)volume, (void*)handle);
    handle->player_volume = volume;

    return QM_EOK;
}

int32_t qm_ai_audio_set_local_volume_up(qm_ai_handle_t handle)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    uint32_t volume = qm_ai_audio_get_local_volume(handle);
    volume += VOLUME_DELTA;

    if (volume > VOLUME_MAX) {
        volume = VOLUME_MAX;
    }

    qm_ai_audio_set_local_volume(handle, volume);

    return QM_EOK;
}

int32_t qm_ai_audio_set_local_volume_down(qm_ai_handle_t handle)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    uint32_t volume = qm_ai_audio_get_local_volume(handle);
    volume -= VOLUME_DELTA;

    if (volume < VOLUME_MIN || volume > VOLUME_MAX) {
        volume = VOLUME_MIN;
    }

    qm_ai_audio_set_local_volume(handle, volume);

    return QM_EOK;
}

uint32_t qm_ai_audio_get_local_volume(qm_ai_handle_t handle)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    if (handle->player_volume < VOLUME_MIN || handle->player_volume > VOLUME_MAX) {
        handle->player_volume = VOLUME_DEFAULT;
    }

    return handle->player_volume;
}

int32_t qm_ai_audio_player_data(qm_ai_handle_t handle, uint8_t *data, uint32_t size)
{
    if (handle == NULL || !handle->audio_driver_ready) {
        QM_LOGE(AI_TAG, "sdk handle is null or audio driver is not ready, please check");
        return QM_EEMPTY;
    }

    if (handle->ai_start_flag) {
        QM_LOGE(AI_TAG, "this function is not supported after sdk started");
        return QM_EINVAL;
    }

    handle->driver_handle->write(data, size);

    return QM_EOK;
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

        ret = handle->driver_handle->read((void *)audio_buf, sizeof(audio_buf), QM_WAIT_FOREVER);
        if (ret == QM_EOK) {
            if (handle->mic_upload_flag) {
                qm_ai_rtc_send_audio_data(audio_buf, sizeof(audio_buf));
                qm_msleep(handle->rtc_cfg.duration);
            }
        }

        if (!handle->ai_start_flag) {
            QM_LOGI(AI_TAG, "ai func stop, quit ai task");
            break;
        }
    }

    qm_sem_signal(&handle->audio_task_quit_sem);

    qm_task_exit_psram(&handle->audio_task);
}

static void ai_recv_audio_data(void *data, int len, void *user_data)
{
    if (user_data == NULL) {
        return;
    }

    qm_audio_data_push(QM_AUDIO_TYPE_AGENT, (uint8_t *)data, len);
}

/*
    inner callback
*/
static void ai_inner_event_callback(qm_input_event_t *input_event, void *arg)
{
    if (input_event->arg == NULL) {
        return;
    }

    qm_ai_handle_t handle = (qm_ai_handle_t)(input_event->arg);

    // QM_LOGD(AI_TAG, "inner event recv: %04x", input_event->sub_event);

    switch (input_event->sub_event)
    {
        case AI_INNER_SUB_EVENT_ID_AGENT_JOINED:
            ai_corpus_request(handle, QM_AI_VOICE_CONTROL_PARAM_CORPUS_OPEN);
        break;

        case AI_INNER_SUB_EVENT_ID_WAKE_UP:
            if (handle->ai_start_flag) {
                ai_abort_request(handle, QM_AI_VOICE_CONTROL_PARAM_ABORT_WITH_WORD);
            } else {
                qm_ai_start(handle);
            }
        break;

        case AI_INNER_SUB_EVENT_ID_AGENT_ABORT:
            qm_ai_rtc_clear_buffer_data();
        break;

        case AI_INNER_SUB_EVENT_ID_AGENT_STATE:
            int state = (int)input_event->value;
            switch (state) {
                case QM_AI_AGENT_STATE_SPEAKING:
                case QM_AI_AGENT_STATE_THINKING:
                    qm_audio_play(QM_AUDIO_TYPE_AGENT);
                break;

                default:
                break;
            }
        break;

        default:
        break;
    }
}

static void ai_inner_stream_event_callback(qm_ai_stream_event_t *event, void *user_data)
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

            qm_event_ext_post(AI_INNER_EVENT_ID, AI_INNER_SUB_EVENT_ID_AGENT_STATE, (void*)event->data.agent_state.state, 0, ai_handle);
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

static void ai_inner_link_event_callback(qm_ai_link_event_type_e event, void *user_data)
{
    if (user_data == NULL) {
        return;
    }

    qm_ai_handle_t ai_handle = (qm_ai_handle_t)user_data;

    // QM_LOGD(AI_TAG, "link recv event: %d", event);

    switch (event) {
        case QM_AI_LINK_EVENT_TYPE_AGENT_JOINED:
            qm_event_ext_post(AI_INNER_EVENT_ID, AI_INNER_SUB_EVENT_ID_AGENT_JOINED, NULL, 0, ai_handle);
        break;

        case QM_AI_LINK_EVENT_TYPE_JOIN_CHANNEL:
            ai_handle->mic_upload_flag = 1;
        break;

        case QM_AI_LINK_EVENT_TYPE_LEAVE_CHANNEL:
            ai_handle->mic_upload_flag = 0;
        break;

        case QM_AI_LINK_EVENT_TYPE_CONNECT_LOST:
            ai_handle->mic_upload_flag = 0;
            qm_event_ext_post(AI_INNER_EVENT_ID, AI_INNER_SUB_EVENT_ID_LINK_ERROR, NULL, 0, ai_handle);
        break;

        case QM_AI_LINK_EVENT_TYPE_AUDIO_MUTE:
            qm_audio_mute();
            qm_ai_rtc_clear_buffer_data();
        break;

        default:
        break;
    }
}

/*
    audio cb
*/
static int qm_audio_play_data_cb(qm_audio_type_t type, uint8_t *data, int len, void *user_data)
{
    if (user_data == NULL) {
        return QM_EEMPTY;
    }

    qm_ai_handle_t handle = (qm_ai_handle_t)user_data;
    handle->driver_handle->write(data, len);

    return QM_EOK;
}
