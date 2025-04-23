/*
* SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: CC0-1.0
*/

#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include "sdkconfig.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "esp_chip_info.h"
//#include "esp_flash.h"
//#include "esp_system.h"
#include "esp_log.h"

#include "driver/ledc.h"
#include "driver/i2c_master.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_st77916.h"
#include "esp_lcd_touch_cst816s.h"
#include "esp_lvgl_port.h"
// #include "esp_timer.h"

#include "audio_thread.h"

#include "qm.h"
#include "qm_kv.h"

#include "lvgl.h"
#include "ui.h"
#include "vars.h"

#define LOG_TAG "app"

#define EXAMPLE_LCD_HOST            (SPI2_HOST)
#define EXAMPLE_PIN_NUM_LCD_PCLK    (11)
#define EXAMPLE_PIN_NUM_LCD_CS      (12)
#define EXAMPLE_PIN_NUM_LCD_DATA0   (13)
#define EXAMPLE_PIN_NUM_LCD_DATA1   (14)
#define EXAMPLE_PIN_NUM_LCD_DATA2   (21)
#define EXAMPLE_PIN_NUM_LCD_DATA3   (47)
#define EXAMPLE_PIN_NUM_LCD_RST     (48)
#define EXAMPLE_PIN_NUM_LCD_BL      (3)

#define EXAMPLE_TP_PORT             (I2C_NUM_1)
#define EXAMPLE_PIN_NUM_TP_SDA      (39)
#define EXAMPLE_PIN_NUM_TP_SCL      (38)
#define EXAMPLE_PIN_NUM_TP_RST      (42)
#define EXAMPLE_PIN_NUM_TP_INT      (40)

#define EXAMPLE_LCD_H_RES           (360)
#define EXAMPLE_LCD_V_RES           (360)
#define EXAMPLE_LCD_BIT_PER_PIXEL   (16)

static SemaphoreHandle_t refresh_finish = NULL;

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_touch_handle_t tp_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

static lv_disp_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

IRAM_ATTR static bool notify_refresh_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    BaseType_t need_yield = pdFALSE;

    xSemaphoreGiveFromISR(refresh_finish, &need_yield);
    return (need_yield == pdTRUE);
}

static int bl_brightness_percent = 0;

void bsp_lcd_bl_set(int brightness_percent) {
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    bl_brightness_percent = brightness_percent;
    ESP_LOGI(LOG_TAG, "Setting LCD backlight: %d%%", brightness_percent);
    uint32_t duty_cycle = (1023 * brightness_percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}

int bsp_lcd_bl_brightness_percent_get(void) {
    return bl_brightness_percent;
}

void bsp_lcd_bl_off(void) {
    if (bl_brightness_percent) {
        bsp_lcd_bl_set(0);
    }
}

void bsp_lcd_bl_on(void) {
    if (!bl_brightness_percent) {
        bsp_lcd_bl_set(100);
    }
}

void bsp_lcd_bl_init(void) {
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = EXAMPLE_PIN_NUM_LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
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
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&LCD_backlight_timer);
    ledc_channel_config(&LCD_backlight_channel);

    bsp_lcd_bl_off(); // bsp_lcd_bl_on();
}

void bsp_lcd_tp_init(void)
{
    i2c_master_bus_handle_t bus_handle;

    const i2c_master_bus_config_t i2c_bus_conf = {
        .i2c_port = EXAMPLE_TP_PORT,
        .sda_io_num = EXAMPLE_PIN_NUM_TP_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_TP_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &bus_handle));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle);
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = EXAMPLE_PIN_NUM_TP_RST,
        .int_gpio_num = EXAMPLE_PIN_NUM_TP_INT,
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

    esp_err_t err = esp_lcd_touch_new_i2c_cst816s(io_handle, &tp_cfg, &tp_handle);
}

static const st77916_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xF0, (uint8_t[]){0x28}, 1, 0},
    {0xF2, (uint8_t[]){0x28}, 1, 0},
    {0x73, (uint8_t[]){0xF0}, 1, 0},
    {0x7C, (uint8_t[]){0xD1}, 1, 0},
    {0x83, (uint8_t[]){0xE0}, 1, 0},
    {0x84, (uint8_t[]){0x61}, 1, 0},
    {0xF2, (uint8_t[]){0x82}, 1, 0},
    {0xF0, (uint8_t[]){0x00}, 1, 0},
    {0xF0, (uint8_t[]){0x01}, 1, 0},
    {0xF1, (uint8_t[]){0x01}, 1, 0},
    {0xB0, (uint8_t[]){0x56}, 1, 0},
    {0xB1, (uint8_t[]){0x4D}, 1, 0},
    {0xB2, (uint8_t[]){0x24}, 1, 0},
    {0xB4, (uint8_t[]){0x87}, 1, 0},
    {0xB5, (uint8_t[]){0x44}, 1, 0},
    {0xB6, (uint8_t[]){0x8B}, 1, 0},
    {0xB7, (uint8_t[]){0x40}, 1, 0},
    {0xB8, (uint8_t[]){0x86}, 1, 0},
    {0xBA, (uint8_t[]){0x00}, 1, 0},
    {0xBB, (uint8_t[]){0x08}, 1, 0},
    {0xBC, (uint8_t[]){0x08}, 1, 0},
    {0xBD, (uint8_t[]){0x00}, 1, 0},
    {0xC0, (uint8_t[]){0x80}, 1, 0},
    {0xC1, (uint8_t[]){0x10}, 1, 0},
    {0xC2, (uint8_t[]){0x37}, 1, 0},
    {0xC3, (uint8_t[]){0x80}, 1, 0},
    {0xC4, (uint8_t[]){0x10}, 1, 0},
    {0xC5, (uint8_t[]){0x37}, 1, 0}, 
    {0xC6, (uint8_t[]){0xA9}, 1, 0}, 
    {0xC7, (uint8_t[]){0x41}, 1, 0}, 
    {0xC8, (uint8_t[]){0x01}, 1, 0}, 
    {0xC9, (uint8_t[]){0xA9}, 1, 0}, 
    {0xCA, (uint8_t[]){0x41}, 1, 0}, 
    {0xCB, (uint8_t[]){0x01}, 1, 0}, 
    {0xD0, (uint8_t[]){0x91}, 1, 0}, 
    {0xD1, (uint8_t[]){0x68}, 1, 0}, 
    {0xD2, (uint8_t[]){0x68}, 1, 0}, 
    {0xF5, (uint8_t[]){0x00,0xA5}, 2, 0},
    {0xDD, (uint8_t[]){0x4F}, 1, 0},
    {0xDE, (uint8_t[]){0x4F}, 1, 0},
    {0xF1, (uint8_t[]){0x10}, 1, 0},
    {0xF0, (uint8_t[]){0x00}, 1, 0},
    {0xF0, (uint8_t[]){0x02}, 1, 0},
    {0xE0, (uint8_t[]){0xF0,0x0A,0x10,0x09,0x09,0x36,0x35,0x33,0x4A,0x29,0x15,0x15,0x2E,0x34}, 14, 0},
    {0xE1, (uint8_t[]){0xF0,0x0A,0x0F,0x08,0x08,0x05,0x34,0x33,0x4A,0x39,0x15,0x15,0x2D,0x33}, 14, 0},
    {0xF0, (uint8_t[]){0x10}, 1, 0},
    {0xF3, (uint8_t[]){0x10}, 1, 0},
    {0xE0, (uint8_t[]){0x07}, 1, 0},
    {0xE1, (uint8_t[]){0x00}, 1, 0},
    {0xE2, (uint8_t[]){0x00}, 1, 0},
    {0xE3, (uint8_t[]){0x00}, 1, 0},
    {0xE4, (uint8_t[]){0xE0}, 1, 0},
    {0xE5, (uint8_t[]){0x06}, 1, 0},
    {0xE6, (uint8_t[]){0x21}, 1, 0},
    {0xE7, (uint8_t[]){0x01}, 1, 0},
    {0xE8, (uint8_t[]){0x05}, 1, 0},
    {0xE9, (uint8_t[]){0x02}, 1, 0},
    {0xEA, (uint8_t[]){0xDA}, 1, 0},
    {0xEB, (uint8_t[]){0x00}, 1, 0},
    {0xEC, (uint8_t[]){0x00}, 1, 0},
    {0xED, (uint8_t[]){0x0F}, 1, 0},
    {0xEE, (uint8_t[]){0x00}, 1, 0},
    {0xEF, (uint8_t[]){0x00}, 1, 0},
    {0xF8, (uint8_t[]){0x00}, 1, 0},
    {0xF9, (uint8_t[]){0x00}, 1, 0},
    {0xFA, (uint8_t[]){0x00}, 1, 0},
    {0xFB, (uint8_t[]){0x00}, 1, 0},
    {0xFC, (uint8_t[]){0x00}, 1, 0},
    {0xFD, (uint8_t[]){0x00}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xFF, (uint8_t[]){0x00}, 1, 0},
    {0x60, (uint8_t[]){0x40}, 1, 0},
    {0x61, (uint8_t[]){0x04}, 1, 0},
    {0x62, (uint8_t[]){0x00}, 1, 0},
    {0x63, (uint8_t[]){0x42}, 1, 0},
    {0x64, (uint8_t[]){0xD9}, 1, 0},
    {0x65, (uint8_t[]){0x00}, 1, 0},
    {0x66, (uint8_t[]){0x00}, 1, 0},
    {0x67, (uint8_t[]){0x00}, 1, 0},
    {0x68, (uint8_t[]){0x00}, 1, 0},
    {0x69, (uint8_t[]){0x00}, 1, 0},
    {0x6A, (uint8_t[]){0x00}, 1, 0},
    {0x6B, (uint8_t[]){0x00}, 1, 0},
    {0x70, (uint8_t[]){0x40}, 1, 0},
    {0x71, (uint8_t[]){0x03}, 1, 0},
    {0x72, (uint8_t[]){0x00}, 1, 0},
    {0x73, (uint8_t[]){0x42}, 1, 0},
    {0x74, (uint8_t[]){0xD8}, 1, 0},
    {0x75, (uint8_t[]){0x00}, 1, 0},
    {0x76, (uint8_t[]){0x00}, 1, 0},
    {0x77, (uint8_t[]){0x00}, 1, 0},
    {0x78, (uint8_t[]){0x00}, 1, 0},
    {0x79, (uint8_t[]){0x00}, 1, 0},
    {0x7A, (uint8_t[]){0x00}, 1, 0},
    {0x7B, (uint8_t[]){0x00}, 1, 0},
    {0x80, (uint8_t[]){0x48}, 1, 0},
    {0x81, (uint8_t[]){0x00}, 1, 0},
    {0x82, (uint8_t[]){0x06}, 1, 0},
    {0x83, (uint8_t[]){0x02}, 1, 0},
    {0x84, (uint8_t[]){0xD6}, 1, 0},
    {0x85, (uint8_t[]){0x04}, 1, 0},
    {0x86, (uint8_t[]){0x00}, 1, 0},
    {0x87, (uint8_t[]){0x00}, 1, 0},
    {0x88, (uint8_t[]){0x48}, 1, 0},
    {0x89, (uint8_t[]){0x00}, 1, 0},
    {0x8A, (uint8_t[]){0x08}, 1, 0},
    {0x8B, (uint8_t[]){0x02}, 1, 0},
    {0x8C, (uint8_t[]){0xD8}, 1, 0},
    {0x8D, (uint8_t[]){0x04}, 1, 0},
    {0x8E, (uint8_t[]){0x00}, 1, 0},
    {0x8F, (uint8_t[]){0x00}, 1, 0},
    {0x90, (uint8_t[]){0x48}, 1, 0},
    {0x91, (uint8_t[]){0x00}, 1, 0},
    {0x92, (uint8_t[]){0x0A}, 1, 0},
    {0x93, (uint8_t[]){0x02}, 1, 0},
    {0x94, (uint8_t[]){0xDA}, 1, 0},
    {0x95, (uint8_t[]){0x04}, 1, 0},
    {0x96, (uint8_t[]){0x00}, 1, 0},
    {0x97, (uint8_t[]){0x00}, 1, 0},
    {0x98, (uint8_t[]){0x48}, 1, 0},
    {0x99, (uint8_t[]){0x00}, 1, 0},
    {0x9A, (uint8_t[]){0x0C}, 1, 0},
    {0x9B, (uint8_t[]){0x02}, 1, 0},
    {0x9C, (uint8_t[]){0xDC}, 1, 0}, 
    {0x9D, (uint8_t[]){0x04}, 1, 0}, 
    {0x9E, (uint8_t[]){0x00}, 1, 0}, 
    {0x9F, (uint8_t[]){0x00}, 1, 0}, 
    {0xA0, (uint8_t[]){0x48}, 1, 0}, 
    {0xA1, (uint8_t[]){0x00}, 1, 0}, 
    {0xA2, (uint8_t[]){0x05}, 1, 0}, 
    {0xA3, (uint8_t[]){0x02}, 1, 0}, 
    {0xA4, (uint8_t[]){0xD5}, 1, 0}, 
    {0xA5, (uint8_t[]){0x04}, 1, 0}, 
    {0xA6, (uint8_t[]){0x00}, 1, 0}, 
    {0xA7, (uint8_t[]){0x00}, 1, 0}, 
    {0xA8, (uint8_t[]){0x48}, 1, 0}, 
    {0xA9, (uint8_t[]){0x00}, 1, 0}, 
    {0xAA, (uint8_t[]){0x07}, 1, 0}, 
    {0xAB, (uint8_t[]){0x02}, 1, 0}, 
    {0xAC, (uint8_t[]){0xD7}, 1, 0}, 
    {0xAD, (uint8_t[]){0x04}, 1, 0}, 
    {0xAE, (uint8_t[]){0x00}, 1, 0}, 
    {0xAF, (uint8_t[]){0x00}, 1, 0}, 
    {0xB0, (uint8_t[]){0x48}, 1, 0}, 
    {0xB1, (uint8_t[]){0x00}, 1, 0}, 
    {0xB2, (uint8_t[]){0x09}, 1, 0}, 
    {0xB3, (uint8_t[]){0x02}, 1, 0}, 
    {0xB4, (uint8_t[]){0xD9}, 1, 0}, 
    {0xB5, (uint8_t[]){0x04}, 1, 0}, 
    {0xB6, (uint8_t[]){0x00}, 1, 0}, 
    {0xB7, (uint8_t[]){0x00}, 1, 0}, 
    {0xB8, (uint8_t[]){0x48}, 1, 0}, 
    {0xB9, (uint8_t[]){0x00}, 1, 0}, 
    {0xBA, (uint8_t[]){0x0B}, 1, 0},
    {0xBB, (uint8_t[]){0x02}, 1, 0},
    {0xBC, (uint8_t[]){0xDB}, 1, 0},
    {0xBD, (uint8_t[]){0x04}, 1, 0},
    {0xBE, (uint8_t[]){0x00}, 1, 0},
    {0xBF, (uint8_t[]){0x00}, 1, 0},
    {0xC0, (uint8_t[]){0x10}, 1, 0},
    {0xC1, (uint8_t[]){0x47}, 1, 0},
    {0xC2, (uint8_t[]){0x56}, 1, 0},
    {0xC3, (uint8_t[]){0x65}, 1, 0},
    {0xC4, (uint8_t[]){0x74}, 1, 0}, 
    {0xC5, (uint8_t[]){0x88}, 1, 0}, 
    {0xC6, (uint8_t[]){0x99}, 1, 0}, 
    {0xC7, (uint8_t[]){0x01}, 1, 0}, 
    {0xC8, (uint8_t[]){0xBB}, 1, 0}, 
    {0xC9, (uint8_t[]){0xAA}, 1, 0}, 
    {0xD0, (uint8_t[]){0x10}, 1, 0}, 
    {0xD1, (uint8_t[]){0x47}, 1, 0}, 
    {0xD2, (uint8_t[]){0x56}, 1, 0}, 
    {0xD3, (uint8_t[]){0x65}, 1, 0}, 
    {0xD4, (uint8_t[]){0x74}, 1, 0}, 
    {0xD5, (uint8_t[]){0x88}, 1, 0}, 
    {0xD6, (uint8_t[]){0x99}, 1, 0}, 
    {0xD7, (uint8_t[]){0x01}, 1, 0}, 
    {0xD8, (uint8_t[]){0xBB}, 1, 0}, 
    {0xD9, (uint8_t[]){0xAA}, 1, 0}, 
    {0xF3, (uint8_t[]){0x01}, 1, 0}, 
    {0xF0, (uint8_t[]){0x00}, 1, 0}, 
    {0x21, (uint8_t[]){0x00}, 0, 0}, 
    {0x11, (uint8_t[]){0x00}, 0, 120}, 
    {0x29, (uint8_t[]){0x00}, 0, 0}, 
};

void bsp_lcd_init(void) {
    ESP_LOGI(LOG_TAG, "Initialize QSPI bus");
    const spi_bus_config_t buscfg = ST77916_PANEL_BUS_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_PCLK,
                                                                EXAMPLE_PIN_NUM_LCD_DATA0,
                                                                EXAMPLE_PIN_NUM_LCD_DATA1,
                                                                EXAMPLE_PIN_NUM_LCD_DATA2,
                                                                EXAMPLE_PIN_NUM_LCD_DATA3,
                                                                EXAMPLE_LCD_H_RES * 10 * sizeof(uint16_t));
    ESP_ERROR_CHECK(spi_bus_initialize(EXAMPLE_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(LOG_TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = ST77916_PANEL_IO_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_CS, NULL, NULL);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(LOG_TAG, "Install ST77916 panel driver");
    const st77916_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,         // Uncomment these line if use custom initialization commands
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st77916_lcd_init_cmd_t),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,     // Implemented by LCD command `36h`
        .bits_per_pixel = EXAMPLE_LCD_BIT_PER_PIXEL,    // Implemented by LCD command `3Ah` (16/18)
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st77916(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);
}

void app_lvgl(void)
{
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    /* Add LCD screen */
    ESP_LOGD(LOG_TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * 180 * sizeof(uint16_t),
        .double_buffer = 0,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_spiram = 1,
            .swap_bytes = 1,
            .buff_dma = 1,
            .full_refresh = 1,
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    
    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = tp_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
}

static uint8_t wifi_icon_state = 0;

#include "qm_time.h"

static void periodic_timer_callback(void* arg) {
    qm_time_t t = qm_time(NULL);
    struct qm_tm *tm = qm_localtime(&t);
    // time_t s = esp_timer_get_time() / 1000 / 1000;
    // struct tm *t = localtime(&s);

    char temp[33];
    sprintf(temp, "%02d:%02d", tm->tm_hour, tm->tm_min);
    lv_label_set_text(objects.text_time, temp);
    switch (tm->tm_wday) {
        case 0: lv_label_set_text(objects.text_weekday, "星期日"); break;
        case 1: lv_label_set_text(objects.text_weekday, "星期一"); break;
        case 2: lv_label_set_text(objects.text_weekday, "星期二"); break;
        case 3: lv_label_set_text(objects.text_weekday, "星期三"); break;
        case 4: lv_label_set_text(objects.text_weekday, "星期四"); break;
        case 5: lv_label_set_text(objects.text_weekday, "星期五"); break;
        case 6: lv_label_set_text(objects.text_weekday, "星期六"); break;
    }
    // sprintf(temp, "%04d%s%02d%s%02d%s", tm->tm_year + 100, "年", tm->tm_mon + 1, "月", tm->tm_mday, "日");
    sprintf(temp, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    lv_label_set_text(objects.text_date, temp);

    if (objects.image_wifi1 != NULL) {
        static bool_t last_connect = false;
        // 改变图标
        lvgl_port_lock(0);
        extern bool_t get_connect(void);
        if (get_connect() != last_connect) {
            last_connect = get_connect();
            if (last_connect) {
                lv_image_set_src(objects.image_wifi1, "/lvgl/images/wifi_connected.bin");
            } else {
                lv_image_set_src(objects.image_wifi1, "/lvgl/images/wifi_disconnected.bin");
            }
        }
        lv_image_set_src(objects.image_weather, "/lvgl/images/weather_day_sunny.bin");
        lvgl_port_unlock();
        wifi_icon_state ^= 1;
    }
}

#include "esp_spiffs.h"
#include "bsp_audio.h"
#include "qm_audio_ws.h"

#define EMOTION_NUM 21

static char emotions[EMOTION_NUM][16] = {
    "neutral", // 中性
    "happy", // 开心
    "funny", // 有趣
    "laughing", // 大笑
    "sad", // 悲伤
    "angry", // 生气
    "crying", // 哭泣
    "loving", // 爱
    "embarrassed", // 尴尬
    "surprised", // 惊讶
    "shocked", // 震惊
    "thinking", // 思考
    "winking", // 眨眼
    "cool", // 酷
    "relaxed", // 放松
    "delicious", // 美味
    "kissy", // 亲吻
    "confident", // 自信
    "silly", // 愚蠢
    "sleepy", // 困倦
    "confused", // 困惑
};

static int8_t get_emotion_idx(char *s) {
    for (uint8_t i = 0; i < EMOTION_NUM; i++) {
        if (strcmp(emotions[i], s) == 0) {
            return i + 1;
        }
    }
    return 0;
}

static void audio_ws_recv_event_cb(qm_audio_ws_recv_evt_t *event, void *user_data) {
    // switch (event->recv_type) {
    //     case QM_AUDIO_WS_RECV_TYPE_LLM: {
    //         int8_t idx = get_emotion_idx(event->data.llm.emotion);
    //         if (idx) {
    //             set_var_emotion(idx);
    //         }
    //         break;
    //     }
    //     case QM_AUDIO_WS_RECV_TYPE_TTS: {
    //         set_var_subtitles(event->data.tts.text);
    //         break;
    //     }
    //     default: {
    //         break;
    //     }
    // }
}

static qm_timer_t turnoff_timer = { 0 };
static int        turnoff_timeout_tbl[4] = { 30000, 60000, 300000, 2147483647 }; // 30秒, 1分, 5分, 永不 //  { 15000, 30000, 45000, 60000 };

void turnoff_timer_reset(void) {
    set_var_turnoff_value(qm_now_ms());
    bsp_lcd_bl_on();
}

static void turnoff_timer_handler(qm_timer_t *timer,  void *arg) {
    int scr_tmo = get_var_scr_tmo_value(); // 0 - 30秒, 1 - 1分, 2 - 5分, 3 - Never
    if (scr_tmo < 0 || scr_tmo > 3) {
        scr_tmo = 0;
    }
    // printf("%ld, %ld, %d\n", qm_now_ms(), get_var_turnoff_value(), turnoff_timeout_tbl[scr_tmo]);
    if (qm_now_ms() - get_var_turnoff_value() >= turnoff_timeout_tbl[scr_tmo]) {
        bsp_lcd_bl_off();
        if (!bl_brightness_percent) {
            extern void goto_main_page(void);
            goto_main_page();
        }
    }
}

static void lcd_task(void *arg) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/lvgl",
        .partition_label = "lvgl",
        .max_files = 10,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    bsp_lcd_tp_init();
    bsp_lcd_init();
    bsp_lcd_bl_init(); 
    app_lvgl();
    
    lvgl_port_lock(0); // 锁定lvgl接口 
    ui_init();
    lvgl_port_unlock(); // 解锁lvgl接口

    vTaskDelay(500);

    bsp_lcd_bl_on();

    set_var_turnoff_value(qm_now_ms());
    qm_timer_new  (&turnoff_timer, turnoff_timer_handler, NULL, 1000, 1);
    qm_timer_start(&turnoff_timer);

    bsp_audio_resgiter(audio_ws_recv_event_cb, NULL);

    while (1) {
        lvgl_port_lock(0); // 锁定lvgl接口，以保护UI更新过程。 
        ui_tick();
        lvgl_port_unlock(); // 解锁lvgl接口以允许其他任务访问。 
        vTaskDelay(pdMS_TO_TICKS(100)); // 函数延迟1毫秒，以避免过度占用CPU资源。
    }
}

void lcd_init(void) {
    audio_thread_create(NULL, "lcd_task", lcd_task, NULL, 8 * 1024, 11, true, 1); // 提高优先级, 抢ram!!!
}
