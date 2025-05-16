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
    FLOW_GLOBAL_VARIABLE_OTA_PERCENT = 5
};

// Native global variables

extern int32_t get_var_volume_value();
extern void set_var_volume_value(int32_t value);
extern int32_t get_var_brightness_top();
extern void set_var_brightness_top(int32_t value);
extern int32_t get_var_brightness_bottom();
extern void set_var_brightness_bottom(int32_t value);
extern const char *get_var_subtitles();
extern void set_var_subtitles(const char *value);
extern int32_t get_var_emotion_idx();
extern void set_var_emotion_idx(int32_t value);
extern int32_t get_var_ota_percent();
extern void set_var_ota_percent(int32_t value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/