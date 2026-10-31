#include "bsp_touch.h"

#include "qm_gpio.h"
#include "qm_kernel.h"
#include "qm_log.h"

#define LOG_TAG "bsp_touch"
#define BSP_TOUCH_POLL_MS 20U
#define BSP_TOUCH_STABLE_SAMPLES 3U
#define BSP_TOUCH_TASK_STACK 2048
#define BSP_TOUCH_TASK_PRIO 5

static qm_gpio_dev_t s_gpio = {
    .port = BSP_TOUCH_GPIO,
    .config = {
        .mode = QM_GPIO_MODE_INPUT,
        .pull_en = QM_GPIO_PULLDOWN_ONLY,
    },
};

static uint8_t s_inited;
static qm_task_t s_task;
static bsp_touch_press_callback_t s_press_callback;
static void *s_press_callback_arg;

static void touch_monitor_task(void *arg)
{
    uint8_t stable_level;
    uint8_t candidate_level;
    uint8_t candidate_count = 0;

    (void)arg;
    qm_msleep(600);
    stable_level = qm_gpio_get_level(&s_gpio);
    candidate_level = stable_level;
    QM_LOGI(LOG_TAG, "ready gpio%d level=%u", BSP_TOUCH_GPIO,
            (unsigned)stable_level);

    while (1) {
        uint8_t level;

        qm_msleep(BSP_TOUCH_POLL_MS);
        level = qm_gpio_get_level(&s_gpio);
        if (level != candidate_level) {
            candidate_level = level;
            candidate_count = 1;
        } else if (candidate_count < BSP_TOUCH_STABLE_SAMPLES) {
            candidate_count++;
        }

        if (candidate_count >= BSP_TOUCH_STABLE_SAMPLES &&
            stable_level != candidate_level) {
            stable_level = candidate_level;
            QM_LOGI(LOG_TAG, "state=%s level=%u",
                    stable_level == BSP_TOUCH_ACTIVE_LEVEL ? "touched" : "released",
                    (unsigned)stable_level);
            if (stable_level == BSP_TOUCH_ACTIVE_LEVEL && s_press_callback != NULL) {
                s_press_callback(s_press_callback_arg);
            }
        }
    }
}

qm_err_t bsp_touch_init(void)
{
    if (s_inited) {
        return QM_EOK;
    }

    if (qm_gpio_init(&s_gpio) != QM_EOK) {
        QM_LOGE(LOG_TAG, "gpio%d input init fail", BSP_TOUCH_GPIO);
        return -QM_EIO;
    }

    s_inited = 1;
    if (qm_task_new(&s_task, "touch", touch_monitor_task, NULL,
                    BSP_TOUCH_TASK_STACK, BSP_TOUCH_TASK_PRIO) != 0) {
        s_inited = 0;
        QM_LOGE(LOG_TAG, "monitor task create fail");
        return -QM_EIO;
    }

    QM_LOGI(LOG_TAG, "init ok gpio%d input pulldown", BSP_TOUCH_GPIO);
    return QM_EOK;
}

uint8_t bsp_touch_get_level(void)
{
    if (!s_inited) {
        return 0;
    }
    return qm_gpio_get_level(&s_gpio);
}

uint8_t bsp_touch_is_active(void)
{
    return bsp_touch_get_level() == BSP_TOUCH_ACTIVE_LEVEL;
}

void bsp_touch_set_press_callback(bsp_touch_press_callback_t callback, void *arg)
{
    s_press_callback = callback;
    s_press_callback_arg = arg;
}
