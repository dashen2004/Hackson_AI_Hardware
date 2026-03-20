#include "app_lvgl.h"
#include "bsp_lcd.h"
#include "qm_work.h"

#include "esp_spiffs.h"
#include "esp_lvgl_port.h"

#include "ui.h"
#include "vars.h"

#define LOG_TAG "app_lvgl"

#define LVGL_TURNOFF_TIME_MS   (15 * 1000)

typedef struct {
    esp_lcd_panel_io_handle_t lcd_io_handle;
    esp_lcd_panel_handle_t lcd_panel_handle;
    esp_lcd_touch_handle_t lcd_tp_handle;

    qm_task_t lvgl_task;
    qm_work_t lvgl_turnoff_work;
} app_lvgl_handle_t;

static app_lvgl_handle_t app_lvgl_handle = { 0 };
static lv_disp_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

static void app_lvgl_port_init(void);

static void lvgl_turnoff_handler(void *arg)
{
    QM_LOGI(LOG_TAG, "lvgl_turnoff_handler");

    bsp_lcd_bl_dim();

    if (get_current_screen() == SCREEN_ID_PAGE_POMODORO) {
        app_lvgl_turnoff_reset(0);
    } else if (get_current_screen() != SCREEN_ID_PAGE_SLEEP) {
        extern void goto_page_by_id(int8_t screen_id);
        goto_page_by_id(SCREEN_ID_PAGE_SLEEP);
    }
}

void app_lvgl_turnoff_reset(uint8_t ctrl_light)
{
    if (ctrl_light) {
        bsp_lcd_bl_on();
    }

    qm_cancel_delayed_action(&app_lvgl_handle.lvgl_turnoff_work);
    qm_post_delayed_action(&app_lvgl_handle.lvgl_turnoff_work, lvgl_turnoff_handler, NULL, LVGL_TURNOFF_TIME_MS);
}

static void lvgl_task(void *arg) 
{
    lvgl_port_lock(0); // 锁定lvgl接口 
    ui_init();
    lvgl_port_unlock(); // 解锁lvgl接口

    vTaskDelay(500);

    app_lvgl_turnoff_reset(1);

    while (1) {
        lvgl_port_lock(0);
        ui_tick();
        lvgl_port_unlock();

        // 避免过度占用CPU资源
        vTaskDelay(pdMS_TO_TICKS(50)); 
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
        QM_LOGW(LOG_TAG, "esp_vfs_spiffs_register init failed");
        vTaskDelete(NULL);
        return;
    }

    app_lvgl_port_init();

    qm_task_new_to_core(&app_lvgl_handle.lvgl_task, "lvgl_task", lvgl_task, &app_lvgl_handle, 10 * 1024, 4, 1);
}

void app_lvgl_lcd_init(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle)
{
    app_lvgl_handle.lcd_io_handle = io_handle;
    app_lvgl_handle.lcd_panel_handle = panel_handle;
}

void app_lvgl_touch_init(esp_lcd_touch_handle_t tp_handle)
{
    app_lvgl_handle.lcd_tp_handle = tp_handle;
}

static void app_lvgl_port_init(void)
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
