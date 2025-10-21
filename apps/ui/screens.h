#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *page_main;
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
    lv_obj_t *text_time;
    lv_obj_t *text_weekday;
    lv_obj_t *text_date;
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
    lv_obj_t *rgb_bar_slider;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_PAGE_MAIN = 1,
    SCREEN_ID_PAGE_MENU = 2,
    SCREEN_ID_PAGE_MODE = 3,
    SCREEN_ID_PAGE_BRIGHTNESS = 4,
    SCREEN_ID_PAGE_SETUP = 5,
    SCREEN_ID_PAGE_SETUP_VOLUME = 6,
    SCREEN_ID_PAGE_SETUP_SCREEN = 7,
    SCREEN_ID_PAGE_SETUP_LANGUAGE = 8,
    SCREEN_ID_PAGE_SETUP_ABOUT = 9,
    SCREEN_ID_PAGE_NETCONFIG = 10,
    SCREEN_ID_PAGE_OTA = 11,
    SCREEN_ID_PAGE_RECOG = 12,
    SCREEN_ID_PAGE_RGB = 13,
};

void create_screen_page_main();
void tick_screen_page_main();

void create_screen_page_menu();
void tick_screen_page_menu();

void create_screen_page_mode();
void tick_screen_page_mode();

void create_screen_page_brightness();
void tick_screen_page_brightness();

void create_screen_page_setup();
void tick_screen_page_setup();

void create_screen_page_setup_volume();
void tick_screen_page_setup_volume();

void create_screen_page_setup_screen();
void tick_screen_page_setup_screen();

void create_screen_page_setup_language();
void tick_screen_page_setup_language();

void create_screen_page_setup_about();
void tick_screen_page_setup_about();

void create_screen_page_netconfig();
void tick_screen_page_netconfig();

void create_screen_page_ota();
void tick_screen_page_ota();

void create_screen_page_recog();
void tick_screen_page_recog();

void create_screen_page_rgb();
void tick_screen_page_rgb();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/