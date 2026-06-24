#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_wifi_connected;
extern const lv_img_dsc_t img_wifi_disconnected;
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
extern const lv_img_dsc_t img_back;
extern const lv_img_dsc_t img_timer_reset;
extern const lv_img_dsc_t img_timer_pause;
extern const lv_img_dsc_t img_alarm_add;
extern const lv_img_dsc_t img_timer_start;
extern const lv_img_dsc_t img_tomato;
extern const lv_img_dsc_t img_bat;
extern const lv_img_dsc_t img_wifi;
extern const lv_img_dsc_t img_timer_stop;
extern const lv_img_dsc_t img_confirm;
extern const lv_img_dsc_t img_del;
extern const lv_img_dsc_t img_icon_main;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[56];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/