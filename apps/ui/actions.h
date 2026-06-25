#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void goto_page_by_id(int8_t screen_id);
void ui_boot_sleep_enter(void);
void ui_boot_leave_sleep(void);
void ui_on_cloud_connected(void);

extern void event_all_handler(lv_event_t * e);

extern void action_alarm_add_click(lv_event_t * e);
extern void action_alarm_item_long_press(lv_event_t * e);
extern void action_alarm_confirm_click(lv_event_t * e);
extern void action_alarm_del_click(lv_event_t * e);
extern void action_alarm_weekday_01_click(lv_event_t * e);
extern void action_alarm_weekday_02_click(lv_event_t * e);
extern void action_alarm_weekday_03_click(lv_event_t * e);
extern void action_alarm_weekday_04_click(lv_event_t * e);
extern void action_alarm_weekday_05_click(lv_event_t * e);
extern void action_alarm_weekday_06_click(lv_event_t * e);
extern void action_alarm_weekday_07_click(lv_event_t * e);

extern void action_count_up_click(lv_event_t * e);
extern void action_count_down_click(lv_event_t * e);
extern void roller_hour_event_cb(lv_event_t * e);
extern void roller_min_event_cb(lv_event_t * e);
extern void roller_sec_event_cb(lv_event_t * e);
extern void action_timer_opt_click(lv_event_t * e);
extern void action_timer_stop_click(lv_event_t * e);
extern void action_timer_reset_click(lv_event_t * e);

extern void action_tomato_opt_click(lv_event_t * e);
extern void action_tomato_stop_click(lv_event_t * e);
extern void action_eye_opt_click(lv_event_t * e);
extern void action_eye_stop_click(lv_event_t * e);
extern void action_study_pause_click(lv_event_t * e);
extern void action_study_skip_click(lv_event_t * e);
extern void action_study_result_confirm_click(lv_event_t * e);

extern void action_open_eye_click(lv_event_t * e);
extern void action_open_pomodoro_click(lv_event_t * e);
extern void action_open_alarm_click(lv_event_t * e);
extern void action_open_timer_click(lv_event_t * e);
extern void action_open_timetable_click(lv_event_t * e);
extern void action_open_checkin_click(lv_event_t * e);
extern void action_open_study_click(lv_event_t * e);
extern void action_open_settings_click(lv_event_t * e);

extern void action_settings_sound_click(lv_event_t * e);
extern void action_settings_net_click(lv_event_t * e);
extern void action_settings_mute_changed(lv_event_t * e);
extern void action_settings_volume_changed(lv_event_t * e);
extern void action_settings_net_confirm_click(lv_event_t * e);
extern void action_settings_net_cancel_click(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/