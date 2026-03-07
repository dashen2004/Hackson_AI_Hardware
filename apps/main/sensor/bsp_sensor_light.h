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

#define TCS_CHIP_ID         (0x18)
#define TCS34083_I2C_ADDR   (0x39)

// Registers
#define TCS_ENABLE          (0x80)
#define TCS_ATIME           (0x81)
#define TCS_WTIME           (0x83)
#define TCS_AILTL           (0x84)
#define TCS_AILTH           (0x85)
#define TCS_AIHTL           (0x86)
#define TCS_AIHTH           (0x87)
#define TCS_ID              (0x92)
#define TCS_STATUS          (0x93)
#define TCS_CDATAL          (0x95)
#define TCS_RDATAL          (0x97)
#define TCS_GDATAL          (0x99)
#define TCS_BDATAL          (0x9B)
#define TCS_IRDATAL         (0x9D)
#define TCS_GAIN            (0xAA)
#define TCS_AGC             (0xB1)

// ENABLE bits
#define TCS_ENABLE_PON      (1 << 0)
#define TCS_ENABLE_AEN      (1 << 1)
#define TCS_ENABLE_FDEN     (1 << 6)

qm_err_t bsp_sensor_light_init(void);
qm_err_t bsp_sensor_light_read(float *lux);

#ifdef __cplusplus
}
#endif

#endif