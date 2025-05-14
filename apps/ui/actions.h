#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void event_all_handler(lv_event_t * e);

extern void action_gesture_handler(lv_event_t * e);
extern void action_home_click(lv_event_t * e);
extern void action_recog_click(lv_event_t * e);
extern void action_brightness_click(lv_event_t * e);
extern void action_setup_click(lv_event_t * e);
extern void action_mode_click(lv_event_t * e);
extern void action_volume_click(lv_event_t * e);
extern void action_screen_click(lv_event_t * e);
extern void action_language_click(lv_event_t * e);
extern void action_about_click(lv_event_t * e);
extern void action_page_brightness_event_cb(lv_event_t * e);
extern void action_brightness_top_changed(lv_event_t * e);
extern void action_brightness_bottom_changed(lv_event_t * e);
extern void action_brightness_light_onoff_click(lv_event_t * e);
extern void action_nightlamp_click(lv_event_t * e);
extern void action_idle_click(lv_event_t * e);
extern void action_read_click(lv_event_t * e);
extern void action_sleep_click(lv_event_t * e);
extern void action_volume_mute_click(lv_event_t * e);
extern void action_screen_30s_click(lv_event_t * e);
extern void action_screen_1min_click(lv_event_t * e);
extern void action_screen_5min_click(lv_event_t * e);
extern void action_screen_never_click(lv_event_t * e);
extern void action_volume_value_changed(lv_event_t * e);
extern void action_language_cn_click(lv_event_t * e);
extern void action_language_en_click(lv_event_t * e);
extern void action_page_brightness_pressed(lv_event_t * e);
extern void action_wifi_click(lv_event_t * e);
extern void action_network_config_cancel_click(lv_event_t * e);
extern void action_network_config_submit_click(lv_event_t * e);
extern void action_factory_reset_click(lv_event_t * e);
extern void action_return_home_click(lv_event_t * e);
extern void action_back_click(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/