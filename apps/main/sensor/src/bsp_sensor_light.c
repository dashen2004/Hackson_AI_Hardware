#include "bsp_sensor_tof.h"
#include "driver/i2c_master.h"

#include "qm_gpio.h"

#define I2C_MASTER_SCL_IO           8
#define I2C_MASTER_SDA_IO           9
#define I2C_MASTER_FREQ_HZ          400 * 1000
#define I2C_MASTER_PORT             0

#define I2C_DEVICE_ADDR             0x5C

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

static qm_gpio_dev_t nc_int_io = {
    .port = 38,
    .config = {
        .mode = QM_GPIO_MODE_INPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    },
};

void i2c_master_init(void)
{
    qm_gpio_init(&nc_int_io);

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_PORT,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_DEVICE_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
}

esp_err_t i2c_write(uint8_t *data, size_t len)
{
    return i2c_master_transmit(dev_handle, data, len, -1);
}

esp_err_t i2c_read(uint8_t *data, size_t len)
{
    return i2c_master_receive(dev_handle, data, len, -1);
}