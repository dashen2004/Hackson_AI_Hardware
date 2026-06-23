#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *page_sleep;
    lv_obj_t *page_main;
    lv_obj_t *page_alarm;
    lv_obj_t *page_timer;
    lv_obj_t *page_pomodoro;
    lv_obj_t *page_weather;
    lv_obj_t *page_alarm_edit;

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
    lv_obj_t *canvas_camera;
    lv_obj_t *btn_camera;
    lv_obj_t *label_camera;

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

    // weather page objs
    lv_obj_t *label_city;
    lv_obj_t *img_weather;
    lv_obj_t *label_weather;
    lv_obj_t *label_temperature;

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

} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_PAGE_NONE = 0,
    SCREEN_ID_PAGE_SLEEP = 1,
    SCREEN_ID_PAGE_MAIN = 2,
    SCREEN_ID_PAGE_ALARM = 3,
    SCREEN_ID_PAGE_TIMER = 4,
    SCREEN_ID_PAGE_POMODORO = 5,
    SCREEN_ID_PAGE_WEATHER = 6,

    // extra pages
    SCREEN_ID_PAGE_ALARM_EDIT = 7,
};

void create_screen_page_sleep();
void tick_screen_page_sleep();

void create_screen_page_main();
void tick_screen_page_main();

void create_screen_page_alarm();
void tick_screen_page_alarm();

void create_screen_page_timer();
void tick_screen_page_timer();

void create_screen_page_pomodoro();
void tick_screen_page_pomodoro();

void create_screen_page_weather();
void tick_screen_page_weather();

void create_screen_page_alarm_edit();
void tick_screen_page_alarm_edit();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

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