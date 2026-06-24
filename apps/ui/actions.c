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
#include "app_audio_prompt.h"
#include "ai_main.h"
#include "qm_ai_api.h"
#include "xjiang_cloud.h"
#include "settings_images.h"

#define loadScreenEx    loadScreen

static enum ScreensEnum cur_screen_index = SCREEN_ID_PAGE_MAIN;
static enum ScreensEnum cur_level1_page = SCREEN_ID_PAGE_MAIN;
static uint8_t screen_slide_lock = 0;
static uint32_t sceen_slide_tick = 0;
static lv_point_t s_press_pt;
static bool s_dragging = false;

#define UI_DRAG_THRESHOLD_PX  12
#define UI_CLICK_OK(e)        (ui_click_allowed() && hit_test(e))
#define UI_ROW_CLICK_OK(e)    (ui_settings_row_click_ok(e))

static bool ui_settings_row_click_ok(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (!bsp_lcd_is_ready()) {
        return false;
    }
    if (code != LV_EVENT_CLICKED) {
        return false;
    }
    if (screen_slide_lock && qm_now_ms() < sceen_slide_tick + 300) {
        return false;
    }
    return true;
}

static bool is_settings_sub_page(enum ScreensEnum screen_id)
{
    return screen_id == SCREEN_ID_PAGE_SETTINGS_SOUND
        || screen_id == SCREEN_ID_PAGE_SETTINGS_NET;
}

static bool is_level1_page(enum ScreensEnum screen_id)
{
    return screen_id == SCREEN_ID_PAGE_MAIN
        || screen_id == SCREEN_ID_PAGE_APPS1
        || screen_id == SCREEN_ID_PAGE_APPS2;
}

static bool is_level2_page(enum ScreensEnum screen_id)
{
    return !is_level1_page(screen_id) && screen_id != SCREEN_ID_PAGE_SLEEP
        && screen_id != SCREEN_ID_PAGE_NONE;
}

static int get_level1_slot(enum ScreensEnum screen_id)
{
    switch (screen_id) {
        case SCREEN_ID_PAGE_MAIN:   return 0;
        case SCREEN_ID_PAGE_APPS1:  return 1;
        case SCREEN_ID_PAGE_APPS2:  return 2;
        default:                    return -1;
    }
}

static bool hit_test(lv_event_t * e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    return lv_obj_hit_test((lv_obj_t *)lv_event_get_target(e), &pt);
}

static bool ui_click_allowed(void)
{
    if (!bsp_lcd_is_ready()) {
        return false;
    }
    if (s_dragging) {
        return false;
    }
    if (screen_slide_lock && qm_now_ms() < sceen_slide_tick + 300) {
        return false;
    }
    return true;
}

static void ui_track_touch(lv_event_code_t event)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }

    if (event == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &s_press_pt);
        s_dragging = false;
        return;
    }

    if (event == LV_EVENT_PRESSING) {
        lv_point_t pt;
        int dx;
        int dy;

        lv_indev_get_point(indev, &pt);
        dx = pt.x - s_press_pt.x;
        dy = pt.y - s_press_pt.y;
        if ((dx * dx + dy * dy) > (UI_DRAG_THRESHOLD_PX * UI_DRAG_THRESHOLD_PX)) {
            s_dragging = true;
        }
        return;
    }

    if (event == LV_EVENT_GESTURE) {
        s_dragging = true;
    }
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
                lvgl_var_tomato_state_set(TOMATO_STATE_BREAK_PAUSE);
                ui_tick();
            break;

            default:
            break;
        }
    }

    if (cur_screen_index == SCREEN_ID_PAGE_EYE) {
        if (lvgl_var_eye_state_get() == EYE_STATE_RUNNING) {
            lvgl_var_eye_state_set(EYE_STATE_PAUSE);
            ui_tick();
        }
    }

    if (is_level1_page(screen_id)) {
        cur_level1_page = screen_id;
        ui_update_page_indicator(get_level1_slot(screen_id));
    }

    cur_screen_index = screen_id;
    loadScreenEx(screen_id);
    lvgl_port_unlock();
}

static void goto_next_level1_page(void)
{
    switch (cur_screen_index) {
        case SCREEN_ID_PAGE_MAIN:
            goto_page_by_id(SCREEN_ID_PAGE_APPS1);
        break;
        case SCREEN_ID_PAGE_APPS1:
            goto_page_by_id(SCREEN_ID_PAGE_APPS2);
        break;
        default:
        break;
    }
}

static void goto_prev_level1_page(void)
{
    switch (cur_screen_index) {
        case SCREEN_ID_PAGE_APPS2:
            goto_page_by_id(SCREEN_ID_PAGE_APPS1);
        break;
        case SCREEN_ID_PAGE_APPS1:
            goto_page_by_id(SCREEN_ID_PAGE_MAIN);
        break;
        default:
        break;
    }
}

void action_open_eye_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_EYE);
    }
}

void action_open_pomodoro_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_POMODORO);
    }
}

void action_open_alarm_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_ALARM);
    }
}

void action_open_timer_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_TIMER);
    }
}

void action_open_timetable_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_TIMETABLE);
    }
}

void action_open_checkin_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_CHECKIN);
    }
}

void action_open_settings_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_SETTINGS);
    }
}

void action_settings_sound_click(lv_event_t * e)
{
    if (UI_ROW_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_SETTINGS_SOUND);
        scr_settings_sound_sync();
    }
}

void action_settings_net_click(lv_event_t * e)
{
    if (UI_ROW_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_SETTINGS_NET);
    }
}

void action_settings_mute_changed(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    qm_ai_handle_t handle = ai_main_get_handle();
    if (handle == NULL) {
        return;
    }

    bool muted = lv_obj_has_state(objects.settings_mute_sw, LV_STATE_CHECKED);
    qm_ai_audio_set_player_mute(handle, muted ? 1 : 0);
}

void action_settings_volume_changed(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    qm_ai_handle_t handle = ai_main_get_handle();
    if (handle == NULL || objects.settings_volume_slider == NULL) {
        return;
    }

    int32_t volume = lv_slider_get_value(objects.settings_volume_slider);
    qm_ai_audio_set_local_volume(handle, (uint32_t)volume);

    if (lv_obj_has_state(objects.settings_mute_sw, LV_STATE_CHECKED)) {
        lv_obj_remove_state(objects.settings_mute_sw, LV_STATE_CHECKED);
    }
}

void action_settings_net_confirm_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        xjiang_cloud_reset();
    }
}

void action_settings_net_cancel_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        goto_page_by_id(SCREEN_ID_PAGE_SETTINGS);
    }
}

// ******** 总体事件(来用亮屏) ******** //
void event_all_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_PRESSED || event == LV_EVENT_PRESSING || event == LV_EVENT_GESTURE) {
        ui_track_touch(event);
    }

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

    if (lv_scr_act() == objects.page_eye && event == LV_EVENT_CLICKED) {
        bsp_lcd_bl_on();
        return;
    }

    if (qm_now_ms() > sceen_slide_tick + 300) {
        screen_slide_lock = 0;
    }

    if (event == LV_EVENT_GESTURE && lv_scr_act() != objects.page_sleep && !screen_slide_lock) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

        screen_slide_lock = 1;
        sceen_slide_tick = qm_now_ms();

        if (is_level1_page(cur_screen_index)) {
            switch (dir) {
                case LV_DIR_LEFT:
                    goto_next_level1_page();
                break;

                case LV_DIR_RIGHT:
                    goto_prev_level1_page();
                break;

                default:
                break;
            }
        } else if (is_level2_page(cur_screen_index) && dir == LV_DIR_RIGHT) {
            if (is_settings_sub_page(cur_screen_index)) {
                goto_page_by_id(SCREEN_ID_PAGE_SETTINGS);
            } else {
                goto_page_by_id(cur_level1_page);
            }
        }

        return;
    }
}

/* alarm page action */
void action_alarm_add_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_enter_page_alarm_add();
        goto_page_by_id(SCREEN_ID_PAGE_ALARM_EDIT);
    }
}

void action_alarm_item_long_press(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        int32_t index = (int32_t)lv_event_get_user_data(e);

        scr_enter_page_alarm_edit(index);
        goto_page_by_id(SCREEN_ID_PAGE_ALARM_EDIT);
    }
}

void action_alarm_confirm_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_alarm_edit_confirm();
        goto_page_by_id(SCREEN_ID_PAGE_ALARM);
    }
}

void action_alarm_del_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_alarm_edit_exit();
        goto_page_by_id(SCREEN_ID_PAGE_ALARM);
    }
}

void action_alarm_weekday_01_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_weekday_check_state_toggle(0);
    }
}

void action_alarm_weekday_02_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_weekday_check_state_toggle(1);
    }
}

void action_alarm_weekday_03_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_weekday_check_state_toggle(2);
    }
}

void action_alarm_weekday_04_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_weekday_check_state_toggle(3);
    }
}

void action_alarm_weekday_05_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_weekday_check_state_toggle(4);
    }
}

void action_alarm_weekday_06_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_weekday_check_state_toggle(5);
    }
}

void action_alarm_weekday_07_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        scr_weekday_check_state_toggle(6);
    }
}



/*  timer page action  */
void action_count_up_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        lvgl_var_timer_type_set(TIMER_TYPE_COUNT_UP);

        app_lvgl_turnoff_reset(1);
    }
}

void action_count_down_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        lvgl_var_timer_type_set(TIMER_TYPE_COUNT_DOWN);
        
        app_lvgl_turnoff_reset(1);
    }
}

void action_timer_opt_click(lv_event_t * e) 
{
    if (UI_CLICK_OK(e)) {
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
    if (UI_CLICK_OK(e)) {
        lvgl_var_timer_state_set(TIMER_STATE_IDLE);

        app_lvgl_turnoff_reset(1);
    }
}

void action_timer_reset_click(lv_event_t * e) 
{
    if (UI_CLICK_OK(e)) {
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
    if (UI_CLICK_OK(e)) {
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
    if (UI_CLICK_OK(e)) {
        lvgl_var_tomato_state_set(TOMATO_STATE_IDLE);

        app_lvgl_turnoff_reset(1);
    }
}

void action_eye_opt_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        uint8_t state = lvgl_var_eye_state_get();

        switch (state) {
            case EYE_STATE_IDLE:
                lvgl_var_eye_state_set(EYE_STATE_RUNNING);
                app_audio_play_ybjc_start();
            break;

            case EYE_STATE_PAUSE:
                lvgl_var_eye_state_set(EYE_STATE_RUNNING);
            break;

            case EYE_STATE_RUNNING:
                lvgl_var_eye_state_set(EYE_STATE_PAUSE);
            break;

            default:
            break;
        }

        app_lvgl_turnoff_reset(1);
    }
}

void action_eye_stop_click(lv_event_t * e)
{
    if (UI_CLICK_OK(e)) {
        lvgl_var_eye_state_set(EYE_STATE_IDLE);

        app_lvgl_turnoff_reset(1);
    }
}
