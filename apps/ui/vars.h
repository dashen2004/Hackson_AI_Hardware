#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_VOLUME_VALUE = 0,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_TOP = 1,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_BOTTOM = 2,
    FLOW_GLOBAL_VARIABLE_SUBTITLES = 3,
    FLOW_GLOBAL_VARIABLE_EMOTION_IDX = 4,
};

// Native global variables

extern bool get_var_mute_value();
extern void set_var_mute_value(bool value, bool flag);
extern int32_t get_var_volume_value();
extern void set_var_volume_value(int32_t value, bool flag);
extern bool get_var_brightness_onoff(void);
extern void set_var_brightness_onoff(bool value);
extern int32_t get_var_brightness_top();
extern void set_var_brightness_top(int32_t value);
extern int32_t get_var_brightness_bottom();
extern void set_var_brightness_bottom(int32_t value);

extern int32_t get_var_scr_tmo_value(void);
extern void set_var_scr_tmo_value(int32_t value, bool flag);
extern uint32_t get_var_turnoff_value(void);
extern void set_var_turnoff_value(uint32_t value);

extern int32_t get_var_language_value();
extern void set_var_language_value(int32_t value, bool flag);

extern bool get_var_awss_value(void);
extern void set_var_awss_value(bool value, bool flag);

char *get_var_city(void);
void set_var_city(char *value, size_t size);
int8_t get_var_weather_id(void);
void set_var_weather_id(int8_t value);
int8_t get_var_temperature(void);
void set_var_temperature(int8_t value);

int8_t get_var_light_mode(void);
void set_var_light_mode(int8_t value, bool flag);

// void set_var_weather_info_got(bool timeout, char *__city, int __weather_id, int __temperature);
// bool get_var_weather_info_got(void);

extern int8_t get_var_emotion_id(void);
extern void set_var_emotion_id(int8_t value);
extern char *get_var_subtitles();
extern void set_var_subtitles(const char *value);

int8_t get_var_ota_status(void);
void set_var_ota_status(int8_t value, bool need_lock);
int8_t get_var_ota_percent(void);
void set_var_ota_percent(int8_t value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/