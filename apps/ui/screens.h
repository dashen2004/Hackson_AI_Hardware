#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *page_main;
    lv_obj_t *page_menu;
    lv_obj_t *page_mode;
    lv_obj_t *page_brightness;
    lv_obj_t *page_setup;
    lv_obj_t *page_setup_volume;
    lv_obj_t *page_setup_screen;
    lv_obj_t *page_setup_language;
    lv_obj_t *page_setup_about;
    lv_obj_t *page_recog;
    lv_obj_t *brightness_top;
    lv_obj_t *brightness_bottom;
    lv_obj_t *setup_volume_value;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *icon_wifi;
    lv_obj_t *icon_weather;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *menu_home;
    lv_obj_t *menu_speech;
    lv_obj_t *menu_setup;
    lv_obj_t *menu_mode;
    lv_obj_t *menu_brightness;
    lv_obj_t *mode_nightlamp;
    lv_obj_t *mode_read;
    lv_obj_t *mode_relaxation;
    lv_obj_t *mode_sleep;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *brightness_light_onoff;
    lv_obj_t *slider_container;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *setup_volume_mute_onoff;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *setup_screen_tmo_30s;
    lv_obj_t *obj24;
    lv_obj_t *setup_screen_tmo_1min;
    lv_obj_t *obj25;
    lv_obj_t *setup_screen_tmo_5min;
    lv_obj_t *obj26;
    lv_obj_t *setup_screen_tmo_never;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *obj29;
    lv_obj_t *setup_language_cn;
    lv_obj_t *obj30;
    lv_obj_t *setup_language_en;
    lv_obj_t *obj31;
    lv_obj_t *obj32;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *obj35;
    lv_obj_t *obj36;
    lv_obj_t *obj37;
    lv_obj_t *obj38;
    lv_obj_t *obj39;
    lv_obj_t *obj40;
    lv_obj_t *emotion;
    lv_obj_t *subtitles;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_PAGE_MAIN = 1,
    SCREEN_ID_PAGE_MENU = 2,
    SCREEN_ID_PAGE_MODE = 3,
    SCREEN_ID_PAGE_BRIGHTNESS = 4,
    SCREEN_ID_PAGE_SETUP = 5,
    SCREEN_ID_PAGE_SETUP_VOLUME = 6,
    SCREEN_ID_PAGE_SETUP_SCREEN = 7,
    SCREEN_ID_PAGE_SETUP_LANGUAGE = 8,
    SCREEN_ID_PAGE_SETUP_ABOUT = 9,
    SCREEN_ID_PAGE_RECOG = 10,
};

void create_screen_page_main();
void tick_screen_page_main();

void create_screen_page_menu();
void tick_screen_page_menu();

void create_screen_page_mode();
void tick_screen_page_mode();

void create_screen_page_brightness();
void tick_screen_page_brightness();

void create_screen_page_setup();
void tick_screen_page_setup();

void create_screen_page_setup_volume();
void tick_screen_page_setup_volume();

void create_screen_page_setup_screen();
void tick_screen_page_setup_screen();

void create_screen_page_setup_language();
void tick_screen_page_setup_language();

void create_screen_page_setup_about();
void tick_screen_page_setup_about();

void create_screen_page_recog();
void tick_screen_page_recog();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/