#include <stdio.h>
#include <time.h>
#include "actions.h"
#include "ui.h"
#include "vars.h"
#include "screens.h"

#include "esp_timer.h"

#include "bsp_light.h"

/*
    有些gif刚加载时会白色闪烁, 所以在lv_gif.c中做一些屏蔽处理
    1. 添加全部变量 static uint8_t hidden_frame_counter = 0;
    2. 
 */

/*
 * 关于手势 与 按钮clicked冲突: 
 *     在pressed事件中, 如果original_target为page, 则disable button, 直到released事件或gesture事件
 * 关于按钮按下后滑出按钮区域后还能触发clicked: 
 *     在clicked事件中判断手指是否还在按钮上(hit_test)
 */

#define loadScreenEx    loadScreen
// extern void loadScreenEx(enum ScreensEnum screenId);

static uint8_t emotion_index = 0;

static bool hit_test(lv_event_t * e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    return lv_obj_hit_test((lv_obj_t *)lv_event_get_target(e), &pt);
}

void action_page_brightness_event_cb(lv_event_t * e) {
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    if (event == LV_EVENT_PRESSED) {
        // if (obj == objects.page_brightness && !need_restore) {
            printf("pressed\n");
        //     need_restore = true;
        //     if (!(last_brightness_light_onoff_disabled = lv_obj_has_state(objects.brightness_light_onoff, LV_STATE_DISABLED))) { lv_obj_add_state(objects.brightness_light_onoff, LV_STATE_DISABLED); }
        //     if (!(last_brightness_top_disabled = lv_obj_has_state(objects.brightness_top, LV_STATE_DISABLED))) { lv_obj_add_state(objects.brightness_top, LV_STATE_DISABLED); }
        //     if (!(last_brightness_bottom_disabled = lv_obj_has_state(objects.brightness_bottom, LV_STATE_DISABLED))) { lv_obj_add_state(objects.brightness_bottom, LV_STATE_DISABLED); }
        // }
    } else if (event == LV_EVENT_RELEASED || event == LV_EVENT_GESTURE) {
        if (event == LV_EVENT_RELEASED) {
            printf("released\n");
        } else {
            printf("gesture\n");
        }
        // if (need_restore) {
        //     need_restore = 0;
        //     if (!last_brightness_light_onoff_disabled) { lv_obj_remove_state(objects.brightness_light_onoff, LV_STATE_DISABLED); }
        //     if (!last_brightness_top_disabled) { lv_obj_remove_state(objects.brightness_top, LV_STATE_DISABLED); }
        //     if (!last_brightness_bottom_disabled) { lv_obj_remove_state(objects.brightness_bottom, LV_STATE_DISABLED); }
        // }
        if (event == LV_EVENT_GESTURE) {
//            lv_indev_wait_release(lv_indev_get_act()); 
            // if (obj == objects.page_brightness) {
            //     lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
            //     if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
            //         loadScreenEx(SCREEN_ID_PAGE_MAIN);
            //     }
            // }
        }
    } else if (event == LV_EVENT_CLICKED) { 
        printf("clicked\n");
        if (obj == objects.brightness_light_onoff) {
            if (hit_test(e)) {
                if (get_var_brightness_light_onoff()) {
                    set_var_brightness_light_onoff(false);
                } else {
                    set_var_brightness_light_onoff(true);
                }
            }
        }
    }
}

void action_gesture_handler(lv_event_t * e) {
    lv_indev_wait_release(lv_indev_get_act()); // 避免与CLICKED冲突

    lv_obj_t *currentPage = lv_scr_act();
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (currentPage == objects.page_main) {
        if (dir == LV_DIR_LEFT) {
            switch (emotion_index) {
                case 0: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion01.gif"); break;
                case 1: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion02.gif"); break;
                case 2: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion03.gif"); break;
                case 3: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion04.gif"); break;
                case 4: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion05.gif"); break;
                case 5: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion06.gif"); break;
                case 6: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion07.gif"); break;
                case 7: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion08.gif"); break;
                case 8: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion09.gif"); break;
                case 9: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion10.gif"); break;
                case 10: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion11.gif"); break;
                case 11: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion12.gif"); break;
                case 12: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion13.gif"); break;
                case 13: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion14.gif"); break;
                case 14: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion15.gif"); break;
                case 15: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion16.gif"); break;
                case 16: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion17.gif"); break;
                case 17: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion18.gif"); break;
                case 18: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion19.gif"); break;
                case 19: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion20.gif"); break;
                case 20: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion21.gif"); break;
                default: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion01.gif"); break;
            }
            if (emotion_index < 20) {
                emotion_index++;
            } else {
                emotion_index = 0;
            }
            loadScreenEx(SCREEN_ID_PAGE_RECOG);
        } else if (dir == LV_DIR_RIGHT) {            
            // lv_obj_add_event_cb(objects.page_brightness, event_hook, LV_EVENT_ALL, NULL);
            loadScreenEx(SCREEN_ID_PAGE_BRIGHTNESS);
            set_var_brightness_light_onoff(bsp_light_store_get()->on_off);
            // set_var_brightness_light_onoff(get_var_brightness_light_onoff());
        } else if (dir == LV_DIR_TOP) {
            loadScreenEx(SCREEN_ID_PAGE_MENU);
        } else if (dir == LV_DIR_BOTTOM) {
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
    else if (currentPage == objects.page_brightness) {
        // if (last_page_brightness_pressed_obj == objects.page_brightness) {
            if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
                loadScreenEx(SCREEN_ID_PAGE_MAIN);
            }
        // }
    }
    else if (currentPage == objects.page_mode || currentPage == objects.page_brightness || currentPage == objects.page_setup || currentPage == objects.page_recog) {
        if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
            loadScreenEx(SCREEN_ID_PAGE_MAIN);
        }
    }
    else if (currentPage == objects.page_setup_volume || currentPage == objects.page_setup_screen || currentPage == objects.page_setup_language || currentPage == objects.page_setup_about) {
        if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 菜单页点击主页按键
void action_memu_home_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_MAIN);
    }
}

// 菜单页点击语音按键
void action_menu_speech_click(lv_event_t * e) {
    if (hit_test(e)) {
        switch (emotion_index) {
            case 0: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion01.gif"); break;
            case 1: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion02.gif"); break;
            case 2: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion03.gif"); break;
            case 3: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion04.gif"); break;
            case 4: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion05.gif"); break;
            case 5: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion06.gif"); break;
            case 6: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion07.gif"); break;
            case 7: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion08.gif"); break;
            case 8: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion09.gif"); break;
            case 9: lv_gif_set_src(objects.emotion,  "/lvgl/images/emotion10.gif"); break;
            case 10: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion11.gif"); break;
            case 11: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion12.gif"); break;
            case 12: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion13.gif"); break;
            case 13: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion14.gif"); break;
            case 14: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion15.gif"); break;
            case 15: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion16.gif"); break;
            case 16: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion17.gif"); break;
            case 17: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion18.gif"); break;
            case 18: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion19.gif"); break;
            case 19: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion20.gif"); break;
            case 20: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion21.gif"); break;
            default: lv_gif_set_src(objects.emotion, "/lvgl/images/emotion01.gif"); break;
        }
        if (emotion_index < 20) {
            emotion_index++;
        } else {
            emotion_index = 0;
        }
        loadScreenEx(SCREEN_ID_PAGE_RECOG);
    }
}

// 菜单页点击亮度按键
void action_menu_brightness_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_BRIGHTNESS);
        set_var_brightness_light_onoff(bsp_light_store_get()->on_off);
    }
}

// 菜单页点击设置按键
void action_menu_setup_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

// 菜单页点击模式按键
void action_menu_mode_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_MODE);
        set_var_mode(bsp_light_store_get()->mode);
    }
}

// 设置页点击声音设置
void action_setup_volume_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_SETUP_VOLUME);
    }
}

// 设置页点击自动息屏
void action_setup_screen_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_SETUP_SCREEN);
    }
}

// 设置页点击语言
void action_setup_language_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_SETUP_LANGUAGE);
    }
}

// 设置页点击关于设备
void action_setup_about_click(lv_event_t * e) {
    if (hit_test(e)) {
        loadScreenEx(SCREEN_ID_PAGE_SETUP_ABOUT);
    }
}

// 亮度设置页拖动亮度(上)
void action_brightness_top_changed(lv_event_t * e) {
    // lv_obj_t * slider = lv_event_get_target(e);
    // int value = lv_slider_get_value(slider);
    // set_var_brightness_top(value);
}

// 亮度设置页拖动亮度(下)
void action_brightness_bottom_changed(lv_event_t * e) {
    // lv_obj_t * slider = lv_event_get_target(e);
    // int value = lv_slider_get_value(slider);
    // set_var_brightness_bottom(value);
}

// 亮度设置页点击开关
void action_brightness_light_onoff_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_brightness_light_onoff()) {
            set_var_brightness_light_onoff(false);
        } else {
            set_var_brightness_light_onoff(true);
        }
    }
}

// 模式页点击小夜灯
void action_mode_nightlamp_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_mode() != 1) {
           set_var_mode(1);
        }
    }
}

// 模式页点击阅读
void action_mode_read_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_mode() != 2) {
            set_var_mode(2);
        }
    }
}

// 模式页点击休闲
void action_mode_relaxation_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_mode() != 3) {
            set_var_mode(3);
        }
    }
}

// 模式页点击睡觉
void action_mode_sleep_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_mode() != 4) {
            set_var_mode(4);
        }
    }
}

// 声音设置页点击静音
void action_setup_volume_mute_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_setup_volume_mute_onoff()) {
            set_var_setup_volume_mute_onoff(false);
        } else {
            set_var_setup_volume_mute_onoff(true);
        }
    }
}

// 自动息屏页点击30S
void action_setup_screen_30s_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_screen_tmo() != 0) {
            set_var_screen_tmo(0);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 自动息屏页点击1分钟
void action_setup_screen_1min_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_screen_tmo() != 1) {
            set_var_screen_tmo(1);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 自动息屏页点击5分钟
void action_setup_screen_5min_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_screen_tmo() != 2) {
            set_var_screen_tmo(2);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 自动息屏页点击永不
void action_setup_screen_never_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_screen_tmo() != 3) {
            set_var_screen_tmo(3);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 声音设置页拖动音量
void action_volume_value_changed(lv_event_t * e) {
    // lv_obj_t * slider = lv_event_get_target(e);
    // int value = lv_slider_get_value(slider);
    // set_var_setup_volume_value(value);
}

void action_setup_language_cn_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_language() != 0) {
            set_var_language(0);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

void action_setup_language_en_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_language() != 1) {
            set_var_language(1);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}
