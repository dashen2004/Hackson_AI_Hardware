#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#include "ui.h"
#include "screens.h"
#include "actions.h"
#include "vars.h"

#if defined(EEZ_FOR_LVGL)

void ui_init() {
    eez_flow_init(assets, sizeof(assets), (lv_obj_t **)&objects, sizeof(objects), images, sizeof(images), actions);
}

void ui_tick() {
    eez_flow_tick();
    tick_screen(g_currentScreen);
}

#else

#include <stdio.h>
#include <string.h>

static int16_t currentScreen = -1;

lv_obj_t *ui_get_screen_by_id(enum ScreensEnum screenId)
{
    switch (screenId) {
        case SCREEN_ID_PAGE_SLEEP:           return objects.page_sleep;
        case SCREEN_ID_PAGE_MAIN:            return objects.page_main;
        case SCREEN_ID_PAGE_APPS1:           return objects.page_apps1;
        case SCREEN_ID_PAGE_APPS2:           return objects.page_apps2;
        case SCREEN_ID_PAGE_ALARM:           return objects.page_alarm;
        case SCREEN_ID_PAGE_TIMER:           return objects.page_timer;
        case SCREEN_ID_PAGE_POMODORO:        return objects.page_pomodoro;
        case SCREEN_ID_PAGE_EYE:             return objects.page_eye;
        case SCREEN_ID_PAGE_TIMETABLE:       return objects.page_timetable;
        case SCREEN_ID_PAGE_CHECKIN:         return objects.page_checkin;
        case SCREEN_ID_PAGE_STUDY:           return objects.page_study;
        case SCREEN_ID_PAGE_STUDY_LIGHT_DETECT: return objects.page_study_light_detect;
        case SCREEN_ID_PAGE_STUDY_LIGHT_HINT: return objects.page_study_light_hint;
        case SCREEN_ID_PAGE_SETTINGS:        return objects.page_settings;
        case SCREEN_ID_PAGE_ALARM_EDIT:      return objects.page_alarm_edit;
        case SCREEN_ID_PAGE_SETTINGS_SOUND:   return objects.page_settings_sound;
        case SCREEN_ID_PAGE_SETTINGS_NET:     return objects.page_settings_net;
        default:                             return NULL;
    }
}

void loadScreen(enum ScreensEnum screenId) {
    lv_obj_t *screen = ui_get_screen_by_id(screenId);
    if (screen == NULL) {
        return;
    }

    currentScreen = screenId - 1;
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

enum ScreensEnum get_current_screen(void) {
    return currentScreen + 1;
}

void ui_init() {
    create_screens();

    goto_page_by_id(SCREEN_ID_PAGE_MAIN);
}

void ui_tick() {
    tick_screen(currentScreen);

    if (scr_study_light_detect_is_active()) {
        tick_study_light_detect();
    }

    if (lvgl_var_timer_state_get() == TIMER_STATE_RUNNING) {
        tick_screen(SCREEN_ID_PAGE_TIMER - 1);
    }
}

#endif
