
#include "qm.h"
#include "qm_kv.h"

#include "xjiang_cloud.h"

#include "audio_main.h"
#include "audio_cloud.h"
#include "audio_light.h"
#include "audio_beacon.h"

#include "lcd.h"

void qm_application_start(void)
{    

    QM_LOGD("1", "Ver:%s", CONFIG_AUDIO_LIGHT_HW_VERSION);

    bool mute = false, awss = false;
    int buffer_len = 0, volume = 0, scr_tmo = 0, lang = 0;
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

    audio_light_init();

    // audio_beacon_init();

#if (CONFIG_LCD_DISABLE_AUDIO == 0)
    audio_cloud_init(); // codec init
    // 音量/静音初始化(from kv)
    set_var_mute_value(mute, false);
    set_var_volume_value(volume, false);
#endif

#if (CONFIG_LCD_DISABLE_CLOUD == 0)
        xjiang_cloud_init();
#endif

    // 屏幕超时
    set_var_scr_tmo_value(scr_tmo, false);
    // 语言
    set_var_language_value(lang, false);
    // 配网标志
    set_var_awss_value(awss, false);
}
