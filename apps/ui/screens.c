#include <stdio.h>
#include <string.h>

#include "fonts.h"
#include "screens.h"
#include "actions.h"
#include "vars.h"
#include "ui.h"

#include "qm_iot_weather.h"
#include "qm_time.h"

#include "bsp_lcd.h"

objects_t objects;

lv_font_t *font_misans_8px = NULL;
lv_font_t *font_misans_24px = NULL;
lv_font_t *font_misans_40px = NULL;
lv_font_t *font_misans_64px = NULL;
lv_font_t *font_pingfang_12px = NULL;
lv_font_t *font_pingfang_14px = NULL;
lv_font_t *font_pingfang_20px = NULL;
lv_font_t *font_pingfang_24px = NULL;

void create_screen_page_sleep() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_sleep = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 240, 240);
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
                    lv_obj_t *obj = lv_gif_create(parent_obj);
                    objects.img_expression = obj;
                    lv_obj_set_pos(obj, 24, 30);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_gif_set_src(obj, "/lvgl/gifs/emotion20.gif");
                }
            }
        }
    }
    
    tick_screen_page_sleep();
}

void tick_screen_page_sleep() {
}

void create_screen_page_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // container wifi_state bat_state
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 240, 60);
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
                    // wifi_state
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.wifi_state = obj;
                    lv_obj_set_pos(obj, 91, 23);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/wifi.bin");
                }
                {
                    // bat_state
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.bat_state = obj;
                    lv_obj_set_pos(obj, 130, 25);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/bat.bin");
                }
            }
        }

        {
            // container time weekday date
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 60);
            lv_obj_set_size(obj, 240, 180);
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
                    // text_time
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_time = obj;
                    lv_obj_set_pos(obj, 0, -50);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "00:00");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_64px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // text_weekday
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.text_weekday = obj;
                    lv_obj_set_pos(obj, 0, 0);
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
                    lv_obj_set_pos(obj, 4, 30);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "2025年01月01日");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
    
    tick_screen_page_main();
}

const char weekday_tbl[7][16] = { "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", };

void tick_screen_page_main() {
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

void create_screen_page_alarm() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_alarm = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 240, 57);
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
                    // label_alarm
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.label_alarm = obj;
                    lv_obj_set_pos(obj, 54, 30);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "闹钟");
                }
                {
                    // container_add
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_add = obj;
                    lv_obj_set_pos(obj, 136, -1);
                    lv_obj_set_size(obj, 104, 57);
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
                            // img_add
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.img_add = obj;
                            lv_obj_set_pos(obj, 25, 32);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, "/lvgl/images/alarm_add.bin");
                        }
                    }
                }
            }
        }
        {
            // list_alarm
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.list_alarm = obj;
            lv_obj_set_pos(obj, 33, 61);
            lv_obj_set_size(obj, 175, 183);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scroll_dir(obj, LV_DIR_VER);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.obj0 = obj;
                    lv_obj_set_pos(obj, 0, 5);
                    lv_obj_set_size(obj, 175, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_switch_create(parent_obj);
                            lv_obj_set_pos(obj, 115, 5);
                            lv_obj_set_size(obj, 30, 15);
                            lv_obj_add_state(obj, LV_STATE_CHECKED);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1dca1b), LV_PART_INDICATOR | LV_STATE_CHECKED);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -40, -5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "07:00");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj1 = obj;
                            lv_obj_set_pos(obj, 0, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "一");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj2 = obj;
                            lv_obj_set_pos(obj, 11, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "二");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj3 = obj;
                            lv_obj_set_pos(obj, 22, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "三");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj4 = obj;
                            lv_obj_set_pos(obj, 33, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "四");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj5 = obj;
                            lv_obj_set_pos(obj, 44, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "五");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj6 = obj;
                            lv_obj_set_pos(obj, 55, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "六");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj7 = obj;
                            lv_obj_set_pos(obj, 66, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "日");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj8 = obj;
                            lv_obj_set_pos(obj, 118, 23);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "起床");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.obj9 = obj;
                    lv_obj_set_pos(obj, 0, 5);
                    lv_obj_set_size(obj, 175, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_switch_create(parent_obj);
                            lv_obj_set_pos(obj, 115, 5);
                            lv_obj_set_size(obj, 30, 15);
                            lv_obj_add_state(obj, LV_STATE_CHECKED);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1dca1b), LV_PART_INDICATOR | LV_STATE_CHECKED);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -40, -5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "08:20");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj10 = obj;
                            lv_obj_set_pos(obj, 0, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "一");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj11 = obj;
                            lv_obj_set_pos(obj, 11, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "二");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj12 = obj;
                            lv_obj_set_pos(obj, 22, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "三");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj13 = obj;
                            lv_obj_set_pos(obj, 33, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "四");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj14 = obj;
                            lv_obj_set_pos(obj, 44, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "五");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj15 = obj;
                            lv_obj_set_pos(obj, 55, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "六");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj16 = obj;
                            lv_obj_set_pos(obj, 66, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "日");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj17 = obj;
                            lv_obj_set_pos(obj, 118, 23);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "上课");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.obj18 = obj;
                    lv_obj_set_pos(obj, 0, 5);
                    lv_obj_set_size(obj, 175, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_switch_create(parent_obj);
                            lv_obj_set_pos(obj, 115, 5);
                            lv_obj_set_size(obj, 30, 15);
                            lv_obj_add_state(obj, LV_STATE_CHECKED);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1dca1b), LV_PART_INDICATOR | LV_STATE_CHECKED);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -40, -5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "11:00");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj19 = obj;
                            lv_obj_set_pos(obj, 0, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "一");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj20 = obj;
                            lv_obj_set_pos(obj, 11, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "二");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj21 = obj;
                            lv_obj_set_pos(obj, 22, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "三");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj22 = obj;
                            lv_obj_set_pos(obj, 33, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "四");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj23 = obj;
                            lv_obj_set_pos(obj, 44, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "五");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj24 = obj;
                            lv_obj_set_pos(obj, 55, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "六");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj25 = obj;
                            lv_obj_set_pos(obj, 66, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "日");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj26 = obj;
                            lv_obj_set_pos(obj, 118, 23);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "吃饭");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.obj27 = obj;
                    lv_obj_set_pos(obj, 0, 5);
                    lv_obj_set_size(obj, 175, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_switch_create(parent_obj);
                            lv_obj_set_pos(obj, 115, 5);
                            lv_obj_set_size(obj, 30, 15);
                            lv_obj_add_state(obj, LV_STATE_CHECKED);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1dca1b), LV_PART_INDICATOR | LV_STATE_CHECKED);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -40, -5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "21:00");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj28 = obj;
                            lv_obj_set_pos(obj, 0, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "一");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj29 = obj;
                            lv_obj_set_pos(obj, 11, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "二");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj30 = obj;
                            lv_obj_set_pos(obj, 22, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "三");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj31 = obj;
                            lv_obj_set_pos(obj, 33, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "四");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj32 = obj;
                            lv_obj_set_pos(obj, 44, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "五");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj33 = obj;
                            lv_obj_set_pos(obj, 55, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "六");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj34 = obj;
                            lv_obj_set_pos(obj, 66, 25);
                            lv_obj_set_size(obj, 10, 10);
                            lv_obj_set_style_text_font(obj, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "日");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj35 = obj;
                            lv_obj_set_pos(obj, 118, 23);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "睡觉");
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_page_alarm();
}

void tick_screen_page_alarm() {
}

void create_screen_page_timer() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_timer = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 66);
            lv_obj_set_size(obj, 240, 90);
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
                    // label_count
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.label_count = obj;
                    lv_obj_set_pos(obj, 27, 35);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "00:00:00");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 160);
            lv_obj_set_size(obj, 240, 80);
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
                    // container_start
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_start = obj;
                    lv_obj_set_pos(obj, 90, 0);
                    lv_obj_set_size(obj, 61, 76);
                    lv_obj_add_event_cb(obj, action_timer_opt_click, LV_EVENT_CLICKED, (void *)0);
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
                            // img_start
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.img_start = obj;
                            lv_obj_set_pos(obj, 12, 10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, "/lvgl/images/timer_start.bin");
                        }
                    }
                }
                {
                    // container_reset
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_reset = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 81, 76);
                    lv_obj_add_event_cb(obj, action_timer_reset_click, LV_EVENT_CLICKED, (void *)0);
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
                            // img_reset
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.img_reset = obj;
                            lv_obj_set_pos(obj, 40, 10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, "/lvgl/images/timer_reset.bin");
                        }
                    }
                }
                {
                    // container_stop
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_stop = obj;
                    lv_obj_set_pos(obj, 159, 0);
                    lv_obj_set_size(obj, 81, 76);
                    lv_obj_add_event_cb(obj, action_timer_stop_click, LV_EVENT_CLICKED, (void *)0);
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
                            // img_stop
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.img_stop = obj;
                            lv_obj_set_pos(obj, 5, 10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, "/lvgl/images/timer_stop.bin");
                        }
                    }
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 5);
            lv_obj_set_size(obj, 240, 74);
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
                    // btn_bg
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.btn_bg = obj;
                    lv_obj_set_pos(obj, 48, 30);
                    lv_obj_set_size(obj, 144, 26);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // container_count_down
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_count_down = obj;
                    lv_obj_set_pos(obj, 120, 12);
                    lv_obj_set_size(obj, 120, 54);
                    lv_obj_add_event_cb(obj, action_count_down_click, LV_EVENT_CLICKED, (void *)0);
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
                            // btn_count_down
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn_count_down = obj;
                            lv_obj_set_pos(obj, 0, 20);
                            lv_obj_set_size(obj, 70, 24);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // label_count_down
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.label_count_down = obj;
                                    lv_obj_set_pos(obj, 1, -4);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, font_pingfang_14px, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "倒计时");
                                }
                            }
                        }
                    }
                }
                {
                    // container_count_up
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_count_up = obj;
                    lv_obj_set_pos(obj, 0, 12);
                    lv_obj_set_size(obj, 120, 54);
                    lv_obj_add_event_cb(obj, action_count_up_click, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // btn_count_up
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.btn_count_up = obj;
                            lv_obj_set_pos(obj, 50, 20);
                            lv_obj_set_size(obj, 70, 24);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 26, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_ofs_y(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // label_count_up
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.label_count_up = obj;
                                    lv_obj_set_pos(obj, 1, -4);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, font_pingfang_14px, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "正计时");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    tick_screen_page_timer();
}

void tick_screen_page_timer() {
    static int8_t last_timer_state = TIMER_STATE_NONE;
    int8_t new_timer_state = lvgl_var_timer_state_get();

    static int8_t last_timer_type = TIMER_TYPE_NONE;
    int8_t new_timer_type = lvgl_var_timer_type_get();

    if (new_timer_type != last_timer_type) {
        last_timer_type = new_timer_type;

        last_timer_state = TIMER_STATE_NONE;
        lvgl_var_timer_state_set(TIMER_STATE_IDLE);

        if (new_timer_type == TIMER_TYPE_COUNT_UP) {
            lv_obj_clear_flag(objects.container_count_up, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(objects.btn_count_up, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.label_count_up, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_add_flag(objects.container_count_down, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(objects.btn_count_down, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.label_count_down, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (new_timer_type == TIMER_TYPE_COUNT_DOWN) {
            lv_obj_clear_flag(objects.container_count_down, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(objects.btn_count_down, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.label_count_down, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_add_flag(objects.container_count_up, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(objects.btn_count_up, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.label_count_up, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

        } else {
            // ignore
        }
    }

    static uint8_t cur_hour = 0;
    static uint8_t cur_min = 0;
    static uint8_t cur_sec = 0;
    static char time[9] = {0};

    static uint32_t count_down_time = 30 * 60;
    static uint32_t count_down_rest = 0;
    static uint8_t count_down_hour = 0;
    static uint8_t count_down_min = 0;
    static uint8_t count_down_sec = 0;

    if (new_timer_state != last_timer_state) {
        last_timer_state = new_timer_state;

        switch (new_timer_state) {
            case TIMER_STATE_RUNNING:
                lv_img_set_src(objects.img_start, "/lvgl/images/timer_pause.bin");
            break;

            case TIMER_STATE_PAUSE:
                lv_img_set_src(objects.img_start, "/lvgl/images/timer_start.bin");
            break;

            case TIMER_STATE_IDLE:
                lv_img_set_src(objects.img_start, "/lvgl/images/timer_start.bin");

                if (lvgl_var_timer_type_get() == TIMER_TYPE_COUNT_UP) {
                    lv_label_set_text(objects.label_count, "00:00:00");
                } else {
                    lv_label_set_text(objects.label_count, "00:30:00");
                }

                cur_hour = cur_min = cur_sec = 0;
                count_down_hour = count_down_min = count_down_sec = 0;
            break;
        }
    }

    // timer running
    static uint32_t last_time = 0;

    if (new_timer_state == TIMER_STATE_RUNNING) {
        if (qm_now_ms() - last_time > 1000) {
            last_time = qm_now_ms();

            cur_sec++;

            if (cur_sec == 60) {
                cur_sec = 0;
                cur_min++;

                if (cur_min == 60) {
                    cur_min = 0;
                    cur_hour++;

                    if (cur_hour >= 24) {
                        lvgl_var_timer_state_set(TIMER_STATE_PAUSE);
                    }
                }
            }

            if (lvgl_var_timer_type_get() == TIMER_TYPE_COUNT_UP) {
                qm_snprintf(time, 9, "%02d:%02d:%02d", cur_hour, cur_min, cur_sec);
            } else {
                count_down_rest = count_down_time - (cur_hour * 60 + cur_min * 60 + cur_sec);
                if (count_down_rest <= 0) {
                    lvgl_var_timer_state_set(TIMER_STATE_PAUSE);
                }

                count_down_hour = count_down_rest / 3600;
                count_down_min = count_down_rest % 3600 / 60;
                count_down_sec = count_down_rest % 60;

                qm_snprintf(time, 9, "%02d:%02d:%02d", count_down_hour, count_down_min, count_down_sec);
            }

            lv_label_set_text(objects.label_count, time);
        }
    }
}

void create_screen_page_pomodoro() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_pomodoro = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // tomato_spinner
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.tomato_spinner = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 240, 240);
            lv_arc_set_range(obj, 0, 10000);
            lv_arc_set_value(obj, 10000);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 360);
            lv_arc_set_rotation(obj, -90);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0xffff3000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 14, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 53, 39);
            lv_obj_set_size(obj, 133, 163);
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
                    // tomato_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.tomato_icon = obj;
                    lv_obj_set_pos(obj, 49, 21);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/tomato.bin");
                }
                {
                    // tomato_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.tomato_label = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "番茄时钟");
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // container_tomato_opt
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_tomato_opt = obj;
                    lv_obj_set_pos(obj, 35, 99);
                    lv_obj_set_size(obj, 64, 64);
                    lv_obj_add_event_cb(obj, tomato_opt_event_all_handler, LV_EVENT_ALL, (void *)0);
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
                            // tomato_opt
                            lv_obj_t *obj = lv_image_create(parent_obj);
                            objects.tomato_opt = obj;
                            lv_obj_set_pos(obj, 14, 14);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_image_set_src(obj, "/lvgl/images/timer_start.bin");
                        }
                    }
                }
            }
        }
    }

    tick_screen_page_pomodoro();
}

void tick_screen_page_pomodoro() {
    static uint32_t tomato_counter = 5 * 60;
    static uint32_t cur_counter = 0;
    uint32_t arc_new_val = 0;
    uint8_t tomato_div = lv_arc_get_max_value(objects.tomato_spinner) / tomato_counter;

    static enum TomatoState last_tomato_state = TOMATO_STATE_NONE;
    enum TomatoState new_tomato_state = lvgl_var_tomato_state_get();

    static uint32_t last_time = 0;

    if (new_tomato_state != last_tomato_state) {
        last_tomato_state = new_tomato_state;

        switch (new_tomato_state) {
            case TOMATO_STATE_RUNNING:
            case TOMATO_STATE_BREAK:
                lv_image_set_src(objects.tomato_opt, "/lvgl/images/timer_pause.bin");
                last_time = qm_now_ms();
            break;

            case TOMATO_STATE_RUNNING_PAUSE:
            case TOMATO_STATE_BREAK_PAUSE:
                lv_image_set_src(objects.tomato_opt, "/lvgl/images/timer_start.bin");
            break;

            case TOMATO_STATE_IDLE:
                lv_image_set_src(objects.tomato_opt, "/lvgl/images/timer_start.bin");
                lv_label_set_text(objects.tomato_label, "番茄时钟");
                lv_arc_set_value(objects.tomato_spinner, lv_arc_get_max_value(objects.tomato_spinner));

                cur_counter = 0;
            break;

            default:
            break;
        }
    }

    if (new_tomato_state == TOMATO_STATE_RUNNING) {
        if (qm_now_ms() - last_time > 1000) {
            last_time = qm_now_ms();

            arc_new_val = lv_arc_get_max_value(objects.tomato_spinner) - cur_counter * tomato_div;

            lv_arc_set_value(objects.tomato_spinner, arc_new_val);

            char time[6];
            uint8_t cur_min = (tomato_counter - cur_counter) / 60;
            uint8_t cur_sec = (tomato_counter - cur_counter) % 60;

            qm_snprintf(time, 6, "%02d:%02d", cur_min, cur_sec);

            lv_label_set_text(objects.tomato_label, time);

            cur_counter++;

            if (cur_counter == tomato_counter) {
                lvgl_var_tomato_state_set(TOMATO_STATE_IDLE);
            }
        }
    }
}

void create_screen_page_weather() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_weather = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 240, 240);
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
                    // label_city
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.label_city = obj;
                    lv_obj_set_pos(obj, 90, 32);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "宁波市");
                }
                {
                    // img_weather
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.img_weather = obj;
                    lv_obj_set_pos(obj, 90, 80);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/w0.bin");
                }
                {
                    // label_weather
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.label_weather = obj;
                    lv_obj_set_pos(obj, 70, 165);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "晴");
                }
                {
                    // label_temperature
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.label_temperature = obj;
                    lv_obj_set_pos(obj, 140, 165);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "16℃");
                }
            }
        }
    }
    
    tick_screen_page_weather();
}

const char weather_name_tbl[48][32] = {
    "晴", "多云", "阴", "阵雨", "雷阵雨", "雷阵雨伴有冰雹", "雨夹雪", "小雨", 
    "中雨", "大雨", "暴雨", "未知", "未知", "阵雪", "小雪", "中雪",
    "大雪", "暴雪", "雾", "冻雨", "沙尘暴", "未知", "未知", "未知",
    "未知", "未知", "未知", "未知", "未知", "浮尘", "晴", "多云",
    "雾", "阵雨", "阵雪", "浮尘", "沙尘暴", "未知", "未知", "未知",
    "未知", "未知", "未知", "未知", "未知", "霾", "霾", "未知",
};

void tick_screen_page_weather() {
    static int8_t weather_id_last = -1;
    static int8_t temperature_last = -128; 
    int8_t weather_id = lvgl_var_weather_id_get();
    if (weather_id > QM_IOT_WEATHER_TYPE_HAZE_NIGHT) {
        weather_id = QM_IOT_WEATHER_TYPE_HAZE_NIGHT;
    }

    // 更新 天气图标 / 天气
    if (weather_id != -1) {
        char s[64];
        sprintf(s, "/lvgl/images/w%d.bin", weather_id);
        lv_image_set_src(objects.img_weather, s);
        lv_label_set_text(objects.label_weather, weather_name_tbl[weather_id]);
    }

    if (weather_id != -1) {
        // 更新城市
        {
            char *new_val = lvgl_var_city_get();
            char *cur_val = lv_label_get_text(objects.label_city);
            if (new_val != cur_val) {
                lv_label_set_text(objects.label_city, new_val);
            }
        }
        // 更新温度
        {
            int8_t new_val = lvgl_var_temperature_get();
            if (new_val != temperature_last) {
                temperature_last = new_val;
                char s[16];
                sprintf(s, "%d℃", new_val);
                lv_label_set_text(objects.label_temperature, s);
            }
        }
    }
}

// tick functions
typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_page_sleep,
    tick_screen_page_main,
    tick_screen_page_alarm,
    tick_screen_page_timer,
    tick_screen_page_pomodoro,
    tick_screen_page_weather,
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

    if (font_misans_8px == NULL) {
        font_misans_8px = (lv_font_t *)&ui_font_misans_8px;
    }
    if (font_misans_24px == NULL) {
        font_misans_24px = (lv_font_t *)&ui_font_misans_24px;
    }
    if (font_misans_40px == NULL) {
        font_misans_40px = (lv_font_t *)&ui_font_misans_40px;
    }
    if (font_misans_64px == NULL) {
        font_misans_64px = (lv_font_t *)&ui_font_misans_64px;
    }
    if (font_pingfang_12px == NULL) {
        font_pingfang_12px = (lv_font_t *)&ui_font_pingfang_sc_regular_12px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
    }
    if (font_pingfang_14px == NULL) {
        font_pingfang_14px = (lv_font_t *)&ui_font_pingfang_sc_regular_14px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
    }
    if (font_pingfang_20px == NULL) {
        font_pingfang_20px = (lv_font_t *)&ui_font_pingfang_sc_regular_20px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
    }
    if (font_pingfang_24px == NULL) {
        font_pingfang_24px = (lv_font_t *)&ui_font_pingfang_sc_regular_24px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
    }
    
    create_screen_page_sleep();
    create_screen_page_main();
    create_screen_page_alarm();
    create_screen_page_timer();
    create_screen_page_pomodoro();
    create_screen_page_weather();
}
