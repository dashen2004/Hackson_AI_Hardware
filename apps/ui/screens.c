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
#include "settings_images.h"

#include "qm_iot_weather.h"
#include "qm_time.h"

#include "bsp_lcd.h"
#include "alarm_manager.h"
#include "widgets/roller/lv_roller.h"
#include "widgets/slider/lv_slider.h"

#include "ai_main.h"
#include "qm_ai_api.h"
#include "bsp_sensor_light.h"
#include "bsp_camera.h"
#include "esp_log.h"
#include "xjiang_cloud.h"

#define LOG_TAG "ui_study"

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

static void page_alarm_alarm_list_init(void);
static void page_timer_countdown_roller_init(void);
static void page_alarm_edit_init(void);

static lv_obj_t *g_page_dots[3][3];

static void ui_style_clear_decor(lv_obj_t *obj)
{
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_PRESSED);
}

static void ui_style_apps_page_bg(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a1845), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff120820), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void ui_create_page_indicator(lv_obj_t *parent, int page_slot)
{
    const int dot_x[3] = {108, 118, 128};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *dot = lv_obj_create(parent);
        g_page_dots[page_slot][i] = dot;
        lv_obj_set_pos(dot, dot_x[i], 218);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    }
}

void ui_update_page_indicator(int active_index)
{
    for (int p = 0; p < 3; p++) {
        for (int d = 0; d < 3; d++) {
            if (g_page_dots[p][d] == NULL) {
                continue;
            }
            lv_color_t color = (d == active_index) ? lv_color_hex(0xffd7a018) : lv_color_hex(0xff444444);
            lv_obj_set_style_bg_color(g_page_dots[p][d], color, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

static void ui_create_status_bar(lv_obj_t *parent)
{
    lv_obj_t *wifi = lv_image_create(parent);
    lv_obj_set_pos(wifi, 91, 23);
    lv_image_set_src(wifi, "/lvgl/images/wifi.bin");

    lv_obj_t *bat = lv_image_create(parent);
    lv_obj_set_pos(bat, 130, 25);
    lv_image_set_src(bat, "/lvgl/images/bat.bin");
}

static lv_obj_t *ui_create_app_tile(lv_obj_t *parent, int x, int y, const char *icon_path,
                                    const char *label_text, lv_event_cb_t cb)
{
    lv_obj_t *img = lv_image_create(parent);
    lv_obj_set_pos(img, x, y);
    lv_image_set_src(img, icon_path);
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(img, cb, LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *label = lv_label_create(parent);
        lv_label_set_text(label, label_text);
        lv_obj_set_width(label, 72);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, font_pingfang_14px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    }
    return img;
}

#define APP_ICON_W         64
#define APP_ICON_GAP_X     20
#define APP_GRID_OFFSET_X  8
#define APP_ROW1_Y         52
#define APP_ROW2_Y         138
#define APP_GRID_X0        ((240 - (APP_ICON_W * 2 + APP_ICON_GAP_X)) / 2 + APP_GRID_OFFSET_X)
#define APP_COL2_X         (APP_GRID_X0 + APP_ICON_W + APP_ICON_GAP_X)
#define APP_COL_CENTER_X   ((240 - APP_ICON_W) / 2 + APP_GRID_OFFSET_X)

static lv_obj_t *ui_create_placeholder_page(const char *title)
{
    lv_obj_t *obj = lv_obj_create(0);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *label = lv_label_create(obj);
        lv_label_set_text(label, title);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 10);
    }
    return obj;
}

#define UI_LIST_ICON_Y      36
#define UI_LIST_AREA_Y      78
#define UI_LIST_ROW_H       41
#define UI_LIST_VISIBLE_CNT 3
#define UI_LIST_AREA_H      (UI_LIST_ROW_H * UI_LIST_VISIBLE_CNT)
#define UI_LIST_TEXT_PAD_Y  11
#define UI_LIST_LEFT_X      50
#define UI_LIST_RIGHT_X     114
#define UI_LIST_RIGHT_W     76
#define UI_LIST_DIVIDER_X   42
#define UI_LIST_DIVIDER_W   156
#define UI_COLOR_ORANGE     0xfff1b100
#define UI_COLOR_DIVIDER    0xff2e2e2e
#define UI_COLOR_GREY       0xff999999

#define UI_TIMER_ICON_Y     28
#define UI_TIMER_COUNT_Y    -20
#define UI_TIMER_HINT_Y     20
#define UI_TIMER_BTN_Y      150
#define UI_TIMER_BTN_OPT_X  50
#define UI_TIMER_BTN_STOP_X 120

static void ui_add_list_divider(lv_obj_t *parent, int y)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_pos(line, UI_LIST_DIVIDER_X, y);
    lv_obj_set_size(line, UI_LIST_DIVIDER_W, 1);
    ui_style_clear_decor(line);
    lv_obj_set_style_bg_color(line, lv_color_hex(UI_COLOR_DIVIDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static lv_obj_t *ui_create_list_page(const char *icon_path)
{
    lv_obj_t *obj = lv_obj_create(0);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *img = lv_image_create(obj);
        lv_obj_set_pos(img, 0, UI_LIST_ICON_Y);
        lv_obj_set_style_align(img, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_image_set_src(img, icon_path);
    }
    return obj;
}

static lv_obj_t *ui_create_list_scroll(lv_obj_t *page)
{
    lv_obj_t *list = lv_obj_create(page);
    lv_obj_set_pos(list, 0, UI_LIST_AREA_Y);
    lv_obj_set_size(list, 240, UI_LIST_AREA_H);
    ui_style_clear_decor(list);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    return list;
}

static void ui_add_list_row(lv_obj_t *parent, int y, const char *left, const char *right,
                            lv_color_t left_color, lv_color_t right_color)
{
    int text_y = y + UI_LIST_TEXT_PAD_Y;

    lv_obj_t *left_lbl = lv_label_create(parent);
    lv_label_set_text(left_lbl, left);
    lv_obj_set_pos(left_lbl, UI_LIST_LEFT_X, text_y);
    lv_obj_set_style_text_color(left_lbl, left_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(left_lbl, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *right_lbl = lv_label_create(parent);
    lv_label_set_text(right_lbl, right);
    lv_obj_set_pos(right_lbl, UI_LIST_RIGHT_X, text_y);
    lv_obj_set_width(right_lbl, UI_LIST_RIGHT_W);
    lv_obj_set_style_text_align(right_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(right_lbl, right_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(right_lbl, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_add_list_divider(parent, y + UI_LIST_ROW_H - 1);
}

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
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, "/lvgl/images/icon_main_s.bin");
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
        ui_create_page_indicator(obj, 0);
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

void create_screen_page_apps1() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_apps1 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    ui_style_apps_page_bg(obj);
    ui_create_status_bar(obj);
    ui_create_app_tile(obj, APP_GRID_X0, APP_ROW1_Y, "/lvgl/images/icon_ybjc.bin", "眼保健操", action_open_eye_click);
    ui_create_app_tile(obj, APP_COL2_X, APP_ROW1_Y, "/lvgl/images/icon_tomato.bin", "番茄钟", action_open_pomodoro_click);
    ui_create_app_tile(obj, APP_GRID_X0, APP_ROW2_Y, "/lvgl/images/icon_alarm.bin", "闹钟", action_open_alarm_click);
    ui_create_app_tile(obj, APP_COL2_X, APP_ROW2_Y, "/lvgl/images/icon_timer.bin", "定时器", action_open_timer_click);
    ui_create_page_indicator(obj, 1);
    tick_screen_page_apps1();
}

void tick_screen_page_apps1() {
}

void create_screen_page_apps2() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_apps2 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    ui_style_apps_page_bg(obj);
    ui_create_status_bar(obj);
    ui_create_app_tile(obj, APP_GRID_X0, APP_ROW1_Y, "/lvgl/images/icon_timetable.bin", "课程表", action_open_timetable_click);
    ui_create_app_tile(obj, APP_COL2_X, APP_ROW1_Y, "/lvgl/images/icon_checklist.bin", "打卡列表", action_open_checkin_click);
    ui_create_app_tile(obj, APP_COL_CENTER_X, APP_ROW2_Y, "/lvgl/images/icon_settings.bin", "系统设置", action_open_settings_click);
    ui_create_page_indicator(obj, 2);
    {
        lv_obj_t *btn = lv_obj_create(obj);
        objects.btn_study_entry = btn;
        lv_obj_set_pos(btn, APP_GRID_X0, APP_ROW2_Y);
        lv_obj_set_size(btn, APP_ICON_W, APP_ICON_W + 20);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(btn, action_open_study_click, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    tick_screen_page_apps2();
}

void tick_screen_page_apps2() {
}

void create_screen_page_eye() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_eye = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.eye_icon = obj;
            lv_obj_set_pos(obj, 0, UI_TIMER_ICON_Y);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, "/lvgl/images/icon_inner_ybjc.bin");
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.eye_count = obj;
            lv_obj_set_pos(obj, 0, UI_TIMER_COUNT_Y);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "05:00");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.eye_hint = obj;
            lv_obj_set_pos(obj, 0, UI_TIMER_HINT_Y);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "剩余时间");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(UI_COLOR_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_eye_opt = obj;
            lv_obj_set_pos(obj, UI_TIMER_BTN_OPT_X, UI_TIMER_BTN_Y);
            lv_obj_set_size(obj, 70, 60);
            lv_obj_add_event_cb(obj, action_eye_opt_click, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *obj = lv_image_create(objects.container_eye_opt);
                objects.eye_opt = obj;
                lv_obj_set_pos(obj, 26, 12);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_image_set_src(obj, "/lvgl/images/timer_start.bin");
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_eye_stop = obj;
            lv_obj_set_pos(obj, UI_TIMER_BTN_STOP_X, UI_TIMER_BTN_Y);
            lv_obj_set_size(obj, 70, 60);
            lv_obj_add_event_cb(obj, action_eye_stop_click, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *obj = lv_image_create(objects.container_eye_stop);
                objects.eye_stop = obj;
                lv_obj_set_pos(obj, 8, 12);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_image_set_src(obj, "/lvgl/images/timer_stop.bin");
            }
        }
    }

    tick_screen_page_eye();
}

void tick_screen_page_eye() {
    static uint32_t eye_counter = 0;
    static uint32_t cur_counter = 0;
    char time[6];

    static enum EyeState last_eye_state = EYE_STATE_NONE;
    enum EyeState new_eye_state = lvgl_var_eye_state_get();
    enum EyeState prev_eye_state = last_eye_state;

    static uint32_t last_time = 0;

    if (new_eye_state != last_eye_state) {
        last_eye_state = new_eye_state;

        switch (new_eye_state) {
            case EYE_STATE_RUNNING:
                if (prev_eye_state == EYE_STATE_IDLE) {
                    eye_counter = lvgl_var_eye_duration_get();
                    cur_counter = 0;
                }
                lv_image_set_src(objects.eye_opt, "/lvgl/images/timer_pause.bin");
                last_time = qm_now_ms();

                qm_snprintf(time, 6, "%02d:%02d",
                            (eye_counter - cur_counter) / 60,
                            (eye_counter - cur_counter) % 60);
                lv_label_set_text(objects.eye_count, time);
            break;

            case EYE_STATE_PAUSE:
                lv_image_set_src(objects.eye_opt, "/lvgl/images/timer_start.bin");
            break;

            case EYE_STATE_IDLE:
                lv_image_set_src(objects.eye_opt, "/lvgl/images/timer_start.bin");

                qm_snprintf(time, 6, "%02d:%02d",
                            lvgl_var_eye_duration_get() / 60,
                            lvgl_var_eye_duration_get() % 60);
                lv_label_set_text(objects.eye_count, time);

                cur_counter = 0;
            break;

            default:
            break;
        }
    }

    if (new_eye_state == EYE_STATE_RUNNING) {
        if (qm_now_ms() - last_time > 1000) {
            last_time = qm_now_ms();

            if (cur_counter == eye_counter + 1) {
                lvgl_var_eye_state_set(EYE_STATE_IDLE);
                cur_counter = 0;
                return;
            }

            qm_snprintf(time, 6, "%02d:%02d",
                        (eye_counter - cur_counter) / 60,
                        (eye_counter - cur_counter) % 60);
            lv_label_set_text(objects.eye_count, time);

            cur_counter++;
        }
    }
}

void create_screen_page_timetable() {
    static const struct {
        const char *time;
        const char *subject;
    } rows[] = {
        { "08:00", "语文" },
        { "09:00", "数学" },
        { "10:00", "英语" },
        { "11:00", "午休" },
        { "13:00", "美术" },
        { "14:00", "品德" },
        { "15:00", "体育" },
    };

    lv_obj_t *obj = ui_create_list_page("/lvgl/images/icon_inner_timetable.bin");
    objects.page_timetable = obj;
    lv_obj_t *list = ui_create_list_scroll(obj);

    for (uint8_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        ui_add_list_row(list,
                        i * UI_LIST_ROW_H,
                        rows[i].time,
                        rows[i].subject,
                        lv_color_hex(UI_COLOR_ORANGE),
                        lv_color_hex(0xffffffff));
    }

    tick_screen_page_timetable();
}

void tick_screen_page_timetable() {
}

void create_screen_page_checkin() {
    static const struct {
        const char *name;
        const char *status;
        uint8_t done;
    } rows[] = {
        { "晨间唤醒", "已完成", 1 },
        { "眼保健操", "未完成", 0 },
        { "居家学习", "未完成", 0 },
        { "户外活动", "未完成", 0 },
        { "睡前准备", "未完成", 0 },
    };

    lv_obj_t *obj = ui_create_list_page("/lvgl/images/icon_inner_checklist.bin");
    objects.page_checkin = obj;
    lv_obj_t *list = ui_create_list_scroll(obj);

    for (uint8_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        lv_color_t status_color = rows[i].done ? lv_color_hex(UI_COLOR_ORANGE) : lv_color_hex(UI_COLOR_GREY);
        ui_add_list_row(list,
                        i * UI_LIST_ROW_H,
                        rows[i].name,
                        rows[i].status,
                        lv_color_hex(0xffffffff),
                        status_color);
    }

    tick_screen_page_checkin();
}

void tick_screen_page_checkin() {
}

#define STUDY_DURATION_SEC        (20 * 60)
#define UI_COLOR_STUDY_GREEN      0xff33cc66
#define UI_STUDY_ARC_X            18
#define UI_STUDY_ARC_Y            72
#define UI_STUDY_ARC_SIZE         72
#define UI_STUDY_TEXT_X           102
#define UI_STUDY_TEXT_Y           72
#define UI_STUDY_BTN_Y            164
#define UI_STUDY_BTN_PAUSE_X      28
#define UI_STUDY_BTN_SKIP_X       128
#define UI_STUDY_BTN_W            84
#define UI_STUDY_BTN_H            36
#define UI_STUDY_BADGE_Y          38
#define UI_COLOR_STUDY_RED        0xffe53935
#define UI_STUDY_HINT_ICON_Y      36
#define UI_STUDY_HINT_TITLE_Y      8
#define UI_STUDY_HINT_DESC_Y       40
#define STUDY_LIGHT_DETECT_TOTAL_MS     (30 * 1000)
#define STUDY_LIGHT_DETECT_WINDOW_SEC   5
#define STUDY_LIGHT_DETECT_PAIR_WINDOWS 2
#define STUDY_LIGHT_SUCCESS_SHOW_MS     3000
#define STUDY_CAPTURE_INTERVAL_MS       5000
#define STUDY_POSE_BAD_CONSEC_FRAMES      2
#define STUDY_POSE_NOBODY_CONSEC_FRAMES   3
#define STUDY_POSE_HINT_SHOW_MS         5000
#define STUDY_POSE_HINT_WINDOW_MS       (5 * 60 * 1000)
#define STUDY_POSE_HINT_MAX_PER_WINDOW   3

typedef enum {
    STUDY_DETECT_RESULT_NONE = 0,
    STUDY_DETECT_RESULT_SUCCESS,
    STUDY_DETECT_RESULT_FAIL,
} study_detect_result_t;
#define UI_STUDY_DETECT_ICON_Y        48
#define UI_STUDY_DETECT_TITLE_Y       24
#define UI_STUDY_DETECT_SUCC_TITLE_Y  8
#define UI_STUDY_DETECT_SUBTITLE_Y    32
#define UI_STUDY_DETECT_SCORE_Y       56
#define UI_STUDY_DETECT_TITLE_BOUNCE_PX 8
#define UI_STUDY_DETECT_TITLE_ANIM_MS   450
#define UI_COLOR_STUDY_HINT_BLUE      0xff5eb8e8
#define UI_STUDY_RESULT_ICON_Y        28
#define UI_STUDY_RESULT_ICON_TITLE_GAP 10
#define UI_STUDY_RESULT_TEXT_GAP         8
#define UI_STUDY_RESULT_BTN_W        120
#define UI_STUDY_RESULT_BTN_H         36
#define UI_STUDY_RESULT_BTN_BOTTOM_PAD 24

typedef enum {
    STUDY_STATE_IDLE = 0,
    STUDY_STATE_RUNNING,
    STUDY_STATE_PAUSE,
} study_state_t;

typedef enum {
    STUDY_LIGHT_HINT_NONE = 0,
    STUDY_LIGHT_HINT_HIGH,
    STUDY_LIGHT_HINT_LOW,
    STUDY_LIGHT_HINT_POSE_WRONG,
    STUDY_LIGHT_HINT_CAMERA_BLOCK,
} study_light_hint_type_t;

typedef enum {
    LIGHT_WINDOW_INVALID = 0,
    LIGHT_WINDOW_OK,
    LIGHT_WINDOW_HIGH,
    LIGHT_WINDOW_LOW,
} light_window_result_t;

static study_state_t s_study_state = STUDY_STATE_IDLE;
static uint32_t s_study_remaining = STUDY_DURATION_SEC;
static uint32_t s_study_last_tick = 0;

static bool s_light_detect_active = false;
static study_detect_result_t s_light_detect_result = STUDY_DETECT_RESULT_NONE;
static uint32_t s_light_detect_start_ms = 0;
static uint32_t s_light_detect_last_sample_ms = 0;
static uint32_t s_light_detect_result_ms = 0;
static float s_light_window_buf[STUDY_LIGHT_DETECT_WINDOW_SEC];
static uint8_t s_light_window_samples = 0;
static light_window_result_t s_pair_results[STUDY_LIGHT_DETECT_PAIR_WINDOWS];
static uint8_t s_pair_result_idx = 0;
static study_light_hint_type_t s_light_detect_hint = STUDY_LIGHT_HINT_NONE;

static bool s_study_capture_active = false;
static uint32_t s_study_capture_seq = 0;
static uint32_t s_study_capture_last_ms = 0;

static uint8_t s_pose_bad_consec = 0;
static uint8_t s_pose_nobody_consec = 0;
static bool s_study_pose_hint_showing = false;
static uint32_t s_study_pose_hint_until_ms = 0;
static uint32_t s_pose_bad_hint_window_ms = 0;
static uint8_t s_pose_bad_hint_count = 0;
static uint32_t s_pose_block_hint_window_ms = 0;
static uint8_t s_pose_block_hint_count = 0;

static void scr_study_show_result(bool success);
static void scr_study_capture_start(void);
static void scr_study_capture_tick(void);
static void scr_study_session_tick(void);
static void study_hint_set_content(study_light_hint_type_t type);
static void study_pose_reset_counters(void);
static bool study_pose_hint_rate_ok(uint32_t *window_ms, uint8_t *count);
static void study_pose_hint_count_inc(uint32_t *window_ms, uint8_t *count);
static void study_pose_show_hint(study_light_hint_type_t type);

static void study_result_layout_refresh(void)
{
    if (objects.study_result_title == NULL
            || objects.study_result_subtitle == NULL
            || objects.study_result_extra == NULL) {
        return;
    }

    lv_obj_align_to(objects.study_result_subtitle, objects.study_result_title,
                    LV_ALIGN_OUT_BOTTOM_MID, 0, UI_STUDY_RESULT_TEXT_GAP);
    lv_obj_align_to(objects.study_result_extra, objects.study_result_subtitle,
                    LV_ALIGN_OUT_BOTTOM_MID, 0, UI_STUDY_RESULT_TEXT_GAP);
}

static void study_detect_title_y_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_translate_y((lv_obj_t *)var, v, LV_PART_MAIN);
}

static void study_light_detect_title_anim_stop(void)
{
    if (objects.study_light_detect_title == NULL) {
        return;
    }

    lv_anim_delete(objects.study_light_detect_title, study_detect_title_y_anim_cb);
    lv_obj_set_style_translate_y(objects.study_light_detect_title, 0, LV_PART_MAIN);
}

static void study_light_detect_title_anim_start(void)
{
    lv_obj_t *label = objects.study_light_detect_title;

    if (label == NULL) {
        return;
    }

    study_light_detect_title_anim_stop();

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, label);
    lv_anim_set_values(&anim, 0, -UI_STUDY_DETECT_TITLE_BOUNCE_PX);
    lv_anim_set_duration(&anim, UI_STUDY_DETECT_TITLE_ANIM_MS);
    lv_anim_set_playback_duration(&anim, UI_STUDY_DETECT_TITLE_ANIM_MS);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&anim, study_detect_title_y_anim_cb);
    lv_anim_start(&anim);
}

static void study_hint_set_content(study_light_hint_type_t type)
{
    if (objects.study_light_hint_icon == NULL) {
        return;
    }

    switch (type) {
        case STUDY_LIGHT_HINT_HIGH:
            lv_image_set_src(objects.study_light_hint_icon, "/lvgl/images/hint_light_high.bin");
            lv_label_set_text(objects.study_light_hint_title, "光线过亮");
            lv_obj_set_style_text_color(objects.study_light_hint_title,
                                        lv_color_hex(UI_COLOR_ORANGE),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(objects.study_light_hint_desc, "光线晃眼睛，记得拉上窗帘哟");
            break;
        case STUDY_LIGHT_HINT_LOW:
            lv_image_set_src(objects.study_light_hint_icon, "/lvgl/images/hint_light_low.bin");
            lv_label_set_text(objects.study_light_hint_title, "光线过暗");
            lv_obj_set_style_text_color(objects.study_light_hint_title,
                                        lv_color_hex(UI_COLOR_STUDY_HINT_BLUE),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(objects.study_light_hint_desc, "光线暗暗的，快打开小台灯吧～");
            break;
        case STUDY_LIGHT_HINT_POSE_WRONG:
            lv_image_set_src(objects.study_light_hint_icon, "/lvgl/images/hint_pose_wrong.bin");
            lv_label_set_text(objects.study_light_hint_title, "坐姿不正确");
            lv_obj_set_style_text_color(objects.study_light_hint_title,
                                        lv_color_hex(UI_COLOR_ORANGE),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(objects.study_light_hint_desc, "身体坐直一点，小卫士看到你快歪啦");
            break;
        case STUDY_LIGHT_HINT_CAMERA_BLOCK:
            lv_image_set_src(objects.study_light_hint_icon, "/lvgl/images/hint_camera_block.bin");
            lv_label_set_text(objects.study_light_hint_title, "摄像头遮挡");
            lv_obj_set_style_text_color(objects.study_light_hint_title,
                                        lv_color_hex(UI_COLOR_ORANGE),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(objects.study_light_hint_desc, "我好像看不清啦，请帮我调整一下位置");
            break;
        default:
            break;
    }
}

static void study_light_detect_reset_ui(void)
{
    if (objects.study_light_detect_title == NULL) {
        return;
    }

    lv_label_set_text(objects.study_light_detect_title, "光线检测中...");
    lv_obj_set_style_text_color(objects.study_light_detect_title,
                                lv_color_hex(0xffffffff),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(objects.study_light_detect_title,
                               font_pingfang_16px,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(objects.study_light_detect_title, LV_ALIGN_CENTER, 0, UI_STUDY_DETECT_TITLE_Y);

    if (objects.study_light_detect_subtitle != NULL) {
        lv_obj_add_flag(objects.study_light_detect_subtitle, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.study_light_detect_score != NULL) {
        lv_obj_add_flag(objects.study_light_detect_score, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.study_light_detect_icon != NULL) {
        lv_image_set_src(objects.study_light_detect_icon, "/lvgl/images/hint_light_succ.bin");
    }

    study_light_detect_title_anim_start();
}

static void study_light_detect_show_result(study_detect_result_t result)
{
    study_light_detect_title_anim_stop();

    s_light_detect_active = false;
    s_light_detect_result = result;
    s_light_detect_result_ms = qm_now_ms();

    if (result == STUDY_DETECT_RESULT_SUCCESS) {
        if (objects.study_light_detect_icon != NULL) {
            lv_image_set_src(objects.study_light_detect_icon, "/lvgl/images/hint_light_succ.bin");
        }
        if (objects.study_light_detect_title != NULL) {
            lv_label_set_text(objects.study_light_detect_title, "光线打卡成功");
            lv_obj_set_style_text_color(objects.study_light_detect_title,
                                        lv_color_hex(0xffffffff),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(objects.study_light_detect_title,
                                       font_pingfang_20px,
                                       LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(objects.study_light_detect_title, LV_ALIGN_CENTER, 0, UI_STUDY_DETECT_SUCC_TITLE_Y);
        }
        if (objects.study_light_detect_subtitle != NULL) {
            lv_label_set_text(objects.study_light_detect_subtitle, "光线亮度刚刚好");
            lv_obj_clear_flag(objects.study_light_detect_subtitle, LV_OBJ_FLAG_HIDDEN);
        }
        if (objects.study_light_detect_score != NULL) {
            lv_label_set_text(objects.study_light_detect_score, "+10分");
            lv_obj_set_style_text_color(objects.study_light_detect_score,
                                        lv_color_hex(UI_COLOR_ORANGE),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(objects.study_light_detect_score, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (objects.study_light_detect_icon != NULL) {
            lv_image_set_src(objects.study_light_detect_icon, "/lvgl/images/hint_light_fail.bin");
        }
        if (objects.study_light_detect_title != NULL) {
            lv_label_set_text(objects.study_light_detect_title, "光线打卡失败");
            lv_obj_set_style_text_color(objects.study_light_detect_title,
                                        lv_color_hex(0xffffffff),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(objects.study_light_detect_title,
                                       font_pingfang_20px,
                                       LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(objects.study_light_detect_title, LV_ALIGN_CENTER, 0, UI_STUDY_DETECT_SUCC_TITLE_Y);
        }
        if (objects.study_light_detect_subtitle != NULL) {
            lv_label_set_text(objects.study_light_detect_subtitle, "光线亮度不太合适哦");
            lv_obj_clear_flag(objects.study_light_detect_subtitle, LV_OBJ_FLAG_HIDDEN);
        }
        if (objects.study_light_detect_score != NULL) {
            lv_label_set_text(objects.study_light_detect_score, "+0分");
            lv_obj_set_style_text_color(objects.study_light_detect_score,
                                        lv_color_hex(UI_COLOR_STUDY_RED),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(objects.study_light_detect_score, LV_OBJ_FLAG_HIDDEN);
        }
    }

    goto_page_by_id(SCREEN_ID_PAGE_STUDY_LIGHT_DETECT);
}

static void study_light_detect_show_success(void)
{
    study_light_detect_show_result(STUDY_DETECT_RESULT_SUCCESS);
}

static void study_light_detect_show_failure(void)
{
    study_light_detect_show_result(STUDY_DETECT_RESULT_FAIL);
}

static void study_light_detect_show_hint(bool high)
{
    iot_func_req_corpus(high ? 6 : 5);
    study_hint_set_content(high ? STUDY_LIGHT_HINT_HIGH : STUDY_LIGHT_HINT_LOW);
    s_light_detect_hint = high ? STUDY_LIGHT_HINT_HIGH : STUDY_LIGHT_HINT_LOW;
    if (get_current_screen() != SCREEN_ID_PAGE_STUDY_LIGHT_HINT) {
        goto_page_by_id(SCREEN_ID_PAGE_STUDY_LIGHT_HINT);
    }
}

static void study_light_detect_enter_study(void)
{
    study_light_detect_title_anim_stop();
    s_light_detect_active = false;
    s_light_detect_result = STUDY_DETECT_RESULT_NONE;
    scr_study_session_start();
    goto_page_by_id(SCREEN_ID_PAGE_STUDY);
}

static void study_light_detect_window_reset(void)
{
    s_light_window_samples = 0;
}

static light_window_result_t study_light_window_classify(float avg, uint8_t samples)
{
    if (samples == 0) {
        return LIGHT_WINDOW_INVALID;
    }
    if (avg > BSP_SENSOR_LUX_HIGH_HINT) {
        return LIGHT_WINDOW_HIGH;
    }
    if (avg < BSP_SENSOR_LUX_LOW_HINT) {
        return LIGHT_WINDOW_LOW;
    }
    return LIGHT_WINDOW_OK;
}

static bool study_light_pick_hint_high(light_window_result_t r0, light_window_result_t r1)
{
    if (r1 == LIGHT_WINDOW_HIGH) {
        return true;
    }
    if (r1 == LIGHT_WINDOW_LOW) {
        return false;
    }
    if (r0 == LIGHT_WINDOW_HIGH) {
        return true;
    }
    return false;
}

static void study_light_detect_pair_reset(void)
{
    s_pair_result_idx = 0;
    s_pair_results[0] = LIGHT_WINDOW_INVALID;
    s_pair_results[1] = LIGHT_WINDOW_INVALID;
}

static void study_light_detect_evaluate_pair(void)
{
    light_window_result_t r0 = s_pair_results[0];
    light_window_result_t r1 = s_pair_results[1];

    ESP_LOGI(LOG_TAG, "light pair r0=%d r1=%d", r0, r1);

    study_light_detect_pair_reset();

    if (r0 == LIGHT_WINDOW_OK && r1 == LIGHT_WINDOW_OK) {
        study_light_detect_show_success();
        return;
    }

    if (r0 == LIGHT_WINDOW_INVALID && r1 == LIGHT_WINDOW_INVALID) {
        return;
    }

    if (r1 != LIGHT_WINDOW_HIGH && r1 != LIGHT_WINDOW_LOW &&
        r0 != LIGHT_WINDOW_HIGH && r0 != LIGHT_WINDOW_LOW) {
        return;
    }

    study_light_detect_show_hint(study_light_pick_hint_high(r0, r1));
}

static void study_light_detect_do_settlement(void)
{
    float avg = 0;
    uint8_t samples = s_light_window_samples;
    light_window_result_t result;

    if (samples >= 3) {
        float min_val = s_light_window_buf[0];
        float max_val = s_light_window_buf[0];
        float sum = 0;
        for (uint8_t i = 0; i < samples; i++) {
            float val = s_light_window_buf[i];
            if (val < min_val) {
                min_val = val;
            }
            if (val > max_val) {
                max_val = val;
            }
            sum += val;
        }
        sum -= min_val;
        sum -= max_val;
        avg = sum / (samples - 2);
    } else if (samples > 0) {
        float sum = 0;
        for (uint8_t i = 0; i < samples; i++) {
            sum += s_light_window_buf[i];
        }
        avg = sum / samples;
    }

    result = study_light_window_classify(avg, samples);
    ESP_LOGI(LOG_TAG, "light window[%u] avg=%.1f samples=%u result=%d",
             s_pair_result_idx, avg, samples, result);

    study_light_detect_window_reset();

    if (s_pair_result_idx >= STUDY_LIGHT_DETECT_PAIR_WINDOWS) {
        return;
    }

    s_pair_results[s_pair_result_idx++] = result;

    if (s_pair_result_idx >= STUDY_LIGHT_DETECT_PAIR_WINDOWS) {
        study_light_detect_evaluate_pair();
    }
}

void scr_study_light_detect_start(void)
{
    s_light_detect_active = true;
    s_light_detect_result = STUDY_DETECT_RESULT_NONE;
    s_light_detect_start_ms = qm_now_ms();
    s_light_detect_last_sample_ms = 0;
    s_light_detect_hint = STUDY_LIGHT_HINT_NONE;
    study_light_detect_window_reset();
    study_light_detect_pair_reset();
    study_light_detect_reset_ui();
    s_study_state = STUDY_STATE_IDLE;
}

bool scr_study_light_detect_is_active(void)
{
    return s_light_detect_active || s_light_detect_result != STUDY_DETECT_RESULT_NONE;
}

void tick_study_light_detect(void)
{
    if (s_light_detect_result != STUDY_DETECT_RESULT_NONE) {
        if (qm_now_ms() - s_light_detect_result_ms >= STUDY_LIGHT_SUCCESS_SHOW_MS) {
            study_light_detect_enter_study();
        }
        return;
    }

    if (!s_light_detect_active) {
        return;
    }

    if (qm_now_ms() - s_light_detect_start_ms >= STUDY_LIGHT_DETECT_TOTAL_MS) {
        ESP_LOGI(LOG_TAG, "light detect timeout 30s, show failure");
        study_light_detect_show_failure();
        return;
    }

    if (qm_now_ms() - s_light_detect_last_sample_ms < 1000) {
        return;
    }
    s_light_detect_last_sample_ms = qm_now_ms();

    float lux = 0;
    if (bsp_sensor_light_read(&lux) == QM_EOK) {
        s_light_window_buf[s_light_window_samples] = lux;
        s_light_window_samples++;
        ESP_LOGI(LOG_TAG, "light sample w=%u/%u lux=%.1f",
                 s_light_window_samples, STUDY_LIGHT_DETECT_WINDOW_SEC, lux);
    } else {
        ESP_LOGW(LOG_TAG, "light sample read failed");
    }

    if (s_light_window_samples >= STUDY_LIGHT_DETECT_WINDOW_SEC) {
        study_light_detect_do_settlement();
    }
}

static void study_ui_update_countdown(void)
{
    if (objects.study_count == NULL || objects.study_arc == NULL) {
        return;
    }

    uint32_t mins = (s_study_remaining + 59) / 60;
    if (mins == 0 && s_study_remaining > 0) {
        mins = 1;
    }

    char buf[8];
    qm_snprintf(buf, sizeof(buf), "%u", (unsigned)mins);
    lv_label_set_text(objects.study_count, buf);
    lv_obj_align_to(objects.study_count, objects.study_arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_value(objects.study_arc, (int32_t)s_study_remaining);
}

void scr_study_session_start(void)
{
    s_study_state = STUDY_STATE_RUNNING;
    s_study_remaining = STUDY_DURATION_SEC;
    s_study_last_tick = qm_now_ms();
    study_ui_update_countdown();
    scr_study_capture_start();
}

static void scr_study_capture_start(void)
{
    s_study_capture_active = true;
    s_study_capture_seq = 0;
    s_study_capture_last_ms = qm_now_ms();
    s_study_pose_hint_showing = false;
    study_pose_reset_counters();
    ESP_LOGI(LOG_TAG, "study capture started");
}

void scr_study_capture_stop(void)
{
    if (!s_study_capture_active) {
        return;
    }
    s_study_capture_active = false;
    s_study_pose_hint_showing = false;
    study_pose_reset_counters();
    ESP_LOGI(LOG_TAG, "study capture stopped, total=%u", (unsigned)s_study_capture_seq);
}

bool scr_study_capture_is_active(void)
{
    return s_study_capture_active;
}

bool scr_study_pose_hint_is_showing(void)
{
    return s_study_pose_hint_showing;
}

static void study_pose_reset_counters(void)
{
    s_pose_bad_consec = 0;
    s_pose_nobody_consec = 0;
}

static bool study_pose_hint_rate_ok(uint32_t *window_ms, uint8_t *count)
{
    uint32_t now = qm_now_ms();

    if (*window_ms == 0 || now - *window_ms >= STUDY_POSE_HINT_WINDOW_MS) {
        *window_ms = now;
        *count = 0;
    }
    return *count < STUDY_POSE_HINT_MAX_PER_WINDOW;
}

static void study_pose_hint_count_inc(uint32_t *window_ms, uint8_t *count)
{
    uint32_t now = qm_now_ms();

    if (*window_ms == 0 || now - *window_ms >= STUDY_POSE_HINT_WINDOW_MS) {
        *window_ms = now;
        *count = 0;
    }
    (*count)++;
}

static void study_pose_show_hint(study_light_hint_type_t type)
{
    study_hint_set_content(type);
    s_light_detect_hint = type;
    s_study_pose_hint_showing = true;
    s_study_pose_hint_until_ms = qm_now_ms() + STUDY_POSE_HINT_SHOW_MS;
    study_pose_reset_counters();
    if (get_current_screen() != SCREEN_ID_PAGE_STUDY_LIGHT_HINT) {
        goto_page_by_id(SCREEN_ID_PAGE_STUDY_LIGHT_HINT);
    }
    ESP_LOGI(LOG_TAG, "study pose hint show type=%d", (int)type);
}

void scr_study_pose_on_frame_result(yolo11_pose_frame_status_t status)
{
    if (!s_study_capture_active || s_study_pose_hint_showing) {
        return;
    }

    if (status == YOLO11_POSE_FRAME_NO_PERSON) {
        s_pose_nobody_consec++;
        s_pose_bad_consec = 0;
        if (s_pose_nobody_consec >= STUDY_POSE_NOBODY_CONSEC_FRAMES
                && study_pose_hint_rate_ok(&s_pose_block_hint_window_ms, &s_pose_block_hint_count)) {
            study_pose_hint_count_inc(&s_pose_block_hint_window_ms, &s_pose_block_hint_count);
            study_pose_show_hint(STUDY_LIGHT_HINT_CAMERA_BLOCK);
        }
        return;
    }

    s_pose_nobody_consec = 0;

    if (status == YOLO11_POSE_FRAME_BAD_POSTURE) {
        s_pose_bad_consec++;
        if (s_pose_bad_consec >= STUDY_POSE_BAD_CONSEC_FRAMES
                && study_pose_hint_rate_ok(&s_pose_bad_hint_window_ms, &s_pose_bad_hint_count)) {
            study_pose_hint_count_inc(&s_pose_bad_hint_window_ms, &s_pose_bad_hint_count);
            study_pose_show_hint(STUDY_LIGHT_HINT_POSE_WRONG);
        }
        return;
    }

    s_pose_bad_consec = 0;
}

static void scr_study_capture_tick(void)
{
    char name[16];

    if (!s_study_capture_active) {
        return;
    }

    if (qm_now_ms() - s_study_capture_last_ms < STUDY_CAPTURE_INTERVAL_MS) {
        return;
    }

    if (bsp_camera_capture_busy()) {
        ESP_LOGI(LOG_TAG, "study capture #%u skipped, camera busy",
                 (unsigned)(s_study_capture_seq + 1));
        return;
    }

    s_study_capture_last_ms = qm_now_ms();
    s_study_capture_seq++;
    qm_snprintf(name, sizeof(name), "%03u.jpg", (unsigned)s_study_capture_seq);
    ESP_LOGI(LOG_TAG, "study capture #%u trigger, name=%s avail=%d",
             (unsigned)s_study_capture_seq, name, bsp_camera_is_available() ? 1 : 0);
    bsp_camera_capture_upload_named_with_pose(name);
}

void scr_study_session_pause_toggle(void)
{
    if (s_study_state == STUDY_STATE_RUNNING) {
        s_study_state = STUDY_STATE_PAUSE;
        if (objects.study_btn_pause != NULL) {
            lv_label_set_text(lv_obj_get_child(objects.study_btn_pause, 0), "继续");
        }
    } else if (s_study_state == STUDY_STATE_PAUSE) {
        s_study_state = STUDY_STATE_RUNNING;
        s_study_last_tick = qm_now_ms();
        if (objects.study_btn_pause != NULL) {
            lv_label_set_text(lv_obj_get_child(objects.study_btn_pause, 0), "暂停");
        }
    }
}

void scr_study_session_pause_if_running(void)
{
    if (s_study_state == STUDY_STATE_RUNNING) {
        s_study_state = STUDY_STATE_PAUSE;
        if (objects.study_btn_pause != NULL) {
            lv_label_set_text(lv_obj_get_child(objects.study_btn_pause, 0), "继续");
        }
    }
}

void scr_study_session_skip(void)
{
    s_study_state = STUDY_STATE_IDLE;
    scr_study_capture_stop();
    scr_study_show_result(false);
}

void scr_study_result_confirm(void)
{
    s_study_remaining = STUDY_DURATION_SEC;
    s_study_state = STUDY_STATE_IDLE;
    if (objects.study_btn_pause != NULL) {
        lv_label_set_text(lv_obj_get_child(objects.study_btn_pause, 0), "暂停");
    }
    study_ui_update_countdown();
}

static void scr_study_show_result(bool success)
{
    if (objects.study_result_icon == NULL) {
        return;
    }

    scr_study_capture_stop();

    if (success) {
        lv_image_set_src(objects.study_result_icon, "/lvgl/images/hint_light_succ.bin");
        lv_label_set_text(objects.study_result_title, "坐姿打卡成功");
        lv_label_set_text(objects.study_result_subtitle, "累计合格20分钟");
        lv_label_set_text(objects.study_result_extra, "+10分");
        lv_obj_set_style_text_color(objects.study_result_extra,
                                    lv_color_hex(UI_COLOR_ORANGE),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(objects.study_result_extra,
                                   font_pingfang_20px,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(objects.study_result_extra, 0,
                                         LV_PART_MAIN | LV_STATE_DEFAULT);
        if (objects.study_result_btn != NULL) {
            lv_label_set_text(lv_obj_get_child(objects.study_result_btn, 0), "确认");
        }
    } else {
        lv_image_set_src(objects.study_result_icon, "/lvgl/images/hint_light_fail.bin");
        lv_label_set_text(objects.study_result_title, "坐姿打卡失败");
        lv_label_set_text(objects.study_result_subtitle, "合格时长不足20分钟");
        lv_label_set_text(objects.study_result_extra, "再坚持一下就能打卡啦");
        lv_obj_set_style_text_color(objects.study_result_extra,
                                    lv_color_hex(UI_COLOR_ORANGE),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(objects.study_result_extra,
                                   font_pingfang_16px,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(objects.study_result_extra, 2,
                                         LV_PART_MAIN | LV_STATE_DEFAULT);
        if (objects.study_result_btn != NULL) {
            lv_label_set_text(lv_obj_get_child(objects.study_result_btn, 0), "知道了");
        }
    }

    study_result_layout_refresh();
    goto_page_by_id(SCREEN_ID_PAGE_STUDY_RESULT);
}

void scr_study_entry_btn_set_visible(bool visible)
{
    if (objects.btn_study_entry == NULL) {
        return;
    }

    if (visible) {
        lv_obj_clear_flag(objects.btn_study_entry, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(objects.btn_study_entry, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t *ui_create_study_pill_btn(lv_obj_t *parent, int x, int y, int w, int h,
                                           uint32_t bg_color, uint32_t text_color,
                                           const char *text, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, h / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, lv_color_hex(text_color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(label);
    }
    return btn;
}

void create_screen_page_study() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_study = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *badge = lv_obj_create(parent_obj);
            lv_obj_set_size(badge, 124, 24);
            lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, UI_STUDY_BADGE_Y);
            lv_obj_set_style_bg_color(badge, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(badge, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(badge, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(badge, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(badge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(badge, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
            {
                lv_obj_t *dot = lv_obj_create(badge);
                lv_obj_set_size(dot, 6, 6);
                lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);
                lv_obj_set_style_bg_color(dot, lv_color_hex(UI_COLOR_STUDY_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
            }
            {
                lv_obj_t *label = lv_label_create(badge);
                lv_label_set_text(label, "摄像头已开启");
                lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_STUDY_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(label, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
            }
        }
        {
            lv_obj_t *arc = lv_arc_create(parent_obj);
            objects.study_arc = arc;
            lv_obj_set_pos(arc, UI_STUDY_ARC_X, UI_STUDY_ARC_Y);
            lv_obj_set_size(arc, UI_STUDY_ARC_SIZE, UI_STUDY_ARC_SIZE);
            lv_arc_set_rotation(arc, 270);
            lv_arc_set_bg_angles(arc, 0, 360);
            lv_arc_set_range(arc, 0, STUDY_DURATION_SEC);
            lv_arc_set_value(arc, STUDY_DURATION_SEC);
            lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_arc_width(arc, 5, LV_PART_MAIN);
            lv_obj_set_style_arc_width(arc, 5, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(arc, lv_color_hex(0xff555555), LV_PART_MAIN);
            lv_obj_set_style_arc_color(arc, lv_color_hex(UI_COLOR_STUDY_GREEN), LV_PART_INDICATOR);
            lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
            lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
        }
        {
            lv_obj_t *label = lv_label_create(parent_obj);
            objects.study_count = label;
            lv_label_set_text(label, "20");
            lv_obj_set_width(label, UI_STUDY_ARC_SIZE);
            lv_obj_set_style_text_color(label, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(label, font_misans_40px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align_to(label, objects.study_arc, LV_ALIGN_CENTER, 0, 0);
        }
        {
            lv_obj_t *title = lv_label_create(parent_obj);
            lv_obj_set_pos(title, UI_STUDY_TEXT_X, UI_STUDY_TEXT_Y);
            lv_obj_set_width(title, 120);
            lv_label_set_text(title, "居家学习打卡");
            lv_obj_set_style_text_color(title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(title, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *desc = lv_label_create(parent_obj);
            lv_obj_set_pos(desc, UI_STUDY_TEXT_X, UI_STUDY_TEXT_Y + 24);
            lv_obj_set_width(desc, 120);
            lv_label_set_text(desc, "坐端正、离书远一点，标准坐姿坚持20分钟");
            lv_obj_set_style_text_color(desc, lv_color_hex(UI_COLOR_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(desc, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_line_space(desc, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        objects.study_btn_pause = ui_create_study_pill_btn(parent_obj,
            UI_STUDY_BTN_PAUSE_X, UI_STUDY_BTN_Y, UI_STUDY_BTN_W, UI_STUDY_BTN_H,
            UI_COLOR_ORANGE, 0xff000000, "暂停", action_study_pause_click);
        objects.study_btn_skip = ui_create_study_pill_btn(parent_obj,
            UI_STUDY_BTN_SKIP_X, UI_STUDY_BTN_Y, UI_STUDY_BTN_W, UI_STUDY_BTN_H,
            0xff333333, 0xffffffff, "跳过", action_study_skip_click);
    }

    tick_screen_page_study();
}

void create_screen_page_study_light_detect() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_study_light_detect = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *icon = lv_image_create(obj);
        objects.study_light_detect_icon = icon;
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, UI_STUDY_DETECT_ICON_Y);
        lv_image_set_src(icon, "/lvgl/images/hint_light_succ.bin");
    }
    {
        lv_obj_t *title = lv_label_create(obj);
        objects.study_light_detect_title = title;
        lv_label_set_text(title, "光线检测中...");
        lv_obj_set_style_text_color(title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(title, LV_ALIGN_CENTER, 0, UI_STUDY_DETECT_TITLE_Y);
    }
    {
        lv_obj_t *subtitle = lv_label_create(obj);
        objects.study_light_detect_subtitle = subtitle;
        lv_label_set_text(subtitle, "光线亮度刚刚好");
        lv_obj_set_style_text_color(subtitle, lv_color_hex(UI_COLOR_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(subtitle, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, UI_STUDY_DETECT_SUBTITLE_Y);
        lv_obj_add_flag(subtitle, LV_OBJ_FLAG_HIDDEN);
    }
    {
        lv_obj_t *score = lv_label_create(obj);
        objects.study_light_detect_score = score;
        lv_label_set_text(score, "+10分");
        lv_obj_set_style_text_color(score, lv_color_hex(UI_COLOR_ORANGE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(score, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(score, LV_ALIGN_CENTER, 0, UI_STUDY_DETECT_SCORE_Y);
        lv_obj_add_flag(score, LV_OBJ_FLAG_HIDDEN);
    }

    tick_screen_page_study_light_detect();
}

void tick_screen_page_study_light_detect() {
}

void create_screen_page_study_light_hint() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_study_light_hint = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *icon = lv_image_create(obj);
        objects.study_light_hint_icon = icon;
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, UI_STUDY_HINT_ICON_Y);
        lv_image_set_src(icon, "/lvgl/images/hint_light_high.bin");
    }
    {
        lv_obj_t *title = lv_label_create(obj);
        objects.study_light_hint_title = title;
        lv_label_set_text(title, "光线过亮");
        lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_ORANGE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(title, LV_ALIGN_CENTER, 0, UI_STUDY_HINT_TITLE_Y);
    }
    {
        lv_obj_t *desc = lv_label_create(obj);
        objects.study_light_hint_desc = desc;
        lv_obj_set_width(desc, 200);
        lv_label_set_text(desc, "光线晃眼睛，记得拉上窗帘哟");
        lv_obj_set_style_text_color(desc, lv_color_hex(UI_COLOR_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(desc, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(desc, LV_ALIGN_CENTER, 0, UI_STUDY_HINT_DESC_Y);
    }

    tick_screen_page_study_light_hint();
}

void tick_screen_page_study_light_hint() {
    if (!s_study_pose_hint_showing) {
        return;
    }

    if (qm_now_ms() >= s_study_pose_hint_until_ms) {
        s_study_pose_hint_showing = false;
        s_light_detect_hint = STUDY_LIGHT_HINT_NONE;
        goto_page_by_id(SCREEN_ID_PAGE_STUDY);
        return;
    }

    scr_study_capture_tick();
    scr_study_session_tick();
}

static void scr_study_session_tick(void)
{
    if (s_study_state != STUDY_STATE_RUNNING) {
        return;
    }

    if (qm_now_ms() - s_study_last_tick < 1000) {
        return;
    }
    s_study_last_tick = qm_now_ms();

    if (s_study_remaining == 0) {
        return;
    }

    s_study_remaining--;
    study_ui_update_countdown();

    if (s_study_remaining == 0) {
        s_study_state = STUDY_STATE_IDLE;
        scr_study_show_result(true);
    }
}

void create_screen_page_study_result() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page_study_result = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *icon = lv_image_create(obj);
        objects.study_result_icon = icon;
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, UI_STUDY_RESULT_ICON_Y);
        lv_image_set_src(icon, "/lvgl/images/hint_light_succ.bin");
    }
    {
        lv_obj_t *title = lv_label_create(obj);
        objects.study_result_title = title;
        lv_label_set_text(title, "坐姿打卡成功");
        lv_obj_set_style_text_color(title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(title, objects.study_result_icon,
                        LV_ALIGN_OUT_BOTTOM_MID, 0, UI_STUDY_RESULT_ICON_TITLE_GAP);
    }
    {
        lv_obj_t *subtitle = lv_label_create(obj);
        objects.study_result_subtitle = subtitle;
        lv_obj_set_width(subtitle, 200);
        lv_label_set_text(subtitle, "累计合格20分钟");
        lv_obj_set_style_text_color(subtitle, lv_color_hex(UI_COLOR_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(subtitle, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(subtitle, objects.study_result_title,
                        LV_ALIGN_OUT_BOTTOM_MID, 0, UI_STUDY_RESULT_TEXT_GAP);
    }
    {
        lv_obj_t *extra = lv_label_create(obj);
        objects.study_result_extra = extra;
        lv_obj_set_width(extra, 200);
        lv_label_set_text(extra, "+10分");
        lv_obj_set_style_text_color(extra, lv_color_hex(UI_COLOR_ORANGE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(extra, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(extra, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(extra, objects.study_result_subtitle,
                        LV_ALIGN_OUT_BOTTOM_MID, 0, UI_STUDY_RESULT_TEXT_GAP);
    }
    {
        objects.study_result_btn = ui_create_study_pill_btn(obj,
            0, 0, UI_STUDY_RESULT_BTN_W, UI_STUDY_RESULT_BTN_H,
            UI_COLOR_ORANGE, 0xff000000, "确认", action_study_result_confirm_click);
        lv_obj_align(objects.study_result_btn, LV_ALIGN_BOTTOM_MID, 0, -UI_STUDY_RESULT_BTN_BOTTOM_PAD);
    }

    tick_screen_page_study_result();
}

void tick_screen_page_study_result() {
}

void tick_screen_page_study() {
    if (get_current_screen() != SCREEN_ID_PAGE_STUDY) {
        return;
    }

    scr_study_capture_tick();
    scr_study_session_tick();
}

#define UI_SETTINGS_TITLE_Y     30
#define UI_SETTINGS_TITLE_H     24
#define UI_SETTINGS_ROW_PAD_Y   5
#define UI_SETTINGS_ICON_H      50
#define UI_SETTINGS_DIV_H       1
#define UI_SETTINGS_ROW_BLOCK_H (UI_SETTINGS_ROW_PAD_Y + UI_SETTINGS_ICON_H + \
                                  UI_SETTINGS_ROW_PAD_Y + UI_SETTINGS_DIV_H)
#define UI_SETTINGS_ROW1_Y      (UI_SETTINGS_TITLE_Y + UI_SETTINGS_TITLE_H + 10)
#define UI_SETTINGS_ROW2_Y      (UI_SETTINGS_ROW1_Y + UI_SETTINGS_ROW_BLOCK_H + UI_SETTINGS_ROW_PAD_Y)
#define UI_SETTINGS_ROW_W       240
#define UI_SETTINGS_ICON_X      36
#define UI_SETTINGS_TEXT_X      94
#define UI_SETTINGS_ARROW_X     186
#define UI_COLOR_PANEL          0xff1a1a1a
#define UI_COLOR_BTN_GREY       0xff333333
#define UI_COLOR_RED            0xffe53935
#define UI_COLOR_BLUE           0xff2196f3
#define UI_SOUND_PANEL_W        200
#define UI_SOUND_PANEL_H        120
#define UI_SOUND_MUTE_ROW_H     52
#define UI_SOUND_VOL_SLIDER_X   12
#define UI_SOUND_VOL_SLIDER_W   (UI_SOUND_PANEL_W - 24)
#define UI_SOUND_VOL_SLIDER_Y   82
#define UI_SOUND_VOL_SLIDER_H   8

static void ui_settings_obj_flat(lv_obj_t *obj)
{
    ui_style_clear_decor(obj);
    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}

static void ui_settings_mute_switch_style(lv_obj_t *sw)
{
    lv_obj_set_style_pad_all(sw, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(sw, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sw, lv_color_hex(UI_COLOR_BTN_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sw, lv_color_hex(UI_COLOR_RED), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_width(sw, 18, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_height(sw, 18, LV_PART_KNOB | LV_STATE_DEFAULT);
}

static void ui_settings_volume_slider_style(lv_obj_t *slider)
{
    lv_obj_set_style_pad_all(slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(slider, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(slider, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(slider, lv_color_hex(UI_COLOR_BTN_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(slider, lv_color_hex(UI_COLOR_BLUE), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_width(slider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_height(slider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
}

static lv_obj_t *ui_create_settings_page(const char *title)
{
    lv_obj_t *obj = lv_obj_create(0);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_event_cb(obj, event_all_handler, LV_EVENT_ALL, (void *)0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *label = lv_label_create(obj);
        lv_label_set_text(label, title);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, font_pingfang_20px, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, UI_SETTINGS_TITLE_Y);
    }
    return obj;
}

static void ui_settings_row_child(lv_obj_t *child)
{
    lv_obj_remove_flag(child, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(child, LV_OBJ_FLAG_EVENT_BUBBLE);
}

static void ui_settings_divider_nonclickable(lv_obj_t *line)
{
    ui_style_clear_decor(line);
    lv_obj_set_style_bg_color(line, lv_color_hex(UI_COLOR_DIVIDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(line, LV_OBJ_FLAG_EVENT_BUBBLE);
}

static lv_obj_t *ui_add_settings_menu_row(lv_obj_t *page, int y, const lv_image_dsc_t *icon,
                                          const char *text, lv_event_cb_t cb)
{
    lv_obj_t *block = lv_obj_create(page);
    lv_obj_set_pos(block, 0, y);
    lv_obj_set_size(block, UI_SETTINGS_ROW_W, UI_SETTINGS_ROW_BLOCK_H);
    ui_style_clear_decor(block);
    lv_obj_set_style_pad_all(block, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(block, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(block, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(block, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(block, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_ext_click_area(block, 6);

    lv_obj_t *img = lv_image_create(block);
    lv_obj_set_pos(img, UI_SETTINGS_ICON_X, UI_SETTINGS_ROW_PAD_Y);
    lv_image_set_src(img, icon);
    ui_settings_row_child(img);

    lv_obj_t *lbl = lv_label_create(block);
    lv_label_set_text(lbl, text);
    lv_obj_set_pos(lbl, UI_SETTINGS_TEXT_X,
                   UI_SETTINGS_ROW_PAD_Y + (UI_SETTINGS_ICON_H - 16) / 2);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_settings_row_child(lbl);

    lv_obj_t *arrow = lv_image_create(block);
    lv_obj_set_pos(arrow, UI_SETTINGS_ARROW_X,
                   UI_SETTINGS_ROW_PAD_Y + (UI_SETTINGS_ICON_H - 40) / 2);
    lv_image_set_src(arrow, &img_setup_more);
    ui_settings_row_child(arrow);

    lv_obj_t *line = lv_obj_create(block);
    lv_obj_set_pos(line, UI_LIST_DIVIDER_X, UI_SETTINGS_ROW_BLOCK_H - UI_SETTINGS_DIV_H);
    lv_obj_set_size(line, UI_LIST_DIVIDER_W, UI_SETTINGS_DIV_H);
    ui_settings_divider_nonclickable(line);

    return block;
}

static lv_obj_t *ui_create_settings_btn(lv_obj_t *parent, int x, int y, int w, int h,
                                        const char *text, lv_color_t bg, lv_color_t fg,
                                        lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    ui_style_clear_decor(btn);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, h / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, fg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(lbl, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

void scr_settings_sound_sync(void)
{
    if (objects.settings_mute_sw == NULL || objects.settings_volume_slider == NULL) {
        return;
    }

    qm_ai_handle_t handle = ai_main_get_handle();
    if (handle == NULL) {
        return;
    }

    uint32_t volume = qm_ai_audio_get_local_volume(handle);
    if (volume < 10) {
        volume = 10;
    } else if (volume > 100) {
        volume = 100;
    }
    lv_slider_set_value(objects.settings_volume_slider, (int32_t)volume, LV_ANIM_OFF);

    bool_t mute = qm_ai_audio_get_player_mute_state(handle);
    if (mute) {
        lv_obj_add_state(objects.settings_mute_sw, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(objects.settings_mute_sw, LV_STATE_CHECKED);
    }
}

void create_screen_page_settings() {
    lv_obj_t *obj = ui_create_settings_page("系统设置");
    objects.page_settings = obj;

    ui_add_settings_menu_row(obj, UI_SETTINGS_ROW1_Y, &img_setup_volume, "声音设置",
                             action_settings_sound_click);
    ui_add_settings_menu_row(obj, UI_SETTINGS_ROW2_Y, &img_setup_wifi, "设备配网",
                             action_settings_net_click);

    tick_screen_page_settings();
}

void tick_screen_page_settings() {
}

void create_screen_page_settings_sound() {
    lv_obj_t *obj = ui_create_settings_page("声音设置");
    objects.page_settings_sound = obj;
    objects.settings_mute_sw = NULL;
    objects.settings_volume_slider = NULL;

    lv_obj_t *panel = lv_obj_create(obj);
    lv_obj_set_size(panel, UI_SOUND_PANEL_W, UI_SOUND_PANEL_H);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 10);
    ui_settings_obj_flat(panel);
    lv_obj_set_style_radius(panel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(panel, lv_color_hex(UI_COLOR_PANEL), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *mute_lbl = lv_label_create(panel);
    lv_label_set_text(mute_lbl, "静音模式");
    lv_obj_set_pos(mute_lbl, 12, (UI_SOUND_MUTE_ROW_H - 16) / 2);
    lv_obj_set_style_text_color(mute_lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mute_lbl, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_settings_row_child(mute_lbl);

    objects.settings_mute_sw = lv_switch_create(panel);
    lv_obj_set_size(objects.settings_mute_sw, 44, 24);
    lv_obj_set_pos(objects.settings_mute_sw, UI_SOUND_PANEL_W - 12 - 44,
                   (UI_SOUND_MUTE_ROW_H - 24) / 2);
    ui_settings_mute_switch_style(objects.settings_mute_sw);
    lv_obj_add_event_cb(objects.settings_mute_sw, action_settings_mute_changed,
                        LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_flag(objects.settings_mute_sw, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *divider = lv_obj_create(panel);
    lv_obj_set_pos(divider, 12, UI_SOUND_MUTE_ROW_H - 1);
    lv_obj_set_size(divider, UI_SOUND_PANEL_W - 24, 1);
    ui_settings_divider_nonclickable(divider);

    objects.settings_volume_slider = lv_slider_create(panel);
    lv_obj_set_pos(objects.settings_volume_slider, UI_SOUND_VOL_SLIDER_X, UI_SOUND_VOL_SLIDER_Y);
    lv_obj_set_size(objects.settings_volume_slider, UI_SOUND_VOL_SLIDER_W, UI_SOUND_VOL_SLIDER_H);
    lv_slider_set_range(objects.settings_volume_slider, 10, 100);
    ui_settings_volume_slider_style(objects.settings_volume_slider);
    lv_obj_add_event_cb(objects.settings_volume_slider, action_settings_volume_changed,
                        LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_flag(objects.settings_volume_slider, LV_OBJ_FLAG_EVENT_BUBBLE);

    tick_screen_page_settings_sound();
}

void tick_screen_page_settings_sound() {
}

void create_screen_page_settings_net() {
    lv_obj_t *obj = ui_create_settings_page("设备配网");
    objects.page_settings_net = obj;

    lv_obj_t *hint = lv_label_create(obj);
    lv_label_set_text(hint, "将清除WiFi配置\n进入配网模式\n是否继续?");
    lv_obj_set_width(hint, 180);
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_COLOR_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(hint, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, -10);

    ui_create_settings_btn(obj, 40, 168, 70, 36, "确认",
                           lv_color_hex(UI_COLOR_ORANGE), lv_color_hex(0xff000000),
                           action_settings_net_confirm_click);
    ui_create_settings_btn(obj, 130, 168, 70, 36, "取消",
                           lv_color_hex(UI_COLOR_BTN_GREY), lv_color_hex(0xffffffff),
                           action_settings_net_cancel_click);

    tick_screen_page_settings_net();
}

void tick_screen_page_settings_net() {
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
                    lv_obj_set_pos(obj, 95, 30);
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
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
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
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
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
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
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
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
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
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
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
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.tomato_icon = obj;
            lv_obj_set_pos(obj, 0, UI_TIMER_ICON_Y);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, "/lvgl/images/icon_inner_tomato.bin");
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.tomato_count = obj;
            lv_obj_set_pos(obj, 0, UI_TIMER_COUNT_Y);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "25:00");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_misans_40px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.tomato_label = obj;
            lv_obj_set_pos(obj, 0, UI_TIMER_HINT_Y);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "番茄时钟");
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(UI_COLOR_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, font_pingfang_16px, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_tomato_opt = obj;
            lv_obj_set_pos(obj, UI_TIMER_BTN_OPT_X, UI_TIMER_BTN_Y);
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
                lv_obj_t *obj = lv_image_create(objects.container_tomato_opt);
                objects.tomato_opt = obj;
                lv_obj_set_pos(obj, 26, 12);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_image_set_src(obj, "/lvgl/images/timer_start.bin");
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.container_tomato_stop = obj;
            lv_obj_set_pos(obj, UI_TIMER_BTN_STOP_X, UI_TIMER_BTN_Y);
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
                lv_obj_t *obj = lv_image_create(objects.container_tomato_stop);
                objects.tomato_stop = obj;
                lv_obj_set_pos(obj, 8, 12);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_image_set_src(obj, "/lvgl/images/timer_stop.bin");
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

// tick functions
typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_page_sleep,
    tick_screen_page_main,
    tick_screen_page_apps1,
    tick_screen_page_apps2,
    tick_screen_page_alarm,
    tick_screen_page_timer,
    tick_screen_page_pomodoro,
    tick_screen_page_eye,
    tick_screen_page_timetable,
    tick_screen_page_checkin,
    tick_screen_page_study,
    tick_screen_page_settings,
    tick_screen_page_alarm_edit,
    tick_screen_page_settings_sound,
    tick_screen_page_settings_net,
    tick_screen_page_study_light_hint,
    tick_screen_page_study_light_detect,
    tick_screen_page_study_result,
};
void tick_screen(int screen_index) {
    if (screen_index < 0 || screen_index >= (int)(sizeof(tick_screen_funcs) / sizeof(tick_screen_funcs[0]))) {
        return;
    }
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

    create_screen_page_sleep();
    create_screen_page_main();
    create_screen_page_apps1();
    create_screen_page_apps2();
    create_screen_page_alarm();
    create_screen_page_timer();
    create_screen_page_pomodoro();
    create_screen_page_eye();
    create_screen_page_timetable();
    create_screen_page_checkin();
    create_screen_page_study();
    create_screen_page_settings();
    create_screen_page_alarm_edit();
    create_screen_page_settings_sound();
    create_screen_page_settings_net();
    create_screen_page_study_light_hint();
    create_screen_page_study_light_detect();
    create_screen_page_study_result();

    ui_update_page_indicator(0);

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

    if (alarm_list_count > 0) {
        alarm_t *alarm_list = malloc(sizeof(alarm_t) * alarm_list_count);
        if (alarm_list != NULL) {
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
        }
    }

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

