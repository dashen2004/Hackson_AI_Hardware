#include <stdio.h>
#include <time.h>
#include "actions.h"
#include "ui.h"
#include "vars.h"
#include "screens.h"

#include "qm.h"
#include "qm_time.h"
#include "qm_work.h"
#include "qm_utils_timer.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

// #include "bsp_audio.h"
// #include "bsp_light.h"
// #include "audio_light.h"
// #include "audio_main.h"

#include "bsp_lcd.h"


/*
    有些gif刚加载时会白色闪烁, 所以在lv_gif.c中做一些屏蔽处理
    1. 添加全部变量 static uint8_t hidden_frame_counter = 0;
    2. 
 */

/*
 * 关于手势 与 按钮clicked冲突: 
 *     在pressed事件中, 如果original_target为page, 则disable button, 直到released事件或gesture事件
 * 关于按钮按下后滑出按钮区域后还能触发clicked: 
 *     在clicked事件中判断手指是否还在按钮上(hit_test)
 */

#define loadScreenEx    loadScreen
// extern void loadScreenEx(enum ScreensEnum screenId);

extern int bsp_lcd_bl_brightness_percent_get(void);

// static uint8_t emotion_index = 0;

static bool hit_test(lv_event_t * e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    return lv_obj_hit_test((lv_obj_t *)lv_event_get_target(e), &pt);
}

extern uint8_t lv_port_init_flag;

static enum ScreensEnum cur_screen_index = 0;

void goto_page_by_id(enum ScreensEnum screen_id)
{
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    cur_screen_index = screen_id;
    loadScreenEx(screen_id);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

void goto_sleep_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    cur_screen_index = SCREEN_ID_PAGE_SLEEP;
    loadScreenEx(SCREEN_ID_PAGE_SLEEP);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

void goto_main_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    cur_screen_index = SCREEN_ID_PAGE_MAIN;
    loadScreenEx(SCREEN_ID_PAGE_MAIN);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

void goto_alarm_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    cur_screen_index = SCREEN_ID_PAGE_ALARM;
    loadScreenEx(SCREEN_ID_PAGE_ALARM);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

void goto_timer_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    cur_screen_index = SCREEN_ID_PAGE_TIMER;
    loadScreenEx(SCREEN_ID_PAGE_TIMER);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

void goto_pomodoro_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    cur_screen_index = SCREEN_ID_PAGE_POMODORO;
    loadScreenEx(SCREEN_ID_PAGE_POMODORO);
    lvgl_set_var_tomato_state(TOMATO_STATE_IDLE);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

void goto_weather_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    cur_screen_index = SCREEN_ID_PAGE_WEATHER;
    loadScreenEx(SCREEN_ID_PAGE_WEATHER);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

static void goto_next_page(void)
{
    switch (cur_screen_index) {
        case SCREEN_ID_PAGE_MAIN:       goto_alarm_page(); break;
        case SCREEN_ID_PAGE_ALARM:      goto_timer_page(); break;
        case SCREEN_ID_PAGE_TIMER:      goto_pomodoro_page(); break;
        case SCREEN_ID_PAGE_POMODORO:   goto_weather_page(); break;
        case SCREEN_ID_PAGE_WEATHER:    goto_main_page(); break;
        default: break;
    }
}

static void goto_prev_page(void)
{
    switch (cur_screen_index) {
        case SCREEN_ID_PAGE_MAIN:       goto_weather_page(); break;
        case SCREEN_ID_PAGE_ALARM:      goto_main_page(); break;
        case SCREEN_ID_PAGE_TIMER:      goto_alarm_page(); break;
        case SCREEN_ID_PAGE_POMODORO:   goto_timer_page(); break;
        case SCREEN_ID_PAGE_WEATHER:    goto_pomodoro_page(); break;
        default: break;
    }
}

// ******** 总体事件(来用亮屏) ******** //
void event_all_handler(lv_event_t * e) {
    lv_event_code_t event = lv_event_get_code(e);
    int current_brightness_percent = bsp_lcd_bl_brightness_percent_get();
    if (event <= LV_EVENT_HOVER_LEAVE) { // Input device events
        turnoff_timer_reset(0);
    }

    if (!current_brightness_percent) {
        return;
    }

    if (lv_scr_act() == objects.page_sleep && event == LV_EVENT_CLICKED) {
        goto_main_page();
        return;
    }

    if (event == LV_EVENT_GESTURE && lv_scr_act() != objects.page_sleep) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

        switch (dir) {
            case LV_DIR_LEFT:
                goto_next_page();
            break;
            
            case LV_DIR_RIGHT:
                goto_prev_page();
            break;

            default:
            break;
        }

        return;
    }
}

/*  action  */
void action_tomato_opt_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        switch (lvgl_get_var_tomato_state()) {
            case TOMATO_STATE_IDLE:
            case TOMATO_STATE_RUNNING_PAUSE:
                lvgl_set_var_tomato_state(TOMATO_STATE_RUNNING);
            break;

            case TOMATO_STATE_BREAK_PAUSE:
                lvgl_set_var_tomato_state(TOMATO_STATE_BREAK);
            break;

            case TOMATO_STATE_RUNNING:
                lvgl_set_var_tomato_state(TOMATO_STATE_RUNNING_PAUSE);
            break;

            case TOMATO_STATE_BREAK:
                lvgl_set_var_tomato_state(TOMATO_STATE_BREAK_PAUSE);
            break;

            default:
            break;
        }

        turnoff_timer_reset(0);
    }
}
