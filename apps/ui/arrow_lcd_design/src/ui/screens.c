#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;

static void event_handler_cb_page_brightness_slider_brightness_top(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_slider_get_value(ta);
            set_var_brightness_top(value);
        }
    }
}

static void event_handler_cb_page_brightness_slider_brightness_bottom(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_slider_get_value(ta);
            set_var_brightness_bottom(value);
        }
    }
}

static void event_handler_cb_page_setup_volume_slider_volume_value(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_slider_get_value(ta);
            set_var_volume_value(value);
        }
    }
}

void create_screen_page_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // image_wifi
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi = obj;
            lv_obj_set_pos(obj, 165, 14);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_wifi_connected);
        }
        {
            // image_wifi_level_s_1
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_1 = obj;
            lv_obj_set_pos(obj, 178, 37);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_wifi_level_s_1);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // image_wifi_level_s_2
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_2 = obj;
            lv_obj_set_pos(obj, 174, 31);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_wifi_level_s_2);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // image_wifi_level_s_3
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_3 = obj;
            lv_obj_set_pos(obj, 170, 25);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_wifi_level_s_3);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // image_wifi_level_s_4
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_wifi_level_s_4 = obj;
            lv_obj_set_pos(obj, 167, 20);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_wifi_level_s_4);
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
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE|LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // image_weather_unknown
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_weather_unknown = obj;
                    lv_obj_set_pos(obj, 92, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_weather_none);
                }
                {
                    // text_weather_unknown
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_weather_unknown = obj;
                    lv_obj_set_pos(obj, 165, 19);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "无法获取当前\n天气");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // image_weather
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_weather = obj;
                    lv_obj_set_pos(obj, 88, 7);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_weather_day_sunny);
                }
                {
                    // text_city
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_city = obj;
                    lv_obj_set_pos(obj, 194, 8);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "宁波");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_weather
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_weather = obj;
                    lv_obj_set_pos(obj, 194, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "晴");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_temperature
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_temperature = obj;
                    lv_obj_set_pos(obj, 254, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "15℃");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_time
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_time = obj;
                    lv_obj_set_pos(obj, 0, -24);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "12:00");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_misans, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_weekday
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_weekday = obj;
                    lv_obj_set_pos(obj, 0, 51);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "星期一");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_date
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_date = obj;
                    lv_obj_set_pos(obj, 4, 83);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "2025年03月14日");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    // image_logo
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_logo = obj;
                    lv_obj_set_pos(obj, 105, 105);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_logo);
                }
            }
        }
    }
    
    tick_screen_page_main();
}

void tick_screen_page_main() {
}

void create_screen_page_menu() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_menu = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
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
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_menu_home_released, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_menu_home_pressed, NULL);
                    lv_obj_add_event_cb(obj, action_home_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_speech
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_speech = obj;
                    lv_obj_set_pos(obj, 24, 88);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_menu_speech_released, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_menu_speech_pressed, NULL);
                    lv_obj_add_event_cb(obj, action_recog_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_setup
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_setup = obj;
                    lv_obj_set_pos(obj, 53, 178);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_menu_setup_released, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_menu_setup_pressed, NULL);
                    lv_obj_add_event_cb(obj, action_setup_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_mode
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_mode = obj;
                    lv_obj_set_pos(obj, 163, 178);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_menu_mode_released, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_menu_home_pressed, NULL);
                    lv_obj_add_event_cb(obj, action_mode_click, LV_EVENT_CLICKED, (void *)0);
                }
                {
                    // menu_brightness
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.menu_brightness = obj;
                    lv_obj_set_pos(obj, 192, 88);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_menu_brightness_released, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_menu_brightness_pressed, NULL);
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
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // mode_nightlamp
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_nightlamp = obj;
            lv_obj_set_pos(obj, 144, 64);
            lv_obj_set_size(obj, 72, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_mode_nightlamp_unchecked, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_mode_nightlamp_unchecked, NULL);
            lv_obj_add_event_cb(obj, action_nightlamp_click, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // mode_read
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_read = obj;
            lv_obj_set_pos(obj, 45, 144);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_mode_read_unchecked, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_mode_read_unchecked, NULL);
            lv_obj_add_event_cb(obj, action_read_click, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // mode_relaxation
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_relaxation = obj;
            lv_obj_set_pos(obj, 251, 144);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_mode_relaxation_unchecked, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_mode_relaxation_unchecked, NULL);
            lv_obj_add_event_cb(obj, action_idle_click, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // mode_sleep
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.mode_sleep = obj;
            lv_obj_set_pos(obj, 144, 232);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 72);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_mode_sleep_unchecked, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_mode_sleep_unchecked, NULL);
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
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_img_set_src(obj, &img_back);
                }
            }
        }
    }
    
    tick_screen_page_mode();
}

void tick_screen_page_mode() {
}

void create_screen_page_brightness() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_brightness = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // image_brightness_light_onoff
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_brightness_light_onoff = obj;
            lv_obj_set_pos(obj, 148, 39);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_brightness_light_off);
            lv_obj_add_event_cb(obj, action_brightness_light_onoff_click, LV_EVENT_CLICKED, (void *)0);
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
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // slider_brightness_top
                    lv_obj_t *obj = lv_slider_create(parent_obj);
                    objects.slider_brightness_top = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 294, 48);
                    lv_slider_set_range(obj, 8, 100);
                    lv_obj_add_event_cb(obj, action_brightness_top_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
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
                    lv_img_set_src(obj, &img_brightness_low);
                }
                {
                    // image_brightness_high1
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_brightness_high1 = obj;
                    lv_obj_set_pos(obj, 266, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_brightness_high);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
                }
                {
                    // slider_brightness_bottom
                    lv_obj_t *obj = lv_slider_create(parent_obj);
                    objects.slider_brightness_bottom = obj;
                    lv_obj_set_pos(obj, 0, 82);
                    lv_obj_set_size(obj, 294, 48);
                    lv_slider_set_range(obj, 8, 100);
                    lv_obj_add_event_cb(obj, action_brightness_bottom_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
                    lv_obj_add_event_cb(obj, event_handler_cb_page_brightness_slider_brightness_bottom, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_GESTURE_BUBBLE);
                    lv_obj_add_state(obj, LV_STATE_DISABLED);
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
                    lv_label_set_text(obj, "关闭");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // image_brightness_low2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_brightness_low2 = obj;
                    lv_obj_set_pos(obj, 11, 98);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_brightness_low);
                }
                {
                    // image_brightness_high2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_brightness_high2 = obj;
                    lv_obj_set_pos(obj, 266, 98);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_brightness_high);
                }
                {
                    // text_brightness_bottom_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_brightness_bottom_value = obj;
                    lv_obj_set_pos(obj, -2, 70);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "关闭");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_img_set_src(obj, &img_back);
                }
            }
        }
    }
    
    tick_screen_page_brightness();
}

void tick_screen_page_brightness() {
    {
        int32_t new_val = get_var_brightness_top();
        int32_t cur_val = lv_slider_get_value(objects.slider_brightness_top);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.slider_brightness_top;
            lv_slider_set_value(objects.slider_brightness_top, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
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
    }
}

void create_screen_page_setup() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
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
                    lv_img_set_src(obj, &img_setup_volume);
                }
                {
                    // text_setup_volume
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_volume = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "声音设置");
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_more);
                    lv_obj_add_event_cb(obj, action_volume_click, LV_EVENT_CLICKED, (void *)0);
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
                    lv_img_set_src(obj, &img_setup_screen);
                }
                {
                    // text_setup_screen
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_screen = obj;
                    lv_obj_set_pos(obj, 70, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "自动熄屏");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 6);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_more);
                    lv_obj_add_event_cb(obj, action_screen_click, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 64, 156);
            lv_obj_set_size(obj, 252, 50);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_language_click, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_language);
                }
                {
                    // text_setup_language
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_language = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "语言");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_more);
                    lv_obj_add_event_cb(obj, action_language_click, LV_EVENT_CLICKED, (void *)0);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_wifi);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_more);
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
                    lv_img_set_src(obj, &img_setup_about);
                }
                {
                    // text_setup_about
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_about = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "关于设备");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_more);
                    lv_obj_add_event_cb(obj, action_about_click, LV_EVENT_CLICKED, (void *)0);
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
                    lv_img_set_src(obj, &img_factory_reset);
                }
                {
                    // text_setup_factoryreset
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_setup_factoryreset = obj;
                    lv_obj_set_pos(obj, 70, 14);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "恢复出厂");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 212, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_setup_more);
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
                    lv_img_set_src(obj, &img_back);
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
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // text_network_config_infomation
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_network_config_infomation = obj;
                            lv_obj_set_pos(obj, 50, 17);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "点击确定\n设备进入配网状态");
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_line_space(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // text_network_config_cancel
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_network_config_cancel = obj;
                            lv_obj_set_pos(obj, 16, 90);
                            lv_obj_set_size(obj, 88, 40);
                            lv_label_set_text(obj, "取消");
                            lv_obj_add_event_cb(obj, action_network_config_cancel_click, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff545454), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // text_network_config_submit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.text_network_config_submit = obj;
                            lv_obj_set_pos(obj, 72, 38);
                            lv_obj_set_size(obj, 88, 40);
                            lv_label_set_text(obj, "确定");
                            lv_obj_add_event_cb(obj, action_network_config_submit_click, LV_EVENT_CLICKED, (void *)0);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00b988), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
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
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_img_create(parent_obj);
            lv_obj_set_pos(obj, 38, 100);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_setup_volume_mute);
        }
        {
            // text_volume_mute
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_volume_mute = obj;
            lv_obj_set_pos(obj, 76, 104);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "静音功能");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_volume_adjust
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_volume_adjust = obj;
            lv_obj_set_pos(obj, 38, 164);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "音量调节");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // slider_volume_value
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.slider_volume_value = obj;
            lv_obj_set_pos(obj, 38, 195);
            lv_obj_set_size(obj, 285, 48);
            lv_obj_add_event_cb(obj, action_volume_value_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_page_setup_volume_slider_volume_value, LV_EVENT_ALL, 0);
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
            lv_obj_set_style_text_font(obj, &ui_font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_percent_100
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_percent_100 = obj;
            lv_obj_set_pos(obj, 290, 249);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "100%");
            lv_obj_set_style_text_font(obj, &ui_font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_volume_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_volume_value = obj;
            lv_obj_set_pos(obj, 0, 76);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "50%");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_misans_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_setup_volume_mute_close, NULL);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_setup_volume_mute_close, NULL);
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
                    lv_img_set_src(obj, &img_back);
                }
            }
        }
    }
    
    tick_screen_page_setup_volume();
}

void tick_screen_page_setup_volume() {
    {
        int32_t new_val = get_var_volume_value();
        int32_t cur_val = lv_slider_get_value(objects.slider_volume_value);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.slider_volume_value;
            lv_slider_set_value(objects.slider_volume_value, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_page_setup_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
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
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_30s
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_30s = obj;
                            lv_obj_set_pos(obj, 196, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, &img_setup_selected);
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
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_1min
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_1min = obj;
                            lv_obj_set_pos(obj, 196, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, &img_setup_selected);
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
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_5min
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_5min = obj;
                            lv_obj_set_pos(obj, 196, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, &img_setup_selected);
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
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_screen_tmo_never
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_screen_tmo_never = obj;
                            lv_obj_set_pos(obj, 196, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, &img_setup_selected);
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
                    lv_img_set_src(obj, &img_back);
                }
            }
        }
    }
    
    tick_screen_page_setup_screen();
}

void tick_screen_page_setup_screen() {
}

void create_screen_page_setup_language() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup_language = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
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
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_language_cn
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_language_cn = obj;
                            lv_obj_set_pos(obj, 203, 8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, &img_setup_selected);
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
                            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // setup_language_en
                            lv_obj_t *obj = lv_img_create(parent_obj);
                            objects.setup_language_en = obj;
                            lv_obj_set_pos(obj, 203, 8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_img_set_src(obj, &img_setup_selected);
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
                    lv_img_set_src(obj, &img_back);
                }
            }
        }
    }
    
    tick_screen_page_setup_language();
}

void tick_screen_page_setup_language() {
}

void create_screen_page_setup_about() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_setup_about = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // container_about_info
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // line_1
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // line_2
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // line_3
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_about_sw_ver_value
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_about_sw_ver_value = obj;
                    lv_obj_set_pos(obj, 120, 178);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "v1.0.8");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_img_set_src(obj, &img_back);
                }
            }
        }
    }
    
    tick_screen_page_setup_about();
}

void tick_screen_page_setup_about() {
}

void create_screen_page_netconfig() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_netconfig = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
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
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // text_netconfig_reboot_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_reboot_title = obj;
                    lv_obj_set_pos(obj, 0, 166);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "3秒后重启设备");
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_img_set_src(obj, &img_network_config_prepare);
                }
                {
                    // text_netconfig_prepare_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_prepare_title = obj;
                    lv_obj_set_pos(obj, 0, 233);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "准备配网...");
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            {
                lv_obj_t *parent_obj = obj;
                {
                    // image_netconfig_prepare_2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_prepare_2 = obj;
                    lv_obj_set_pos(obj, 120, 64);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_network_config_prepare);
                }
                {
                    // image_netconfig_wifi_level_1
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_1 = obj;
                    lv_obj_set_pos(obj, 176, 136);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_wifi_level_1);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // image_netconfig_wifi_level_2
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_2 = obj;
                    lv_obj_set_pos(obj, 169, 126);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_wifi_level_2);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // image_netconfig_wifi_level_3
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_3 = obj;
                    lv_obj_set_pos(obj, 162, 115);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_wifi_level_3);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                {
                    // image_netconfig_wifi_level_4
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.image_netconfig_wifi_level_4 = obj;
                    lv_obj_set_pos(obj, 155, 104);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_wifi_level_4);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 21, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff333333), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_img_set_src(obj, &img_network_config_success);
                }
                {
                    // text_netconfig_success_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_netconfig_success_title = obj;
                    lv_obj_set_pos(obj, 0, 233);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "配网完成");
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    // bar_ota_percent
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.bar_ota_percent = obj;
                    lv_obj_set_pos(obj, 90, 71);
                    lv_obj_set_size(obj, 180, 8);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
    
    tick_screen_page_ota();
}

void tick_screen_page_ota() {
}

void create_screen_page_recog() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_recog = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, action_gesture_handler, LV_EVENT_GESTURE, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // image_recog_emotion
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.image_recog_emotion = obj;
            lv_obj_set_pos(obj, 90, 24);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_emotion01);
        }
        {
            // text_recog_subtitles
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.text_recog_subtitles = obj;
            lv_obj_set_pos(obj, 38, 210);
            lv_obj_set_size(obj, 284, 94);
            lv_textarea_set_max_length(obj, 256);
            lv_textarea_set_text(obj, "2024年3月23日坑爹的LVGL显示中文一直不太友好，如果是显示固定的中的中中的中中的中中的中LV");
            lv_textarea_set_one_line(obj, false);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_set_style_text_font(obj, &ui_font_pingfang_sc_regular_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 156, 2);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_back);
                }
            }
        }
    }
    
    tick_screen_page_recog();
}

void tick_screen_page_recog() {
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
