#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void goto_page_by_id(int8_t screen_id);

extern void event_all_handler(lv_event_t * e);

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

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/