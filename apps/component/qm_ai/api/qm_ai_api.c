#include "qm.h"
#include "qm_ai_api.h"
#include "qm_ai_inner_rtc_api.h"
#include "qm_work.h"
#include "qm_event.h"
#include "qm_ai_license.h"
#include "qm_ai_volume.h"

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

typedef struct qm_ai_t
{
    uint32_t handle_id;
    char version[16];

    void* transport_handle;

    bool ai_start_flag;
    bool link_license_ready;
    bool audio_driver_ready;

    qm_ai_driver_type_t drvier_type;
    qm_ai_driver_param_t *driver_handle;
    bool audio_timeout_flag;

    qm_ai_rtc_cfg_t rtc_cfg;

    qm_ai_event_handler_t event_cb;

    qm_task_t audio_task;
};

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
        handle->drvier_type = QM_AI_DRIVER_TYPE_DEFAULT;
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

        handle->drvier_type = QM_AI_DRIVER_TYPE_CUSTOM;
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

static void qm_ai_event_callback(qm_input_event_t *input_event, void *arg)
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

        // case CONFIG_AI_SUB_EVENT_VOL_REGISTERED:
        //      volume_registered = 1;
        // break;

        // case CONFIG_AI_SUB_EVENT_INTERRUPT:
        //      interrupt_flag = 1;
        // break;

        // case CONFIG_AI_SUB_EVENT_VOL_CONTROL:
        //      driver->cmd(volume)
        // break;

        default:
        break;
    }

    if (handle->event_cb) {
        qm_ai_event_t event = {0};
        event.type = (qm_ai_event_type_t)event_type;
        handle->event_cb(handle, &event, NULL);
    }
}

/**
 * @brief 初始化IOT句柄
 * 
 * @details 
 * 
 */
qm_ai_handle_t qm_ai_open(qm_ai_comm_type_t comm_type, void *arg)
{
    qm_ai_handle_t ai_handle = (qm_ai_handle_t)qm_malloc(sizeof(struct qm_ai_t));

    if (ai_handle == NULL) {
        return NULL;
    }

    memset(ai_handle, 0, sizeof(struct qm_ai_t));

    return ai_handle;
}

/**
 * @brief 初始化ai模块
 * 
 * @details 
 * 
 */
int32_t qm_ai_init(qm_ai_handle_t handle, void *transport_handle)
{
    if (handle == NULL) {
        return QM_EINIT;
    }

    qm_event_register(QM_EVENT_QM_AI, qm_ai_event_callback, (void*)handle);

    // register and start license module
    handle->transport_handle = transport_handle;
    qm_ai_register_module(handle, QM_AI_MOD_LICENSE, NULL);

    return QM_EOK;
}

/**
 * @brief 注册模块
 * 
 * @details 
 * 
 */
int32_t qm_ai_register_module(qm_ai_handle_t handle, qm_ai_module_t mod, void *arg)
{
    qm_ai_license_config_t *config = malloc(sizeof(qm_ai_license_config_t));
    config->type = QM_LICENSE_RTC_QM_IOT;
    config->transport_handle = handle->transport_handle;
    config->ai_handle = handle;

    switch (mod){
        case QM_AI_MOD_LICENSE:
            qm_ai_license_init(config);
            qm_ai_license_start();
        break;

        case  QM_AI_MOD_VOLUME:
            qm_ai_volume_init(config);
            qm_ai_volume_start();
        break;

        case  QM_AI_MOD_VOICE_CONTROL:
            // qm_ai_voice_control_register(arg)
        break;

        default:
            QM_LOGE(AI_TAG, "Error, unsupported module %d", mod);
            return QM_EINVAL;
        break;
    }

    return QM_EOK;
}

/**
 * @brief 注册信息流事件
 * 
 * @details 
 * 
 */
int32_t qm_ai_set_stream_register_option(qm_ai_handle_t handle, qm_ai_stream_cb_t handler, void *arg)
{
    handle->rtc_cfg.stream_cb = handler;
    return QM_EOK;
}

int32_t qm_ai_set_event_register_option(qm_ai_handle_t handle, qm_ai_event_handler_t handler, void *arg)
{
    handle->event_cb = handler;
    return QM_EOK;
}

static void qm_ai_inner_event_callback(qm_input_event_t *input_event, void *arg)
{
    int event_type = input_event->sub_event;

    switch (event_type)
    {
        // case CONFIG_AI_SUB_INNER_EVENT_WAKE_UP:
        //     if (quit_flag) {
        //         qm_ai_rtc_join();
        //         quit_flag = 0;
        //     }
        // break;

        // case CONFIG_AI_SUB_EVENT_INTERRUPT:
        //      qm_ai_rtc_reset_data();
        // break;

        default:
        break;
    }
}

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
        // qm_ai_rtc_send_audio_data(audio_buf, sizeof(audio_buf));
        if (ret == QM_EOK) {
            handle->driver_handle->write((void *)audio_buf, sizeof(audio_buf));
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
    // handle->driver_handle->write(data, len);
}

/**
 * @brief 启动AI模块
 * 
 * @details 
 * 
 */
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
