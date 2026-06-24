#ifndef BSP_SENSOR_LIGHT_H
#define BSP_SENSOR_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

#define TCS_CHIP_ID                 (0x18)
#define TCS34083_I2C_ADDR           (0x39)

/* TCS34083 register map (datasheet tcs34083m) */
#define TCS_ENABLE                  (0x80)
#define TCS_ATIME                   (0x81)
#define TCS_ID                      (0x92)
#define TCS_STATUS2                 (0xA3)
#define TCS_ASTATUS                 (0x94)
#define TCS_CFG1                    (0xAA)
#define TCS_ASTEP_L                 (0xCA)
#define TCS_ASTEP_H                 (0xCB)

#define TCS_ENABLE_PON              (1 << 0)
#define TCS_ENABLE_AEN              (1 << 1)
#define TCS_STATUS2_AVALID          (1 << 6)

#define TCS34083_ASTEP_RECOMMEND    (599)
#define TCS34083_ATIME_RECOMMEND    (29)
#define TCS34083_AGAIN_RECOMMEND    (3)     /* 4x */
#define TCS34083_ALS_DATA_BYTES     (13)    /* ASTATUS + ADATA0..5 */

qm_err_t bsp_sensor_light_init(void);
qm_err_t bsp_sensor_light_read(float *lux);

#ifdef __cplusplus
}
#endif

#endif
