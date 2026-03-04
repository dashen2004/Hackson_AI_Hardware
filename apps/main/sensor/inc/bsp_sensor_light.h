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

#define TCS34083_I2C_ADDR        0x39

// Command bit
#define TCS_CMD_BIT              0x80

// Registers
#define TCS_ENABLE               0x00
#define TCS_ATIME                0x01
#define TCS_WTIME                0x03
#define TCS_AILTL                0x04
#define TCS_AILTH                0x05
#define TCS_AIHTL                0x06
#define TCS_AIHTH                0x07
#define TCS_STATUS               0x13
#define TCS_CDATAL               0x14
#define TCS_RDATAL               0x16
#define TCS_GDATAL               0x18
#define TCS_BDATAL               0x1A
#define TCS_IRDATAL              0x1C
#define TCS_ID                   0x12
#define TCS_CONTROL              0x0F

// ENABLE bits
#define TCS_ENABLE_PON           0x01
#define TCS_ENABLE_AEN           0x02
#define TCS_ENABLE_AIEN          0x10

typedef struct {
    i2c_master_dev_handle_t dev;
    gpio_num_t int_gpio;

    uint8_t gain;         // 当前增益
    uint8_t atime;        // 积分时间
} tcs34083_t;

qm_err_t bsp_sensor_light_init(void);

#ifdef __cplusplus
}
#endif

#endif