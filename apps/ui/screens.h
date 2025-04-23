#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *page_nowifi_main;
    lv_obj_t *page_main;
    lv_obj_t *page_menu;
    lv_obj_t *page_mode;
    lv_obj_t *page_brightness;
    lv_obj_t *page_setup;
    lv_obj_t *page_setup_volume;
    lv_obj_t *page_setup_screen;
    lv_obj_t *page_setup_language;
    lv_obj_t *page_setup_about;
    // lv_obj_t *page_recog;
    lv_obj_t *page_network_config_prepare;
    lv_obj_t *page_network_config_process;
    lv_obj_t *page_network_config_success;
    lv_obj_t *slider_brightness_top;
    lv_obj_t *slider_brightness_bottom;
    lv_obj_t *slider_volume_value;
    lv_obj_t *image_logo;
    lv_obj_t *image_wifi0;
    lv_obj_t *text_city;
    lv_obj_t *text_weather;
    lv_obj_t *text_temperature;
    lv_obj_t *text_weather_unknown;
    lv_obj_t *text_time;
    lv_obj_t *text_weekday;
    lv_obj_t *text_date;
    lv_obj_t *image_wifi1;
    lv_obj_t *image_weather;
    lv_obj_t *image_weather_unknown;
    lv_obj_t *menu_bg;
    lv_obj_t *menu_home;
    lv_obj_t *menu_speech;
    lv_obj_t *menu_setup;
    lv_obj_t *menu_mode;
    lv_obj_t *menu_brightness;
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
    lv_obj_t *text_about_ip_addr;
    lv_obj_t *text_about_mac_addr;
    lv_obj_t *text_about_hw_ver;
    lv_obj_t *text_about_sw_ver;
    lv_obj_t *text_about_device_info;
    lv_obj_t *text_about_ip_addr_value;
    lv_obj_t *text_about_mac_addr_value;
    lv_obj_t *text_about_hw_ver_value;
    lv_obj_t *text_about_sw_ver_value;
    lv_obj_t *text_about_device_info_value;
    lv_obj_t *emotion;
    lv_obj_t *subtitles;
    lv_obj_t *text_network_config_prepare;
    lv_obj_t *image_wifi_level_1;
    lv_obj_t *image_wifi_level_2;
    lv_obj_t *image_wifi_level_3;
    lv_obj_t *image_wifi_level_4;
    lv_obj_t *text_network_config_process;
    lv_obj_t *text_network_config_success;
    lv_obj_t *text_network_config_success_countdown;
    lv_obj_t *obj0;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_PAGE_NOWIFI_MAIN = 1,
    SCREEN_ID_PAGE_MAIN = 2,
    SCREEN_ID_PAGE_MENU = 3,
    SCREEN_ID_PAGE_MODE = 4,
    SCREEN_ID_PAGE_BRIGHTNESS = 5,
    SCREEN_ID_PAGE_SETUP = 6,
    SCREEN_ID_PAGE_SETUP_VOLUME = 7,
    SCREEN_ID_PAGE_SETUP_SCREEN = 8,
    SCREEN_ID_PAGE_SETUP_LANGUAGE = 9,
    SCREEN_ID_PAGE_SETUP_ABOUT = 10,
    SCREEN_ID_PAGE_NETWORK_CONFIG_PREPARE = 11,
    SCREEN_ID_PAGE_NETWORK_CONFIG_PROCESS = 12,
    SCREEN_ID_PAGE_NETWORK_CONFIG_SUCCESS = 13,
    // SCREEN_ID_PAGE_RECOG = 14,
};

void create_screen_page_nowifi_main();
void tick_screen_page_nowifi_main();

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

void create_screen_page_recog();
void tick_screen_page_recog();

void create_screen_page_network_config_prepare();
void tick_screen_page_network_config_prepare();

void create_screen_page_network_config_process();
void tick_screen_page_network_config_process();

void create_screen_page_network_config_success();
void tick_screen_page_network_config_success();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/