#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_wifi_connected;
extern const lv_img_dsc_t img_wifi_disconnected;
extern const lv_img_dsc_t img_menu_brightness_pressed;
extern const lv_img_dsc_t img_menu_brightness_released;
extern const lv_img_dsc_t img_menu_home_pressed;
extern const lv_img_dsc_t img_menu_home_released;
extern const lv_img_dsc_t img_menu_mode_pressed;
extern const lv_img_dsc_t img_menu_mode_released;
extern const lv_img_dsc_t img_menu_setup_pressed;
extern const lv_img_dsc_t img_menu_setup_released;
extern const lv_img_dsc_t img_menu_speech_pressed;
extern const lv_img_dsc_t img_menu_speech_released;
extern const lv_img_dsc_t img_mode_nightlamp_checked;
extern const lv_img_dsc_t img_mode_nightlamp_unchecked;
extern const lv_img_dsc_t img_mode_relaxation_checked;
extern const lv_img_dsc_t img_mode_relaxation_unchecked;
extern const lv_img_dsc_t img_mode_sleep_checked;
extern const lv_img_dsc_t img_mode_sleep_unchecked;
extern const lv_img_dsc_t img_mode_read_checked;
extern const lv_img_dsc_t img_mode_read_unchecked;
extern const lv_img_dsc_t img_setup_volume;
extern const lv_img_dsc_t img_setup_screen;
extern const lv_img_dsc_t img_setup_language;
extern const lv_img_dsc_t img_setup_about;
extern const lv_img_dsc_t img_setup_more;
extern const lv_img_dsc_t img_setup_volume_mute;
extern const lv_img_dsc_t img_setup_volume_mute_close;
extern const lv_img_dsc_t img_setup_volume_mute_open;
extern const lv_img_dsc_t img_setup_selected;
extern const lv_img_dsc_t img_brightness_light_off;
extern const lv_img_dsc_t img_brightness_light_on;
extern const lv_img_dsc_t img_brightness_low;
extern const lv_img_dsc_t img_brightness_high;
extern const lv_img_dsc_t img_weather_blizzard;
extern const lv_img_dsc_t img_weather_stormy;
extern const lv_img_dsc_t img_weather_hail;
extern const lv_img_dsc_t img_weather_heavy_snow;
extern const lv_img_dsc_t img_weather_heavy_rain;
extern const lv_img_dsc_t img_weather_day_cloudy;
extern const lv_img_dsc_t img_weather_night_cloudy;
extern const lv_img_dsc_t img_weather_windy;
extern const lv_img_dsc_t img_weather_thunder_shower;
extern const lv_img_dsc_t img_weather_tornado;
extern const lv_img_dsc_t img_weather_day_sunny;
extern const lv_img_dsc_t img_weather_night_sunny;
extern const lv_img_dsc_t img_weather_sand_storm;
extern const lv_img_dsc_t img_weather_foggy;
extern const lv_img_dsc_t img_weather_light_snow;
extern const lv_img_dsc_t img_weather_light_rain;
extern const lv_img_dsc_t img_weather_overcast;
extern const lv_img_dsc_t img_weather_sleet;
extern const lv_img_dsc_t img_weather_shower;
extern const lv_img_dsc_t img_weather_moderate_rain;
extern const lv_img_dsc_t img_emotion01;
extern const lv_img_dsc_t img_emotion02;
extern const lv_img_dsc_t img_emotion03;
extern const lv_img_dsc_t img_emotion04;
extern const lv_img_dsc_t img_emotion05;
extern const lv_img_dsc_t img_emotion06;
extern const lv_img_dsc_t img_emotion07;
extern const lv_img_dsc_t img_emotion08;
extern const lv_img_dsc_t img_emotion09;
extern const lv_img_dsc_t img_emotion10;
extern const lv_img_dsc_t img_emotion11;
extern const lv_img_dsc_t img_emotion12;
extern const lv_img_dsc_t img_emotion13;
extern const lv_img_dsc_t img_emotion14;
extern const lv_img_dsc_t img_emotion15;
extern const lv_img_dsc_t img_emotion16;
extern const lv_img_dsc_t img_emotion17;
extern const lv_img_dsc_t img_emotion18;
extern const lv_img_dsc_t img_emotion19;
extern const lv_img_dsc_t img_emotion20;
extern const lv_img_dsc_t img_emotion21;
extern const lv_img_dsc_t img_weather_none;
extern const lv_img_dsc_t img_logo;
extern const lv_img_dsc_t img_setup_wifi;
extern const lv_img_dsc_t img_network_config_prepare;
extern const lv_img_dsc_t img_wifi_level_1;
extern const lv_img_dsc_t img_wifi_level_2;
extern const lv_img_dsc_t img_wifi_level_3;
extern const lv_img_dsc_t img_wifi_level_4;
extern const lv_img_dsc_t img_network_config_success;
extern const lv_img_dsc_t img_wifi_level_s_1;
extern const lv_img_dsc_t img_wifi_level_s_2;
extern const lv_img_dsc_t img_wifi_level_s_3;
extern const lv_img_dsc_t img_wifi_level_s_4;
extern const lv_img_dsc_t img_back;
extern const lv_img_dsc_t img_factory_reset;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[89];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/