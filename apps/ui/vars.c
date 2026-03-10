#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp_lcd.h"

#include "qm_kv.h"
#include "esp_lvgl_port.h"

#include "qm_utils_timer.h"

static char var_city[64] = {0};
static int8_t var_weather_id = -1;
static int8_t var_temperature = 25;

static int8_t var_timer_type = TIMER_TYPE_COUNT_UP;
static int8_t var_timer_state = TIMER_STATE_IDLE;

static int8_t var_tomato_state = TOMATO_STATE_IDLE;

static int32_t  var_scr_tmo = 0;
static qm_utils_time_t var_turnoff = {0};
static qm_utils_time_t var_screen_operation_tick = {0};

// timer
int8_t lvgl_get_var_timer_type(void) {
    return var_timer_type;
}

void lvgl_set_var_timer_type(int8_t type) {
    if (type <= TIMER_TYPE_NONE || type >= TIMER_TYPE_MAX) {
        return;
    }

    var_timer_type = type;
}

int8_t lvgl_get_var_timer_state(void) {
    return var_timer_state;
}

void lvgl_set_var_timer_state(int8_t state) {
    if (state <= TIMER_STATE_NONE || state >= TIMER_STATE_MAX) {
        return;
    }

    var_timer_state = state;
}

// tomato
int8_t lvgl_get_var_tomato_state(void) {
    return var_tomato_state;
}

void lvgl_set_var_tomato_state(int8_t state) {
    if (state <= TOMATO_STATE_NONE || state >= TOMATO_STATE_MAX) {
        return;
    }

    var_tomato_state = state;
}

char *get_var_city(void) {
    return var_city;
}

void set_var_city(char *value, size_t size) {
    lv_strncpy(var_city, value, size);
}

int32_t get_var_scr_tmo_value(void) {
    return var_scr_tmo;
}

void set_var_scr_tmo_value(int32_t value, bool flag) {
    var_scr_tmo = value;
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    if (flag) {
        // qm_kv_set(SCR_TMO_KEY_NAME, &var_scr_tmo, sizeof(int32_t), 1);
    }
}

uint32_t get_var_turnoff_value(void) {
    return qm_utils_time_is_expired(&var_turnoff);
}

void set_var_turnoff_value(uint32_t value) {
    qm_utils_time_countdown_ms(&var_turnoff, value);
}

int8_t get_var_weather_id(void) {
    return var_weather_id;
}

void set_var_weather_id(int8_t value) {
    var_weather_id = value;
}

int8_t get_var_temperature(void) {
    return var_temperature;
}

void set_var_temperature(int8_t value) {
    var_temperature = value;
}

uint32_t get_var_screen_operation_tick(void) {
    return qm_utils_time_is_expired(&var_screen_operation_tick);
}

void set_var_screen_operation_tick(void) {
    qm_utils_time_countdown_ms(&var_screen_operation_tick, 5 * 1000);
}
