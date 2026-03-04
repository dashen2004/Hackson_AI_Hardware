#ifndef BSP_I2C_H
#define BSP_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

qm_err_t bsp_i2c_init(void);
qm_err_t bsp_i2c_add_device(i2c_device_config_t *dev_config, i2c_master_dev_handle_t *dev_handle);
qm_err_t bsp_i2c_master_write(i2c_master_dev_handle_t dev_handle, const uint8_t *data, size_t size, int timeout_ms);
qm_err_t bsp_i2c_master_read(i2c_master_dev_handle_t dev_handle, uint8_t *write_data, size_t write_size, uint8_t *read_data, size_t read_size, int timeout_ms);
#ifdef __cplusplus
}
#endif

#endif