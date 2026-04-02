#include "qm.h"
#include "qm_work.h"
#include "qm_gpio.h"
#include "bsp_bat.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "driver/rtc_io.h"


typedef struct 
{
    qm_timer_t timer;
    qm_gpio_dev_t usb_v;
    qm_gpio_dev_t usb_n;
    qm_work_t  ind_timer;

}bsp_bat_handle_t;

static bsp_bat_handle_t g_bat_handle = 
{
    .usb_v = {
        .port = 0,
        .config = {
            .mode = QM_GPIO_MODE_INPUT,
            .pull_en = QM_GPIO_PULLDOWN_ONLY,
        },
    },

    .usb_n = {
        .port = 0,
        .config = {
            .mode = QM_GPIO_MODE_INPUT,
            .pull_en = QM_GPIO_PULLDOWN_ONLY,
        },
    },
};

static qm_gpio_dev_t audio_enable_io = {
    .port = 45,
    .config = {
        .mode = QM_GPIO_MODE_OUTPUT,
        .pull_en = QM_GPIO_PULLDOWN_ONLY,
    }
};

static qm_gpio_dev_t nt26_enable_io = {
    .port = 14,
    .config = {
        .mode = QM_GPIO_MODE_OUTPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    }
};

static qm_gpio_dev_t nt26_rst_io = {
    .port = 21,
    .config = {
        .mode = QM_GPIO_MODE_OUTPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    }
};

static void ind_timeout_handle(void *arg);

static void bat_ticks_timeout(qm_timer_t *timer, void *arg)
{
    uint32_t sum = 0;
    uint8_t level = qm_gpio_get_level(&g_bat_handle.usb_v);
    uint8_t level2 = qm_gpio_get_level(&g_bat_handle.usb_n);

    esp_adc_cal_characteristics_t characteristics;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_BITWIDTH_12, 0, &characteristics);
    esp_adc_cal_get_voltage(ADC1_CHANNEL_8, &characteristics, &sum);

    sum = sum * (150 + 685) / 150;
    QM_LOGD("1", "bat:%d/%d_%d", level, level2, sum);

    if(level2){
        return ;
    }
    
    if(sum <= 3300){
        // qm_event_post(CONFIG_AUDIO_MAIN_EVENT_ID, CONFIG_AUDIO_SUB_EVENT_BAT_LEVEL_LOW, NULL, 0);
    }
}

qm_err_t bsp_bat_init(void)
{
    // qm_gpio_init(&g_bat_handle.usb_v);
    // qm_gpio_init(&g_bat_handle.usb_n);

    // qm_timer_new(&g_bat_handle.timer, bat_ticks_timeout, NULL, CONFIG_AUDIO_BAT_LOW_TIMEOUT, 1);
    // qm_timer_start(&g_bat_handle.timer);

    qm_gpio_init(&audio_enable_io);
    qm_gpio_set_level(&audio_enable_io, QM_TRUE);

    qm_gpio_init(&nt26_enable_io);
    
    qm_gpio_init(&nt26_rst_io);
    qm_gpio_set_level(&nt26_rst_io, QM_TRUE);

    bsp_bat_enable_4g();

    return QM_EOK;
}

qm_err_t bsp_bat_enable_4g(void)
{
    qm_gpio_set_level(&nt26_enable_io, QM_FALSE);

    return QM_EOK;
}

qm_err_t bsp_bat_restart_4g(void)
{
    qm_gpio_set_level(&nt26_enable_io, QM_TRUE);
    qm_msleep(2000);
    qm_gpio_set_level(&nt26_enable_io, QM_FALSE);

    return QM_EOK;
}

qm_err_t bsp_bat_reset_4g(void)
{
    qm_gpio_set_level(&nt26_rst_io, QM_FALSE);
    qm_msleep(500);
    qm_gpio_set_level(&nt26_rst_io, QM_TRUE);

    return QM_EOK;
}

qm_err_t bsp_bat_cancel_standby(void)
{
    return qm_cancel_delayed_action(&g_bat_handle.ind_timer);
}

qm_err_t bsp_bat_enter_standby(int standby_time)
{
    qm_cancel_delayed_action(&g_bat_handle.ind_timer);
    qm_post_delayed_action(&g_bat_handle.ind_timer, ind_timeout_handle, NULL, standby_time);
    return QM_EOK;
}

static void ind_timeout_handle(void *arg)
{
    esp_sleep_enable_ext0_wakeup(1, 0);

    rtc_gpio_pulldown_dis(1);
    rtc_gpio_pullup_dis(1);

    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(audio_enable_io.port),
        .mode = GPIO_MODE_DISABLE,
        //for powersave reasons, the GPIO should not be floating, select pullup
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(audio_enable_io.port, 0);

    // gpio_config_t cfg2 = {
    //     .pin_bit_mask = BIT64(45),
    //     .mode = GPIO_MODE_DISABLE,
    //     //for powersave reasons, the GPIO should not be floating, select pullup
    //     .pull_up_en = false,
    //     .pull_down_en = false,
    //     .intr_type = GPIO_INTR_DISABLE,
    // };
    // gpio_config(&cfg2);
    // gpio_set_level(45, 0);
    
    // gpio_reset_pin(10);

    // rtc_gpio_init(10);
    // rtc_gpio_set_direction(10, RTC_GPIO_MODE_INPUT_OUTPUT_OD);
    // rtc_gpio_pullup_dis(10);
    // rtc_gpio_pulldown_dis(10);

    // rtc_gpio_set_level(10, 1);
    // rtc_gpio_set_level(10, 0);

    esp_deep_sleep_start();
}