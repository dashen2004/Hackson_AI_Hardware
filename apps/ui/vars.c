#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "screens.h"

#include "bsp_light.h"

static char var_mode_nightlamp_title[33] = "小夜灯";
static char var_mode_read_title[33] = "阅读";
static char var_mode_relaxation_title[33] = "休闲";
static char var_mode_sleep_title[33] = "睡觉";
static char var_setup_volume_title[33] = "声音设置";
static char var_setup_screen_title[33] = "自动息屏";
static char var_setup_language_title[33] = "语言";
static char var_setup_about_title[33] = "关于设备";
static char var_setup_volume_mute_title[33] = "静音功能";
static char var_setup_volume_adjust_title[33] = "音量调节";
static char var_screen_tmo_30s_disp[33] = "30秒";
static char var_screen_tmo_1min_disp[33] = "1分钟";
static char var_screen_tmo_5min_disp[33] = "5分钟";
static char var_screen_tmo_never_disp[33] = "永不";
static char var_language_select_title[33] = "语言选择";
static char var_setup_about_ip_addr_title[33] = "IP地址";
static char var_setup_about_mac_addr_title[33] = "MAC地址";
static char var_setup_about_hw_ver_title[33] = "硬件版本";
static char var_setup_about_sw_ver_title[33] = "软件版本";
static char var_setup_about_device_info_title[33] = "设备信息";
static char var_setup_about_ip_addr_value[33] = "192.168.1.1";
static char var_setup_about_mac_addr_value[33] = "E8:65:38:25:5A:2E";
static char var_setup_about_hw_ver_value[33] = "v1.0.1";
static char var_setup_about_sw_ver_value[33] = "v1.0.8";
static char var_setup_about_device_info_value[33] = "--";

static char var_setup_volume_value_s[33] = "50%";
static char var_brightness_top_s[33] = "50%";
static char var_brightness_bottom_s[33] = "50%";

static char var_subtitles[512] = { 0 };

static char var_time[33];
static char var_week[33];
static char var_date[33];
static char var_city[33];
static char var_weather[33];
static char var_temperature[33];
// static uint8_t mode = 0; // 0 - unknown, 1 - 小夜灯, 2 - 阅读, 3 - 休闲, 4 - 睡觉
static bool var_setup_volume_mute_onoff = false;
static int32_t var_setup_volume_value = 50;
// static int32_t var_brightness_top = 50;
// static int32_t var_brightness_bottom = 50;
// static bool var_brightness_light_onoff = false;
static int32_t var_screen_tmo = 0;
static int32_t var_language = 0;

static int8_t  last_emotion = 0;
static int8_t  var_emotion  = 0;

const char *get_var_time() {
    return var_time;
}

void set_var_time(const char *value) {
    strcpy(var_time, value);
}

const char *get_var_week() {
    return var_week;
}

void set_var_week(const char *value) {
    strcpy(var_week, value);
}

const char *get_var_date() {
    return var_date;
}

void set_var_date(const char *value) {
    strcpy(var_date, value);
}

const char *get_var_city() {
    return var_city;
}

void set_var_city(const char *value) {
    strcpy(var_city, value);
}

const char *get_var_weather() {
    return var_weather;
}

void set_var_weather(const char *value) {
    strcpy(var_weather, value);
}

const char *get_var_temperature() {
    return var_temperature;
}

void set_var_temperature(const char *value) {
    strcpy(var_temperature, value);
}

// bool get_var_mode_nightlamp_checked() {
//     return (mode == 1);
// }

// void set_var_mode_nightlamp_checked(bool value) {
//     if (value) {
//         mode = 1;
//     } else if (mode == 1) {
//         mode = 0;
//     }
// }

const char *get_var_mode_nightlamp_title() {
    return var_mode_nightlamp_title;
}

void set_var_mode_nightlamp_title(const char *value) {
    strcpy(var_mode_nightlamp_title, value);
}

// bool get_var_mode_read_checked() {
//     return (mode == 2);
// }

// void set_var_mode_read_checked(bool value) {
//     if (value) {
//         mode = 2;
//     } else if (mode == 2) {
//         mode = 0;
//     }
// }

const char *get_var_mode_read_title() {
    return var_mode_read_title;
}

void set_var_mode_read_title(const char *value) {
    strcpy(var_mode_read_title, value);
}

// bool get_var_mode_relaxation_checked() {
//     return (mode == 3);
// }

// void set_var_mode_relaxation_checked(bool value) {
//     if (value) {
//         mode = 3;
//     } else if (mode == 3) {
//         mode = 0;
//     }
// }

const char *get_var_mode_relaxation_title() {
    return var_mode_relaxation_title;
}

void set_var_mode_relaxation_title(const char *value) {
    strcpy(var_mode_relaxation_title, value);
}

// bool get_var_mode_sleep_checked() {
//     return (mode == 4);
// }

// void set_var_mode_sleep_checked(bool value) {
//     if (value) {
//         mode = 4;
//     } else if (mode == 4) {
//         mode = 0;
//     }
// }

const char *get_var_mode_sleep_title() {
    return var_mode_sleep_title;
}

void set_var_mode_sleep_title(const char *value) {
    strcpy(var_mode_sleep_title, value);
}

const char *get_var_setup_volume_title() {
    return var_setup_volume_title;
}

void set_var_setup_volume_title(const char *value) {
    strcpy(var_setup_volume_title, value);
}

const char *get_var_setup_screen_title() {
    return var_setup_screen_title;
}

void set_var_setup_screen_title(const char *value) {
    strcpy(var_setup_screen_title, value);
}

const char *get_var_setup_language_title() {
    return var_setup_language_title;
}

void set_var_setup_language_title(const char *value) {
    strcpy(var_setup_language_title, value);
}

const char *get_var_setup_about_title() {
    return var_setup_about_title;
}

void set_var_setup_about_title(const char *value) {
    strcpy(var_setup_about_title, value);
}

const char *get_var_setup_volume_mute_title() {
    return var_setup_volume_mute_title;
}

void set_var_setup_volume_mute_title(const char *value) {
    strcpy(var_setup_volume_mute_title, value);
}

extern void set_var_setup_volume_mute_onoff(bool value);

bool get_var_setup_volume_mute_onoff() {
    return var_setup_volume_mute_onoff;
}

void set_var_setup_volume_mute_onoff(bool value) {
    if (var_setup_volume_mute_onoff != value) {
        var_setup_volume_mute_onoff = value;
        if (!var_setup_volume_mute_onoff) {
            lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
            lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
            lv_obj_clear_state(objects.setup_volume_value, LV_STATE_DISABLED);
            lv_obj_add_flag(objects.setup_volume_value, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0xff23f9c1), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0xff565656), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/setup_volume_mute_open.bin", NULL);
            lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/setup_volume_mute_open.bin", NULL);
            lv_obj_add_state(objects.setup_volume_value, LV_STATE_DISABLED);
            lv_obj_remove_flag(objects.setup_volume_value, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0x024F3B), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

const char *get_var_setup_volume_adjust_title() {
    return var_setup_volume_adjust_title;
}

void set_var_setup_volume_adjust_title(const char *value) {
    strcpy(var_setup_volume_adjust_title, value);
}

int32_t get_var_setup_volume_value() {
    return var_setup_volume_value;
}

void set_var_setup_volume_value(int32_t value) {
    var_setup_volume_value = value;
}

const char *get_var_setup_volume_value_s() {
    sprintf(var_setup_volume_value_s, "%ld%%", var_setup_volume_value);
    return var_setup_volume_value_s;
}

void set_var_setup_volume_value_s(const char *value) {
    // do nothing
}

int32_t get_var_brightness_top() {
    bsp_light_store_t *p = bsp_light_store_get();
    return ( p->up_bri / 100 );
//    return var_brightness_top;
}

// static int32_t var_old_brightness_top = 0;
// static int32_t var_old_brightness_bottom = 0;

void set_var_brightness_top(int32_t value) {
    // if (!var_brightness_light_onoff) {
    //     var_brightness_top = 0;
    // } else {
    //     var_brightness_top = value;
    // }
    // var_old_brightness_top = value;
    printf("value: %ld\n", value);
    bsp_light_bri_up_set(value);
}

const char *get_var_brightness_top_s() {
    bsp_light_store_t *p = bsp_light_store_get();
    if (!p->on_off || p->up_bri == 0) {
        sprintf(var_brightness_top_s, "关闭");
    } else {
        sprintf(var_brightness_top_s, "%d%%", p->up_bri / 100);
    }
    return var_brightness_top_s;
}

void set_var_brightness_top_s(const char *value) {
    // do nothing
}

int32_t get_var_brightness_bottom() {
    bsp_light_store_t *p = bsp_light_store_get();
    return (p->down_bri / 100);
//    return var_brightness_bottom;
}

void set_var_brightness_bottom(int32_t value) {
    // if (!var_brightness_light_onoff) {
    //     var_brightness_bottom = 0;
    // } else {
    //     var_brightness_bottom = value;
    // }
    // var_old_brightness_bottom = value;
    printf("value: %ld\n", value);
    bsp_light_bri_down_set(value);
}

const char *get_var_brightness_bottom_s() {
    bsp_light_store_t *p = bsp_light_store_get();
    if (!p->on_off || p->down_bri == 0) {
        sprintf(var_brightness_bottom_s, "关闭");
    } else {
        sprintf(var_brightness_bottom_s, "%d%%", p->down_bri / 100);
    }
    return var_brightness_bottom_s;
}

void set_var_brightness_bottom_s(const char *value) {
    // do nothing
}

bool get_var_brightness_light_onoff() {
    bsp_light_store_t *p = bsp_light_store_get();
    return p->on_off;
}

void set_var_brightness_light_onoff(bool value) {
    bsp_light_store_t *p = bsp_light_store_get();
//    if (p->on_off != value) {
        // var_brightness_light_onoff = value;
        if (!value) {
            // var_old_brightness_top = var_brightness_top;
            // var_old_brightness_bottom = var_brightness_bottom;
            // var_brightness_top = var_brightness_bottom = 0;
            lv_img_set_src(objects.brightness_light_onoff, "/lvgl/images/brightness_light_off.bin");
            lv_obj_add_state(objects.brightness_top, LV_STATE_DISABLED);
            lv_obj_remove_flag(objects.brightness_top, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_state(objects.brightness_bottom, LV_STATE_DISABLED);
            lv_obj_remove_flag(objects.brightness_bottom, LV_OBJ_FLAG_CLICKABLE);
            // bsp_light_bri_up_set(var_brightness_top);
            // bsp_light_bri_down_set(var_brightness_bottom);
            bsp_light_bri_on_off(false);
        } else {
            // var_brightness_top = var_old_brightness_top;
            // var_brightness_bottom = var_old_brightness_bottom;
            lv_img_set_src(objects.brightness_light_onoff, "/lvgl/images/brightness_light_on.bin");
            lv_obj_clear_state(objects.brightness_top, LV_STATE_DISABLED);
            lv_obj_add_flag(objects.brightness_top, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_state(objects.brightness_bottom, LV_STATE_DISABLED);
            lv_obj_add_flag(objects.brightness_bottom, LV_OBJ_FLAG_CLICKABLE);
            bsp_light_bri_on_off(true);
            // bsp_light_bri_up_set(var_brightness_top);
            // bsp_light_bri_down_set(var_brightness_bottom);
        }
    // }
}

int32_t get_var_mode() {
    bsp_light_store_t *p = bsp_light_store_get();
    return (p->mode);
    // return mode;
}

void set_var_mode(int32_t value) {
    bsp_light_store_t *p = bsp_light_store_get();
   if (p->mode != value) {
        bsp_light_mode(value);
   }
    switch (value) {
        case BSP_LIGHT_MODE_NIGHT: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
        case BSP_LIGHT_MODE_READ: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
        case BSP_LIGHT_MODE_IDLE: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
        case BSP_LIGHT_MODE_SLEEP: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_checked.bin", NULL);
            break;
        }
        default: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
    }
}

int32_t get_var_screen_tmo() {
    return var_screen_tmo;
}

void set_var_screen_tmo(int32_t value) {
    if (var_screen_tmo != value) {
        var_screen_tmo = value;
        switch (var_screen_tmo) {
            case 1:
                lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
            case 2:
                lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
            case 3:
                lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
            default: // 0
                lv_obj_remove_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
        }
    }
}

int32_t get_var_language() {
    return var_language;
}

void set_var_language(int32_t value) {
    if (var_language != value) {
        var_language = value;
        switch (var_language) {
            case 1:
                lv_obj_add_flag(objects.setup_language_cn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(objects.setup_language_en, LV_OBJ_FLAG_HIDDEN);
                break;
            default: // 0
                lv_obj_remove_flag(objects.setup_language_cn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_language_en, LV_OBJ_FLAG_HIDDEN);
                break;
        }
    }
}

const char *get_var_screen_tmo_30s_disp() {
    return var_screen_tmo_30s_disp;
}

void set_var_screen_tmo_30s_disp(const char *value) {
    strcpy(var_screen_tmo_30s_disp, value);
}

const char *get_var_screen_tmo_1min_disp() {
    return var_screen_tmo_1min_disp;
}

void set_var_screen_tmo_1min_disp(const char *value) {
    strcpy(var_screen_tmo_1min_disp, value);
}

const char *get_var_screen_tmo_5min_disp() {
    return var_screen_tmo_5min_disp;
}

void set_var_screen_tmo_5min_disp(const char *value) {
    strcpy(var_screen_tmo_5min_disp, value);
}

const char *get_var_screen_tmo_never_disp() {
    return var_screen_tmo_never_disp;
}

void set_var_screen_tmo_never_disp(const char *value) {
    strcpy(var_screen_tmo_never_disp, value);
}

const char *get_var_language_select_title() {
    return var_language_select_title;
}

void set_var_language_select_title(const char *value) {
    strcpy(var_language_select_title, value);
}

const char *get_var_setup_about_ip_addr_title() {
    return var_setup_about_ip_addr_title;
}

void set_var_setup_about_ip_addr_title(const char *value) {
    strcpy(var_setup_about_ip_addr_title, value);
}

const char *get_var_setup_about_ip_addr_value() {
    return var_setup_about_ip_addr_value;
}

void set_var_setup_about_ip_addr_value(const char *value) {
    strcpy(var_setup_about_ip_addr_value, value);
}

const char *get_var_setup_about_mac_addr_title() {
    return var_setup_about_mac_addr_title;
}

void set_var_setup_about_mac_addr_title(const char *value) {
    strcpy(var_setup_about_mac_addr_title, value);
}

const char *get_var_setup_about_mac_addr_value() {
    return var_setup_about_mac_addr_value;
}

void set_var_setup_about_mac_addr_value(const char *value) {
    strcpy(var_setup_about_mac_addr_value, value);
}

const char *get_var_setup_about_hw_ver_title() {
    return var_setup_about_hw_ver_title;
}

void set_var_setup_about_hw_ver_title(const char *value) {
    strcpy(var_setup_about_hw_ver_title, value);
}

const char *get_var_setup_about_hw_ver_value() {
    return var_setup_about_hw_ver_value;
}

void set_var_setup_about_hw_ver_value(const char *value) {
    strcpy(var_setup_about_hw_ver_value, value);
}

const char *get_var_setup_about_sw_ver_title() {
    return var_setup_about_sw_ver_title;
}

void set_var_setup_about_sw_ver_title(const char *value) {
    strcpy(var_setup_about_sw_ver_title, value);
}

const char *get_var_setup_about_sw_ver_value() {
    return var_setup_about_sw_ver_value;
}

void set_var_setup_about_sw_ver_value(const char *value) {
    strcpy(var_setup_about_sw_ver_value, value);
}

const char *get_var_setup_about_device_info_title() {
    return var_setup_about_device_info_title;
}

void set_var_setup_about_device_info_title(const char *value) {
    strcpy(var_setup_about_device_info_title, value);
}

const char *get_var_setup_about_device_info_value() {
    return var_setup_about_device_info_value;
}

void set_var_setup_about_device_info_value(const char *value){
    strcpy(var_setup_about_device_info_value, value);
}

const char *get_var_subtitles() {
    return var_subtitles;
}

void set_var_subtitles(const char *value) {
    if (value != NULL) {
        strcpy(var_subtitles, value);
    }
}

bool is_emotion_changed(void) {
    return (last_emotion != var_emotion);
}

int8_t get_var_emotion(void) {
    return var_emotion;
}

void set_var_emotion(const int8_t emotion) {
    var_emotion = emotion;
    // printf("emotion: %d, %d\n", var_emotion, last_emotion);
}

void update_last_emotion(void) {
    last_emotion = var_emotion;
}