#include "bsp_lcd.h"
#include "qm_gpio.h"

/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include "audio_thread.h"
#include <esp_mac.h>
#include <esp_spiffs.h>

#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lcd_gc9a01.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_touch_cst816s.h"

#define TEST_LCD_HOST               SPI3_HOST
#define TEST_LCD_H_RES              (240)
#define TEST_LCD_V_RES              (240)
#define TEST_LCD_BIT_PER_PIXEL      (16)

#define TEST_PIN_NUM_LCD_CS         (GPIO_NUM_18)
#define TEST_PIN_NUM_LCD_PCLK       (GPIO_NUM_16)
#define TEST_PIN_NUM_LCD_DATA0      (GPIO_NUM_15)
#define TEST_PIN_NUM_LCD_RST        (GPIO_NUM_8)
#define TEST_PIN_NUM_LCD_DC         (GPIO_NUM_17)

#define TEST_DELAY_TIME_MS          (3000)

#define LOG_TAG "bsp_lcd"

static char *TAG = "gc9a01_test";
static SemaphoreHandle_t refresh_finish = NULL;

static qm_gpio_dev_t bl_gpio = {
    .port = GPIO_NUM_3,
    .config = {
        .mode = QM_GPIO_MODE_OUTPUT,
        .pull_en = QM_GPIO_PULLDOWN_ONLY,
    },
};

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t tp_handle = NULL;

static lv_disp_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

void lcd_init(void);

/***** backlight *****/
uint8_t lv_port_init_flag = 0;
static int bl_brightness_percent = 0;

void bsp_lcd_bl_set(int brightness_percent) {
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    bl_brightness_percent = brightness_percent;
    if(brightness_percent == 0) {
        ESP_LOGI(LOG_TAG, "Turning off LCD backlight");
        esp_lcd_panel_disp_on_off(panel_handle, false);
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_5, false);
        return;
    } else {
        esp_lcd_panel_disp_on_off(panel_handle, true);
    }
    ESP_LOGI(LOG_TAG, "Setting LCD backlight: %d", brightness_percent);
    uint32_t duty_cycle = (1023 * brightness_percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_5, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_5);
}

int bsp_lcd_bl_brightness_percent_get(void) {
    return bl_brightness_percent;
}

void bsp_lcd_bl_on(void) {
    if (bl_brightness_percent != 100) {
        if(lv_port_init_flag == 0) {
            return;
        }
        lvgl_port_lock(0);
        bsp_lcd_bl_set(100);
        lvgl_port_unlock(); 
    }
}

void bsp_lcd_bl_off(void) {
    if (bl_brightness_percent != 0) {
        if(lv_port_init_flag == 0) {
            return;
        }
        lvgl_port_lock(0);
        bsp_lcd_bl_set(0);
        lvgl_port_unlock(); 
    }
}

void bsp_lcd_bl_dim(void) {
    if (bl_brightness_percent != 10) {
        if(lv_port_init_flag == 0) {
            return;
        }
        lvgl_port_lock(0);
        bsp_lcd_bl_set(10);
        lvgl_port_unlock(); 
    }
}

void bsp_lcd_bl_init(void) {
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = bl_gpio.port,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_5,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_2,
        .duty = 0,
        .hpoint = 0
    };

    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_2,
        .freq_hz = 5000,
        .clk_cfg = LEDC_USE_XTAL_CLK
    };

    ledc_timer_config(&LCD_backlight_timer);
    ledc_channel_config(&LCD_backlight_channel);
}

qm_err_t bsp_lcd_init(void)
{
    // init lcd panel
    // ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = GC9A01_PANEL_BUS_SPI_CONFIG(TEST_PIN_NUM_LCD_PCLK, TEST_PIN_NUM_LCD_DATA0,
                                    TEST_LCD_H_RES * 80 * TEST_LCD_BIT_PER_PIXEL / 16);
    spi_bus_initialize(TEST_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // ESP_LOGI(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = GC9A01_PANEL_IO_SPI_CONFIG(TEST_PIN_NUM_LCD_CS, TEST_PIN_NUM_LCD_DC,
            NULL, NULL);
    // Attach the LCD to the SPI bus
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TEST_LCD_HOST, &io_config, &io_handle);

    // ESP_LOGI(TAG, "Install gc9a01 panel driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TEST_PIN_NUM_LCD_RST,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#endif
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,
    };
    esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, false);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    lcd_init();

    return QM_EOK;
}

esp_err_t bsp_lcd_tp_init(void)
{
    i2c_master_bus_handle_t bus_handle;

    const i2c_master_bus_config_t i2c_bus_conf = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = GPIO_NUM_4,
        .scl_io_num = GPIO_NUM_5,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &bus_handle));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle);
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = TEST_LCD_H_RES,
        .y_max = TEST_LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_7,
        .int_gpio_num = GPIO_NUM_6,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .interrupt_callback = NULL,
    };

    return esp_lcd_touch_new_i2c_cst816s(io_handle, &tp_cfg, &tp_handle);
}

void app_lvgl(void)
{
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_affinity = 0;
    lvgl_port_init(&lvgl_cfg);

    /* Add LCD screen */
    ESP_LOGD(LOG_TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = TEST_LCD_H_RES * TEST_LCD_V_RES / 6,
        .double_buffer = 1,
        .trans_size = 0,
        .hres = TEST_LCD_H_RES,
        .vres = TEST_LCD_V_RES,
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
        .handle = tp_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    lvgl_port_task_wake(LVGL_PORT_EVENT_DISPLAY, NULL);
}

static qm_timer_t turnoff_timer = { 0 };
static int turnoff_timeout_tbl[4] = { 30000, 60000, 300000, 2147483647 }; // 30秒, 1分, 5分, 永不 //  { 15000, 30000, 45000, 60000 };

uint32_t bsp_lcd_get_turnoff_timeout(void)
{
    int scr_tmo = get_var_scr_tmo_value(); // 0 - 30秒, 1 - 1分, 2 - 5分, 3 - Never
    if (scr_tmo < 0 || scr_tmo > 3) {
        scr_tmo = 0;
    }
    return turnoff_timeout_tbl[scr_tmo];
}

static void turnoff_timer_handler(qm_timer_t *timer,  void *arg) 
{
    uint32_t timeout = bsp_lcd_get_turnoff_timeout();

    if (get_var_turnoff_value()) {
        bsp_lcd_bl_dim();
        if (!bl_brightness_percent) {
            extern void goto_main_page(void);
            goto_main_page();
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

static void lcd_task(void *arg) 
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/lvgl",
        .partition_label = "lvgl",
        .max_files = 10,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if(ret != ESP_OK) {
        ESP_LOGW(LOG_TAG, "esp_vfs_spiffs_register init failed");
        vTaskDelete(NULL);
        return;
    }
    ret = bsp_lcd_tp_init();
    if(ret != ESP_OK) {
        ESP_LOGW(LOG_TAG, "bsp_lcd_tp_init init failed");
        vTaskDelete(NULL);
        return;
    }
    // bsp_lcd_init();
    bsp_lcd_bl_init();
    app_lvgl();
    lv_port_init_flag = 1;
    
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

void lcd_init(void) 
{
    audio_thread_create(NULL, "lcd_task", lcd_task, NULL, 10 * 1024, 4, true, 1);
}
