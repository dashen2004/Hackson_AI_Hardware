#include "bsp_sensor_light.h"
#include "driver/i2c_master.h"
#include "qm_gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "bsp_i2c.h"

#define LOG_TAG "TCS34083"

static i2c_master_dev_handle_t tcs_dev_handle;
static i2c_device_config_t tcs_dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x39,
    .scl_speed_hz = 400000,
};

static qm_err_t tcs_write(tcs34083_t *tcs, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { TCS_CMD_BIT | reg, data };
    return bsp_i2c_master_write(tcs->dev, buf, 2, -1);
}

static qm_err_t tcs_read(tcs34083_t *tcs, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t cmd = TCS_CMD_BIT | reg;
    return bsp_i2c_master_read(tcs->dev, &cmd, 1, data, len, -1);
}

/* ---------------- Lux 计算 ---------------- */
double tcs34083_calculate_lux(tcs34083_t *tcs,
                             uint16_t r,
                             uint16_t g,
                             uint16_t b,
                             uint16_t c)
{
    if (c >= 65000) {
        c = 65000;
    }

    if (c == 0) {
        return 0;
    }

    float ir = ((float)r + g + b - c);
    if (ir < 0) ir = 0;

    float r2 = r - ir;
    float g2 = g - ir;
    float b2 = b - ir;

    float integration_ms = (256 - tcs->atime) * 2.4f;
    float gain_multiplier;

    switch (tcs->gain)
    {
        case 0: gain_multiplier = 1.0f; break;
        case 1: gain_multiplier = 4.0f; break;
        case 2: gain_multiplier = 16.0f; break;
        case 3: gain_multiplier = 64.0f; break;
        default: gain_multiplier = 1.0f; break;
    }

    float cpl = (integration_ms * gain_multiplier) / 310.0f;

    if (cpl <= 0.01f)
        return 0;

    float lux = (0.136f * r2 +
                 1.000f * g2 -
                 0.444f * b2) / cpl;

    if (lux < 0)
        lux = 0;

    return lux;
}

static void tcs34083_auto_gain(tcs34083_t *tcs, uint16_t c)
{
    if (c > 50000 && tcs->gain > 0) {
        tcs->gain--;
        tcs_write(tcs, TCS_CONTROL, tcs->gain);
    } else if (c < 10000 && tcs->gain < 3) {
        tcs->gain++;
        tcs_write(tcs, TCS_CONTROL, tcs->gain);
    }

    return;
}

/* ---------------- API ---------------- */
static void tcs34083_init(tcs34083_t *tcs, gpio_num_t int_gpio)
{
    // 读 ID
    uint8_t id;
    tcs_read(tcs, TCS_ID, &id, 1);
    printf("TCS ID = 0x%02X\n", id);

    // 上电
    tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 设置积分时间 50ms
    tcs->atime = 0xFE;
    tcs_write(tcs, TCS_ATIME, tcs->atime);

    // 设置增益 1x
    tcs->gain = 0x00;
    tcs_write(tcs, TCS_CONTROL, tcs->gain);

    // 启用 ALS
    tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN);

    tcs->int_gpio = int_gpio;

    return;
}

static void tcs34083_read_rgb(tcs34083_t *tcs,
                            uint16_t *r,
                            uint16_t *g,
                            uint16_t *b,
                            uint16_t *c,
                            uint16_t *ir)
{
    uint8_t buf[10];
    tcs_read(tcs, TCS_CDATAL, buf, 10);

    *c  = buf[1]<<8 | buf[0];
    *r  = buf[3]<<8 | buf[2];
    *g  = buf[5]<<8 | buf[4];
    *b  = buf[7]<<8 | buf[6];
    *ir = buf[9]<<8 | buf[8];

    return;
}

static void tcs34083_sleep(tcs34083_t *tcs)
{
    tcs_write(tcs, TCS_ENABLE, 0);
}

static void tcs34083_wakeup(tcs34083_t *tcs)
{
    tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN);
}

static void tcs34083_enable_interrupt(tcs34083_t *tcs)
{
    tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN | TCS_ENABLE_AIEN);
}

qm_err_t bsp_sensor_light_init(void)
{
    bsp_i2c_init();
    bsp_i2c_add_device(&tcs_dev_config, &tcs_dev_handle);

    tcs34083_t tcs = {
        .dev = tcs_dev_handle,
        .int_gpio = GPIO_NUM_11,
        .gain = 0,
        .atime = 0
    };

    tcs34083_init(&tcs, tcs.int_gpio);

    // uint16_t r,g,b,c,ir;
    // while (1) {

    //     tcs34083_read_rgb(&tcs, &r,&g,&b,&c,&ir);

    //     printf("R=%u G=%u B=%u C=%u IR=%u\n", r,g,b,c,ir);

    //     tcs34083_auto_gain(&tcs, c);

    //     double lux = tcs34083_calculate_lux(&tcs, r, g, b, c);

    //     printf("Lux: %.2f\n", lux);

    //     vTaskDelay(pdMS_TO_TICKS(500));
    // }

    return QM_EOK;
}
