#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

// lvgl val
int8_t lvgl_get_var_timer_type(void);
void lvgl_set_var_timer_type(int8_t type);
int8_t lvgl_get_var_timer_state(void);
void lvgl_set_var_timer_state(int8_t state);

int8_t lvgl_get_var_tomato_state(void);
void lvgl_set_var_tomato_state(int8_t state);

// original
extern int32_t get_var_scr_tmo_value(void);
extern void set_var_scr_tmo_value(int32_t value, bool flag);
extern uint32_t get_var_turnoff_value(void);
extern void set_var_turnoff_value(uint32_t value);

char *get_var_city(void);
void set_var_city(char *value, size_t size);
int8_t get_var_weather_id(void);
void set_var_weather_id(int8_t value);
int8_t get_var_temperature(void);
void set_var_temperature(int8_t value);

uint32_t get_var_screen_operation_tick(void);
void set_var_screen_operation_tick(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/