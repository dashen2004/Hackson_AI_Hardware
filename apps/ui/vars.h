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
    FLOW_GLOBAL_VARIABLE_TIME = 0,
    FLOW_GLOBAL_VARIABLE_WEEK = 1,
    FLOW_GLOBAL_VARIABLE_DATE = 2,
    FLOW_GLOBAL_VARIABLE_CITY = 3,
    FLOW_GLOBAL_VARIABLE_WEATHER = 4,
    FLOW_GLOBAL_VARIABLE_TEMPERATURE = 5,
    FLOW_GLOBAL_VARIABLE_MODE_NIGHTLAMP_TITLE = 6,
    FLOW_GLOBAL_VARIABLE_MODE_READ_TITLE = 7,
    FLOW_GLOBAL_VARIABLE_MODE_RELAXATION_TITLE = 8,
    FLOW_GLOBAL_VARIABLE_MODE_SLEEP_TITLE = 9,
    FLOW_GLOBAL_VARIABLE_SETUP_VOLUME_TITLE = 10,
    FLOW_GLOBAL_VARIABLE_SETUP_SCREEN_TITLE = 11,
    FLOW_GLOBAL_VARIABLE_SETUP_LANGUAGE_TITLE = 12,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_TITLE = 13,
    FLOW_GLOBAL_VARIABLE_SETUP_VOLUME_MUTE_TITLE = 14,
    FLOW_GLOBAL_VARIABLE_SETUP_VOLUME_MUTE_ONOFF = 15,
    FLOW_GLOBAL_VARIABLE_SETUP_VOLUME_ADJUST_TITLE = 16,
    FLOW_GLOBAL_VARIABLE_SETUP_VOLUME_VALUE = 17,
    FLOW_GLOBAL_VARIABLE_SETUP_VOLUME_VALUE_S = 18,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_TOP = 19,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_TOP_S = 20,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_BOTTOM = 21,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_BOTTOM_S = 22,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_LIGHT_ONOFF = 23,
    FLOW_GLOBAL_VARIABLE_MODE = 24,
    FLOW_GLOBAL_VARIABLE_SCREEN_TMO = 25,
    FLOW_GLOBAL_VARIABLE_LANGUAGE = 26,
    FLOW_GLOBAL_VARIABLE_SCREEN_TMO_30S_DISP = 27,
    FLOW_GLOBAL_VARIABLE_SCREEN_TMO_1MIN_DISP = 28,
    FLOW_GLOBAL_VARIABLE_SCREEN_TMO_5MIN_DISP = 29,
    FLOW_GLOBAL_VARIABLE_SCREEN_TMO_NEVER_DISP = 30,
    FLOW_GLOBAL_VARIABLE_LANGUAGE_SELECT_TITLE = 31,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_IP_ADDR_TITLE = 32,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_IP_ADDR_VALUE = 33,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_MAC_ADDR_TITLE = 34,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_MAC_ADDR_VALUE = 35,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_HW_VER_TITLE = 36,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_HW_VER_VALUE = 37,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_SW_VER_TITLE = 38,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_SW_VER_VALUE = 39,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_DEVICE_INFO_TITLE = 40,
    FLOW_GLOBAL_VARIABLE_SETUP_ABOUT_DEVICE_INFO_VALUE = 41
};

// Native global variables

extern const char *get_var_time();
extern void set_var_time(const char *value);
extern const char *get_var_week();
extern void set_var_week(const char *value);
extern const char *get_var_date();
extern void set_var_date(const char *value);
extern const char *get_var_city();
extern void set_var_city(const char *value);
extern const char *get_var_weather();
extern void set_var_weather(const char *value);
extern const char *get_var_temperature();
extern void set_var_temperature(const char *value);
extern const char *get_var_mode_nightlamp_title();
extern void set_var_mode_nightlamp_title(const char *value);
extern const char *get_var_mode_read_title();
extern void set_var_mode_read_title(const char *value);
extern const char *get_var_mode_relaxation_title();
extern void set_var_mode_relaxation_title(const char *value);
extern const char *get_var_mode_sleep_title();
extern void set_var_mode_sleep_title(const char *value);
extern const char *get_var_setup_volume_title();
extern void set_var_setup_volume_title(const char *value);
extern const char *get_var_setup_screen_title();
extern void set_var_setup_screen_title(const char *value);
extern const char *get_var_setup_language_title();
extern void set_var_setup_language_title(const char *value);
extern const char *get_var_setup_about_title();
extern void set_var_setup_about_title(const char *value);
extern const char *get_var_setup_volume_mute_title();
extern void set_var_setup_volume_mute_title(const char *value);
extern bool get_var_setup_volume_mute_onoff();
extern void set_var_setup_volume_mute_onoff(bool value);
extern const char *get_var_setup_volume_adjust_title();
extern void set_var_setup_volume_adjust_title(const char *value);
extern int32_t get_var_setup_volume_value();
extern void set_var_setup_volume_value(int32_t value);
extern const char *get_var_setup_volume_value_s();
extern void set_var_setup_volume_value_s(const char *value);
extern int32_t get_var_brightness_top();
extern void set_var_brightness_top(int32_t value);
extern const char *get_var_brightness_top_s();
extern void set_var_brightness_top_s(const char *value);
extern int32_t get_var_brightness_bottom();
extern void set_var_brightness_bottom(int32_t value);
extern const char *get_var_brightness_bottom_s();
extern void set_var_brightness_bottom_s(const char *value);
extern bool get_var_brightness_light_onoff();
extern void set_var_brightness_light_onoff(bool value);
extern int32_t get_var_mode();
extern void set_var_mode(int32_t value);
extern int32_t get_var_screen_tmo();
extern void set_var_screen_tmo(int32_t value);
extern int32_t get_var_language();
extern void set_var_language(int32_t value);
extern const char *get_var_screen_tmo_30s_disp();
extern void set_var_screen_tmo_30s_disp(const char *value);
extern const char *get_var_screen_tmo_1min_disp();
extern void set_var_screen_tmo_1min_disp(const char *value);
extern const char *get_var_screen_tmo_5min_disp();
extern void set_var_screen_tmo_5min_disp(const char *value);
extern const char *get_var_screen_tmo_never_disp();
extern void set_var_screen_tmo_never_disp(const char *value);
extern const char *get_var_language_select_title();
extern void set_var_language_select_title(const char *value);
extern const char *get_var_setup_about_ip_addr_title();
extern void set_var_setup_about_ip_addr_title(const char *value);
extern const char *get_var_setup_about_ip_addr_value();
extern void set_var_setup_about_ip_addr_value(const char *value);
extern const char *get_var_setup_about_mac_addr_title();
extern void set_var_setup_about_mac_addr_title(const char *value);
extern const char *get_var_setup_about_mac_addr_value();
extern void set_var_setup_about_mac_addr_value(const char *value);
extern const char *get_var_setup_about_hw_ver_title();
extern void set_var_setup_about_hw_ver_title(const char *value);
extern const char *get_var_setup_about_hw_ver_value();
extern void set_var_setup_about_hw_ver_value(const char *value);
extern const char *get_var_setup_about_sw_ver_title();
extern void set_var_setup_about_sw_ver_title(const char *value);
extern const char *get_var_setup_about_sw_ver_value();
extern void set_var_setup_about_sw_ver_value(const char *value);
extern const char *get_var_setup_about_device_info_title();
extern void set_var_setup_about_device_info_title(const char *value);
extern const char *get_var_setup_about_device_info_value();
extern void set_var_setup_about_device_info_value(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/