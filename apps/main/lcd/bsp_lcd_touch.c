#include "bsp_lcd_touch.h"
#include "bsp_lcd.h"
#include "app_lvgl.h"

#include "driver/i2c_master.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_touch_cst816s.h"

#define TOUCH_I2C_PORT      I2C_NUM_1
#define TOUCH_I2C_SDA_GPIO  GPIO_NUM_4
#define TOUCH_I2C_SCL_GPIO  GPIO_NUM_5

#define TOUCH_RST_GPIO      GPIO_NUM_7
#define TOUCH_INT_GPIO      GPIO_NUM_6

static esp_lcd_panel_io_handle_t lcd_touch_io_handle = NULL;
static esp_lcd_touch_handle_t lcd_touch_handle = NULL;

qm_err_t bsp_lcd_touch_init(void)
{
    i2c_master_bus_handle_t bus_handle;

    const i2c_master_bus_config_t i2c_bus_conf = {
        .i2c_port = TOUCH_I2C_PORT,
        .sda_io_num = TOUCH_I2C_SDA_GPIO,
        .scl_io_num = TOUCH_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &bus_handle));

    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &lcd_touch_io_handle);

    esp_lcd_touch_config_t touch_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = TOUCH_RST_GPIO,
        .int_gpio_num = TOUCH_INT_GPIO,
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

    esp_lcd_touch_new_i2c_cst816s(lcd_touch_io_handle, &touch_cfg, &lcd_touch_handle);

    app_lvgl_touch_init(lcd_touch_handle);

    return QM_EOK;
}