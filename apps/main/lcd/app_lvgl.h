#ifndef APP_LVGL_H
#define APP_LVGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_touch_cst816s.h"

void app_lvgl_init(void);
void app_lvgl_lcd_init(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle);
void app_lvgl_touch_init(esp_lcd_touch_handle_t tp_handle);

void app_lvgl_turnoff_reset(uint8_t light_flag);

#ifdef __cplusplus
}
#endif

#endif