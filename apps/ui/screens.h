#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>
#include <stdbool.h>
#include "yolo11_pose.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *page_sleep;
    lv_obj_t *page_main;
    lv_obj_t *page_apps1;
    lv_obj_t *page_apps2;
    lv_obj_t *page_alarm;
    lv_obj_t *page_timer;
    lv_obj_t *page_pomodoro;
    lv_obj_t *page_eye;
    lv_obj_t *page_timetable;
    lv_obj_t *page_checkin;
    lv_obj_t *page_study;
    lv_obj_t *page_study_light_detect;
    lv_obj_t *page_study_light_hint;
    lv_obj_t *page_study_result;
    lv_obj_t *page_settings;
    lv_obj_t *page_alarm_edit;
    lv_obj_t *page_settings_sound;
    lv_obj_t *page_settings_net;

    // sleep page objs
    lv_obj_t *img_expression;

    // main page objs
    lv_obj_t *wifi_state;
    lv_obj_t *bat_state;
    lv_obj_t *img_weather_main;
    lv_obj_t *text_temp;
    lv_obj_t *text_time;
    lv_obj_t *text_weekday;
    lv_obj_t *text_date;

    // alarm page objs
    lv_obj_t *label_alarm;
    lv_obj_t *container_add;
    lv_obj_t *img_add;
    lv_obj_t *list_alarm;

    // timer page objs
    lv_obj_t *label_count;
    lv_obj_t *disp_countdown;
    lv_obj_t *roller_hour;
    lv_obj_t *roller_min;
    lv_obj_t *roller_sec;
    lv_obj_t *commoa1;
    lv_obj_t *commoa2;
    lv_obj_t *container_start;
    lv_obj_t *img_start;
    lv_obj_t *container_reset;
    lv_obj_t *img_reset;
    lv_obj_t *container_stop;
    lv_obj_t *img_stop;
    lv_obj_t *btn_bg;
    lv_obj_t *container_count_down;
    lv_obj_t *btn_count_down;
    lv_obj_t *label_count_down;
    lv_obj_t *container_count_up;
    lv_obj_t *btn_count_up;
    lv_obj_t *label_count_up;

    // pomodoro page objs
    lv_obj_t *tomato_icon;
    lv_obj_t *tomato_label;
    lv_obj_t *tomato_count;
    lv_obj_t *container_tomato_opt;
    lv_obj_t *tomato_opt;
    lv_obj_t *container_tomato_stop;
    lv_obj_t *tomato_stop;

    // eye page objs
    lv_obj_t *eye_icon;
    lv_obj_t *eye_count;
    lv_obj_t *eye_hint;
    lv_obj_t *container_eye_opt;
    lv_obj_t *eye_opt;
    lv_obj_t *container_eye_stop;
    lv_obj_t *eye_stop;

    // study page objs
    lv_obj_t *study_arc;
    lv_obj_t *study_count;
    lv_obj_t *study_btn_pause;
    lv_obj_t *study_btn_skip;

    // study light detect page objs
    lv_obj_t *study_light_detect_icon;
    lv_obj_t *study_light_detect_title;
    lv_obj_t *study_light_detect_subtitle;
    lv_obj_t *study_light_detect_score;

    // study light hint page objs
    lv_obj_t *study_light_hint_icon;
    lv_obj_t *study_light_hint_title;
    lv_obj_t *study_light_hint_desc;

    // study result page objs
    lv_obj_t *study_result_icon;
    lv_obj_t *study_result_title;
    lv_obj_t *study_result_subtitle;
    lv_obj_t *study_result_extra;
    lv_obj_t *study_result_btn;

    // apps2 hidden entry
    lv_obj_t *btn_study_entry;

    // alarm edit page objs
    lv_obj_t *label_alarm_edit;
    lv_obj_t *btn_alarm_confirm;
    lv_obj_t *btn_alarm_del;
    lv_obj_t *disp_alarm_edit;
    lv_obj_t *roller_alarm_hour;
    lv_obj_t *roller_alarm_min;
    lv_obj_t *btn_weekday_01;
    lv_obj_t *label_weekday_01;
    lv_obj_t *btn_weekday_02;
    lv_obj_t *label_weekday_02;
    lv_obj_t *btn_weekday_03;
    lv_obj_t *label_weekday_03;
    lv_obj_t *btn_weekday_04;
    lv_obj_t *label_weekday_04;
    lv_obj_t *btn_weekday_05;
    lv_obj_t *label_weekday_05;
    lv_obj_t *btn_weekday_06;
    lv_obj_t *label_weekday_06;
    lv_obj_t *btn_weekday_07;
    lv_obj_t *label_weekday_07;

    lv_obj_t *settings_mute_sw;
    lv_obj_t *settings_volume_slider;

} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_PAGE_NONE = 0,
    SCREEN_ID_PAGE_SLEEP = 1,
    SCREEN_ID_PAGE_MAIN = 2,
    SCREEN_ID_PAGE_APPS1 = 3,
    SCREEN_ID_PAGE_APPS2 = 4,
    SCREEN_ID_PAGE_ALARM = 5,
    SCREEN_ID_PAGE_TIMER = 6,
    SCREEN_ID_PAGE_POMODORO = 7,
    SCREEN_ID_PAGE_EYE = 8,
    SCREEN_ID_PAGE_TIMETABLE = 9,
    SCREEN_ID_PAGE_CHECKIN = 10,
    SCREEN_ID_PAGE_STUDY = 11,
    SCREEN_ID_PAGE_SETTINGS = 12,
    SCREEN_ID_PAGE_ALARM_EDIT = 13,
    SCREEN_ID_PAGE_SETTINGS_SOUND = 14,
    SCREEN_ID_PAGE_SETTINGS_NET = 15,
    SCREEN_ID_PAGE_STUDY_LIGHT_HINT = 16,
    SCREEN_ID_PAGE_STUDY_LIGHT_DETECT = 17,
    SCREEN_ID_PAGE_STUDY_RESULT = 18,
};

void create_screen_page_sleep();
void tick_screen_page_sleep();

void create_screen_page_main();
void tick_screen_page_main();

void create_screen_page_apps1();
void tick_screen_page_apps1();

void create_screen_page_apps2();
void tick_screen_page_apps2();

void create_screen_page_alarm();
void tick_screen_page_alarm();

void create_screen_page_timer();
void tick_screen_page_timer();

void create_screen_page_pomodoro();
void tick_screen_page_pomodoro();

void create_screen_page_eye();
void tick_screen_page_eye();

void create_screen_page_timetable();
void tick_screen_page_timetable();

void create_screen_page_checkin();
void tick_screen_page_checkin();

void create_screen_page_study();
void tick_screen_page_study();
void scr_study_session_start(void);
void scr_study_session_pause_toggle(void);
void scr_study_session_skip(void);
void scr_study_session_pause_if_running(void);
void scr_study_entry_btn_set_visible(bool visible);
void scr_study_capture_stop(void);
bool scr_study_capture_is_active(void);
void scr_study_pose_on_frame_result(const yolo11_pose_frame_result_t *result);
bool scr_study_pose_hint_is_showing(void);

void create_screen_page_study_light_detect();
void tick_screen_page_study_light_detect();
void scr_study_light_detect_start(void);
bool scr_study_light_detect_is_active(void);
void tick_study_light_detect(void);

void create_screen_page_study_light_hint();
void tick_screen_page_study_light_hint();

void create_screen_page_study_result();
void tick_screen_page_study_result();
void scr_study_result_confirm(void);

void create_screen_page_settings();
void tick_screen_page_settings();

void create_screen_page_settings_sound();
void tick_screen_page_settings_sound();

void create_screen_page_settings_net();
void tick_screen_page_settings_net();

void scr_settings_sound_sync(void);

void create_screen_page_alarm_edit();
void tick_screen_page_alarm_edit();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

lv_obj_t *ui_get_screen_by_id(enum ScreensEnum screenId);

void ui_update_page_indicator(int active_index);

int32_t scr_add_alarm_item(const char *time, const char *repeat);

int32_t scr_enter_page_alarm_add(void);
int32_t scr_enter_page_alarm_edit(int32_t alarm_index);
int32_t scr_weekday_check_state_toggle(int8_t weekday_index);

int32_t scr_alarm_edit_confirm(void);
int32_t scr_alarm_edit_exit(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
