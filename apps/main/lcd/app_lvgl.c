#include "app_lvgl.h"
#include "bsp_lcd.h"

#include <inttypes.h>
#include "audio_thread.h"
#include <esp_mac.h>
#include <esp_spiffs.h>

#include "esp_log.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lcd_gc9a01.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_touch_cst816s.h"

#define LOG_TAG "app_lvgl"

static lv_disp_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

typedef struct {
    esp_lcd_panel_io_handle_t lcd_io_handle;
    esp_lcd_panel_handle_t lcd_panel_handle;
    esp_lcd_touch_handle_t lcd_tp_handle;
} app_lvgl_handle_t;

static app_lvgl_handle_t app_lvgl_handle = { 0 };

void app_lvgl_lcd_init(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle)
{
    app_lvgl_handle.lcd_io_handle = io_handle;
    app_lvgl_handle.lcd_panel_handle = panel_handle;
}

void app_lvgl_touch_init(esp_lcd_touch_handle_t tp_handle)
{
    app_lvgl_handle.lcd_tp_handle = tp_handle;
}

void app_lvgl_port_init(void)
{
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_affinity = 0;
    lvgl_port_init(&lvgl_cfg);

    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = app_lvgl_handle.lcd_io_handle,
        .panel_handle = app_lvgl_handle.lcd_panel_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES / 6,
        .double_buffer = 1,
        .trans_size = 0,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_spiram = 0,
            .swap_bytes = 1,
            .buff_dma = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    
    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = app_lvgl_handle.lcd_tp_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    lvgl_port_task_wake(LVGL_PORT_EVENT_DISPLAY, NULL);
}

static qm_timer_t turnoff_timer = { 0 };
static int turnoff_timeout_tbl[4] = { 15000, 60000, 300000, 2147483647 }; // 30秒, 1分, 5分, 永不 //  { 15000, 30000, 45000, 60000 };

uint32_t bsp_lcd_get_turnoff_timeout(void)
{
    int scr_tmo = get_var_scr_tmo_value(); // 0 - 30秒, 1 - 1分, 2 - 5分, 3 - Never
    if (scr_tmo < 0 || scr_tmo > 3) {
        scr_tmo = 0;
    }
    return turnoff_timeout_tbl[scr_tmo];
}

extern void goto_page_by_id(int8_t screen_id);

static void turnoff_timer_handler(qm_timer_t *timer,  void *arg) 
{
    if (get_var_turnoff_value()) {
        bsp_lcd_bl_dim();

        if (get_current_screen() == SCREEN_ID_PAGE_POMODORO) {
            set_var_turnoff_value(bsp_lcd_get_turnoff_timeout());
        } else if (get_current_screen() != SCREEN_ID_PAGE_SLEEP) {
            goto_page_by_id(SCREEN_ID_PAGE_SLEEP);
        }
    }
}

void turnoff_timer_reset(uint8_t flag) { // 0 - screen operation, 1 - audio receive

    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    if (flag == 0) {
        set_var_screen_operation_tick();
    } else {
        // if (get_current_screen() != SCREEN_ID_PAGE_RECOG && get_var_screen_operation_tick()) {
        //     goto_recog_page();
        // }
    }
    set_var_turnoff_value(timeout);
    bsp_lcd_bl_on();
}

static void lvgl_task(void *arg) 
{
    lvgl_port_lock(0); // 锁定lvgl接口 
    ui_init();
    lvgl_port_unlock(); // 解锁lvgl接口

    vTaskDelay(500);

    bsp_lcd_bl_on();

    set_var_scr_tmo_value(0, true);
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();
    set_var_turnoff_value(timeout);

    qm_timer_new(&turnoff_timer, turnoff_timer_handler, NULL, 1000, 1);
    qm_timer_start(&turnoff_timer);
    
    while (1) {
        lvgl_port_lock(0); // 锁定lvgl接口，以保护UI更新过程。
        ui_tick();
        lvgl_port_unlock(); // 解锁lvgl接口以允许其他任务访问。 
        vTaskDelay(pdMS_TO_TICKS(50)); // 函数延迟50毫秒，以避免过度占用CPU资源。
    }
}

void app_lvgl_init(void) 
{
    if (app_lvgl_handle.lcd_panel_handle == NULL || app_lvgl_handle.lcd_tp_handle == NULL || app_lvgl_handle.lcd_io_handle == NULL) {
        return;
    }

    esp_vfs_spiffs_conf_t spiff_conf = {
        .base_path = "/lvgl",
        .partition_label = "lvgl",
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&spiff_conf);
    if (ret != ESP_OK) {
        ESP_LOGW(LOG_TAG, "esp_vfs_spiffs_register init failed");
        vTaskDelete(NULL);
        return;
    }

    app_lvgl_port_init();

    audio_thread_create(NULL, "lvgl_task", lvgl_task, NULL, 10 * 1024, 4, true, 1);
}