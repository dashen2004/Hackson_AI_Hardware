#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *page_sleep;
    lv_obj_t *page_main;
    lv_obj_t *page_alarm;
    lv_obj_t *page_timer;
    lv_obj_t *page_pomodoro;
    lv_obj_t *page_weather;

    // sleep page objs
    lv_obj_t *img_expression;

    // main page objs
    lv_obj_t *wifi_state;
    lv_obj_t *bat_state;
    lv_obj_t *text_time;
    lv_obj_t *text_weekday;
    lv_obj_t *text_date;

    // alarm page objs
    lv_obj_t *label_alarm;
    lv_obj_t *container_add;
    lv_obj_t *img_add;
    lv_obj_t *list_alarm;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *obj24;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *obj29;
    lv_obj_t *obj30;
    lv_obj_t *obj31;
    lv_obj_t *obj32;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *obj35;

    // timer page objs
    lv_obj_t *label_count;
    lv_obj_t *container_start;
    lv_obj_t *img_start;
    lv_obj_t *container_reset;
    lv_obj_t *img_reset;
    lv_obj_t *container_stop;
    lv_obj_t *img_stop;
    lv_obj_t *btn_bg;
    lv_obj_t *container_count_down;
    lv_obj_t *btn_count_down;
    lv_obj_t *label_count_down;
    lv_obj_t *container_count_up;
    lv_obj_t *btn_count_up;
    lv_obj_t *label_count_up;

    // pomodoro page objs
    lv_obj_t *tomato_spinner;
    lv_obj_t *tomato_icon;
    lv_obj_t *tomato_label;
    lv_obj_t *container_tomato_opt;
    lv_obj_t *tomato_opt;

    // weather page objs
    lv_obj_t *label_city;
    lv_obj_t *img_weather;
    lv_obj_t *label_weather;
    lv_obj_t *label_temperature;

    lv_obj_t *page_menu;
    lv_obj_t *page_mode;
    lv_obj_t *page_brightness;
    lv_obj_t *page_setup;
    lv_obj_t *page_setup_volume;
    lv_obj_t *page_setup_screen;
    lv_obj_t *page_setup_language;
    lv_obj_t *page_setup_about;
    lv_obj_t *page_netconfig;
    lv_obj_t *page_ota;
    lv_obj_t *page_recog;
    lv_obj_t *page_rgb;
    lv_obj_t *slider_brightness_top;
    lv_obj_t *slider_brightness_bottom;
    lv_obj_t *slider_volume_value;
    lv_obj_t *bar_ota_percent;
    lv_obj_t *image_wifi;
    lv_obj_t *image_wifi_level_s_1;
    lv_obj_t *image_wifi_level_s_2;
    lv_obj_t *image_wifi_level_s_3;
    lv_obj_t *image_wifi_level_s_4;
    lv_obj_t *container_normal;
    lv_obj_t *image_weather_unknown;
    lv_obj_t *text_weather_unknown;
    lv_obj_t *image_weather;
    lv_obj_t *text_city;
    lv_obj_t *text_weather;
    lv_obj_t *text_temperature;
    // lv_obj_t *text_time;
    // lv_obj_t *text_weekday;
    // lv_obj_t *text_date;
    lv_obj_t *container_simple;
    lv_obj_t *image_logo;
    lv_obj_t *menu_bg;
    lv_obj_t *menu_home;
    lv_obj_t *menu_speech;
    lv_obj_t *menu_setup;
    lv_obj_t *menu_mode;
    lv_obj_t *menu_brightness;
    lv_obj_t *menu_rgb;
    lv_obj_t *mode_nightlamp;
    lv_obj_t *mode_read;
    lv_obj_t *mode_relaxation;
    lv_obj_t *mode_sleep;
    lv_obj_t *text_nightlamp;
    lv_obj_t *text_read;
    lv_obj_t *text_relaxation;
    lv_obj_t *text_sleep;
    lv_obj_t *image_brightness_light_onoff;
    lv_obj_t *slider_container;
    lv_obj_t *image_brightness_low1;
    lv_obj_t *image_brightness_high1;
    lv_obj_t *text_brightness_top_value;
    lv_obj_t *image_brightness_low2;
    lv_obj_t *image_brightness_high2;
    lv_obj_t *text_brightness_bottom_value;
    lv_obj_t *text_setup_volume;
    lv_obj_t *text_setup_screen;
    lv_obj_t *text_setup_language;
    lv_obj_t *text_setup_about;
    lv_obj_t *text_setup_wifi;
    lv_obj_t *text_setup_factoryreset;
    lv_obj_t *container_mask;
    lv_obj_t *submit_win;
    lv_obj_t *text_network_config_infomation;
    lv_obj_t *text_network_config_cancel;
    lv_obj_t *text_network_config_submit;
    lv_obj_t *text_volume_mute;
    lv_obj_t *text_volume_adjust;
    lv_obj_t *text_percent_0;
    lv_obj_t *text_percent_100;
    lv_obj_t *text_volume_value;
    lv_obj_t *volume_mute_onoff;
    lv_obj_t *screen_main_bg;
    lv_obj_t *text_screen_tmo_30s;
    lv_obj_t *setup_screen_tmo_30s;
    lv_obj_t *text_screen_tmo_1min;
    lv_obj_t *setup_screen_tmo_1min;
    lv_obj_t *text_screen_tmo_5min;
    lv_obj_t *setup_screen_tmo_5min;
    lv_obj_t *text_screen_tmo_never;
    lv_obj_t *setup_screen_tmo_never;
    lv_obj_t *text_language_select;
    lv_obj_t *language_select_bg;
    lv_obj_t *text_language_zh;
    lv_obj_t *setup_language_cn;
    lv_obj_t *text_language_en;
    lv_obj_t *setup_language_en;
    lv_obj_t *container_about_info;
    lv_obj_t *text_about_ip_addr;
    lv_obj_t *text_about_ip_addr_value;
    lv_obj_t *line_1;
    lv_obj_t *text_about_mac_addr;
    lv_obj_t *text_about_mac_addr_value;
    lv_obj_t *line_2;
    lv_obj_t *text_about_hw_ver;
    lv_obj_t *text_about_sw_ver;
    lv_obj_t *line_3;
    lv_obj_t *text_about_hw_ver_value;
    lv_obj_t *text_about_sw_ver_value;
    lv_obj_t *container_netconfig_reboot;
    lv_obj_t *text_netconfig_reboot_title;
    lv_obj_t *container_netconfig_prepare;
    lv_obj_t *image_netconfig_prepare_1;
    lv_obj_t *text_netconfig_prepare_title;
    lv_obj_t *container_netconfig_process;
    lv_obj_t *image_netconfig_prepare_2;
    lv_obj_t *image_netconfig_wifi_level_1;
    lv_obj_t *image_netconfig_wifi_level_2;
    lv_obj_t *image_netconfig_wifi_level_3;
    lv_obj_t *image_netconfig_wifi_level_4;
    lv_obj_t *text_netconfig_process_title;
    lv_obj_t *text_return_home;
    lv_obj_t *container_netconfig_success;
    lv_obj_t *image_netconfig_success;
    lv_obj_t *text_netconfig_success_title;
    lv_obj_t *text_netconfig_success_countdown;
    lv_obj_t *container_ota_process;
    lv_obj_t *text_ota_title;
    lv_obj_t *text_ota_subtitle;
    lv_obj_t *container_failure;
    lv_obj_t *text_ota_failure_title;
    lv_obj_t *text_ota_failure_code;
    lv_obj_t *image_recog_emotion;
    lv_obj_t *text_recog_subtitles;
    lv_obj_t *led_display;
    lv_obj_t *rgb_select_red;
    lv_obj_t *rgb_select_orange;
    lv_obj_t *rgb_select_blue;
    lv_obj_t *rgb_select_green;
    lv_obj_t *rgb_select_purple;
    lv_obj_t *rgb_select_colorful;
    lv_obj_t *rgb_selected;
    lv_obj_t *rgb_bar;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_PAGE_SLEEP = 1,
    SCREEN_ID_PAGE_MAIN = 2,
    SCREEN_ID_PAGE_ALARM = 3,
    SCREEN_ID_PAGE_TIMER = 4,
    SCREEN_ID_PAGE_POMODORO = 5,
    SCREEN_ID_PAGE_WEATHER = 6,

    SCREEN_ID_PAGE_MENU = 32,
    SCREEN_ID_PAGE_MODE = 42,
    SCREEN_ID_PAGE_BRIGHTNESS = 52,
    SCREEN_ID_PAGE_SETUP = 62,
    SCREEN_ID_PAGE_SETUP_VOLUME = 63,
    SCREEN_ID_PAGE_SETUP_SCREEN = 73,
    SCREEN_ID_PAGE_SETUP_LANGUAGE = 83,
    SCREEN_ID_PAGE_SETUP_ABOUT = 93,
    SCREEN_ID_PAGE_NETCONFIG = 103,
    SCREEN_ID_PAGE_OTA = 113,
    SCREEN_ID_PAGE_RECOG = 123,
    SCREEN_ID_PAGE_RGB = 133,
};

void create_screen_page_sleep();
void tick_screen_page_sleep();

void create_screen_page_main();
void tick_screen_page_main();

void create_screen_page_alarm();
void tick_screen_page_alarm();

void create_screen_page_timer();
void tick_screen_page_timer();

void create_screen_page_pomodoro();
void tick_screen_page_pomodoro();

void create_screen_page_weather();
void tick_screen_page_weather();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/