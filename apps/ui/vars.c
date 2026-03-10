#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp_lcd.h"

#include "qm_kv.h"
#include "esp_lvgl_port.h"

#include "qm_utils_timer.h"

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

static char var_city[64] = {0};
static int8_t var_weather_id = -1;
static int8_t var_temperature = 25;

static int8_t var_tomato_state = TOMATO_STATE_IDLE;

static char var_subtitles[512] = { 0 };

static int32_t  var_scr_tmo = 0;
static qm_utils_time_t var_turnoff = {0};
static int32_t  var_language = 0;
// static int8_t   last_emotion = 0;
static int8_t   var_emotion_id = 0;
static bool     var_awss = false;
static int8_t   var_ota_status = 0; // 0 - process, 1 - success, 2 - failure
static int8_t   var_ota_percent = 0;

static bool     var_recog_page_leave = 0; // 从识别页离开
static qm_utils_time_t var_screen_operation_tick = {0};

static char     var_ip_addr[32];

int8_t lvgl_get_var_tomato_state(void) {
    return var_tomato_state;
}

void lvgl_set_var_tomato_state(int8_t state) {
    if (state <= TOMATO_STATE_NONE || state >= TOMATO_STATE_MAX) {
        return;
    }

    var_tomato_state = state;
}

char *get_var_city(void) {
    return var_city;
}

void set_var_city(char *value, size_t size) {
    lv_strncpy(var_city, value, size);
}

int32_t get_var_scr_tmo_value(void) {
    return var_scr_tmo;
}

void set_var_scr_tmo_value(int32_t value, bool flag) {
    var_scr_tmo = value;
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    if (flag) {
        // qm_kv_set(SCR_TMO_KEY_NAME, &var_scr_tmo, sizeof(int32_t), 1);
    }
}

uint32_t get_var_turnoff_value(void) {
    return qm_utils_time_is_expired(&var_turnoff);
}

void set_var_turnoff_value(uint32_t value) {
    qm_utils_time_countdown_ms(&var_turnoff, value);
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

int8_t get_var_emotion_id(void) {
    return var_emotion_id;
}

void set_var_emotion_id(int8_t value) {
    var_emotion_id = value;
}

uint32_t get_var_screen_operation_tick(void) {
    return qm_utils_time_is_expired(&var_screen_operation_tick);
}

void set_var_screen_operation_tick(void) {
    qm_utils_time_countdown_ms(&var_screen_operation_tick, 5 * 1000);
}
