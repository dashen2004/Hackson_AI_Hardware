#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COUNTDOWN_LIMIT_HOUR    23
#define COUNTDOWN_LIMIT_MINUTE  59
#define COUNTDOWN_LIMIT_SECOND  59

// enum declarations
enum TimerType {
    TIMER_TYPE_NONE = 0,
    TIMER_TYPE_COUNT_UP,
    TIMER_TYPE_COUNT_DOWN,
    TIMER_TYPE_MAX,
};

enum TimerState {
    TIMER_STATE_NONE = 0,
    TIMER_STATE_IDLE = 1,
    TIMER_STATE_RUNNING = 2,
    TIMER_STATE_PAUSE = 3,
    TIMER_STATE_MAX
};

enum TomatoState {
    TOMATO_STATE_NONE = 0,
    TOMATO_STATE_IDLE = 1,
    TOMATO_STATE_RUNNING = 2,
    TOMATO_STATE_RUNNING_PAUSE = 3,
    TOMATO_STATE_BREAK = 4,
    TOMATO_STATE_BREAK_PAUSE = 5,
    TOMATO_STATE_MAX
};

int8_t lvgl_var_timer_type_get(void);
void lvgl_var_timer_type_set(int8_t type);
int8_t lvgl_var_timer_state_get(void);
void lvgl_var_timer_state_set(int8_t state);
int32_t lvgl_var_timer_countdown_hour_get(void);
void lvgl_var_timer_countdown_hour_set(int32_t hour);
int32_t lvgl_var_timer_countdown_min_get(void);
void lvgl_var_timer_countdown_min_set(int32_t min);
int32_t lvgl_var_timer_countdown_sec_get(void);
void lvgl_var_timer_countdown_sec_set(int32_t sec);

int8_t lvgl_var_tomato_state_get(void);
void lvgl_var_tomato_state_set(int8_t state);

char *lvgl_var_city_get(void);
void lvgl_var_city_set(char *value, int32_t size);
int8_t lvgl_var_weather_id_get(void);
void lvgl_var_weather_id_set(int8_t value);
int8_t lvgl_var_temperature_get(void);
void lvgl_var_temperature_set(int8_t value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/