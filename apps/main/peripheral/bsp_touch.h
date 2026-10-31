#ifndef BSP_TOUCH_H
#define BSP_TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

/* Single touch module SIG input, active high by default. */
#define BSP_TOUCH_GPIO          5
#define BSP_TOUCH_ACTIVE_LEVEL  1

typedef void (*bsp_touch_press_callback_t)(void *arg);

qm_err_t bsp_touch_init(void);
uint8_t bsp_touch_get_level(void);
uint8_t bsp_touch_is_active(void);
void bsp_touch_set_press_callback(bsp_touch_press_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif
