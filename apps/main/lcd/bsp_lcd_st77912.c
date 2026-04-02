#include "bsp_lcd.h"
#include "app_lvgl.h"

#include "qm_gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"

#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_st77912.h"
#include "esp_lvgl_port.h"

#define LOG_TAG "bsp_lcd"

#define LCD_HOST               SPI3_HOST
#define LCD_BIT_PER_PIXEL      (16)

#define LCD_PIN_NUM_CS         (GPIO_NUM_18)
#define LCD_PIN_NUM_PCLK       (GPIO_NUM_16)
#define LCD_PIN_NUM_DATA0      (GPIO_NUM_15)
#define LCD_PIN_NUM_RST        (GPIO_NUM_8)
#define LCD_PIN_NUM_DC         (GPIO_NUM_17)

static qm_gpio_dev_t lcd_bl_gpio = {
    .port = GPIO_NUM_3,
    .config = {
        .mode = QM_GPIO_MODE_OUTPUT,
        .pull_en = QM_GPIO_PULLDOWN_ONLY,
    },
};

static int lcd_bl_brightness_percent = 0;
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
static esp_lcd_panel_handle_t lcd_panel_handle = NULL;

static void lcd_bl_init(void);
static void lcd_bl_set(int brightness_percent);

bool_t bsp_lcd_is_ready(void) 
{
    if (lcd_bl_brightness_percent != LCD_BL_BRIGHTNESS_MIN) {
        return QM_TRUE;
    } else {
        return QM_FALSE;
    }
}

void bsp_lcd_bl_on(void) {
    lvgl_port_lock(0);
    lcd_bl_set(LCD_BL_BRIGHTNESS_MAX);
    lvgl_port_unlock(); 
}

void bsp_lcd_bl_off(void) {
    lvgl_port_lock(0);
    lcd_bl_set(LCD_BL_BRIGHTNESS_MIN);
    lvgl_port_unlock(); 
}

void bsp_lcd_bl_dim(void) {
    lvgl_port_lock(0);
    lcd_bl_set(LCD_BL_BRIGHTNESS_DIM);
    lvgl_port_unlock(); 
}

qm_err_t bsp_lcd_init(void)
{
    // init lcd panel
    // ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = ST77912_PANEL_BUS_SPI_CONFIG(LCD_PIN_NUM_PCLK, LCD_PIN_NUM_DATA0,
                                    LCD_H_RES * 80 * LCD_BIT_PER_PIXEL / 16);
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // ESP_LOGI(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = ST77912_PANEL_IO_SPI_CONFIG(LCD_PIN_NUM_CS, LCD_PIN_NUM_DC,
            NULL, NULL);
    // Attach the LCD to the SPI bus
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &lcd_io_handle);

    // ESP_LOGI(TAG, "Install ST77916 panel driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#endif
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
    };

    esp_lcd_new_panel_st77912(lcd_io_handle, &panel_config, &lcd_panel_handle);
    esp_lcd_panel_reset(lcd_panel_handle);
    esp_lcd_panel_init(lcd_panel_handle);
    esp_lcd_panel_invert_color(lcd_panel_handle, true);
    // esp_lcd_panel_mirror(lcd_panel_handle, true, false);
    esp_lcd_panel_disp_on_off(lcd_panel_handle, true);

    lcd_bl_init();
    lcd_bl_set(LCD_BL_BRIGHTNESS_MIN);

    app_lvgl_lcd_init(lcd_io_handle, lcd_panel_handle);

    return QM_EOK;
}

static void lcd_bl_init(void)
{
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = lcd_bl_gpio.port,
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

static void lcd_bl_set(int brightness_percent)
{
    if (brightness_percent > LCD_BL_BRIGHTNESS_MAX) {
        brightness_percent = LCD_BL_BRIGHTNESS_MAX;
    }

    if (brightness_percent < LCD_BL_BRIGHTNESS_MIN) {
        brightness_percent = LCD_BL_BRIGHTNESS_MIN;
    }

    if (lcd_bl_brightness_percent == brightness_percent) {
        return;
    }

    QM_LOGI(LOG_TAG, "Setting LCD backlight: %d", brightness_percent);
    lcd_bl_brightness_percent = brightness_percent;

    if (brightness_percent == LCD_BL_BRIGHTNESS_MIN) {
        esp_lcd_panel_disp_on_off(lcd_panel_handle, false);

        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_5, false);
    } else {
        esp_lcd_panel_disp_on_off(lcd_panel_handle, true);

        uint32_t duty_cycle = (1023 * brightness_percent) / 100;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_5, duty_cycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_5);
    }
}
