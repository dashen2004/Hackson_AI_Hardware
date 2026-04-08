#include "vars.h"
#include "screens.h"

#include "bsp_lcd.h"
#include "app_lvgl.h"
#include "qm_kv.h"

typedef struct {
    char var_city[64];
    int8_t var_weather_id;
    int8_t var_temperature;

    int8_t var_timer_type;
    int8_t var_timer_state;
    int32_t var_timer_countdown_hour;
    int32_t var_timer_countdown_min;
    int32_t var_timer_countdown_sec;

    int8_t var_tomato_state;
} lvgl_var_t;

static lvgl_var_t lvgl_vars = {
    .var_city = {0},
    .var_weather_id = -1,
    .var_temperature = 25,

    .var_timer_type = TIMER_TYPE_COUNT_UP,
    .var_timer_state = TIMER_STATE_IDLE,
    .var_timer_countdown_hour = 0,
    .var_timer_countdown_min = 30,
    .var_timer_countdown_sec = 0,

    .var_tomato_state = TOMATO_STATE_IDLE,
};

// timer
int8_t lvgl_var_timer_type_get(void) 
{
    return lvgl_vars.var_timer_type;
}

void lvgl_var_timer_type_set(int8_t type) 
{
    if (type <= TIMER_TYPE_NONE || type >= TIMER_TYPE_MAX) {
        return;
    }

    lvgl_vars.var_timer_type = type;
}

int8_t lvgl_var_timer_state_get(void) 
{
    return lvgl_vars.var_timer_state;
}

void lvgl_var_timer_state_set(int8_t state) 
{
    if (state <= TIMER_STATE_NONE || state >= TIMER_STATE_MAX) {
        return;
    }

    lvgl_vars.var_timer_state = state;
}

int32_t lvgl_var_timer_countdown_hour_get(void) 
{
    return lvgl_vars.var_timer_countdown_hour;
}

void lvgl_var_timer_countdown_hour_set(int32_t hour) 
{
    if (hour < 0 || hour > COUNTDOWN_LIMIT_HOUR) {
        return;
    }

    lvgl_vars.var_timer_countdown_hour = hour;
}

int32_t lvgl_var_timer_countdown_min_get(void) 
{
    return lvgl_vars.var_timer_countdown_min;
}

void lvgl_var_timer_countdown_min_set(int32_t min) 
{
    if (min < 0 || min > COUNTDOWN_LIMIT_MINUTE) {
        return;
    }

    lvgl_vars.var_timer_countdown_min = min;
}

int32_t lvgl_var_timer_countdown_sec_get(void) 
{
    return lvgl_vars.var_timer_countdown_sec;
}

void lvgl_var_timer_countdown_sec_set(int32_t sec) 
{
    if (sec < 0 || sec > COUNTDOWN_LIMIT_SECOND) {
        return;
    }

    lvgl_vars.var_timer_countdown_sec = sec;
}

// tomato
int8_t lvgl_var_tomato_state_get(void) 
{
    return lvgl_vars.var_tomato_state;
}

void lvgl_var_tomato_state_set(int8_t state) 
{
    if (state <= TOMATO_STATE_NONE || state >= TOMATO_STATE_MAX) {
        return;
    }

    lvgl_vars.var_tomato_state = state;
}

// city weather temperature
char *lvgl_var_city_get(void) 
{
    return lvgl_vars.var_city;
}

void lvgl_var_city_set(char *value, int32_t size) 
{
    lv_strncpy(lvgl_vars.var_city, value, size);

    tick_screen_page_weather();
}

int8_t lvgl_var_weather_id_get(void) 
{
    return lvgl_vars.var_weather_id;
}

void lvgl_var_weather_id_set(int8_t value) 
{
    lvgl_vars.var_weather_id = value;

    tick_screen_page_weather();
}

int8_t lvgl_var_temperature_get(void) 
{
    return lvgl_vars.var_temperature;
}

void lvgl_var_temperature_set(int8_t value) 
{
    lvgl_vars.var_temperature = value;

    tick_screen_page_weather();
}
