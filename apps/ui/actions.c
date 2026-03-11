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

extern uint8_t lv_port_init_flag;

static enum ScreensEnum cur_screen_index = 0;
static uint8_t tomato_opt_lock = 0;

extern int bsp_lcd_bl_brightness_percent_get(void);

static bool hit_test(lv_event_t * e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    return lv_obj_hit_test((lv_obj_t *)lv_event_get_target(e), &pt);
}

void goto_page_by_id(int8_t screen_id)
{
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    if (cur_screen_index == SCREEN_ID_PAGE_POMODORO) {
        tomato_opt_lock = 0;

        switch (lvgl_get_var_tomato_state()) {
            case TOMATO_STATE_RUNNING:
                lvgl_set_var_tomato_state(TOMATO_STATE_RUNNING_PAUSE);
                ui_tick();
            break;

            case TOMATO_STATE_BREAK:
                lvgl_set_var_timer_state(TOMATO_STATE_BREAK_PAUSE);
                ui_tick();
            break;

            default:
            break;
        }
    }
    cur_screen_index = screen_id;
    loadScreenEx(screen_id);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

static uint8_t page_index[8] = {SCREEN_ID_PAGE_NONE, SCREEN_ID_PAGE_WEATHER, SCREEN_ID_PAGE_MAIN, SCREEN_ID_PAGE_ALARM, 
                            SCREEN_ID_PAGE_TIMER, SCREEN_ID_PAGE_POMODORO, SCREEN_ID_PAGE_WEATHER, SCREEN_ID_PAGE_MAIN};

static void goto_next_page(void)
{
    goto_page_by_id(page_index[cur_screen_index + 1]);
}

static void goto_prev_page(void)
{
    goto_page_by_id(page_index[cur_screen_index - 1]);
}

// ******** 总体事件(来用亮屏) ******** //
void event_all_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if (event <= LV_EVENT_HOVER_LEAVE) { // Input device events
        turnoff_timer_reset(0);
    }

    if (!bsp_lcd_bl_brightness_percent_get()) {
        return;
    }

    if (lv_scr_act() == objects.page_sleep && event == LV_EVENT_CLICKED) {
        goto_page_by_id(SCREEN_ID_PAGE_MAIN);
        return;
    }

    if (lv_scr_act() == objects.page_pomodoro && event == LV_EVENT_CLICKED) {
        bsp_lcd_bl_on();
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
void action_count_up_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        lvgl_set_var_timer_type(TIMER_TYPE_COUNT_UP);

        turnoff_timer_reset(0);
    }
}

void action_count_down_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        lvgl_set_var_timer_type(TIMER_TYPE_COUNT_DOWN);
        
        turnoff_timer_reset(0);
    }
}

void action_timer_opt_click(lv_event_t * e) 
{
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        switch (lvgl_get_var_timer_state()) {
            case TIMER_STATE_IDLE:
            case TIMER_STATE_PAUSE:
                lvgl_set_var_timer_state(TIMER_STATE_RUNNING);
            break;

            case TOMATO_STATE_RUNNING:
                lvgl_set_var_timer_state(TIMER_STATE_PAUSE);
            break;

            default:
            break;
        }

        turnoff_timer_reset(0);
    }
}

void action_timer_stop_click(lv_event_t * e) 
{
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        lvgl_set_var_timer_state(TIMER_STATE_IDLE);

        turnoff_timer_reset(0);
    }
}

void action_timer_reset_click(lv_event_t * e) 
{
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        lvgl_set_var_timer_state(TIMER_STATE_IDLE);

        // reset countdown time
        if (lvgl_get_var_timer_type() == TIMER_TYPE_COUNT_DOWN) {
        }

        turnoff_timer_reset(0);
    }
}

void tomato_opt_event_all_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if (event <= LV_EVENT_HOVER_LEAVE) {
        turnoff_timer_reset(0);
    } else {
        return;
    }

    if (!bsp_lcd_bl_brightness_percent_get()) {
        return;
    }

    uint8_t state = lvgl_get_var_tomato_state();

    if (event == LV_EVENT_CLICKED) {
        if (tomato_opt_lock) {
            tomato_opt_lock = 0;
            return;
        }

        if (!hit_test(e)) {
            return;
        }

        switch (state) {
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
    } else if (event == LV_EVENT_LONG_PRESSED_REPEAT || event == LV_EVENT_LONG_PRESSED) {
        if (state == TOMATO_STATE_RUNNING || state == TOMATO_STATE_BREAK) {
            tomato_opt_lock = 1;
            lvgl_set_var_tomato_state(TOMATO_STATE_IDLE);
        }
    } else {
        // ignore
    }
}
