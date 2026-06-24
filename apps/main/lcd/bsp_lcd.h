#ifndef BSP_LCD_H
#define BSP_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "esp_lcd_panel_ops.h"

#define LCD_H_RES   (240)
#define LCD_V_RES   (240)

#define LCD_BL_BRIGHTNESS_MAX   (100)
#define LCD_BL_BRIGHTNESS_DIM   (100)
#define LCD_BL_BRIGHTNESS_MIN   (0)

qm_err_t bsp_lcd_init(void);
bool_t bsp_lcd_is_ready(void);

void bsp_lcd_bl_on(void);
void bsp_lcd_bl_off(void);
void bsp_lcd_bl_dim(void);

esp_lcd_panel_handle_t bsp_lcd_get_panel_handle(void);

#ifdef __cplusplus
}
#endif

#endif