#include "driver/i2c_master.h"
#include "bsp_i2c.h"
#include "qm_gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#define LOG_TAG "I2C"

#define I2C_SDA 9
#define I2C_SCL 10

static uint8_t i2c_port_0_inited = 0;

static i2c_master_bus_handle_t iic_bus_handle;

qm_err_t bsp_i2c_init(void)
{
    if (i2c_port_0_inited) {
        return QM_EOK;
    }

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL,
        .sda_io_num = I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    if (i2c_new_master_bus(&bus_config, &iic_bus_handle) != ESP_OK) {
        return -QM_ERROR;
    }

    i2c_port_0_inited = 1;

    return QM_EOK;
}

qm_err_t bsp_i2c_add_device(i2c_device_config_t *dev_config, i2c_master_dev_handle_t *dev_handle)
{
    if (i2c_master_bus_add_device(iic_bus_handle, dev_config, dev_handle) != ESP_OK) {
        return -QM_ERROR;
    }

    return QM_EOK;
}

qm_err_t bsp_i2c_master_write(i2c_master_dev_handle_t dev_handle, const uint8_t *data, size_t size, int timeout_ms)
{
    if (i2c_master_transmit(dev_handle, data, size, timeout_ms) != ESP_OK) {
        QM_LOGE(LOG_TAG, "I2C write failed");
        return -QM_ERROR;
    }
    
    return QM_EOK;
}

qm_err_t bsp_i2c_master_read(i2c_master_dev_handle_t dev_handle, uint8_t *write_data, size_t write_size, uint8_t *read_data, size_t read_size, int timeout_ms)
{
    if (i2c_master_transmit_receive(dev_handle, write_data, write_size, read_data, read_size, timeout_ms) != ESP_OK) {
        QM_LOGE(LOG_TAG, "I2C read failed");
        return -QM_ERROR;
    }

    return QM_EOK;
}
