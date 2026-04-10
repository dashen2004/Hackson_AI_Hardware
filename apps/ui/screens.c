#include <stdio.h>
#include <string.h>

#include "core/lv_obj_event.h"
#include "fonts.h"
#include "screens.h"
#include "actions.h"
#include "freertos/idf_additions.h"
#include "misc/lv_event.h"
#include "vars.h"
#include "ui.h"

#include "qm_iot_weather.h"
#include "qm_time.h"

#include "bsp_lcd.h"
#include "alarm_manager.h"
#include "widgets/roller/lv_roller.h"

// alarm
typedef struct {
    lv_obj_t *root;
    lv_obj_t *label_time;
    lv_obj_t *label_repeat;
    lv_obj_t *sw;
} alarm_item_t;

typedef struct {
    lv_obj_t *root;
    uint8_t checked;
} alarm_weekday_t;

typedef struct {
    uint8_t opt_type;   // 1:add 2:edit
    int8_t alarm_index;   // -1: null

    alarm_weekday_t alarm_weekday[7];

    uint8_t hour;
    uint8_t min;
} alarm_edit_t;

static alarm_item_t lv_alarms[ALARM_LIST_MAX_COUNT] = {0};
static int32_t lv_alarm_count = 0;

static alarm_edit_t lv_alarm_edit_handle = {0};

static void page_alarm_alarm_list_init(void);
static void page_timer_countdown_roller_init(void);
static void page_alarm_edit_init(void);

objects_t objects;

lv_font_t *font_misans_8px = NULL;
lv_font_t *font_misans_16px = NULL;
lv_font_t *font_misans_24px = NULL;
lv_font_t *font_misans_40px = NULL;
lv_font_t *font_misans_64px = NULL;
lv_font_t *font_pingfang_12px = NULL;
lv_font_t *font_pingfang_14px = NULL;
lv_font_t *font_pingfang_16px = NULL;
lv_font_t *font_pingfang_20px = NULL;
lv_font_t *font_pingfang_24px = NULL;

const char weather_name_tbl[48][32] = {
    "晴", "多云", "阴", "阵雨", "雷阵雨", "雷阵雨伴有冰雹", "雨夹雪", "小雨", 
    "中雨", "大雨", "暴雨", "未知", "未知", "阵雪", "小雪", "中雪",
    "大雪", "暴雪", "雾", "冻雨", "沙尘暴", "未知", "未知", "未知",
    "未知", "未知", "未知", "未知", "未知", "浮尘", "晴", "多云",
    "雾", "阵雨", "阵雪", "浮尘", "沙尘暴", "未知", "未知", "未知",
    "未知", "未知", "未知", "未知", "未知", "霾", "霾", "未知",
};

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
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.img_expression = obj;
                    lv_obj_set_pos(obj, 24, 30);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/e19.bin");
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
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_temp = obj;
            lv_obj_set_pos(obj, 130, 56);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "15℃");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_weather_main = obj;
            lv_obj_set_pos(obj, 75, 50);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, "/lvgl/images/w0_s.bin");
        }
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
        {
            // text_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_time = obj;
            lv_obj_set_pos(obj, 0, 0);
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
            lv_obj_set_pos(obj, 0, 55);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "星期三");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // text_date
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.text_date = obj;
            lv_obj_set_pos(obj, 0, 80);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "2025年01月01日");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
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

    static int8_t weather_id_last = -1;
    static int8_t temperature_last = -128; 
    int8_t weather_id = lvgl_var_weather_id_get();
    if (weather_id > QM_IOT_WEATHER_TYPE_HAZE_NIGHT) {
        weather_id = QM_IOT_WEATHER_TYPE_HAZE_NIGHT;
    }

    // 更新 天气图标 / 天气
    if (weather_id != -1) {
        if (weather_id != weather_id_last) {
            weather_id_last = weather_id;
            char s[64];
            sprintf(s, "/lvgl/images/w%d_s.bin", weather_id);
            lv_image_set_src(objects.img_weather_main, s);
        }
    }

    if (weather_id != -1) {
        // 更新温度
        int8_t new_val = lvgl_var_temperature_get();
        if (new_val != temperature_last) {
            temperature_last = new_val;
            char s[16];
            sprintf(s, "%d℃", new_val);
            lv_label_set_text(objects.text_temp, s);
        }
    }
}

void create_screen_page_alarm() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_alarm = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
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
            {
                lv_obj_t *parent_obj = obj;
                {
                    // label_alarm
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.label_alarm = obj;
                    lv_obj_set_pos(obj, 55, 30);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "闹钟");
                }
                {
                    // container_add
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.container_add = obj;
                    lv_obj_set_pos(obj, 130, 0);
                    lv_obj_set_size(obj, 92, 57);
                    lv_obj_add_event_cb(obj, action_alarm_add_click, LV_EVENT_CLICKED, (void *)0);
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
            lv_obj_set_pos(obj, 33, 66);
            lv_obj_set_size(obj, 175, 180);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scroll_dir(obj, LV_DIR_VER);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
        }
    }
    
    tick_screen_page_alarm();
}

void tick_screen_page_alarm() {
}

void create_screen_page_alarm_edit() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_alarm_edit = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // disp_alarm_edit
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.disp_alarm_edit = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 166, 97);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // roller_alarm_hour
                    lv_obj_t *obj = lv_roller_create(parent_obj);
                    objects.roller_alarm_hour = obj;
                    lv_obj_set_pos(obj, 14, 0);
                    lv_obj_set_size(obj, 75, 94);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_16px, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_SELECTED | LV_STATE_DEFAULT);
                }
                {
                    // roller_alarm_min
                    lv_obj_t *obj = lv_roller_create(parent_obj);
                    objects.roller_alarm_min = obj;
                    lv_obj_set_pos(obj, 72, 0);
                    lv_obj_set_size(obj, 75, 94);
                    lv_roller_set_selected(obj, 30, LV_ANIM_OFF);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_16px, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_SELECTED | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // label_alarm_edit
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_alarm_edit = obj;
            lv_obj_set_pos(obj, 40, 40);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "创建闹钟");
        }
        {
            // btn_alarm_confirm
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.btn_alarm_confirm = obj;
            lv_obj_set_pos(obj, 168, 29);
            lv_obj_set_size(obj, 43, 43);
            lv_obj_add_event_cb(obj, action_alarm_confirm_click, LV_EVENT_CLICKED, (void *)0);
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
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    lv_obj_set_pos(obj, 10, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/alarm_confirm.bin");
                }
            }
        }
        {
            // btn_alarm_del
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.btn_alarm_del = obj;
            lv_obj_set_pos(obj, 127, 29);
            lv_obj_set_size(obj, 41, 43);
            lv_obj_add_event_cb(obj, action_alarm_del_click, LV_EVENT_CLICKED, (void *)0);
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
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    lv_obj_set_pos(obj, 16, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/alarm_del.bin");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 89);
            lv_obj_set_size(obj, 166, 62);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // btn_weekday_01
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.btn_weekday_01 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 20, 62);
                    lv_obj_add_event_cb(obj, action_alarm_weekday_01_click, LV_EVENT_CLICKED, (void *)0);
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
                            // label_weekday_01
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.label_weekday_01 = obj;
                            lv_obj_set_pos(obj, 2, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "一");
                        }
                    }
                }
                {
                    // btn_weekday_02
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.btn_weekday_02 = obj;
                    lv_obj_set_pos(obj, 24, 0);
                    lv_obj_set_size(obj, 20, 62);
                    lv_obj_add_event_cb(obj, action_alarm_weekday_02_click, LV_EVENT_CLICKED, (void *)0);
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
                            // label_weekday_02
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.label_weekday_02 = obj;
                            lv_obj_set_pos(obj, 2, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "二");
                        }
                    }
                }
                {
                    // btn_weekday_03
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.btn_weekday_03 = obj;
                    lv_obj_set_pos(obj, 48, 0);
                    lv_obj_set_size(obj, 20, 62);
                    lv_obj_add_event_cb(obj, action_alarm_weekday_03_click, LV_EVENT_CLICKED, (void *)0);
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
                            // label_weekday_03
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.label_weekday_03 = obj;
                            lv_obj_set_pos(obj, 2, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "三");
                        }
                    }
                }
                {
                    // btn_weekday_04
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.btn_weekday_04 = obj;
                    lv_obj_set_pos(obj, 73, 0);
                    lv_obj_set_size(obj, 20, 62);
                    lv_obj_add_event_cb(obj, action_alarm_weekday_04_click, LV_EVENT_CLICKED, (void *)0);
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
                            // label_weekday_04
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.label_weekday_04 = obj;
                            lv_obj_set_pos(obj, 2, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "四");
                        }
                    }
                }
                {
                    // btn_weekday_05
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.btn_weekday_05 = obj;
                    lv_obj_set_pos(obj, 97, 0);
                    lv_obj_set_size(obj, 20, 62);
                    lv_obj_add_event_cb(obj, action_alarm_weekday_05_click, LV_EVENT_CLICKED, (void *)0);
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
                            // label_weekday_05
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.label_weekday_05 = obj;
                            lv_obj_set_pos(obj, 2, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "五");
                        }
                    }
                }
                {
                    // btn_weekday_06
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.btn_weekday_06 = obj;
                    lv_obj_set_pos(obj, 121, 0);
                    lv_obj_set_size(obj, 20, 62);
                    lv_obj_add_event_cb(obj, action_alarm_weekday_06_click, LV_EVENT_CLICKED, (void *)0);
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
                            // label_weekday_06
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.label_weekday_06 = obj;
                            lv_obj_set_pos(obj, 2, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "六");
                        }
                    }
                }
                {
                    // btn_weekday_07
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.btn_weekday_07 = obj;
                    lv_obj_set_pos(obj, 145, 0);
                    lv_obj_set_size(obj, 20, 62);
                    lv_obj_add_event_cb(obj, action_alarm_weekday_07_click, LV_EVENT_CLICKED, (void *)0);
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
                            // label_weekday_07
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.label_weekday_07 = obj;
                            lv_obj_set_pos(obj, 2, 6);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, font_pingfang_12px, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "日");
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_page_alarm_edit();
}

void tick_screen_page_alarm_edit() {
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
            // label_count
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_count = obj;
            lv_obj_set_pos(obj, 0, -7);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "00:00:00");
        }
        {
            // disp_countdown
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.disp_countdown = obj;
            lv_obj_set_pos(obj, 0, 66);
            lv_obj_set_size(obj, 240, 94);
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
                    // roller_hour
                    lv_obj_t *obj = lv_roller_create(parent_obj);
                    objects.roller_hour = obj;
                    lv_obj_set_pos(obj, 26, 0);
                    lv_obj_set_size(obj, 55, 94);
                    lv_obj_add_event_cb(obj, roller_hour_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_scroll_dir(obj, LV_DIR_VER);

                    // main part
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

                    // selected part
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED | LV_STATE_DEFAULT);
                }
                {
                    // roller_min
                    lv_obj_t *obj = lv_roller_create(parent_obj);
                    objects.roller_min = obj;
                    lv_obj_set_pos(obj, 92, 0);
                    lv_obj_set_size(obj, 55, 94);
                    lv_obj_add_event_cb(obj, roller_min_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_scroll_dir(obj, LV_DIR_VER);

                    // main part
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // selected part
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED | LV_STATE_DEFAULT);
                }
                {
                    // roller_sec
                    lv_obj_t *obj = lv_roller_create(parent_obj);
                    objects.roller_sec = obj;
                    lv_obj_set_pos(obj, 158, 0);
                    lv_obj_set_size(obj, 55, 94);
                    lv_obj_add_event_cb(obj, roller_sec_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_scroll_dir(obj, LV_DIR_VER);

                    // main part
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // selected part
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_SELECTED | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED | LV_STATE_DEFAULT);
                }
                {
                    // commoa1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.commoa1 = obj;
                    lv_obj_set_pos(obj, 81, 31);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, ":");
                }
                {
                    // commoa2
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.commoa2 = obj;
                    lv_obj_set_pos(obj, 147, 31);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, ":");
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

            lv_obj_clear_flag(objects.label_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.disp_countdown, LV_OBJ_FLAG_HIDDEN);
        } else if (new_timer_type == TIMER_TYPE_COUNT_DOWN) {
            lv_obj_clear_flag(objects.container_count_down, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(objects.btn_count_down, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.label_count_down, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_add_flag(objects.container_count_up, LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(objects.btn_count_up, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.label_count_up, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_add_flag(objects.label_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.disp_countdown, LV_OBJ_FLAG_HIDDEN);
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

                if (lvgl_var_timer_type_get() == TIMER_TYPE_COUNT_DOWN) {
                    count_down_time = lvgl_var_timer_countdown_hour_get() * 60 * 60 + lvgl_var_timer_countdown_min_get() * 60 + lvgl_var_timer_countdown_sec_get();

                    snprintf(time, 9, "%02ld:%02ld:%02ld", lvgl_var_timer_countdown_hour_get(), lvgl_var_timer_countdown_min_get(), lvgl_var_timer_countdown_sec_get());
                    lv_label_set_text(objects.label_count, time);
                    
                    lv_obj_clear_flag(objects.label_count, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(objects.disp_countdown, LV_OBJ_FLAG_HIDDEN);
                }
            break;

            case TIMER_STATE_PAUSE:
                lv_img_set_src(objects.img_start, "/lvgl/images/timer_start.bin");
            break;

            case TIMER_STATE_IDLE:
                lv_img_set_src(objects.img_start, "/lvgl/images/timer_start.bin");

                if (lvgl_var_timer_type_get() == TIMER_TYPE_COUNT_UP) {
                    lv_label_set_text(objects.label_count, "00:00:00");
                } else {
                    lv_obj_add_flag(objects.label_count, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(objects.disp_countdown, LV_OBJ_FLAG_HIDDEN);
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
                count_down_rest = count_down_time - (cur_hour * 60 * 60 + cur_min * 60 + cur_sec);
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
            {
                // tomato_icon
                lv_obj_t *obj = lv_image_create(parent_obj);
                objects.tomato_icon = obj;
                lv_obj_set_pos(obj, 0, -66);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_image_set_src(obj, "/lvgl/images/tomato.bin");
                lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            {
                // tomato_label
                lv_obj_t *obj = lv_label_create(parent_obj);
                objects.tomato_label = obj;
                lv_obj_set_pos(obj, 0, -26);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_label_set_text(obj, "番茄时钟");
                lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            {
                // tomato_count
                lv_obj_t *obj = lv_label_create(parent_obj);
                objects.tomato_count = obj;
                lv_obj_set_pos(obj, 0, 12);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(obj, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(obj, "25:00");
            }
            {
                // container_tomato_opt
                lv_obj_t *obj = lv_obj_create(parent_obj);
                objects.container_tomato_opt = obj;
                lv_obj_set_pos(obj, 50, 150);
                lv_obj_set_size(obj, 70, 60);
                lv_obj_add_event_cb(obj, action_tomato_opt_click, LV_EVENT_CLICKED, (void *)0);
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
                        lv_obj_set_pos(obj, 26, 12);
                        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                        lv_image_set_src(obj, "/lvgl/images/timer_start.bin");
                    }
                }
            }
            {
                // container_tomato_stop
                lv_obj_t *obj = lv_obj_create(parent_obj);
                objects.container_tomato_stop = obj;
                lv_obj_set_pos(obj, 120, 150);
                lv_obj_set_size(obj, 70, 60);
                lv_obj_add_event_cb(obj, action_tomato_stop_click, LV_EVENT_CLICKED, (void *)0);
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
                        // tomato_stop
                        lv_obj_t *obj = lv_image_create(parent_obj);
                        objects.tomato_stop = obj;
                        lv_obj_set_pos(obj, 8, 12);
                        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                        lv_image_set_src(obj, "/lvgl/images/timer_stop.bin");
                    }
                }
            }
        }
    }

    tick_screen_page_pomodoro();
}

void tick_screen_page_pomodoro() {
    static uint32_t tomato_counter = 0;
    static uint32_t cur_counter = 0;
    char time[6];

    static enum TomatoState last_tomato_state = TOMATO_STATE_NONE;
    enum TomatoState new_tomato_state = lvgl_var_tomato_state_get();

    static uint32_t last_time = 0;

    if (new_tomato_state != last_tomato_state) {
        last_tomato_state = new_tomato_state;

        switch (new_tomato_state) {
            case TOMATO_STATE_RUNNING:
                tomato_counter = lvgl_var_tomato_focus_get();

                lv_label_set_text(objects.tomato_label, "专注中");
                lv_image_set_src(objects.tomato_opt, "/lvgl/images/timer_pause.bin");
                last_time = qm_now_ms();

                qm_snprintf(time, 6, "%02d:%02d", lvgl_var_tomato_focus_get() / 60, lvgl_var_tomato_focus_get() % 60);
                lv_label_set_text(objects.tomato_count, time);
            break;

            case TOMATO_STATE_BREAK:
                tomato_counter = lvgl_var_tomato_break_get();

                lv_label_set_text(objects.tomato_label, "休息中");
                lv_image_set_src(objects.tomato_opt, "/lvgl/images/timer_pause.bin");
                last_time = qm_now_ms();

                qm_snprintf(time, 6, "%02d:%02d", lvgl_var_tomato_break_get() / 60, lvgl_var_tomato_break_get() % 60);
                lv_label_set_text(objects.tomato_count, time);
            break;

            case TOMATO_STATE_RUNNING_PAUSE:
            case TOMATO_STATE_BREAK_PAUSE:
                lv_image_set_src(objects.tomato_opt, "/lvgl/images/timer_start.bin");
            break;

            case TOMATO_STATE_IDLE:
                lv_label_set_text(objects.tomato_label, "番茄时钟");
                lv_image_set_src(objects.tomato_opt, "/lvgl/images/timer_start.bin");

                qm_snprintf(time, 6, "%02d:%02d", lvgl_var_tomato_focus_get() / 60, lvgl_var_tomato_focus_get() % 60);
                lv_label_set_text(objects.tomato_count, time);

                cur_counter = 0;
            break;

            default:
            break;
        }
    }

    if (new_tomato_state == TOMATO_STATE_RUNNING || new_tomato_state == TOMATO_STATE_BREAK) {
        if (qm_now_ms() - last_time > 1000) {
            last_time = qm_now_ms();

            if (cur_counter == tomato_counter + 1) {
                if (new_tomato_state == TOMATO_STATE_RUNNING) {
                    lvgl_var_tomato_state_set(TOMATO_STATE_BREAK);
                } else {
                    lvgl_var_tomato_state_set(TOMATO_STATE_RUNNING);
                }

                cur_counter = 0;

                return;
            }

            uint8_t cur_min = (tomato_counter - cur_counter) / 60;
            uint8_t cur_sec = (tomato_counter - cur_counter) % 60;

            qm_snprintf(time, 6, "%02d:%02d", cur_min, cur_sec);
            lv_label_set_text(objects.tomato_count, time);

            cur_counter++;
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

void tick_screen_page_weather() {
    static int8_t weather_id_last = -1;
    static int8_t temperature_last = -128; 
    int8_t weather_id = lvgl_var_weather_id_get();
    if (weather_id > QM_IOT_WEATHER_TYPE_HAZE_NIGHT) {
        weather_id = QM_IOT_WEATHER_TYPE_HAZE_NIGHT;
    }

    // 更新 天气图标 / 天气
    if (weather_id != -1) {
        if (weather_id_last != weather_id) {
            weather_id_last = weather_id;
            char s[64];
            sprintf(s, "/lvgl/images/w%d.bin", weather_id);
            lv_image_set_src(objects.img_weather, s);
            lv_label_set_text(objects.label_weather, weather_name_tbl[weather_id]);
        }
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
    tick_screen_page_alarm_edit,
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
    if (font_misans_16px == NULL) {
        font_misans_16px = (lv_font_t *)&ui_font_misans_16px;
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
    if (font_pingfang_16px == NULL) {
        font_pingfang_16px = (lv_font_t *)&ui_font_pingfang_sc_regular_16px; // lv_binfont_create("/lvgl/fonts/pingfang_20px.bin");
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
    create_screen_page_alarm_edit();

    page_alarm_alarm_list_init();
    page_timer_countdown_roller_init();
    page_alarm_edit_init();
}

/*
    custom static functions
    func name: page_* + obj_opt
*/ 
static void page_alarm_alarm_list_init(void)
{
    int32_t alarm_list_count = 0;
    alarm_manager_get_count(&alarm_list_count);
    alarm_t *alarm_list = malloc(sizeof(alarm_t) * alarm_list_count);
    alarm_manager_get_alarm_list(alarm_list);

    for (int32_t i = 0; i < alarm_list_count; i++) {
        char time[10];
        sprintf(time, "%02d:%02d", alarm_list[i].hour, alarm_list[i].min);

        char repeat[50];

        if (alarm_list[i].repeat_mask == 0) {
            memcpy(repeat, "One-time\0", 9);
        } else {
            uint8_t repeat_mask = alarm_list[i].repeat_mask;
            sprintf(repeat, "%s%s%s%s%s%s%s",
            repeat_mask & (1 << 0) ? "Mon, " : "",
            repeat_mask & (1 << 1) ? "Tue, " : "",
            repeat_mask & (1 << 2) ? "Wed, " : "",
            repeat_mask & (1 << 3) ? "Thu, " : "",
            repeat_mask & (1 << 4) ? "Fri, " : "",
            repeat_mask & (1 << 5) ? "Sat, " : "",
            repeat_mask & (1 << 6) ? "Sun, " : "");

            // cut last , 
            repeat[strlen(repeat) - 2] = '\0';
        }

        scr_add_alarm_item(time, repeat);
    }

    free(alarm_list);

    lv_alarm_count = alarm_list_count;
}

static void page_alarm_alarm_list_update(void)
{
    for (int i = 0; i < lv_alarm_count; i++) {
        lv_obj_del(lv_alarms[i].root);
    }

    lv_alarm_count = 0;

    page_alarm_alarm_list_init();

    lv_obj_scroll_to_y(objects.list_alarm, 0, LV_ANIM_OFF);
}

static void page_timer_countdown_roller_init(void)
{
    // add roller options
    char hour_opts[100], minute_opts[200], second_opts[200];

    for (int i = 0; i <= COUNTDOWN_LIMIT_HOUR; i++) {
        if (i == COUNTDOWN_LIMIT_HOUR) {
            sprintf(hour_opts + i * 3, "%02d", i);
        } else {
            sprintf(hour_opts + i * 3, "%02d\n", i);
        }
    }
    lv_roller_set_options(objects.roller_hour, hour_opts, LV_ROLLER_MODE_INFINITE);

    for (int i = 0; i <= COUNTDOWN_LIMIT_MINUTE; i++) {
        if (i == COUNTDOWN_LIMIT_MINUTE) {
            sprintf(minute_opts + i * 3, "%02d", i);
        } else {
            sprintf(minute_opts + i * 3, "%02d\n", i);
        }
    }
    lv_roller_set_options(objects.roller_min, minute_opts, LV_ROLLER_MODE_INFINITE);

    for (int i = 0; i <= COUNTDOWN_LIMIT_SECOND; i++) {
        if (i == COUNTDOWN_LIMIT_SECOND) {
            sprintf(second_opts + i * 3, "%02d", i);
        } else {
            sprintf(second_opts + i * 3, "%02d\n", i);
        }
    }
    lv_roller_set_options(objects.roller_sec, second_opts, LV_ROLLER_MODE_INFINITE);

    // set roller value
    lv_roller_set_selected(objects.roller_hour, lvgl_var_timer_countdown_hour_get(), LV_ANIM_OFF);
    lv_roller_set_selected(objects.roller_min, lvgl_var_timer_countdown_min_get(), LV_ANIM_OFF);
    lv_roller_set_selected(objects.roller_sec, lvgl_var_timer_countdown_sec_get(), LV_ANIM_OFF);
}

static void set_weekday_check_state(int8_t weekday_index, uint8_t checked)
{
    lv_obj_t *obj = lv_alarm_edit_handle.alarm_weekday[weekday_index].root;

    if (checked) {
        lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff1b100), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_alarm_edit_handle.alarm_weekday[weekday_index].checked = checked;
}

static void page_alarm_edit_init(void)
{
    lv_alarm_edit_handle.alarm_weekday[0].root = objects.label_weekday_01;
    lv_alarm_edit_handle.alarm_weekday[1].root = objects.label_weekday_02;
    lv_alarm_edit_handle.alarm_weekday[2].root = objects.label_weekday_03;
    lv_alarm_edit_handle.alarm_weekday[3].root = objects.label_weekday_04;
    lv_alarm_edit_handle.alarm_weekday[4].root = objects.label_weekday_05;
    lv_alarm_edit_handle.alarm_weekday[5].root = objects.label_weekday_06;
    lv_alarm_edit_handle.alarm_weekday[6].root = objects.label_weekday_07;

    // set weekday check state
    set_weekday_check_state(0, 0);
    set_weekday_check_state(1, 0);
    set_weekday_check_state(2, 0);
    set_weekday_check_state(3, 0);
    set_weekday_check_state(4, 0);
    set_weekday_check_state(5, 0);
    set_weekday_check_state(6, 0);

    // add roller options
    char hour_opts[500], minute_opts[500];

    for (int i = 0; i <= COUNTDOWN_LIMIT_HOUR; i++) {
        if (i == COUNTDOWN_LIMIT_HOUR) {
            sprintf(hour_opts + i * 7, "    %02d", i);
        } else {
            sprintf(hour_opts + i * 7, "    %02d\n", i);
        }
    }
    lv_roller_set_options(objects.roller_alarm_hour, hour_opts, LV_ROLLER_MODE_INFINITE);

    for (int i = 0; i <= COUNTDOWN_LIMIT_MINUTE; i++) {
        if (i == COUNTDOWN_LIMIT_MINUTE) {
            sprintf(minute_opts + i * 7, "%02d    ", i);
        } else {
            sprintf(minute_opts + i * 7, "%02d    \n", i);
        }
    }
    lv_roller_set_options(objects.roller_alarm_min, minute_opts, LV_ROLLER_MODE_INFINITE);

    // set roller value
    lv_roller_set_selected(objects.roller_alarm_hour, 8, LV_ANIM_OFF);
    lv_roller_set_selected(objects.roller_alarm_min, 0, LV_ANIM_OFF);
}

// custom ext functions
int32_t scr_add_alarm_item(const char *time, const char *repeat)
{
    alarm_item_t item;

    if (lv_alarm_count >= ALARM_LIST_MAX_COUNT) {
        return -1;
    }

    lv_obj_t *obj = lv_obj_create(objects.list_alarm);

    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 175, 60);
    lv_obj_add_event_cb(obj, action_alarm_item_long_press, LV_EVENT_LONG_PRESSED, (void*)lv_alarm_count);
    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT); 
    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT); 
    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT); 
    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5d5d5d), 0);
    lv_obj_set_style_radius(obj, 5, 0);
    lv_obj_set_style_bg_opa(obj, 150, 0);

    // 时间
    lv_obj_t *label_time = lv_label_create(obj);
    lv_obj_set_pos(label_time, 7, 15);  // 这里可以保留（相对父）
    lv_obj_set_size(label_time, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(label_time, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_time, font_misans_24px, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(label_time, time);

    // 开关
    lv_obj_t *sw = lv_switch_create(obj);
    lv_obj_set_pos(sw, 125, 20);
    lv_obj_set_size(sw, 40, 20);
    lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xfff3b200), LV_PART_INDICATOR | LV_STATE_CHECKED);

    // repeat
    lv_obj_t *label_repeat = lv_label_create(obj);
    lv_obj_set_pos(label_repeat, 10, 43);
    lv_obj_set_size(label_repeat, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(label_repeat, lv_color_hex(0xfff3b200), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_repeat, font_misans_8px, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(label_repeat, repeat);

    item.root = obj;
    item.label_time = label_time;
    item.label_repeat = label_repeat;
    item.sw = sw;

    lv_alarms[lv_alarm_count] = item;
    lv_alarm_count++;

    return 0;
}

int32_t scr_del_alarm_item(int32_t index)
{
    return 0;
}

int32_t scr_enter_page_alarm_add(void)
{
    lv_label_set_text(objects.label_alarm_edit, "创建闹钟");

    lv_alarm_edit_handle.opt_type = 1;

    set_weekday_check_state(0, 0);
    set_weekday_check_state(1, 0);
    set_weekday_check_state(2, 0);
    set_weekday_check_state(3, 0);
    set_weekday_check_state(4, 0);
    set_weekday_check_state(5, 0);
    set_weekday_check_state(6, 0);

    // current time
    qm_time_t t = qm_time(NULL);
    struct qm_tm *tm = qm_localtime(&t);

    lv_roller_set_selected(objects.roller_alarm_hour, tm->tm_hour, LV_ANIM_OFF);
    lv_roller_set_selected(objects.roller_alarm_min, tm->tm_min, LV_ANIM_OFF);

    return 0;
}

int32_t scr_enter_page_alarm_edit(int32_t alarm_index)
{
    lv_label_set_text(objects.label_alarm_edit, "编辑闹钟");

    lv_alarm_edit_handle.opt_type = 2;
    lv_alarm_edit_handle.alarm_index = alarm_index;

    alarm_t alarm = {0};
    alarm_manager_get_alarm(&alarm, alarm_index);

    lv_roller_set_selected(objects.roller_alarm_hour, alarm.hour, LV_ANIM_OFF);
    lv_roller_set_selected(objects.roller_alarm_min, alarm.min, LV_ANIM_OFF);

    // set weekday check state
    set_weekday_check_state(0, alarm.repeat_mask & (1 << 0));
    set_weekday_check_state(1, alarm.repeat_mask & (1 << 1));
    set_weekday_check_state(2, alarm.repeat_mask & (1 << 2));
    set_weekday_check_state(3, alarm.repeat_mask & (1 << 3));
    set_weekday_check_state(4, alarm.repeat_mask & (1 << 4));
    set_weekday_check_state(5, alarm.repeat_mask & (1 << 5));
    set_weekday_check_state(6, alarm.repeat_mask & (1 << 6));

    return 0;
}

int32_t scr_weekday_check_state_toggle(int8_t weekday_index)
{
    set_weekday_check_state(weekday_index, !lv_alarm_edit_handle.alarm_weekday[weekday_index].checked);

    return 0;
}

int32_t scr_alarm_edit_confirm(void)
{
    if (lv_alarm_edit_handle.opt_type != 1 && lv_alarm_edit_handle.opt_type != 2) {
        return -1;
    }

    lv_alarm_edit_handle.hour = lv_roller_get_selected(objects.roller_alarm_hour);
    lv_alarm_edit_handle.min = lv_roller_get_selected(objects.roller_alarm_min);

    uint8_t repeat_mask = 0;
    for (int8_t i = 0; i < 7; i++) {
        if (lv_alarm_edit_handle.alarm_weekday[i].checked) {
            repeat_mask |= (1 << i);
        }
    }

    alarm_t alarm = {
        .hour = lv_alarm_edit_handle.hour,
        .min = lv_alarm_edit_handle.min,
        .repeat_mask = repeat_mask,
        .enable = 1,
    };

    if (lv_alarm_edit_handle.opt_type == 1) {
        alarm_manager_add_alarm(alarm);
    } else {
        alarm_manager_mod_alarm(alarm, lv_alarm_edit_handle.alarm_index);
    }

    page_alarm_alarm_list_update();

    return 0;
}

int32_t scr_alarm_edit_exit(void)
{
    if (lv_alarm_edit_handle.opt_type != 1 && lv_alarm_edit_handle.opt_type != 2) {
        return -1;
    }

    if (lv_alarm_edit_handle.opt_type == 1) {
        // ignore
    } else {
        // delete alarm
        alarm_manager_del_alarm(lv_alarm_edit_handle.alarm_index);
    }

    page_alarm_alarm_list_update();

    return 0;
}

