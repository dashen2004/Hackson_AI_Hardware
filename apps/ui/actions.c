#include <stdio.h>
#include <time.h>
#include "actions.h"
#include "ui.h"
#include "vars.h"
#include "screens.h"

#include "qm.h"
#include "qm_time.h"
#include "qm_work.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "bsp_light.h"
#include "audio_light.h"


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

// static uint8_t emotion_index = 0;

static bool hit_test(lv_event_t * e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    return lv_obj_hit_test((lv_obj_t *)lv_event_get_target(e), &pt);
}

// ******** 首页 ******** //
void goto_main_page(void) {
    
    loadScreenEx(SCREEN_ID_PAGE_MAIN);
}

// ******** 菜单页面 ******** //
static void goto_menu_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_MENU);
}

// ******** 灯光模式页面 ******** //
void action_nightlamp_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_NIGHT) {
        set_var_light_mode(BSP_LIGHT_MODE_NIGHT, true);
    }
}

void action_read_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_READ) {
        set_var_light_mode(BSP_LIGHT_MODE_READ, true);
    }
}

void action_idle_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_IDLE) {
        set_var_light_mode(BSP_LIGHT_MODE_IDLE, true);
    }
}

void action_sleep_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_SLEEP) {
        set_var_light_mode(BSP_LIGHT_MODE_SLEEP, true);
    }
}

static void goto_mode_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_MODE);
}

// ******** 灯光亮度调节页面 ******** //
void action_page_brightness_event_cb(lv_event_t * e) {
    event_all_handler(e);

    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    if (event == LV_EVENT_GESTURE) {
        lv_indev_wait_release(lv_indev_get_act()); 
        if (obj == objects.page_brightness) {
            lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
            if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
                goto_main_page();
            }
        }
    } else if (event == LV_EVENT_CLICKED) { 
        if (obj == objects.image_brightness_light_onoff) {
            if (hit_test(e)) {
                set_var_brightness_onoff(bsp_light_store_get()->on_off ? false : true);
            }
        }
    }
}

static void goto_brightness_page(void) {
    // 先获取led状态和亮度
    loadScreenEx(SCREEN_ID_PAGE_BRIGHTNESS);
} 

// ******** 设置页面 ******** //
static void goto_setup_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP);
}

// ******** 音量设置页面 ******** //
void action_volume_mute_click(lv_event_t * e) {
    if (hit_test(e)) {
        set_var_mute_value(get_var_mute_value() ? false : true, true);
    }
}

static void goto_volume_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP_VOLUME);
}

// ******** 熄屏设置页面 ******** //

// 自动息屏页点击30S
void action_screen_30s_click(lv_event_t * e) {
    if (hit_test(e) && get_var_scr_tmo_value() != 0) {
        set_var_scr_tmo_value(0, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

// 自动息屏页点击1分钟
void action_screen_1min_click(lv_event_t * e) {
    if (hit_test(e) && get_var_scr_tmo_value() != 1) {
        set_var_scr_tmo_value(1, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

// 自动息屏页点击5分钟
void action_screen_5min_click(lv_event_t * e) {
    if (hit_test(e) && get_var_scr_tmo_value() != 2) {
        set_var_scr_tmo_value(2, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

// 自动息屏页点击永不
void action_screen_never_click(lv_event_t * e) {
    if (hit_test(e) && get_var_scr_tmo_value() != 3) {
        set_var_scr_tmo_value(3, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

static void goto_screen_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP_SCREEN);
}

// ******** 语言设置页面 ******** //

static void goto_language_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP_LANGUAGE);
}

// ******** 开启配网 ******** //
void action_network_config_cancel_click(lv_event_t * e) {
    if (hit_test(e) && !lv_obj_has_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

static qm_work_t cloud_reset_delayed_action = { 0 };
extern int xjiang_cloud_reset(void);

static void cloud_reset_delayed_action_handler(void *arg) {
    xjiang_cloud_reset();
}

void action_network_config_submit_click(lv_event_t * e) {
    if (hit_test(e)) {
        
        lv_obj_add_flag  (objects.container_netconfig_prepare, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag  (objects.container_netconfig_process, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag  (objects.container_netconfig_success, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(objects.container_netconfig_reboot,  LV_OBJ_FLAG_HIDDEN);
        loadScreenEx(SCREEN_ID_PAGE_NETCONFIG);
        set_var_awss_value(true, true);
        
        qm_post_delayed_action(&cloud_reset_delayed_action, cloud_reset_delayed_action_handler, NULL, 3000);
        // xjiang_cloud_reset();
    }
}

static void goto_wifi_page(void) {
    if (lv_obj_has_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

// ******** 关于页面 ******** //
static void goto_about_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP_ABOUT);
}

// ******** 语音识别页面 ******** //
static void goto_recog_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_RECOG);
}

// ******** 配网页面 ******** //
static qm_timer_t netconfig_process_timer = { 0 };
static uint32_t   netconfig_start_tick = 0;
static uint8_t    wifi_level = 0;
static uint8_t    netconfig_success = 0;

extern bool_t get_connect(void);
void goto_netconfig_success_page(void);

static void netconfig_process_timer_handler(qm_timer_t *timer,  void *arg) {
    set_var_turnoff_value(qm_now_ms());
    if (netconfig_success == 0) {
        if (qm_now_ms() - netconfig_start_tick >= 3 * 60 * 1000) {
            lvgl_port_lock(0); 
            goto_main_page(); 
            lvgl_port_unlock();

            qm_timer_stop(timer); 
            qm_timer_free(timer);
            return;
        }
        if (get_connect()) {
            lvgl_port_lock(0);
            goto_netconfig_success_page();
            lvgl_port_unlock();
            netconfig_success = 1;
            qm_timer_change(timer, 3000);
            return;
        }

        lvgl_port_lock(0);
        switch (wifi_level) {
            case 0: {
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.image_netconfig_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.image_netconfig_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.image_netconfig_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
                break;
            }
            case 1: {
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.image_netconfig_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.image_netconfig_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
                break;
            }
            case 2: {
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.image_netconfig_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
                break;
            }
            case 3: {
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.image_netconfig_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
                break;
            }
            default: {
                break;
            }
        }
        lvgl_port_unlock();
        if (++wifi_level > 3) {
            wifi_level = 0;
        }
    } else {
        lvgl_port_lock(0);
        goto_main_page();
        lvgl_port_unlock();

        qm_timer_stop(timer); 
        qm_timer_free(timer);
    }
}

void goto_netconfig_process_page(void) {
    lv_obj_add_flag  (objects.container_netconfig_prepare, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (objects.container_netconfig_success, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (objects.container_netconfig_reboot,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.container_netconfig_process, LV_OBJ_FLAG_HIDDEN);
    if (get_current_screen() != SCREEN_ID_PAGE_NETCONFIG) {
        loadScreenEx(SCREEN_ID_PAGE_NETCONFIG);
    }
    if (!qm_timer_is_valid(&netconfig_process_timer)) {
        wifi_level = 0;
        netconfig_success = 0;
        netconfig_start_tick = qm_now_ms();
        qm_timer_new  (&netconfig_process_timer, netconfig_process_timer_handler, NULL, 500, 1);
        qm_timer_start(&netconfig_process_timer);
    }
}

void goto_netconfig_success_page(void) {
    lv_obj_add_flag  (objects.container_netconfig_prepare, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (objects.container_netconfig_process, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (objects.container_netconfig_reboot,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.container_netconfig_success, LV_OBJ_FLAG_HIDDEN);
    if (get_current_screen() != SCREEN_ID_PAGE_NETCONFIG) {
        loadScreenEx(SCREEN_ID_PAGE_NETCONFIG);
    }
}

// ******** ota页面 ******** //
void goto_ota_process_page(void) {
    if (get_current_screen() != SCREEN_ID_PAGE_OTA) {
        loadScreenEx(SCREEN_ID_PAGE_OTA);
    }
}

void goto_ota_success_page(void) {
    if (get_current_screen() != SCREEN_ID_PAGE_OTA) {
        loadScreenEx(SCREEN_ID_PAGE_OTA);
    }
}

void goto_ota_failure_page(void) {
    if (get_current_screen() != SCREEN_ID_PAGE_OTA) {
        loadScreenEx(SCREEN_ID_PAGE_OTA);
    }
}

// ******** 总体手势 ******** //
void action_gesture_handler(lv_event_t * e) {
    lv_indev_wait_release(lv_indev_get_act()); // 避免与CLICKED冲突

    lv_obj_t *currentPage = lv_scr_act();
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (currentPage == objects.page_main) {
        if (dir == LV_DIR_LEFT) {
            goto_recog_page();
        } else if (dir == LV_DIR_RIGHT) {
            goto_brightness_page();
        } else if (dir == LV_DIR_TOP) {
            goto_menu_page();
        } else if (dir == LV_DIR_BOTTOM) {
            goto_setup_page();
        }
    }
    else if (currentPage == objects.page_brightness) {
        if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
            goto_main_page();
        }
    }
    else if (currentPage == objects.page_mode || currentPage == objects.page_brightness || currentPage == objects.page_setup || currentPage == objects.page_recog) {
        if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
            goto_main_page();
        }
    }
    else if (currentPage == objects.page_setup_volume || currentPage == objects.page_setup_screen || currentPage == objects.page_setup_language || currentPage == objects.page_setup_about) {
        if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
            goto_setup_page();
        }
    }
}

// ******** 总体事件(来用亮屏) ******** //
void event_all_handler(lv_event_t * e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_PRESSED) {
        extern void turnoff_timer_reset(void);
        turnoff_timer_reset();
    }
}

void action_home_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_main_page();
    }
}

void action_recog_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_recog_page();
    }
}

void action_setup_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_setup_page();
    }
}

void action_mode_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_mode_page();
    }
}

void action_brightness_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_brightness_page();
    }
}

void action_volume_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_volume_page();
    }
}

void action_screen_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_screen_page();
    }
}

void action_language_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_language_page();
    }
}

void action_wifi_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_wifi_page();
    }
}

void action_about_click(lv_event_t * e) {
    if (hit_test(e)) {
        goto_about_page();
    }
}

void action_language_cn_click(lv_event_t * e) {
    if (hit_test(e) && get_var_language_value() != 0) {
        set_var_language_value(0, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

void action_language_en_click(lv_event_t * e) {
    if (hit_test(e) && get_var_language_value() != 1) {
        set_var_language_value(1, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

