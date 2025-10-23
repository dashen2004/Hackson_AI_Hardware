#include "qm.h"
#include "qm_kv.h"

#include "esp_log.h"
#include "esp_system.h"

#include "lcd.h"
#include "xjiang_cloud.h"

#include "audio_main.h"
#include "audio_cloud.h"
#include "audio_light.h"
// #include "audio_prodtst.h"

#define QM_RESET_TAG        "qm_reset"
#define QM_RESET_UNKNOWN    (1)
#define QM_RESET_NORMAL     (2)

static int qm_reset_flag = 0;

int qm_is_crash_reset(void)
{
    bool ret = QM_FALSE;

    esp_reset_reason_t reset_reason = esp_reset_reason();
    QM_LOGI("1", "reset reason is %d", reset_reason);

    if (reset_reason != ESP_RST_POWERON &&
       qm_reset_flag == QM_RESET_UNKNOWN) {
        ret = QM_TRUE;
    }

    return ret;
}

int qm_set_reset_flag(void)
{
    qm_reset_flag = QM_RESET_NORMAL;
    qm_kv_set(QM_RESET_TAG, &qm_reset_flag, sizeof(int), QM_TRUE);

    return QM_EOK;
}

void qm_application_start(void)
{
    int flag = QM_RESET_UNKNOWN;
    int len = sizeof(int);

    // if (prodtst_config_get() == 1) {
    //     esp_log_set_level_master(ESP_LOG_NONE);
    //     qm_set_log_level(QM_LL_NONE);
    // }

    QM_LOGD("1", "Ver:%s", CONFIG_AUDIO_LIGHT_HW_VERSION);

    bool mute = false, awss = false;        // awss: device is reboot by netconfig or not
    int volume = 0, scr_tmo = 0, lang = 0;  // scr_tmo: screen timeout, lang: language
    int buffer_len = 0;

    qm_kv_get(QM_RESET_TAG, &qm_reset_flag, &len);
    qm_kv_set(QM_RESET_TAG, &flag, sizeof(int), QM_TRUE);

    /**
     *  init value from kv, set default value if failed
     */
    if (qm_kv_get(MUTE_KEY_NAME, &mute, &buffer_len)) {
        mute = false;
    }
    if (qm_kv_get(VOLUME_KEY_NAME, &volume, &buffer_len)) {
        volume = 70;
    }
    if (qm_kv_get(SCR_TMO_KEY_NAME, &scr_tmo, &buffer_len)) {
        scr_tmo = 0;
    }
    if (qm_kv_get(LANG_KEY_NAME, &lang, &buffer_len)) {
        lang = 0;
    }
    if (qm_kv_get(AWSS_KEY_NAME, &awss, &buffer_len)) {
        awss = 0;
    }
    if (awss) {
        qm_kv_del(AWSS_KEY_NAME);
    }

    lcd_init();

    // if (prodtst_config_get() == 0) {
    //     audio_light_init();
    // }
    
    audio_light_init();

#if (CONFIG_LCD_DISABLE_AUDIO == 0)
    audio_cloud_init();

    set_var_mute_value(mute, false);
    set_var_volume_value(volume, false);
#endif

    // if (audio_prodtst_init() == 1) {
    //     return;
    // }

#if (CONFIG_LCD_DISABLE_CLOUD == 0)
    xjiang_cloud_init();
#endif

    set_var_scr_tmo_value(scr_tmo, false);
    set_var_language_value(lang, false);
    set_var_awss_value(awss, false);
}
