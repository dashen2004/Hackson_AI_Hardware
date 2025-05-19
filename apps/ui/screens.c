#include <stdio.h>
#include <string.h>

#include "fonts.h"
#include "screens.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include "qm_iot_weather.h"
#include "qm_time.h"

#include "bsp_light.h"

#include "lcd.h"


objects_t objects;
lv_obj_t *tick_value_change_obj;

lv_font_t *font_misans_12px = NULL;
lv_font_t *font_misans_80px = NULL;
lv_font_t *font_pingfang_16px = NULL;
lv_font_t *font_pingfang_20px = NULL;
lv_font_t *font_pingfang_24px = NULL;

extern int bsp_lcd_bl_brightness_percent_get(void);

static void event_handler_cb_page_brightness_slider_brightness_top(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED && bsp_lcd_bl_brightness_percent_get() != 0) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            turnoff_timer_reset(0);
            int32_t value = lv_slider_get_value(ta);
            set_var_brightness_top(value);
        }
    }
}

static void event_handler_cb_page_brightness_slider_brightness_bottom(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED && bsp_lcd_bl_brightness_percent_get() != 0) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            turnoff_timer_reset(0);
            int32_t value = lv_slider_get_value(ta);
            set_var_brightness_bottom(value);
        }
    }
}

static void event_handler_cb_page_setup_volume_volume_value(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED && bsp_lcd_bl_brightness_percent_get() != 0) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            turnoff_timer_reset(0);
            int32_t value = lv_slider_get_value(ta);
            set_var_volume_value(value, true);
        }
    }
}

void create_screen_page_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // image_wifi1
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi = obj;
            lv_obj_set_pos(obj, 165, 14);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, "/lvgl/images/wifi_disconnected.bin");
        }
        {
            // image_wifi_level_s_1
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_1 = obj;
            lv_obj_set_pos(obj, 178, 37);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, "/lvgl/images/wifi_level_s_1.bin");
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // image_wifi_level_s_2
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_2 = obj;
            lv_obj_set_pos(obj, 174, 31);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, "/lvgl/images/wifi_level_s_2.bin");
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // image_wifi_level_s_3
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_3 = obj;
            lv_obj_set_pos(obj, 170, 25);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, "/lvgl/images/wifi_level_s_3.bin");
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // image_wifi_level_s_4
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_4 = obj;
            lv_obj_set_pos(obj, 167, 20);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, "/lvgl/images/wifi_level_s_4.bin");
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // container_normal
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_normal = obj;
            lv_obj_set_pos(obj, 0, 48);
            lv_obj_set_size(obj, 360, 312);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // image_weather_unknown
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_weather_unknown = obj;
                    lv_obj_set_pos(obj, 92, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/w99.bin");
                }
                {
                    // text_weather_unknown
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_weather_unknown = obj;
                    lv_obj_set_pos(obj, 165, 19);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "无法获取当前\n天气");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // image_weather
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_weather = obj;
                    lv_obj_set_pos(obj, 88, 7);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/w0.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // text_city
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_city = obj;
                    lv_obj_set_pos(obj, 194, 8);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "宁波");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_weather
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_weather = obj;
                    lv_obj_set_pos(obj, 194, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "晴");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_temperature
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_temperature = obj;
                    lv_obj_set_pos(obj, 254, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "25℃");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_time
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_time = obj;
                    lv_obj_set_pos(obj, 0, -24);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "00:00");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_80px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_weekday
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_weekday = obj;
                    lv_obj_set_pos(obj, 0, 51);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "星期三");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_date
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_date = obj;
                    lv_obj_set_pos(obj, 4, 83);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "2025年01月01日");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // container_simple
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_simple = obj;
            lv_obj_set_pos(obj, 0, 48);
            lv_obj_set_size(obj, 360, 312);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_logo = obj;
                    lv_obj_set_pos(obj, 105, 105);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/logo.bin");
                }
            }
        }
    }
    
    tick_screen_page_main();
}

extern bool get_connect(void);

const char weather_name_tbl[48][32] = {
    "晴", "多云", "阴", "阵雨", "雷阵雨", "雷阵雨伴有冰雹", "雨夹雪", "小雨", 
    "中雨", "大雨", "暴雨", "未知", "未知", "阵雪", "小雪", "中雪",
    "大雪", "暴雪", "雾", "冻雨", "沙尘暴", "未知", "未知", "未知",
    "未知", "未知", "未知", "未知", "未知", "浮尘", "晴", "多云",
    "雾", "阵雨", "阵雪", "浮尘", "沙尘暴", "未知", "未知", "未知",
    "未知", "未知", "未知", "未知", "未知", "霾", "霾", "未知",
};

const char weekday_tbl[7][16] = { "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", };

void tick_screen_page_main() {
    // 更新首页显示
    // 1. 简单首页 OR 正常首页
    if (!lv_obj_has_flag(objects.container_simple, LV_OBJ_FLAG_HIDDEN)) {
        if (get_connect()) {
            // 上电后首次连上网
            lv_obj_add_flag(objects.container_simple, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.container_normal, LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(objects.image_weather_unknown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.text_weather_unknown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.text_city, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.text_weather, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag  (objects.text_temperature, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        // 已显示正常首页, 需要更新 城市 / 天气图标 / 天气 / 温度 / 时间 / 星期 / 日期
        static int8_t weather_id_last = -1;
        static int8_t temperature_last = -128; 
        int8_t weather_id = get_var_weather_id();
        if (weather_id > QM_IOT_WEATHER_TYPE_HAZE_NIGHT) {
            weather_id = QM_IOT_WEATHER_TYPE_HAZE_NIGHT;
        }
        if (weather_id != weather_id_last) {
            if (weather_id_last == -1) {
                lv_obj_add_flag  (objects.image_weather_unknown, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag  (objects.text_weather_unknown, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.image_weather, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.text_city, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.text_weather, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.text_temperature, LV_OBJ_FLAG_HIDDEN);
            } else if (weather_id == -1) {
                lv_obj_clear_flag(objects.image_weather_unknown, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.text_weather_unknown, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag  (objects.image_weather, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag  (objects.text_city, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag  (objects.text_weather, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag  (objects.text_temperature, LV_OBJ_FLAG_HIDDEN);
            }
            weather_id_last = weather_id;
            // 更新 天气图标 / 天气
            if (weather_id != -1) {
                char s[64];
                sprintf(s, "/lvgl/images/w%d.bin", weather_id);
                lv_image_set_src(objects.image_weather, s);
                lv_label_set_text(objects.text_weather, weather_name_tbl[weather_id]);
            }
        }
        if (weather_id != -1) {
            // 更新城市
            {
                char *new_val = get_var_city();
                char *cur_val = lv_label_get_text(objects.text_city);
                if (new_val != cur_val) {
                    lv_label_set_text(objects.text_city, new_val);
                }
            }
            // 更新温度
            {
                int8_t new_val = get_var_temperature();
                if (new_val != temperature_last) {
                    temperature_last = new_val;
                    char s[16];
                    sprintf(s, "%d℃", new_val);
                    lv_label_set_text(objects.text_temperature, s);
                }
            }
        }
        // 更新时间 / 星期 / 日期
        char s[64];
        qm_time_t t = qm_time(NULL);
        struct qm_tm *tm = qm_localtime(&t);
        {
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
        }
    }
    // {
    //     // 更新网络图标
    //     static bool connect_last = false;
    //     if (connect_last != get_connect()) {
    //         connect_last = get_connect();
    //         if (connect_last) {
    //             lv_img_set_src(objects.image_wifi, "/lvgl/images/wifi_connected.bin");
    //         } else {
    //             lv_img_set_src(objects.image_wifi, "/lvgl/images/wifi_disconnected.bin");
    //         }
    //     }
    // }
}

void create_screen_page_menu() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_menu = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // menu_bg
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.menu_bg = obj;
            lv_obj_set_pos(obj, 40, 40);
            lv_obj_set_size(obj, 280, 280);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 140, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // menu_home
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_home = obj;
                    lv_obj_set_pos(obj, 108, 20);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/menu_home_released.bin", NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/menu_home_pressed.bin", NULL);
                    lv_obj_add_event_cb(obj, action_home_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_speech
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_speech = obj;
                    lv_obj_set_pos(obj, 24, 88);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/menu_speech_released.bin", NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/menu_speech_pressed.bin", NULL);
                    lv_obj_add_event_cb(obj, action_recog_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_setup
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_setup = obj;
                    lv_obj_set_pos(obj, 53, 178);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/menu_setup_released.bin", NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/menu_setup_pressed.bin", NULL);
                    lv_obj_add_event_cb(obj, action_setup_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_mode
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_mode = obj;
                    lv_obj_set_pos(obj, 163, 178);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/menu_mode_released.bin", NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/menu_home_pressed.bin", NULL);
                    lv_obj_add_event_cb(obj, action_mode_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_brightness
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_brightness = obj;
                    lv_obj_set_pos(obj, 192, 88);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/menu_brightness_released.bin", NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/menu_brightness_pressed.bin", NULL);
                    lv_obj_add_event_cb(obj, action_brightness_click, LV_EVENT_CLICKED, (void *)0);
                }
            }
        }
    }
    
    tick_screen_page_menu();
}

void tick_screen_page_menu() {
}

void create_screen_page_mode() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_mode = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // mode_nightlamp
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_nightlamp = obj;
            lv_obj_set_pos(obj, 144, 64);
            lv_obj_set_size(obj, 72, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_nightlamp_unchecked.bin", NULL);
            lv_obj_add_event_cb(obj, action_nightlamp_click, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // mode_read
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_read = obj;
            lv_obj_set_pos(obj, 45, 144);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_read_unchecked.bin", NULL);
            lv_obj_add_event_cb(obj, action_read_click, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // mode_relaxation
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_relaxation = obj;
            lv_obj_set_pos(obj, 251, 144);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_relaxation_unchecked.bin", NULL);
            lv_obj_add_event_cb(obj, action_idle_click, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // mode_sleep
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_sleep = obj;
            lv_obj_set_pos(obj, 144, 232);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/mode_sleep_unchecked.bin", NULL);
            lv_obj_add_event_cb(obj, action_sleep_click, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // text_nightlamp
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_nightlamp = obj;
            lv_obj_set_pos(obj, 0, -130);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "小夜灯");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff999999), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_read
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_read = obj;
            lv_obj_set_pos(obj, -99, 52);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "阅读");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff999999), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_relaxation
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_relaxation = obj;
            lv_obj_set_pos(obj, 107, 52);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "休闲");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff999999), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_sleep
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_sleep = obj;
            lv_obj_set_pos(obj, 0, 140);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "睡觉");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff999999), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 120, 140);
            lv_obj_set_size(obj, 120, 80);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 39, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
    }
    
    tick_screen_page_mode();
}

void tick_screen_page_mode() {
    static int8_t light_mode_last = BSP_LIGHT_MODE_NONE;
    int8_t new_val = get_var_light_mode();
    if (new_val != light_mode_last) {
        light_mode_last = new_val;
        switch (new_val) {
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
    }
}

void create_screen_page_brightness() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_brightness = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_page_brightness_event_cb, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // image_brightness_light_onoff
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_brightness_light_onoff = obj;
            lv_obj_set_pos(obj, 148, 39);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, "/lvgl/images/brightness_light_off.bin");
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE|LV_OBJ_FLAG_CLICKABLE);
        }
        {
            // slider_container
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.slider_container = obj;
            lv_obj_set_pos(obj, 35, 132);
            lv_obj_set_size(obj, 294, 153);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // slider_brightness_top
                    lv_obj_t *obj = lv_slider_create(parent_obj);
                    objects.slider_brightness_top = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 294, 48);
                    lv_slider_set_range(obj, 8, 100);
                    lv_obj_add_event_cb(obj, event_handler_cb_page_brightness_slider_brightness_top, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff565656), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff9000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // image_brightness_low1
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_brightness_low1 = obj;
                    lv_obj_set_pos(obj, 11, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/brightness_low.bin");
                }
                {
                    // image_brightness_high1
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_brightness_high1 = obj;
                    lv_obj_set_pos(obj, 266, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/brightness_high.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
                }
                {
                    // slider_brightness_bottom
                    lv_obj_t *obj = lv_slider_create(parent_obj);
                    objects.slider_brightness_bottom = obj;
                    lv_obj_set_pos(obj, 0, 82);
                    lv_obj_set_size(obj, 294, 48);
                    lv_slider_set_range(obj, 8, 100);
                    lv_obj_add_event_cb(obj, event_handler_cb_page_brightness_slider_brightness_bottom, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff565656), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff9000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // text_brightness_top_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_brightness_top_value = obj;
                    lv_obj_set_pos(obj, 0, -11);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // image_brightness_low2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_brightness_low2 = obj;
                    lv_obj_set_pos(obj, 11, 98);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/brightness_low.bin");
                }
                {
                    // image_brightness_high2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_brightness_high2 = obj;
                    lv_obj_set_pos(obj, 266, 98);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/brightness_high.bin");
                }
                {
                    // text_brightness_bottom_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_brightness_bottom_value = obj;
                    lv_obj_set_pos(obj, -2, 70);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 280);
            lv_obj_set_size(obj, 360, 80);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
    }
    
    tick_screen_page_brightness();
}

void tick_screen_page_brightness() {
    {
        static bool brightness_onoff_last = false;
        bool new_val = get_var_brightness_onoff();
        if (new_val != brightness_onoff_last) {
            brightness_onoff_last = new_val;
            if (new_val) {
                lv_img_set_src(objects.image_brightness_light_onoff, "/lvgl/images/brightness_light_on.bin");    
            } else {
                lv_img_set_src(objects.image_brightness_light_onoff, "/lvgl/images/brightness_light_off.bin");
            }
        }
    }
    {
        int32_t new_val = get_var_brightness_top();
        int32_t cur_val = lv_slider_get_value(objects.slider_brightness_top);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.slider_brightness_top;
            lv_slider_set_value(objects.slider_brightness_top, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }

        char temp[16];
        if (new_val == 0) { sprintf(temp, "关闭"); }
        else { sprintf(temp, "%ld%%", new_val); }
        if (lv_strcmp(lv_label_get_text(objects.text_brightness_top_value), temp)) {
            lv_label_set_text(objects.text_brightness_top_value, temp);
        }
    }
    {
        int32_t new_val = get_var_brightness_bottom();
        int32_t cur_val = lv_slider_get_value(objects.slider_brightness_bottom);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.slider_brightness_bottom;
            lv_slider_set_value(objects.slider_brightness_bottom, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }

        char temp[16];
        if (new_val == 0) { sprintf(temp, "关闭"); }
        else { sprintf(temp, "%ld%%", new_val); }
        if (lv_strcmp(lv_label_get_text(objects.text_brightness_bottom_value), temp)) {
            lv_label_set_text(objects.text_brightness_bottom_value, temp);
        }
    }
}

void create_screen_page_setup() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 64, 60);
            lv_obj_set_size(obj, 252, 50);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_volume_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_volume.bin");
                }
                {
                    // text_setup_volume
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_volume = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "声音设置");
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_more.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 64, 115);
            lv_obj_set_size(obj, 252, 50);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_screen_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 50);
                    lv_img_set_src(obj, "/lvgl/images/setup_screen.bin");
                }
                {
                    // text_setup_screen
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_screen = obj;
                    lv_obj_set_pos(obj, 70, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "自动息屏");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 6);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_more.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 64, 170);
            lv_obj_set_size(obj, 252, 50);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_wifi_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // text_setup_wifi
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_wifi = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "设备配网");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_wifi.bin");
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_more.bin");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 64, 225);
            lv_obj_set_size(obj, 252, 50);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_about_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_about.bin");
                }
                {
                    // text_setup_about
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_about = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "关于设备");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_more.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 64, 280);
            lv_obj_set_size(obj, 252, 50);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_factory_reset_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/factory_reset.bin");
                }
                {
                    // text_setup_factoryreset
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_factoryreset = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "恢复出厂");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/setup_more.bin");
                    lv_obj_add_event_cb(obj, action_about_click, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 58);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
        {
            // container_mask
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_mask = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 360);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // submit_win
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.submit_win = obj;
                    lv_obj_set_pos(obj, 50, 108);
                    lv_obj_set_size(obj, 260, 144);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_network_config_infomation
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_network_config_infomation = obj;
                            lv_obj_set_pos(obj, 22, 21);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "点击确定后，设备将进入\n重新配网状态\n");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // text_network_config_cancel
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_network_config_cancel = obj;
                            lv_obj_set_pos(obj, 24, 82);
                            lv_obj_set_size(obj, 80, 44);
                            lv_label_set_text(obj, "取消");
                            lv_obj_add_event_cb(obj, action_network_config_cancel_click, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                        }
                        {
                            // text_network_config_submit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_network_config_submit = obj;
                            lv_obj_set_pos(obj, 72, 38);
                            lv_obj_set_size(obj, 80, 44);
                            lv_label_set_text(obj, "确定");
                            lv_obj_add_event_cb(obj, action_network_config_submit_click, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff37ed67), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_page_setup();
}

void tick_screen_page_setup() {
}

void create_screen_page_setup_volume() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup_volume = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_img_create(parent_obj);
            lv_obj_set_pos(obj, 38, 100);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, "/lvgl/images/setup_volume_mute.bin");
        }
        {
            // text_volume_mute
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_volume_mute = obj;
            lv_obj_set_pos(obj, 76, 104);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "静音功能");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_volume_adjust
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_volume_adjust = obj;
            lv_obj_set_pos(obj, 38, 164);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "音量调节");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // slider_volume_value
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.slider_volume_value = obj;
            lv_obj_set_pos(obj, 38, 195);
            lv_obj_set_size(obj, 285, 48);
            // lv_obj_add_event_cb(obj, action_volume_value_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_page_setup_volume_volume_value, LV_EVENT_ALL, 0);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff23f9c1), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff565656), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_percent_0
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_percent_0 = obj;
            lv_obj_set_pos(obj, 38, 249);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "0%");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_percent_100
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_percent_100 = obj;
            lv_obj_set_pos(obj, 290, 249);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "100%");
            lv_obj_set_style_text_font(obj, font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_volume_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_volume_value = obj;
            lv_obj_set_pos(obj, 0, 76);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 280, 65);
            lv_obj_set_size(obj, 80, 100);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_volume_mute_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // volume_mute_onoff
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.volume_mute_onoff = obj;
                    lv_obj_set_pos(obj, 0, 38);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, "/lvgl/images/setup_volume_mute_close.bin", NULL);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
                    lv_obj_set_style_pad_top(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 65);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
    }
    
    tick_screen_page_setup_volume();
}

void tick_screen_page_setup_volume() {
    {
        static bool mute_last = false;
        bool new_val = get_var_mute_value();
        if (new_val != mute_last) {
            mute_last = new_val;
            if (new_val) { // 静音
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
    }
    {
        int32_t new_val = get_var_volume_value();
        int32_t cur_val = lv_slider_get_value(objects.slider_volume_value);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.slider_volume_value;
            lv_slider_set_value(objects.slider_volume_value, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }

        char temp[8];
        sprintf(temp, "%ld%%", new_val);
        if (lv_strcmp(lv_label_get_text(objects.text_volume_value), temp)) {
            lv_label_set_text(objects.text_volume_value, temp);
        }
    }
}

void create_screen_page_setup_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // screen_main_bg
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.screen_main_bg = obj;
            lv_obj_set_pos(obj, 50, 78);
            lv_obj_set_size(obj, 260, 204);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 25, 15);
                    lv_obj_set_size(obj, 220, 36);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, action_screen_30s_click, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_screen_tmo_30s
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_screen_tmo_30s = obj;
                            lv_obj_set_pos(obj, 0, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "30秒");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_30s
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_30s = obj;
                            lv_obj_set_pos(obj, 196, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, "/lvgl/images/setup_selected.bin");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 25, 59);
                    lv_obj_set_size(obj, 220, 36);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, action_screen_1min_click, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_screen_tmo_1min
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_screen_tmo_1min = obj;
                            lv_obj_set_pos(obj, 0, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "1分钟");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_1min
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_1min = obj;
                            lv_obj_set_pos(obj, 196, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, "/lvgl/images/setup_selected.bin");
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 25, 102);
                    lv_obj_set_size(obj, 220, 36);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, action_screen_5min_click, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_screen_tmo_5min
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_screen_tmo_5min = obj;
                            lv_obj_set_pos(obj, 0, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "5分钟");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_5min
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_5min = obj;
                            lv_obj_set_pos(obj, 196, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, "/lvgl/images/setup_selected.bin");
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 25, 145);
                    lv_obj_set_size(obj, 220, 36);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, action_screen_never_click, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_screen_tmo_never
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_screen_tmo_never = obj;
                            lv_obj_set_pos(obj, 0, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "永不");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_never
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_never = obj;
                            lv_obj_set_pos(obj, 196, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, "/lvgl/images/setup_selected.bin");
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                        }
                    }
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 65);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
    }
    
    tick_screen_page_setup_screen();
}

void tick_screen_page_setup_screen() {
    static int32_t scr_tmo_last = -1;
    int32_t new_val = get_var_scr_tmo_value();
    if (new_val != scr_tmo_last) {
        scr_tmo_last = new_val;
        switch (new_val) {
            case 1: // 1分
                lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
            case 2: // 5分
                lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
            case 3: // 永不
                lv_obj_add_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
            default: // 30秒
                lv_obj_remove_flag(objects.setup_screen_tmo_30s, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_1min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_5min, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.setup_screen_tmo_never, LV_OBJ_FLAG_HIDDEN);
                break;
        }
    }
}

void create_screen_page_setup_language() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup_language = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // text_language_select
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_language_select = obj;
            lv_obj_set_pos(obj, 50, 101);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "语言选择");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // language_select_bg
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.language_select_bg = obj;
            lv_obj_set_pos(obj, 50, 126);
            lv_obj_set_size(obj, 260, 108);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 10, 12);
                    lv_obj_set_size(obj, 240, 40);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, action_language_cn_click, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_language_zh
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_language_zh = obj;
                            lv_obj_set_pos(obj, 12, 9);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "简体中文");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_language_cn
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_language_cn = obj;
                            lv_obj_set_pos(obj, 203, 8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, "/lvgl/images/setup_selected.bin");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 10, 55);
                    lv_obj_set_size(obj, 240, 40);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_event_cb(obj, action_language_en_click, LV_EVENT_CLICKED, (void *)0);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_language_en
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_language_en = obj;
                            lv_obj_set_pos(obj, 12, 9);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "English");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_language_en
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_language_en = obj;
                            lv_obj_set_pos(obj, 203, 8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, "/lvgl/images/setup_selected.bin");
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                        }
                    }
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 80);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
    }
    
    tick_screen_page_setup_language();
}

void tick_screen_page_setup_language() {
    static int32_t lang_last = -1;
    int32_t new_val = get_var_language_value();
    if (new_val != lang_last) {
        lang_last = new_val;
        switch (new_val) {
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
}

void create_screen_page_setup_about() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup_about = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_about_info = obj;
            lv_obj_set_pos(obj, 40, 70);
            lv_obj_set_size(obj, 280, 220);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // text_about_ip_addr
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_ip_addr = obj;
                    lv_obj_set_pos(obj, 10, 21);
                    lv_obj_set_size(obj, 110, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "IP地址：");
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_ip_addr_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_ip_addr_value = obj;
                    lv_obj_set_pos(obj, 120, 21);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "192.168.1.1");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_line_create(parent_obj);
                    static lv_point_precise_t line_points[] = {
                        { 0, 0 },
                        { 256, 0 }
                    };
                    lv_line_set_points(obj, line_points, 2);
                    objects.line_1 = obj;
                    lv_obj_set_pos(obj, 12, 54);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0xff4a4a4a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_line_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_mac_addr
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_mac_addr = obj;
                    lv_obj_set_pos(obj, 10, 72);
                    lv_obj_set_size(obj, 110, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "MAC地址：");
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_mac_addr_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_mac_addr_value = obj;
                    lv_obj_set_pos(obj, 120, 72);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "E86538255A2E");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_line_create(parent_obj);
                    static lv_point_precise_t line_points[] = {
                        { 0, 0 },
                        { 256, 0 }
                    };
                    lv_line_set_points(obj, line_points, 2);
                    objects.line_2 = obj;
                    lv_obj_set_pos(obj, 12, 108);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_line_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0xff4a4a4a), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_hw_ver
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_hw_ver = obj;
                    lv_obj_set_pos(obj, 10, 124);
                    lv_obj_set_size(obj, 110, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "硬件版本：");
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_sw_ver
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_sw_ver = obj;
                    lv_obj_set_pos(obj, 10, 178);
                    lv_obj_set_size(obj, 110, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "软件版本：");
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_line_create(parent_obj);
                    static lv_point_precise_t line_points[] = {
                        { 0, 0 },
                        { 256, 0 }
                    };
                    lv_line_set_points(obj, line_points, 2);
                    objects.line_3 = obj;
                    lv_obj_set_pos(obj, 12, 162);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_line_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0xff4a4a4a), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_hw_ver_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_hw_ver_value = obj;
                    lv_obj_set_pos(obj, 120, 124);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "v1.0.1");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_sw_ver_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_sw_ver_value = obj;
                    lv_obj_set_pos(obj, 120, 178);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "v1.0.8");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 60);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
    }
    
    tick_screen_page_setup_about();
}

void tick_screen_page_setup_about() {
    char *new_val = get_var_ip_addr();
    char *cur_val = lv_label_get_text(objects.text_about_ip_addr_value);
    if (strcmp(new_val, cur_val)) {
        lv_label_set_text(objects.text_about_ip_addr_value, new_val);
    }
}

void create_screen_page_netconfig() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_netconfig = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // container_netconfig_reboot
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_netconfig_reboot = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 360);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // text_netconfig_reboot_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_reboot_title = obj;
                    lv_obj_set_pos(obj, 0, 166);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "3秒后重启设备");
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // container_netconfig_prepare
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_netconfig_prepare = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 360);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // image_netconfig_prepare_1
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_prepare_1 = obj;
                    lv_obj_set_pos(obj, 120, 64);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/netconfig_process.bin");
                }
                {
                    // text_netconfig_prepare_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_prepare_title = obj;
                    lv_obj_set_pos(obj, 0, 233);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "准备配网...");
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // container_netconfig_process
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_netconfig_process = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 360);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // image_netconfig_prepare_2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_prepare_2 = obj;
                    lv_obj_set_pos(obj, 120, 64);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/netconfig_process.bin");
                }
                {
                    // image_netconfig_wifi_level_1
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_1 = obj;
                    lv_obj_set_pos(obj, 176, 136);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/wifi_level_1.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // image_netconfig_wifi_level_2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_2 = obj;
                    lv_obj_set_pos(obj, 169, 126);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/wifi_level_2.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // image_netconfig_wifi_level_3
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_3 = obj;
                    lv_obj_set_pos(obj, 162, 115);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/wifi_level_3.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // image_netconfig_wifi_level_4
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_4 = obj;
                    lv_obj_set_pos(obj, 155, 104);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/wifi_level_4.bin");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // text_netconfig_process_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_process_title = obj;
                    lv_obj_set_pos(obj, 0, 233);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "配网中  请耐心等待...");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_line_space(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_return_home
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_return_home = obj;
                    lv_obj_set_pos(obj, 120, 270);
                    lv_obj_set_size(obj, 120, 42);
                    lv_label_set_text(obj, "返回主页");
                    lv_obj_add_event_cb(obj, action_return_home_click, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 21, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        {
            // container_netconfig_success
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_netconfig_success = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 360);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // image_netconfig_success
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_success = obj;
                    lv_obj_set_pos(obj, 120, 64);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/netconfig_success.bin");
                }
                {
                    // text_netconfig_success_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_success_title = obj;
                    lv_obj_set_pos(obj, 0, 233);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "配网完成");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_netconfig_success_countdown
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_success_countdown = obj;
                    lv_obj_set_pos(obj, 0, 270);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "3秒后返回主页");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
    
    tick_screen_page_netconfig();
}

void tick_screen_page_netconfig() {
}

void create_screen_page_ota() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_ota = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // container_ota_process
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_ota_process = obj;
            lv_obj_set_pos(obj, 0, 105);
            lv_obj_set_size(obj, 360, 150);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.bar_ota_percent = obj;
                    lv_obj_set_pos(obj, 90, 71);
                    lv_obj_set_size(obj, 180, 8);
                    // lv_bar_set_value(obj, 20, LV_ANIM_OFF);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00b988), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_ota_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_ota_title = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "设备升级中");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_ota_subtitle
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_ota_subtitle = obj;
                    lv_obj_set_pos(obj, 0, 123);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "请耐心等待...");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // container_failure
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_failure = obj;
            lv_obj_set_pos(obj, 0, 150);
            lv_obj_set_size(obj, 360, 60);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // text_ota_failure_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_ota_failure_title = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "升级失败");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_ota_failure_code
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_ota_failure_code = obj;
                    lv_obj_set_pos(obj, 0, 38);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "错误码F02");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
    
    tick_screen_page_ota();
}

void tick_screen_page_ota() {
    static int ota_status_last = -1;
    int8_t new_val = get_var_ota_status();
    if (new_val != ota_status_last) {
        ota_status_last = new_val;
        switch (new_val) {
            case 1: // 成功
                lv_obj_add_flag  (objects.container_failure, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(objects.text_ota_title, "升级成功");
                lv_bar_set_value(objects.bar_ota_percent, 100, LV_ANIM_OFF);
                lv_label_set_text(objects.text_ota_subtitle, "3秒后重启");
                lv_obj_clear_flag(objects.container_ota_process, LV_OBJ_FLAG_HIDDEN);
                break;
            case 2: // 失败
                lv_obj_add_flag  (objects.container_ota_process, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.container_failure, LV_OBJ_FLAG_HIDDEN);
                break;
            default: // 进行中
                lv_obj_add_flag  (objects.container_failure, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(objects.text_ota_title, "设备升级中");
                set_var_ota_percent(0);
                lv_label_set_text(objects.text_ota_subtitle, "请耐心等待...");
                lv_obj_clear_flag(objects.container_ota_process, LV_OBJ_FLAG_HIDDEN);
                break;
        }
    }
    {
        int32_t new_val = get_var_ota_percent();
        int32_t cur_val = lv_bar_get_value(objects.bar_ota_percent);
        if (new_val != cur_val) {
            lv_bar_set_value(objects.bar_ota_percent, new_val, LV_ANIM_OFF);
        }
    }
}

void create_screen_page_recog() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_recog = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // image_recog_emotion
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_recog_emotion = obj;
            lv_obj_set_pos(obj, 90, 24);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, "/lvgl/images/e0.bin");
        }
        {
            // text_recog_subtitles
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.text_recog_subtitles = obj;
            lv_obj_set_pos(obj, 38, 216);
            lv_obj_set_size(obj, 284, 94);
            lv_textarea_set_max_length(obj, 256);
            lv_textarea_set_text(obj, "");
            lv_textarea_set_one_line(obj, false);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_line_space(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 300);
            lv_obj_set_size(obj, 360, 60);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_back_click, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 2);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, "/lvgl/images/back.bin");
                }
            }
        }
    }
    
    tick_screen_page_recog();
}

void tick_screen_page_recog() {
    {
        char *new_val = get_var_subtitles();
        char *cur_val = lv_textarea_get_text(objects.text_recog_subtitles);
        if (lv_strcmp(new_val, cur_val) != 0) {
            lv_textarea_set_text(objects.text_recog_subtitles, new_val);
        }
    }
    {
        static int8_t emotion_id_last = -1;
        int8_t new_val = get_var_emotion_id();
        if (new_val != emotion_id_last) {
            emotion_id_last = new_val;
            char filename[32];
            sprintf(filename, "/lvgl/images/e%d.bin", new_val);
            lv_image_set_src(objects.image_recog_emotion, filename);
        }
    }
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_page_main,
    tick_screen_page_menu,
    tick_screen_page_mode,
    tick_screen_page_brightness,
    tick_screen_page_setup,
    tick_screen_page_setup_volume,
    tick_screen_page_setup_screen,
    tick_screen_page_setup_language,
    tick_screen_page_setup_about,
    tick_screen_page_netconfig,
    tick_screen_page_ota,
    tick_screen_page_recog,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    if (font_misans_12px == NULL) {
        font_misans_12px = lv_binfont_create("/lvgl/fonts/misans_12px.bin");
    }
    if (font_misans_80px == NULL) {
        font_misans_80px = lv_binfont_create("/lvgl/fonts/misans_80px.bin");
    }
    if (font_pingfang_16px == NULL) {
        font_pingfang_16px = (lv_font_t *)&ui_font_pingfang_sc_regular_16px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
    }
    if (font_pingfang_20px == NULL) {
        font_pingfang_20px = (lv_font_t *)&ui_font_pingfang_sc_regular_20px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
    }
    if (font_pingfang_24px == NULL) {
        font_pingfang_24px = (lv_font_t *)&ui_font_pingfang_sc_regular_24px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
    }
    
    create_screen_page_main();
    create_screen_page_menu();
    create_screen_page_mode();
    create_screen_page_brightness();
    create_screen_page_setup();
    create_screen_page_setup_volume();
    create_screen_page_setup_screen();
    create_screen_page_setup_language();
    create_screen_page_setup_about();
    create_screen_page_netconfig();
    create_screen_page_ota();
    create_screen_page_recog();
}
