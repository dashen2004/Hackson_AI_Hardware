#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "screens.h"

#include "bsp_light.h"
#include "es8311.h"
#include "qm_kv.h"

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

static char var_subtitles[512] = { 0 };

//static char var_time[33];
//static char var_week[33];
//static char var_date[33];
//static char var_city[33];
//static char var_weather[33];
//static char var_temperature[33];
// static uint8_t mode = 0; // 0 - unknown, 1 - 小夜灯, 2 - 阅读, 3 - 休闲, 4 - 睡觉
// static bool var_setup_volume_mute_onoff = false;
static int32_t  var_volume_value = 50;
static int32_t  var_brightness_top = 50;
static int32_t  var_brightness_bottom = 50;
static bool     var_mute = false;
static int32_t  var_scr_tmo = 0;
static uint32_t var_turnoff = 0;
static int32_t  var_language = 0;

static int8_t  last_emotion = 0;
static int8_t  var_emotion  = 0;

static bool    var_awss = false;

// void set_var_setup_volume_mute_onoff(bool value) {
//     // if (var_setup_volume_mute_onoff != value) {
//     //     var_setup_volume_mute_onoff = value;
//     //     if (!var_setup_volume_mute_onoff) {
//     //         lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
//     //         lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
//     //         lv_obj_clear_state(objects.setup_volume_value, LV_STATE_DISABLED);
//     //         lv_obj_add_flag(objects.setup_volume_value, LV_OBJ_FLAG_CLICKABLE);
//     //         lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0xff23f9c1), LV_PART_INDICATOR | LV_STATE_DEFAULT);
//     //         lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0xff565656), LV_PART_MAIN | LV_STATE_DEFAULT);
//     //     } else {
//     //         lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/setup_volume_mute_open.bin", NULL);
//     //         lv_imagebutton_set_src(objects.setup_volume_mute_onoff, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/setup_volume_mute_open.bin", NULL);
//     //         lv_obj_add_state(objects.setup_volume_value, LV_STATE_DISABLED);
//     //         lv_obj_remove_flag(objects.setup_volume_value, LV_OBJ_FLAG_CLICKABLE);
//     //         lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0x024F3B), LV_PART_INDICATOR | LV_STATE_DEFAULT);
//     //         lv_obj_set_style_bg_color(objects.setup_volume_value, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
//     //     }
//     // }
// }

#define MUTE_KEY_NAME       "mute"
#define VOLUME_KEY_NAME     "volume"
#define SCR_TMO_KEY_NAME    "scr_tmo"
#define LANG_KEY_NAME       "lang"
#define AWSS_KEY_NAME       "awss"

bool get_var_mute_value() {
    return var_mute;
}

void set_var_mute_value(bool value, bool flag) {
    var_mute = value;
    es8311_set_voice_mute(value);
    if (flag) {
        qm_kv_set(MUTE_KEY_NAME, &var_mute, sizeof(bool), 1);
    }
}

int32_t get_var_volume_value() {
    return var_volume_value;
}

void set_var_volume_value(int32_t value, bool flag) {
    var_volume_value = value;
    es8311_codec_set_voice_volume(value);
    if (flag) {
        qm_kv_set(VOLUME_KEY_NAME, &var_volume_value, sizeof(int32_t), 1);
    }
}

// const char *get_var_volume_value_s() {
//     sprintf(var_setup_volume_value_s, "%ld%%", var_setup_volume_value);
//     return var_setup_volume_value_s;
// }

// void set_var_setup_volume_value_s(const char *value) {
//     // do nothing
// }

int32_t get_var_brightness_top() {
    return var_brightness_top;
}

// flag: false - 不更新灯的亮度, true - 更新灯的亮度
void set_var_brightness_top(int32_t value, bool flag) {
    var_brightness_top = value;
    if (flag) {
        // printf("bsp_light_bri_up_set - %ld\n", value);
        bsp_light_bri_up_set(value);
    }
}

int32_t get_var_brightness_bottom() {
    return var_brightness_bottom;
}

// flag: false - 不更新灯的亮度, true - 更新灯的亮度
void set_var_brightness_bottom(int32_t value, bool flag) {
    var_brightness_bottom = value;
    if (flag) {
        // printf("bsp_light_bri_up_set - %ld\n", value);
        bsp_light_bri_down_set(value);
    }
}

// const char *get_var_brightness_bottom_s() {
//     bsp_light_store_t *p = bsp_light_store_get();
//     if (!p->on_off || p->down_bri == 0) {
//         sprintf(var_brightness_bottom_s, "关闭");
//     } else {
//         sprintf(var_brightness_bottom_s, "%d%%", p->down_bri / 100);
//     }
//     return var_brightness_bottom_s;
// }

// void set_var_brightness_bottom_s(const char *value) {
//     // do nothing
// }

// bool get_var_brightness_light_onoff() {
//     bsp_light_store_t *p = bsp_light_store_get();
//     return p->on_off;
// }

// void set_var_brightness_light_onoff(bool value) {
// //     bsp_light_store_t *p = bsp_light_store_get();
// // //    if (p->on_off != value) {
// //         // var_brightness_light_onoff = value;
// //         if (!value) {
// //             // var_old_brightness_top = var_brightness_top;
// //             // var_old_brightness_bottom = var_brightness_bottom;
// //             // var_brightness_top = var_brightness_bottom = 0;
// //             lv_img_set_src(objects.image_brightness_light_onoff, "/lvgl/images/brightness_light_off.bin");
// //             lv_obj_add_state(objects.brightness_top, LV_STATE_DISABLED);
// //             lv_obj_remove_flag(objects.brightness_top, LV_OBJ_FLAG_CLICKABLE);
// //             lv_obj_add_state(objects.brightness_bottom, LV_STATE_DISABLED);
// //             lv_obj_remove_flag(objects.brightness_bottom, LV_OBJ_FLAG_CLICKABLE);
// //             // bsp_light_bri_up_set(var_brightness_top);
// //             // bsp_light_bri_down_set(var_brightness_bottom);
// //             bsp_light_bri_on_off(false);
// //         } else {
// //             // var_brightness_top = var_old_brightness_top;
// //             // var_brightness_bottom = var_old_brightness_bottom;
// //             lv_img_set_src(objects.image_brightness_light_onoff, "/lvgl/images/brightness_light_on.bin");
// //             lv_obj_clear_state(objects.brightness_top, LV_STATE_DISABLED);
// //             lv_obj_add_flag(objects.brightness_top, LV_OBJ_FLAG_CLICKABLE);
// //             lv_obj_clear_state(objects.brightness_bottom, LV_STATE_DISABLED);
// //             lv_obj_add_flag(objects.brightness_bottom, LV_OBJ_FLAG_CLICKABLE);
// //             bsp_light_bri_on_off(true);
// //             // bsp_light_bri_up_set(var_brightness_top);
// //             // bsp_light_bri_down_set(var_brightness_bottom);
// //         }
// //     // }
// }

// int32_t get_var_screen_tmo() {
//     extern int screen_turnoff_timeout_get(void);
//     return screen_turnoff_timeout_get();
// }

// void set_var_screen_tmo(int32_t value) {
//     if (get_var_screen_tmo() != value) {
//         extern void screen_turnoff_timeout_set(int timeout);
//         screen_turnoff_timeout_set(value);
//         switch (value) {
//             case 1:
//                 lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_remove_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
//                 break;
//             case 2:
//                 lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_remove_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
//                 break;
//             case 3:
//                 lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_remove_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
//                 break;
//             default: // 0
//                 lv_obj_remove_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
//                 lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
//                 break;
//         }
//     }
// }

int32_t get_var_language_value() {
    return var_language;
}

void set_var_language_value(int32_t value, bool flag) {
    var_language = value;
    if (flag) {
        qm_kv_set(LANG_KEY_NAME, &var_language, sizeof(int32_t), 1);
    }
}

// const char *get_var_screen_tmo_30s_disp() {
//     return var_screen_tmo_30s_disp;
// }

// void set_var_screen_tmo_30s_disp(const char *value) {
//     strcpy(var_screen_tmo_30s_disp, value);
// }

// const char *get_var_screen_tmo_1min_disp() {
//     return var_screen_tmo_1min_disp;
// }

// void set_var_screen_tmo_1min_disp(const char *value) {
//     strcpy(var_screen_tmo_1min_disp, value);
// }

// const char *get_var_screen_tmo_5min_disp() {
//     return var_screen_tmo_5min_disp;
// }

// void set_var_screen_tmo_5min_disp(const char *value) {
//     strcpy(var_screen_tmo_5min_disp, value);
// }

// const char *get_var_screen_tmo_never_disp() {
//     return var_screen_tmo_never_disp;
// }

// void set_var_screen_tmo_never_disp(const char *value) {
//     strcpy(var_screen_tmo_never_disp, value);
// }

// const char *get_var_language_select_title() {
//     return var_language_select_title;
// }

// void set_var_language_select_title(const char *value) {
//     strcpy(var_language_select_title, value);
// }

// const char *get_var_setup_about_ip_addr_title() {
//     return var_setup_about_ip_addr_title;
// }

// void set_var_setup_about_ip_addr_title(const char *value) {
//     strcpy(var_setup_about_ip_addr_title, value);
// }

// const char *get_var_setup_about_ip_addr_value() {
//     return var_setup_about_ip_addr_value;
// }

// void set_var_setup_about_ip_addr_value(const char *value) {
//     strcpy(var_setup_about_ip_addr_value, value);
// }

// const char *get_var_setup_about_mac_addr_title() {
//     return var_setup_about_mac_addr_title;
// }

// void set_var_setup_about_mac_addr_title(const char *value) {
//     strcpy(var_setup_about_mac_addr_title, value);
// }

// const char *get_var_setup_about_mac_addr_value() {
//     return var_setup_about_mac_addr_value;
// }

// void set_var_setup_about_mac_addr_value(const char *value) {
//     strcpy(var_setup_about_mac_addr_value, value);
// }

// const char *get_var_setup_about_hw_ver_title() {
//     return var_setup_about_hw_ver_title;
// }

// void set_var_setup_about_hw_ver_title(const char *value) {
//     strcpy(var_setup_about_hw_ver_title, value);
// }

// const char *get_var_setup_about_hw_ver_value() {
//     return var_setup_about_hw_ver_value;
// }

// void set_var_setup_about_hw_ver_value(const char *value) {
//     strcpy(var_setup_about_hw_ver_value, value);
// }

// const char *get_var_setup_about_sw_ver_title() {
//     return var_setup_about_sw_ver_title;
// }

// void set_var_setup_about_sw_ver_title(const char *value) {
//     strcpy(var_setup_about_sw_ver_title, value);
// }

// const char *get_var_setup_about_sw_ver_value() {
//     return var_setup_about_sw_ver_value;
// }

// void set_var_setup_about_sw_ver_value(const char *value) {
//     strcpy(var_setup_about_sw_ver_value, value);
// }

// const char *get_var_setup_about_device_info_title() {
//     return var_setup_about_device_info_title;
// }

// void set_var_setup_about_device_info_title(const char *value) {
//     strcpy(var_setup_about_device_info_title, value);
// }

// const char *get_var_setup_about_device_info_value() {
//     return var_setup_about_device_info_value;
// }

// void set_var_setup_about_device_info_value(const char *value){
//     strcpy(var_setup_about_device_info_value, value);
// }

int32_t get_var_scr_tmo_value(void) {
    return var_scr_tmo;
}

void set_var_scr_tmo_value(int32_t value, bool flag) {
    var_scr_tmo = value;
    if (flag) {
        qm_kv_set(SCR_TMO_KEY_NAME, &var_scr_tmo, sizeof(int32_t), 1);
    }
}

uint32_t get_var_turnoff_value(void) {
    return var_turnoff;
}

void set_var_turnoff_value(uint32_t value) {
    var_turnoff = value;
}

bool get_var_awss_value(void) {
    // printf("var_awss: %d\n", var_awss);
    return var_awss;
}

void set_var_awss_value(bool value, bool flag) {
    var_awss = value;
    // printf("var_awss: %d\n", var_awss);
    if (flag) {
        qm_kv_set(AWSS_KEY_NAME, &var_awss, sizeof(bool), 1);
    }
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