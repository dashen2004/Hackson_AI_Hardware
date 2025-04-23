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

static qm_timer_t main_page_timer = { 0 };
static bool use_simle_page_last = true;
static bool use_simple_page = true;   // 简单首页: wifi未连接图标 + logo, 普通首页: wifi图标 + 天气 + 时间
static bool weather_info_got_last = false;
static bool weather_info_got = false; // 天气信息获取
static char city[32] = "宁波";
static int  weather_id = 0;
static int  weather_id_last = 0;
static int  temperature = 20;

const char weather_icon_tbl[20][32] = {
    "weather_blizzard.bin",
    "weather_day_cloudy.bin",
    "weather_day_sunny.bin",
    "weather_foggy.bin",
    "weather_hail.bin",
    "weather_heavy_rain.bin",
    "weather_heavy_snow.bin",
    "weather_light_rain.bin",
    "weather_light_snow.bin",
    "weather_moderate_rain.bin",
    "weather_night_cloudy.bin",
    "weather_night_sunny.bin",
    "weather_overcast.bin",
    "weather_sand_storm.bin",
    "weather_shower.bin",
    "weather_sleet.bin",
    "weather_stormy.bin",
    "weather_thunder_shower.bin",
    "weather_tornado.bin",
    "weather_windy.bin",
};

const char weather_text_tbl[20][32] = {
    "暴雪",
    "多云",
    "晴",
    "雾",
    "冰雹",
    "大雨",
    "大雪",
    "小雨",
    "小雪",
    "中雨",
    "多云",
    "晴",
    "阴天",
    "沙尘暴",
    "阵雨",
    "雨夹雪",
    "暴雨",
    "雷阵雨",
    "龙卷风",
    "风",
};

const char weekday_tbl[7][16] = {
    "星期日",
    "星期一",
    "星期二",
    "星期三",
    "星期四",
    "星期五",
    "星期六",
};

extern void goto_main_page(void);
#if 1
extern bool get_connect(void);
#else
static bool connect = false;
#define get_connect()   (connect)
#endif

void weather_info_set(char *__city, int __weather_id, int __temperature) {
    weather_info_got = 1;
    lv_strcpy(city, __city);
    weather_id = __weather_id;
    temperature = __temperature;
}

static void update_weather_ui(void) {
    weather_info_got_last = weather_info_got;
    lvgl_port_lock(0);
    if (weather_info_got) {
        lv_obj_clear_flag(objects.image_weather,         LV_OBJ_FLAG_HIDDEN); // 显示天气图标
        lv_obj_clear_flag(objects.text_city,             LV_OBJ_FLAG_HIDDEN); // 显示城市
        lv_obj_clear_flag(objects.text_weather,          LV_OBJ_FLAG_HIDDEN); // 显示天气
        lv_obj_clear_flag(objects.text_temperature,      LV_OBJ_FLAG_HIDDEN); // 显示温度
        lv_obj_add_flag  (objects.image_weather_unknown, LV_OBJ_FLAG_HIDDEN); // 隐藏未知天气图标
        lv_obj_add_flag  (objects.text_weather_unknown,  LV_OBJ_FLAG_HIDDEN); // 隐藏未知天气文字
    } else {
        lv_obj_add_flag  (objects.image_weather,         LV_OBJ_FLAG_HIDDEN); // 隐藏天气图标
        lv_obj_add_flag  (objects.text_city,             LV_OBJ_FLAG_HIDDEN); // 隐藏城市
        lv_obj_add_flag  (objects.text_weather,          LV_OBJ_FLAG_HIDDEN); // 隐藏天气
        lv_obj_add_flag  (objects.text_temperature,      LV_OBJ_FLAG_HIDDEN); // 隐藏温度
        lv_obj_clear_flag(objects.image_weather_unknown, LV_OBJ_FLAG_HIDDEN); // 显示未知天气图标
        lv_obj_clear_flag(objects.text_weather_unknown,  LV_OBJ_FLAG_HIDDEN); // 显示未知天气文字
    }
    lvgl_port_unlock();
}

static void update_weather(void) {
    if (weather_info_got) {
        char s[64];
        lvgl_port_lock(0);
        if (weather_id != weather_id_last) {
            weather_id_last = weather_id;
            sprintf(s, "/lvgl/images/%s", weather_icon_tbl[weather_id]);
            lv_img_set_src(objects.image_weather, s);
            lv_label_set_text(objects.text_weather, weather_text_tbl[weather_id]);
        }
        if (lv_strcmp(lv_label_get_text(objects.text_city), city)) {
            lv_label_set_text(objects.text_city, city);
        }
        sprintf(s, "%d℃", temperature);
        if (lv_strcmp(lv_label_get_text(objects.text_temperature), s)) {
            lv_label_set_text(objects.text_temperature, s);
        }
        lvgl_port_unlock();
    }
}

static void update_time(void) {
    if (!use_simple_page) {
        char s[64];
        qm_time_t t = qm_time(NULL);
        struct qm_tm *tm = qm_localtime(&t);

        lvgl_port_lock(0);
        sprintf(s, "%02d:%02d", tm->tm_hour, tm->tm_min);
        if (lv_strcmp(lv_label_get_text(objects.text_time), s)) {
            lv_label_set_text(objects.text_time, s);
        }
        if (lv_strcmp(lv_label_get_text(objects.text_weekday), weekday_tbl[tm->tm_wday])) {
            lv_label_set_text(objects.text_weekday, weekday_tbl[tm->tm_wday]);
        }
        sprintf(s, "%04d年%02d月%02d日", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
        if (lv_strcmp(lv_label_get_text(objects.text_date), s)) {
            lv_label_set_text(objects.text_date, s);
        }
        lvgl_port_unlock();
    }
}

static void update_wifi_status(void) {
    static bool last_connect = false;
    lvgl_port_lock(0);
    if (get_connect() != last_connect) {
        last_connect = get_connect();
        if (last_connect) {
            lv_image_set_src(objects.image_wifi1, "/lvgl/images/wifi_connected.bin");
        } else {
            lv_image_set_src(objects.image_wifi1, "/lvgl/images/wifi_disconnected.bin");
        }
    }
    lvgl_port_unlock();
}

static void main_page_timer_handler(qm_timer_t *timer,  void *arg) {
    if (use_simple_page) {
        if (get_connect()) {
            use_simple_page = false;
        }
    }

    lv_obj_t *currentPage = lv_scr_act();
    if (currentPage == objects.page_nowifi_main || currentPage == objects.page_main) {
        if (use_simple_page != use_simle_page_last) {
            use_simle_page_last = use_simple_page;
            lvgl_port_lock(0);
            goto_main_page();
            lvgl_port_unlock();
        } else if (!use_simple_page) {
            // 这里更新wifi状态, 天气, 时间
            if (weather_info_got != weather_info_got_last) {
                update_weather_ui();
            }
            update_time();
            update_weather();
            update_wifi_status();
        }
    }
}

#if 0
static qm_timer_t wifi_test_timer = { 0 };

static void wifi_test_timer_handler(qm_timer_t *timer,  void *arg) {
    if (!connect) {
        connect = true;
    } else {
        weather_info_set("宁波", 5, 25);
    }
}
#endif

void goto_main_page(void) {
    if (use_simple_page) {
        lvgl_port_lock(0);
        loadScreenEx(SCREEN_ID_PAGE_NOWIFI_MAIN);
        lvgl_port_unlock();
    } else {
        update_time();
        update_weather_ui();
        update_weather();
        update_wifi_status();
        loadScreenEx(SCREEN_ID_PAGE_MAIN);
    }
    use_simle_page_last = use_simple_page;
    if (!qm_timer_is_valid(&main_page_timer)) {
        qm_timer_new(&main_page_timer, main_page_timer_handler, NULL, 500, 1);
        qm_timer_start(&main_page_timer);
#if 0
        qm_timer_new(&wifi_test_timer, wifi_test_timer_handler, NULL, 15000, 1);
        qm_timer_start(&wifi_test_timer);
#endif
    }
}

// ******** 菜单页面 ******** //
static void goto_menu_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_MENU);
}

// ******** 灯光模式页面 ******** //
static void set_light_mode(bsp_light_mode_t value, bool flag) {
    switch (value) {
        case BSP_LIGHT_MODE_NIGHT: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
        case BSP_LIGHT_MODE_READ: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
        case BSP_LIGHT_MODE_IDLE: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
        case BSP_LIGHT_MODE_SLEEP: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_checked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_checked.bin", NULL);
            break;
        }
        default: {
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_nightlamp, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_read, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_relaxation, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(objects.mode_sleep, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            break;
        }
    }
    if (flag) {
        bsp_light_mode(value);
        audio_light_snapshot_report();
    }
}

void action_nightlamp_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_NIGHT) {
        set_light_mode(BSP_LIGHT_MODE_NIGHT, true);
    }
}

void action_read_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_READ) {
        set_light_mode(BSP_LIGHT_MODE_READ, true);
    }
}

void action_idle_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_IDLE) {
        set_light_mode(BSP_LIGHT_MODE_IDLE, true);
    }
}

void action_sleep_click(lv_event_t * e) {
    if (hit_test(e) && bsp_light_store_get()->mode != BSP_LIGHT_MODE_SLEEP) {
        set_light_mode(BSP_LIGHT_MODE_SLEEP, true);
    }
}

static void goto_mode_page(void) {
    set_light_mode(bsp_light_store_get()->mode, false);
    loadScreenEx(SCREEN_ID_PAGE_MODE);
}

// ******** 灯光亮度调节页面 ******** //
static void light_onoff(bool value, bool flag) {
    bsp_light_store_t *p = bsp_light_store_get();

    if (!value) {
        lv_img_set_src(objects.image_brightness_light_onoff, "/lvgl/images/brightness_light_off.bin"); 
        lv_obj_add_state(objects.slider_brightness_top, LV_STATE_DISABLED);
        lv_obj_add_state(objects.slider_brightness_bottom, LV_STATE_DISABLED);
        set_var_brightness_top(0, false);
        set_var_brightness_bottom(0, false);
        if (flag) {
            // printf("bsp_light_bri_on_off - false\n");
            bsp_light_bri_on_off(false);
        }
    } else {
        lv_img_set_src(objects.image_brightness_light_onoff, "/lvgl/images/brightness_light_on.bin");
        lv_obj_clear_state(objects.slider_brightness_top, LV_STATE_DISABLED);
        lv_obj_clear_state(objects.slider_brightness_bottom, LV_STATE_DISABLED);
        set_var_brightness_top(p->up_bri / 100, false);
        set_var_brightness_bottom(p->down_bri / 100, false);
        if (flag) {
            // printf("bsp_light_bri_on_off - true\n");
            bsp_light_bri_on_off(true);
        }
    }
    audio_light_snapshot_report();
}

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
                light_onoff(bsp_light_store_get()->on_off ? false : true, true);
            }
        }
    }
}

// void action_brightness_top_changed(lv_event_t * e) {
//     // lv_obj_t * slider = lv_event_get_target(e);
//     // int value = lv_slider_get_value(slider);
//     // printf("changed value: %d\n", value);
//     // set_var_brightness_top(value);
// }

// void action_brightness_bottom_changed(lv_event_t * e) {
//     // lv_obj_t * slider = lv_event_get_target(e);
//     // int value = lv_slider_get_value(slider);
//     // printf("changed value: %d\n", value);
//     // set_var_brightness_bottom(value);
// }

void action_brightness_light_onoff_click(lv_event_t * e) {
    if (hit_test(e)) {
        light_onoff(bsp_light_store_get()->on_off ? false : true, true);
    }
}

static void goto_brightness_page(void) {
    // 先获取led状态和亮度
    bsp_light_store_t *p = bsp_light_store_get();
    light_onoff(p->on_off, false);
    loadScreenEx(SCREEN_ID_PAGE_BRIGHTNESS);
} 

// ******** 设置页面 ******** //
static void goto_setup_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP);
}

// ******** 音量设置页面 ******** //
static void update_volume_page(void) {
    if (get_var_mute_value()) { // 静音
        lv_imagebutton_set_src(objects.volume_mute_onoff, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/setup_volume_mute_open.bin", NULL);
        lv_imagebutton_set_src(objects.volume_mute_onoff, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/setup_volume_mute_open.bin", NULL);
        lv_obj_add_state(objects.slider_volume_value, LV_STATE_DISABLED);
        lv_obj_remove_flag(objects.slider_volume_value, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(objects.slider_volume_value, lv_color_hex(0x024F3B), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.slider_volume_value, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_imagebutton_set_src(objects.volume_mute_onoff, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
        lv_imagebutton_set_src(objects.volume_mute_onoff, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
        lv_obj_clear_state(objects.slider_volume_value, LV_STATE_DISABLED);
        lv_obj_add_flag(objects.slider_volume_value, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(objects.slider_volume_value, lv_color_hex(0xff23f9c1), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.slider_volume_value, lv_color_hex(0xff565656), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void action_volume_mute_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_mute_value()) {
            set_var_mute_value(false, true);
        } else {
            set_var_mute_value(true, true);
        }
        update_volume_page();
    }
}

static void goto_volume_page(void) {
    update_volume_page();
    loadScreenEx(SCREEN_ID_PAGE_SETUP_VOLUME);
}

// ******** 熄屏设置页面 ******** //

static void update_screen_page(void) {
    int32_t scr_tmo = get_var_scr_tmo_value();
    switch (scr_tmo) {
        case 1:
            lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
            break;
        case 2:
            lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
            break;
        case 3:
            lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
            break;
        default: // 0
            lv_obj_remove_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

static void goto_screen_page(void) {
    update_screen_page();
    loadScreenEx(SCREEN_ID_PAGE_SETUP_SCREEN);
}

// ******** 语言设置页面 ******** //
static void update_language_page(void) {
    int32_t lang = get_var_language_value();
    switch (lang) {
        case 1:
            lv_obj_add_flag(objects.setup_language_cn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(objects.setup_language_en, LV_OBJ_FLAG_HIDDEN);
            break;
        default: // 0
            lv_obj_remove_flag(objects.setup_language_cn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.setup_language_en, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}


static void goto_language_page(void) {
    update_language_page();
    loadScreenEx(SCREEN_ID_PAGE_SETUP_LANGUAGE);
}

// ******** 开启配网 ******** //

void action_network_config_cancel_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (!lv_obj_has_flag(objects.obj0, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(objects.obj0, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

#if 0
static qm_work_t delay_reboot = { 0 };

static void delay_reboot_action(void *arg) {
    qm_reboot();
}
#endif

extern int xjiang_cloud_reset(void);

void action_network_config_submit_click(lv_event_t * e) {
    if (hit_test(e)) {
        set_var_awss_value(true, true);

        xjiang_cloud_reset();
    
        #if 0
        qm_post_delayed_action(&delay_reboot, delay_reboot_action, NULL, 100);
    #endif
    }
}

static void goto_wifi_page(void) {
    if (lv_obj_has_flag(objects.obj0, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(objects.obj0, LV_OBJ_FLAG_HIDDEN);
    }
}

// ******** 关于页面 ******** //
static void goto_about_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_SETUP_ABOUT);
}

// ******** 语音识别页面 ******** //
static void goto_recog_page(void) {
    // loadScreenEx(SCREEN_ID_PAGE_RECOG);
}

// ******** 配网页面 ******** //
static qm_timer_t network_config_process_timer = { 0 };
static qm_timer_t network_config_success_timer = { 0 };
static uint32_t   network_config_start_tick = 0;
static uint8_t    wifi_level = 0;
static int8_t     countdown = 3;

static void network_config_success_timer_handler(qm_timer_t *timer,  void *arg) {
    set_var_turnoff_value(qm_now_ms());
    if (--countdown >= 0) {
        char s[8];
        sprintf(s, "(%d)", countdown);
        lvgl_port_lock(0);
        lv_label_set_text(objects.text_network_config_success_countdown, s);
        lvgl_port_unlock();
    } else {
        lvgl_port_lock(0);
        goto_main_page();
        lvgl_port_unlock();
        qm_timer_stop(timer);
        qm_timer_free(timer);
    }
    
}

static void network_config_process_timer_handler(qm_timer_t *timer,  void *arg) {
    set_var_turnoff_value(qm_now_ms());
    if (qm_now_ms() - network_config_start_tick >= 3 * 60 * 1000) {
        #if 1
        lvgl_port_lock(0);
        goto_main_page();
        lvgl_port_unlock();
        #else
        lvgl_port_lock(0);
        loadScreenEx(SCREEN_ID_PAGE_NETWORK_CONFIG_SUCCESS);
        lvgl_port_unlock();
        if (!qm_timer_is_valid(&network_config_success_timer)) {
            countdown = 3;
            qm_timer_new(&network_config_success_timer, network_config_success_timer_handler, NULL, 1000, 1);
            qm_timer_start(&network_config_success_timer);
        }
        #endif
        qm_timer_stop(timer);
        qm_timer_free(timer);
        return;
    }
    if (get_connect()) {
        lvgl_port_lock(0);
        loadScreenEx(SCREEN_ID_PAGE_NETWORK_CONFIG_SUCCESS);
        lvgl_port_unlock();
        if (!qm_timer_is_valid(&network_config_success_timer)) {
            countdown = 3;
            qm_timer_new(&network_config_success_timer, network_config_success_timer_handler, NULL, 1000, 1);
            qm_timer_start(&network_config_success_timer);
        }
        qm_timer_stop(timer);
        qm_timer_free(timer);
        return;
    }

    lvgl_port_lock(0);
    switch (wifi_level) {
        case 0: {
            lv_obj_clear_flag(objects.image_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.image_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.image_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.image_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case 1: {
            lv_obj_clear_flag(objects.image_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.image_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.image_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case 2: {
            lv_obj_clear_flag(objects.image_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.image_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
            break;
        }
        case 3: {
            lv_obj_clear_flag(objects.image_wifi_level_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.image_wifi_level_4, LV_OBJ_FLAG_HIDDEN);
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
}

void goto_network_config_process_page(void) {
    loadScreenEx(SCREEN_ID_PAGE_NETWORK_CONFIG_PROCESS);
    if (!qm_timer_is_valid(&network_config_process_timer)) {
        wifi_level = 0;
        network_config_start_tick = qm_now_ms();
        qm_timer_new(&network_config_process_timer, network_config_process_timer_handler, NULL, 500, 1);
        qm_timer_start(&network_config_process_timer);
    }
}

// ******** 总体手势 ******** //
void action_gesture_handler(lv_event_t * e) {
    lv_indev_wait_release(lv_indev_get_act()); // 避免与CLICKED冲突

    lv_obj_t *currentPage = lv_scr_act();
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (currentPage == objects.page_main || currentPage == objects.page_nowifi_main) {
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
    else if (currentPage == objects.page_mode || currentPage == objects.page_brightness || currentPage == objects.page_setup/* || currentPage == objects.page_recog*/) {
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

// 自动息屏页点击30S
void action_screen_30s_click(lv_event_t * e) {
    if (hit_test(e)) {
        // printf("action_screen_30s_click\n");
        if (get_var_scr_tmo_value() != 0) {
            set_var_scr_tmo_value(0, true);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 自动息屏页点击1分钟
void action_screen_1min_click(lv_event_t * e) {
    if (hit_test(e)) {
        // printf("action_screen_1min_click\n");
        if (get_var_scr_tmo_value() != 1) {
            set_var_scr_tmo_value(1, true);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 自动息屏页点击5分钟
void action_screen_5min_click(lv_event_t * e) {
    if (hit_test(e)) {
        // printf("action_screen_5min_click\n");
        if (get_var_scr_tmo_value() != 2) {
            set_var_scr_tmo_value(2, true);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

// 自动息屏页点击永不
void action_screen_never_click(lv_event_t * e) {
    if (hit_test(e)) {
        // printf("action_screen_never_click\n");
        if (get_var_scr_tmo_value() != 3) {
            set_var_scr_tmo_value(3, true);
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

void action_language_cn_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_language_value() != 0) {
            set_var_language_value(0, true);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

void action_language_en_click(lv_event_t * e) {
    if (hit_test(e)) {
        if (get_var_language_value() != 1) {
            set_var_language_value(1, true);
            loadScreenEx(SCREEN_ID_PAGE_SETUP);
        }
    }
}

