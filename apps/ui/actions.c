#include <stdio.h>
#include <time.h>
#include "actions.h"
#include "misc/lv_event.h"
#include "qm_kernel.h"
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
#include "app_lvgl.h"

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

static enum ScreensEnum cur_screen_index = 0;
static uint8_t screen_slide_lock = 0;
static uint32_t sceen_slide_tick = 0;

static bool hit_test(lv_event_t * e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    return lv_obj_hit_test((lv_obj_t *)lv_event_get_target(e), &pt);
}

void goto_page_by_id(int8_t screen_id)
{
    lvgl_port_lock(0);
    if (cur_screen_index == SCREEN_ID_PAGE_POMODORO) {
        switch (lvgl_var_tomato_state_get()) {
            case TOMATO_STATE_RUNNING:
                lvgl_var_tomato_state_set(TOMATO_STATE_RUNNING_PAUSE);
                ui_tick();
            break;

            case TOMATO_STATE_BREAK:
                lvgl_var_timer_state_set(TOMATO_STATE_BREAK_PAUSE);
                ui_tick();
            break;

            default:
            break;
        }
    }
    cur_screen_index = screen_id;
    loadScreenEx(screen_id);
    lvgl_port_unlock();
    // uint32_t timeout = app_lvgl_get_turnoff_time();
    // set_var_turnoff_value(timeout);
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
        app_lvgl_turnoff_reset(1);
    }

    if (!bsp_lcd_is_ready()) {
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

    if (qm_now_ms() > sceen_slide_tick + 300) {
        screen_slide_lock = 0;
    }

    if (event == LV_EVENT_GESTURE && lv_scr_act() != objects.page_sleep && !screen_slide_lock) {
        screen_slide_lock = 1;
        sceen_slide_tick = qm_now_ms();

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

/* alarm page action */
void action_alarm_add_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_enter_page_alarm_add();
        goto_page_by_id(SCREEN_ID_PAGE_ALARM_EDIT);
    }
}

void action_alarm_item_long_press(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        int32_t index = (int32_t)lv_event_get_user_data(e);

        scr_enter_page_alarm_edit(index);
        goto_page_by_id(SCREEN_ID_PAGE_ALARM_EDIT);
    }
}

void action_alarm_confirm_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_alarm_edit_confirm();
        goto_page_by_id(SCREEN_ID_PAGE_ALARM);
    }
}

void action_alarm_del_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_alarm_edit_exit();
        goto_page_by_id(SCREEN_ID_PAGE_ALARM);
    }
}

void action_alarm_weekday_01_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_weekday_check_state_toggle(0);
    }
}

void action_alarm_weekday_02_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_weekday_check_state_toggle(1);
    }
}

void action_alarm_weekday_03_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_weekday_check_state_toggle(2);
    }
}

void action_alarm_weekday_04_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_weekday_check_state_toggle(3);
    }
}

void action_alarm_weekday_05_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_weekday_check_state_toggle(4);
    }
}

void action_alarm_weekday_06_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_weekday_check_state_toggle(5);
    }
}

void action_alarm_weekday_07_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        scr_weekday_check_state_toggle(6);
    }
}



/*  timer page action  */
void action_count_up_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        lvgl_var_timer_type_set(TIMER_TYPE_COUNT_UP);

        app_lvgl_turnoff_reset(1);
    }
}

void action_count_down_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        lvgl_var_timer_type_set(TIMER_TYPE_COUNT_DOWN);
        
        app_lvgl_turnoff_reset(1);
    }
}

void action_timer_opt_click(lv_event_t * e) 
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        switch (lvgl_var_timer_state_get()) {
            case TIMER_STATE_IDLE:
            case TIMER_STATE_PAUSE:
                lvgl_var_timer_state_set(TIMER_STATE_RUNNING);
            break;

            case TOMATO_STATE_RUNNING:
                lvgl_var_timer_state_set(TIMER_STATE_PAUSE);
            break;

            default:
            break;
        }

        app_lvgl_turnoff_reset(1);
    }
}

void action_timer_stop_click(lv_event_t * e) 
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        lvgl_var_timer_state_set(TIMER_STATE_IDLE);

        app_lvgl_turnoff_reset(1);
    }
}

void action_timer_reset_click(lv_event_t * e) 
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        // reset countdown time
        if (lvgl_var_timer_type_get() == TIMER_TYPE_COUNT_DOWN) {
            lvgl_var_timer_countdown_hour_set(0);
            lvgl_var_timer_countdown_min_set(30);
            lvgl_var_timer_countdown_sec_set(0);

            lv_roller_set_selected(objects.roller_hour, lvgl_var_timer_countdown_hour_get(), LV_ANIM_OFF);
            lv_roller_set_selected(objects.roller_min, lvgl_var_timer_countdown_min_get(), LV_ANIM_OFF);
            lv_roller_set_selected(objects.roller_sec, lvgl_var_timer_countdown_sec_get(), LV_ANIM_OFF);
        }

        lvgl_var_timer_state_set(TIMER_STATE_IDLE);

        app_lvgl_turnoff_reset(1);
    }
}

/*  timer page callback  */
void roller_hour_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        int32_t hour = lv_roller_get_selected(objects.roller_hour);
        lvgl_var_timer_countdown_hour_set(hour);
    }
}

void roller_min_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        int32_t min = lv_roller_get_selected(objects.roller_min);
        lvgl_var_timer_countdown_min_set(min);
    }
}

void roller_sec_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        int32_t sec = lv_roller_get_selected(objects.roller_sec);
        lvgl_var_timer_countdown_sec_set(sec);
    }
}

/* pomodoro page action */
void action_tomato_opt_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        uint8_t state = lvgl_var_tomato_state_get();

        switch (state) {
            case TOMATO_STATE_IDLE:
            case TOMATO_STATE_RUNNING_PAUSE:
                lvgl_var_tomato_state_set(TOMATO_STATE_RUNNING);
            break;

            case TOMATO_STATE_BREAK_PAUSE:
                lvgl_var_tomato_state_set(TOMATO_STATE_BREAK);
            break;

            case TOMATO_STATE_RUNNING:
                lvgl_var_tomato_state_set(TOMATO_STATE_RUNNING_PAUSE);
            break;

            case TOMATO_STATE_BREAK:
                lvgl_var_tomato_state_set(TOMATO_STATE_BREAK_PAUSE);
            break;

            default:
            break;
        }

        app_lvgl_turnoff_reset(1);
    }
}

void action_tomato_stop_click(lv_event_t * e)
{
    if (hit_test(e) && bsp_lcd_is_ready()) {
        lvgl_var_tomato_state_set(TOMATO_STATE_IDLE);

        app_lvgl_turnoff_reset(1);
    }
}
