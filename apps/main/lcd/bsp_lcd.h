#ifndef BSP_LCD_H
#define BSP_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#include "actions.h"
#include "fonts.h"
#include "screens.h"
#include "ui.h"
#include "vars.h"

qm_err_t bsp_lcd_init(void);

void bsp_lcd_bl_on(void);
void bsp_lcd_bl_off(void);
void bsp_lcd_bl_dim(void);

uint32_t bsp_lcd_get_turnoff_timeout(void);
void turnoff_timer_reset(uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif