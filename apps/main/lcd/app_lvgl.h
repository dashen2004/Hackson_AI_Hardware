#ifndef APP_LVGL_H
#define APP_LVGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#include "actions.h"
#include "fonts.h"
#include "screens.h"
#include "ui.h"
#include "vars.h"

#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_touch_cst816s.h"

void app_lvgl_init(void);

void app_lvgl_lcd_init(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle);
void app_lvgl_touch_init(esp_lcd_touch_handle_t tp_handle);

uint32_t bsp_lcd_get_turnoff_timeout(void);
void turnoff_timer_reset(uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif