#include <stdio.h>
#include <time.h>
#include "actions.h"
#include "ui.h"
#include "vars.h"
#include "screens.h"

#include "qm.h"
#include "qm_time.h"
#include "qm_work.h"
#include "qm_utils_timer.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "bsp_light.h"
#include "audio_light.h"
#include "audio_main.h"

#include "lcd.h"


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

extern int bsp_lcd_bl_brightness_percent_get(void);

// static uint8_t emotion_index = 0;

static bool hit_test(lv_event_t * e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    return lv_obj_hit_test((lv_obj_t *)lv_event_get_target(e), &pt);
}

// ******** 首页 ******** //
static qm_timer_t wifi_blink_timer = { 0 };
static uint8_t    wifi_level_s = 0;
extern uint8_t lv_port_init_flag;

static void wifi_blink_timer_handler(qm_timer_t *timer,  void *arg) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    switch (wifi_level_s) {
        case 0: {
            lv_obj_add_flag  (objects.image_wifi_level_s_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_s_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_s_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_s_4, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case 1: {
            lv_obj_add_flag  (objects.image_wifi_level_s_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.image_wifi_level_s_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_s_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_s_4, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case 2: {
            lv_obj_add_flag  (objects.image_wifi_level_s_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.image_wifi_level_s_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.image_wifi_level_s_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_s_4, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case 3: {
            lv_obj_add_flag  (objects.image_wifi_level_s_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.image_wifi_level_s_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.image_wifi_level_s_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.image_wifi_level_s_4, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        default: {
            break;
        }
    }
    lvgl_port_unlock();
    if (++wifi_level_s > 3) {
        wifi_level_s = 0;
    }
}

void wifi_set_disconnected(void) {
    QM_LOGD("wifi","wifi set disconnected\n");
    if (qm_timer_is_valid(&wifi_blink_timer)) {
        qm_timer_stop(&wifi_blink_timer);
        qm_timer_free(&wifi_blink_timer);
    }
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    lv_obj_add_flag(objects.image_wifi_level_s_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.image_wifi_level_s_2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.image_wifi_level_s_3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.image_wifi_level_s_4, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(objects.image_wifi, "/lvgl/images/wifi_disconnected.bin");
    lvgl_port_unlock();
}

void wifi_set_connected(void) {
    QM_LOGD("wifi","wifi set connected\n");
    if (qm_timer_is_valid(&wifi_blink_timer)) {
        qm_timer_stop(&wifi_blink_timer);
        qm_timer_free(&wifi_blink_timer);
    }
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    lv_obj_add_flag(objects.image_wifi_level_s_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.image_wifi_level_s_2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.image_wifi_level_s_3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.image_wifi_level_s_4, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(objects.image_wifi, "/lvgl/images/wifi_connected.bin");
    lvgl_port_unlock();
}

void wifi_set_blink(void) {
    QM_LOGD("wifi","wifi set blink\n");
    if (!qm_timer_is_valid(&wifi_blink_timer)) {
        qm_timer_new  (&wifi_blink_timer, wifi_blink_timer_handler, NULL, 500, 1);
        qm_timer_start(&wifi_blink_timer);
    }
    wifi_level_s = 1;
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    lv_img_set_src(objects.image_wifi, "/lvgl/images/wifi_connected.bin");
    lv_img_set_src(objects.image_wifi_level_s_1, "/lvgl/images/wifi_level_s_1.bin");
    lv_img_set_src(objects.image_wifi_level_s_2, "/lvgl/images/wifi_level_s_2.bin");
    lv_img_set_src(objects.image_wifi_level_s_3, "/lvgl/images/wifi_level_s_3.bin");
    lv_img_set_src(objects.image_wifi_level_s_4, "/lvgl/images/wifi_level_s_4.bin");
    lv_obj_add_flag(objects.image_wifi_level_s_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.image_wifi_level_s_2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.image_wifi_level_s_3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.image_wifi_level_s_4, LV_OBJ_FLAG_HIDDEN);
    lvgl_port_unlock();
}

void goto_main_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    loadScreenEx(SCREEN_ID_PAGE_MAIN);
    lvgl_port_unlock();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

// ******** 菜单页面 ******** //
static void goto_menu_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_MENU);
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

// ******** 灯光模式页面 ******** //
void action_nightlamp_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && bsp_light_store_get()->mode != BSP_LIGHT_MODE_NIGHT) {
        set_var_light_mode(BSP_LIGHT_MODE_NIGHT, true);
        turnoff_timer_reset(0);
    }
}

void action_read_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && bsp_light_store_get()->mode != BSP_LIGHT_MODE_READ) {
        set_var_light_mode(BSP_LIGHT_MODE_READ, true);
        turnoff_timer_reset(0);
    }
}

void action_idle_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && bsp_light_store_get()->mode != BSP_LIGHT_MODE_IDLE) {
        set_var_light_mode(BSP_LIGHT_MODE_IDLE, true);
        turnoff_timer_reset(0);
    }
}

void action_sleep_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && bsp_light_store_get()->mode != BSP_LIGHT_MODE_SLEEP) {
        set_var_light_mode(BSP_LIGHT_MODE_SLEEP, true);
        turnoff_timer_reset(0);
    }
}

static void goto_mode_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    loadScreenEx(SCREEN_ID_PAGE_MODE);
}

// ******** 灯光亮度调节页面 ******** //
void action_page_brightness_event_cb(lv_event_t * e) {
    event_all_handler(e);

    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    // if (event == LV_EVENT_GESTURE) {
    //     lv_indev_wait_release(lv_indev_get_act()); 
    //     if (obj == objects.page_brightness) {
    //         lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    //         if (bsp_lcd_bl_brightness_percent_get() != 0 && (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT)) {
    //             goto_main_page();
    //         }
    //     }
    // } else 
    if (event == LV_EVENT_CLICKED) { 
        if (obj == objects.image_brightness_light_onoff) {
            if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
                set_var_brightness_onoff(bsp_light_store_get()->on_off ? false : true);
            }
        }
    }
}

static void goto_brightness_page(void) {
    // 先获取led状态和亮度
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    loadScreenEx(SCREEN_ID_PAGE_BRIGHTNESS);
} 

static void goto_rgb_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    loadScreenEx(SCREEN_ID_PAGE_RGB);
}

// ******** 设置页面 ******** //
static void goto_setup_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    loadScreenEx(SCREEN_ID_PAGE_SETUP);
}

// ******** 音量设置页面 ******** //
void action_volume_mute_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        set_var_mute_value(get_var_mute_value() ? false : true, true);
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    }
}

static void goto_volume_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    loadScreenEx(SCREEN_ID_PAGE_SETUP_VOLUME);
}

// ******** 熄屏设置页面 ******** //

// 自动息屏页点击30S
void action_screen_30s_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && get_var_scr_tmo_value() != 0) {
        set_var_scr_tmo_value(0, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

// 自动息屏页点击1分钟
void action_screen_1min_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && get_var_scr_tmo_value() != 1) {
        set_var_scr_tmo_value(1, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

// 自动息屏页点击5分钟
void action_screen_5min_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && get_var_scr_tmo_value() != 2) {
        set_var_scr_tmo_value(2, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

// 自动息屏页点击永不
void action_screen_never_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && get_var_scr_tmo_value() != 3) {
        set_var_scr_tmo_value(3, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

static void goto_screen_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    loadScreenEx(SCREEN_ID_PAGE_SETUP_SCREEN);
}

// ******** 语言设置页面 ******** //

static void goto_language_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    loadScreenEx(SCREEN_ID_PAGE_SETUP_LANGUAGE);
}

// ******** 开启配网 ******** //
static qm_timer_t netconfig_reboot_timer = { 0 };
static int8_t     netconfig_reboot_countdown = 3;

extern int xjiang_cloud_reset(void);
extern bool_t get_connect(void);
void goto_netconfig_success_page(void);

static void netconfig_reboot_timer_handler(qm_timer_t *timer,  void *arg) {
    if (--netconfig_reboot_countdown >= 0) {
        char s[32];
        sprintf(s, "%d秒后重启设备", netconfig_reboot_countdown);
        if(lv_port_init_flag == 0) {
            return;
        }
        lvgl_port_lock(0);
        lv_label_set_text(objects.text_netconfig_reboot_title, s);
        lvgl_port_unlock();
    } else {
        qm_timer_stop(timer); 
        qm_timer_free(timer);
        xjiang_cloud_reset();
    }
}


void action_network_config_cancel_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && !lv_obj_has_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

// static qm_work_t cloud_reset_delayed_action = { 0 };

// static void cloud_reset_delayed_action_handler(void *arg) {
//     xjiang_cloud_reset();
// }

static uint8_t win_flag = 0;

void action_network_config_submit_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        if (win_flag == 1) {
            set_var_mute_value(false, true);    // 静音: 默认false
            set_var_volume_value(CONFIG_AUDIP_VOLUME_DEFULT, true);     // 音量: 默认80
            set_var_scr_tmo_value(0, true);     // 熄屏时间: 默认0(30S)
            set_var_language_value(0, false);   // 语言: 默认0(中文)
            xjiang_cloud_reset();
        } else {
            lv_obj_add_flag  (objects.container_netconfig_prepare, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.container_netconfig_process, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.container_netconfig_success, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.container_netconfig_reboot,  LV_OBJ_FLAG_HIDDEN);
            loadScreenEx(SCREEN_ID_PAGE_NETCONFIG);
            set_var_awss_value(true, true);

            if (!qm_timer_is_valid(&netconfig_reboot_timer)) {
                netconfig_reboot_countdown = 3;
                qm_timer_new  (&netconfig_reboot_timer, netconfig_reboot_timer_handler, NULL, 1000, 1);
                qm_timer_start(&netconfig_reboot_timer);
            }
        }
        
        // qm_post_delayed_action(&cloud_reset_delayed_action, cloud_reset_delayed_action_handler, NULL, 3000);
        // xjiang_cloud_reset();
    }
}

static void goto_wifi_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    if (lv_obj_has_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN)) {
        char *s = "点击确定后，设备将进入\n重新配网状态";
        lv_label_set_text(objects.text_network_config_infomation, s);
        win_flag = 0;
        lv_obj_clear_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

static void goto_factory_reset_page(void) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    if (lv_obj_has_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN)) {
        char *s = "点击确定\n设备将恢复出厂设置";
        lv_label_set_text(objects.text_network_config_infomation, s);
        win_flag = 1;
        lv_obj_clear_flag(objects.container_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

// ******** 关于页面 ******** //
static void goto_about_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP_ABOUT);
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
}

// 恢复出厂设置
void action_factory_reset_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_factory_reset_page();
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    }
}

// ******** 语音识别页面 ******** //
void goto_recog_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    set_var_recog_page_leave(0);
    loadScreenEx(SCREEN_ID_PAGE_RECOG);
    lvgl_port_unlock();
}

// ******** 配网页面 ******** //
static qm_timer_t netconfig_process_timer = { 0 };
static qm_timer_t netconfig_success_timer = { 0 };
static qm_utils_time_t   netconfig_start_tick = {0};
static int8_t     wifi_level = 0;
static int8_t     netconfig_success = 0;
static int8_t     netconfig_success_countdown = 3;
static enum ScreensEnum last_screen;

static void netconfig_process_timer_handler(qm_timer_t *timer,  void *arg) {
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);
    if (netconfig_success == 0) {
        if (qm_utils_time_is_expired(&netconfig_start_tick)) {
            if (get_current_screen() == SCREEN_ID_PAGE_NETCONFIG) {
                goto_main_page(); 
            }
            qm_timer_stop(timer); 
            qm_timer_free(timer);
            return;
        }
        if (get_connect()) {
            last_screen = (get_current_screen() == SCREEN_ID_PAGE_NETCONFIG ? SCREEN_ID_PAGE_MAIN : get_current_screen());
            goto_netconfig_success_page();
            netconfig_success = 1;
            qm_timer_change(timer, 3000);
            return;
        }
        if(lv_port_init_flag == 0) {
            if (++wifi_level > 3) {
                wifi_level = 0;
            }
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
        if(lv_port_init_flag == 0) {
            return;
        }
        lvgl_port_lock(0);
        loadScreenEx(last_screen); // goto_main_page();
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
        qm_utils_time_countdown_ms(&netconfig_start_tick, 3 * 60 * 1000);
        qm_timer_new  (&netconfig_process_timer, netconfig_process_timer_handler, NULL, 500, 1);
        qm_timer_start(&netconfig_process_timer);
    }
}

static void netconfig_success_timer_handler(qm_timer_t *timer,  void *arg) {
    if (--netconfig_success_countdown >= 0) {
        char s[32];
        sprintf(s, "%d秒后返回主页", netconfig_success_countdown);
        if(lv_port_init_flag == 0) {
            return;
        }
        lvgl_port_lock(0);
        lv_label_set_text(objects.text_netconfig_success_countdown, s);
        lvgl_port_unlock();
    } else {
        qm_timer_stop(timer); 
        qm_timer_free(timer);
    }
}

void goto_netconfig_success_page(void) {
    if(lv_port_init_flag == 0) {
        return;
    }
    lvgl_port_lock(0);
    lv_obj_add_flag  (objects.container_netconfig_prepare, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (objects.container_netconfig_process, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (objects.container_netconfig_reboot,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.container_netconfig_success, LV_OBJ_FLAG_HIDDEN);
    
    if (!qm_timer_is_valid(&netconfig_success_timer)) {
        netconfig_success_countdown = 3;
        qm_timer_new  (&netconfig_success_timer, netconfig_success_timer_handler, NULL, 1000, 1);
        qm_timer_start(&netconfig_success_timer);
    }

    if (get_current_screen() != SCREEN_ID_PAGE_NETCONFIG) {
        loadScreenEx(SCREEN_ID_PAGE_NETCONFIG);
    }
    lvgl_port_unlock();
}

// ******** ota页面 ******** //
static qm_timer_t ota_success_timer = { 0 };
static int8_t     ota_success_countdown = 3;

static void ota_success_timer_handler(qm_timer_t *timer,  void *arg) {
    if (--ota_success_countdown >= 0) {
        char s[32];
        sprintf(s, "%d秒后重启", ota_success_countdown);
        if(lv_port_init_flag == 0) {
            return;
        }
        lvgl_port_lock(0);
        lv_label_set_text(objects.text_ota_subtitle, s);
        lvgl_port_unlock();
    } else {
        qm_timer_stop(timer); 
        qm_timer_free(timer);
    }
}

void goto_ota_process_page(void) {
    if (get_current_screen() != SCREEN_ID_PAGE_OTA) {
        loadScreenEx(SCREEN_ID_PAGE_OTA);
    }
}

void goto_ota_success_page(void) {
    if (!qm_timer_is_valid(&ota_success_timer)) {
        ota_success_countdown = 3;
        qm_timer_new  (&ota_success_timer, ota_success_timer_handler, NULL, 1000, 1);
        qm_timer_start(&ota_success_timer);
    }
    if (get_current_screen() != SCREEN_ID_PAGE_OTA) {
        loadScreenEx(SCREEN_ID_PAGE_OTA);
    }
}

void goto_ota_failure_page(void) {
    if (get_current_screen() == SCREEN_ID_PAGE_OTA) {
        loadScreenEx(SCREEN_ID_PAGE_MAIN);
    }
}

void action_back_click(lv_event_t * e) {
    lv_obj_t *currentPage = lv_scr_act();
    if (bsp_lcd_bl_brightness_percent_get() != 0) {
        if (currentPage == objects.page_brightness || currentPage == objects.page_mode || currentPage == objects.page_setup || currentPage == objects.page_rgb/* || currentPage == objects.page_recog*/) {
            goto_main_page();
        }
        else if (currentPage == objects.page_recog) {
            set_var_recog_page_leave(1);
            goto_main_page();
        }
        else if (currentPage == objects.page_setup_volume || currentPage == objects.page_setup_screen || currentPage == objects.page_setup_language || currentPage == objects.page_setup_about) {
            goto_setup_page();
        }
    }
}

// // ******** 总体手势 ******** //
// void action_gesture_handler(lv_event_t * e) {
//     lv_indev_wait_release(lv_indev_get_act()); // 避免与CLICKED冲突

//     lv_obj_t *currentPage = lv_scr_act();
//     lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
//     if (bsp_lcd_bl_brightness_percent_get() != 0) {
//         if (currentPage == objects.page_main) {
//             if (dir == LV_DIR_LEFT) {
//                 goto_recog_page();
//             } else if (dir == LV_DIR_RIGHT) {
//                 goto_brightness_page();
//             } else if (dir == LV_DIR_TOP) {
//                 goto_menu_page();
//             } else if (dir == LV_DIR_BOTTOM) {
//                 goto_setup_page();
//             }
//         }
//         else if (currentPage == objects.page_brightness) {
//             if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
//                 goto_main_page();
//             }
//         }
//         else if (currentPage == objects.page_mode || currentPage == objects.page_brightness || currentPage == objects.page_setup) {
//             if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
//                 goto_main_page();
//             }
//         }
//         else if (currentPage == objects.page_recog) {
//             if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
//                 set_var_recog_page_leave(1);
//                 goto_main_page();
//             }
//         }
//         else if (currentPage == objects.page_setup_volume || currentPage == objects.page_setup_screen || currentPage == objects.page_setup_language || currentPage == objects.page_setup_about) {
//             if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
//                 goto_setup_page();
//             }
//         }
//     }
// }

// ******** 总体事件(来用亮屏) ******** //
void event_all_handler(lv_event_t * e) {
    lv_event_code_t event = lv_event_get_code(e);
    int current_brightness_percent = bsp_lcd_bl_brightness_percent_get();
    if (event <= LV_EVENT_HOVER_LEAVE) { // Input device events
        turnoff_timer_reset(0);
    }
    if (event == LV_EVENT_CLICKED && current_brightness_percent && lv_scr_act() == objects.page_main) {
        goto_menu_page();
    }
}

void action_home_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_main_page();
    }
}

void action_recog_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_recog_page();
    }
}

void action_setup_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_setup_page();
    }
}

void action_mode_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_mode_page();
    }
}

void action_brightness_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_brightness_page();
    }
}

void action_rgb_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_rgb_page();
    }
}

void action_volume_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_volume_page();
    }
}

void action_screen_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_screen_page();
    }
}

void action_language_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_language_page();
    }
}

void action_wifi_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_wifi_page();
    }
}

void action_about_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0) {
        goto_about_page();
    }
}

void action_language_cn_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && get_var_language_value() != 0) {
        set_var_language_value(0, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

void action_language_en_click(lv_event_t * e) {
    if (hit_test(e) && bsp_lcd_bl_brightness_percent_get() != 0 && get_var_language_value() != 1) {
        set_var_language_value(1, true);
        loadScreenEx(SCREEN_ID_PAGE_SETUP);
    }
}

void action_return_home_click(lv_event_t * e) {
    goto_main_page();
}
