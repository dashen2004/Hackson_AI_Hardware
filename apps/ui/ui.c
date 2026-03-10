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

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) {
        return 0;
    }
    return ((lv_obj_t **)&objects)[index];
}

void loadScreen(enum ScreensEnum screenId) {
    currentScreen = screenId - 1;
    // printf("currentScreen: %d\n", currentScreen);
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
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

    if (lvgl_get_var_timer_state() == TIMER_STATE_RUNNING) {
        tick_screen(SCREEN_ID_PAGE_TIMER - 1);
    }
}

#endif
