#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_gesture_handler(lv_event_t * e);
extern void action_memu_home_click(lv_event_t * e);
extern void action_menu_speech_click(lv_event_t * e);
extern void action_menu_brightness_click(lv_event_t * e);
extern void action_menu_setup_click(lv_event_t * e);
extern void action_menu_mode_click(lv_event_t * e);
extern void action_setup_volume_click(lv_event_t * e);
extern void action_setup_screen_click(lv_event_t * e);
extern void action_setup_language_click(lv_event_t * e);
extern void action_setup_about_click(lv_event_t * e);
extern void action_page_brightness_event_cb(lv_event_t * e);
extern void action_brightness_top_changed(lv_event_t * e);
extern void action_brightness_bottom_changed(lv_event_t * e);
extern void action_brightness_light_onoff_click(lv_event_t * e);
extern void action_mode_nightlamp_click(lv_event_t * e);
extern void action_mode_relaxation_click(lv_event_t * e);
extern void action_mode_read_click(lv_event_t * e);
extern void action_mode_sleep_click(lv_event_t * e);
extern void action_setup_volume_mute_click(lv_event_t * e);
extern void action_setup_screen_30s_click(lv_event_t * e);
extern void action_setup_screen_1min_click(lv_event_t * e);
extern void action_setup_screen_5min_click(lv_event_t * e);
extern void action_setup_screen_never_click(lv_event_t * e);
extern void action_volume_value_changed(lv_event_t * e);
extern void action_setup_language_cn_click(lv_event_t * e);
extern void action_setup_language_en_click(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/