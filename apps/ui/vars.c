#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lcd.h"

#include "bsp_light.h"
#include "es8311.h"
#include "qm_kv.h"
#include "audio_light.h"

// static char var_mode_nightlamp_title[33] = "小夜灯";
// static char var_mode_read_title[33] = "阅读";
// static char var_mode_relaxation_title[33] = "休闲";
// static char var_mode_sleep_title[33] = "睡觉";
// static char var_setup_volume_title[33] = "声音设置";
// static char var_setup_screen_title[33] = "自动息屏";
// static char var_setup_language_title[33] = "语言";
// static char var_setup_about_title[33] = "关于设备";
// static char var_setup_volume_mute_title[33] = "静音功能";
// static char var_setup_volume_adjust_title[33] = "音量调节";
// static char var_screen_tmo_30s_disp[33] = "30秒";
// static char var_screen_tmo_1min_disp[33] = "1分钟";
// static char var_screen_tmo_5min_disp[33] = "5分钟";
// static char var_screen_tmo_never_disp[33] = "永不";
// static char var_language_select_title[33] = "语言选择";
// static char var_setup_about_ip_addr_title[33] = "IP地址";
// static char var_setup_about_mac_addr_title[33] = "MAC地址";
// static char var_setup_about_hw_ver_title[33] = "硬件版本";
// static char var_setup_about_sw_ver_title[33] = "软件版本";
// static char var_setup_about_device_info_title[33] = "设备信息";
// static char var_setup_about_ip_addr_value[33] = "192.168.1.1";
// static char var_setup_about_mac_addr_value[33] = "E8:65:38:25:5A:2E";
// static char var_setup_about_hw_ver_value[33] = "v1.0.1";
// static char var_setup_about_sw_ver_value[33] = "v1.0.8";
// static char var_setup_about_device_info_value[33] = "--";

static char var_city[64] = "宁波";
static int8_t var_weather_id = 0;
static int8_t var_temperature = 25;

static char var_subtitles[512] = { 0 };

static int32_t  var_scr_tmo = 0;
static uint32_t var_turnoff = 0;
static int32_t  var_language = 0;
// static int8_t   last_emotion = 0;
static int8_t   var_emotion_id = 0;
static bool     var_awss = false;
static int8_t   var_ota_status = 0; // 0 - process, 1 - success, 2 - failure
static int8_t   var_ota_percent = 0;

static bool     var_recog_page_leave = 0; // 从识别页离开
static uint32_t var_screen_operation_tick = 0;

static char     var_ip_addr[32];

char *get_var_city(void) {
    return var_city;
}

void set_var_city(char *value, size_t size) {
    lv_strncpy(var_city, value, size);
}

int8_t get_var_weather_id(void) {
    return var_weather_id;
}

void set_var_weather_id(int8_t value) {
    var_weather_id = value;
}

int8_t get_var_temperature(void) {
    return var_temperature;
}

void set_var_temperature(int8_t value) {
    var_temperature = value;
}

bool get_var_brightness_onoff(void) {
    return bsp_light_store_get()->on_off;
}

void set_var_brightness_onoff(bool value) {
    if (bsp_light_store_get()->on_off != value) {
        bsp_light_bri_on_off(value);
        audio_light_snapshot_report();
    }
}

int32_t get_var_brightness_top() {
    int32_t up_bri = bsp_light_store_get()->up_bri / 100;
    return (bsp_light_store_get()->on_off ? up_bri : 0);
}

// flag: false - 不更新灯的亮度, true - 更新灯的亮度
void set_var_brightness_top(int32_t value) {
    bsp_light_bri_up_set(value);
    audio_light_snapshot_report();
}

int32_t get_var_brightness_bottom() {
    int32_t down_bri = bsp_light_store_get()->down_bri / 100;
    return (bsp_light_store_get()->on_off ? down_bri : 0);
}

// flag: false - 不更新灯的亮度, true - 更新灯的亮度
void set_var_brightness_bottom(int32_t value) {
    bsp_light_bri_down_set(value);
    audio_light_snapshot_report();
}

int8_t get_var_light_mode(void) {
    return bsp_light_store_get()->mode;
}

void set_var_light_mode(int8_t value, bool flag) {
    bsp_light_mode(value);
    audio_light_snapshot_report();
}

bool get_var_mute_value(void) {
    int mute = 0;
#if (CONFIG_LCD_DISABLE_AUDIO == 0)
    mute = bsp_audio_get_voice_mute();
#endif
    return ( mute != 0 );
}

void set_var_mute_value(bool value, bool flag) {
#if (CONFIG_LCD_DISABLE_AUDIO == 0)
    bsp_audio_set_voice_mute(value);
    if (flag) {
        qm_kv_set(MUTE_KEY_NAME, &value, sizeof(bool), 1);
    }
#endif
}

int32_t get_var_volume_value(void) {
    int32_t val = 0;
#if (CONFIG_LCD_DISABLE_AUDIO == 0)
    es8311_codec_get_voice_volume((int *)&val);
#endif
    return val;
}

void set_var_volume_value(int32_t value, bool flag) {
#if (CONFIG_LCD_DISABLE_AUDIO == 0)
    es8311_codec_set_voice_volume(value);
    if (flag) {
        qm_kv_set(VOLUME_KEY_NAME, &value, sizeof(int32_t), 1);
    }
#endif
}

int32_t get_var_scr_tmo_value(void) {
    return var_scr_tmo;
}

void set_var_scr_tmo_value(int32_t value, bool flag) {
    var_scr_tmo = value;
    if (flag) {
        qm_kv_set(SCR_TMO_KEY_NAME, &var_scr_tmo, sizeof(int32_t), 1);
    }
}

int32_t get_var_language_value() {
    return var_language;
}

void set_var_language_value(int32_t value, bool flag) {
    var_language = value;
    if (flag) {
        qm_kv_set(LANG_KEY_NAME, &var_language, sizeof(int32_t), 1);
    }
}

uint32_t get_var_turnoff_value(void) {
    return var_turnoff;
}

void set_var_turnoff_value(uint32_t value) {
    var_turnoff = value;
}

bool get_var_awss_value(void) {
    return var_awss;
}

void set_var_awss_value(bool value, bool flag) {
    var_awss = value;
    if (flag) {
        qm_kv_set(AWSS_KEY_NAME, &var_awss, sizeof(bool), 1);
    }
}

char *get_var_subtitles() {
    return var_subtitles;
}

void set_var_subtitles(const char *value) {
    if (value != NULL) {
        lv_strcpy(var_subtitles, value);
    }
}

int8_t get_var_emotion_id(void) {
    return var_emotion_id;
}

void set_var_emotion_id(int8_t value) {
    var_emotion_id = value;
}

int8_t get_var_ota_status(void) {
    return var_ota_status;
}

#include "esp_lvgl_port.h"

extern void goto_ota_process_page(void);
extern void goto_ota_success_page(void);
extern void goto_ota_failure_page(void);

void set_var_ota_status(int8_t value, bool need_lock) {
    var_ota_status = value;
    if (need_lock) {
        lvgl_port_lock(0);
    }
    switch (value) {
        case 1: goto_ota_success_page(); break;
        case 2: goto_ota_failure_page(); break;
        default: goto_ota_process_page(); break;
    }
    if (need_lock) {
        lvgl_port_unlock();
    }
}

int8_t get_var_ota_percent(void) {
    return var_ota_percent;
}

void set_var_ota_percent(int8_t value) {
    var_ota_percent = value;
}

bool get_var_recog_page_leave(void) {
    return var_recog_page_leave;
}

void set_var_recog_page_leave(bool value) {
    var_recog_page_leave = value;
    var_screen_operation_tick = qm_now_ms();
}

uint32_t get_var_screen_operation_tick(void) {
    return var_screen_operation_tick;
}

void set_var_screen_operation_tick(void) {
    var_screen_operation_tick = qm_now_ms();
}

char *get_var_ip_addr(void) {
    return var_ip_addr;
}

void set_var_ip_addr(char *ip_addr) {
    strcpy(var_ip_addr, ip_addr);
}