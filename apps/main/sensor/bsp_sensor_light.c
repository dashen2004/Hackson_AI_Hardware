#include "bsp_sensor_light.h"
#include "driver/i2c_master.h"
#include "qm_gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "bsp_i2c.h"

#define LOG_TAG "TCS34083"
#define LUX_HIGH_HINT_VALUE (650)
#define LUX_LOW_HINT_VALUE  (100)

typedef struct {
    i2c_master_dev_handle_t dev;
    gpio_num_t int_gpio;

    uint8_t gain;
    uint8_t atime;

    qm_task_t light_task;
} tcs34083_t;

static tcs34083_t g_tcs_handle = {0};

static bool g_light_sensor_ready = false;

static uint8_t lux_high_cnt = 0;
static uint8_t lux_high_flag = 0;
static uint8_t lux_low_cnt = 0;
static uint8_t lux_low_flag = 0;

static i2c_master_dev_handle_t tcs_dev_handle;
static i2c_device_config_t tcs_dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x39,
    .scl_speed_hz = 400000,
};

static qm_gpio_dev_t light_int = {
    .port = GPIO_NUM_11,
    .config = {
        .mode = QM_GPIO_MODE_INPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    },
};

static qm_err_t tcs_write(tcs34083_t *tcs, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { reg, data };
    return bsp_i2c_master_write(tcs->dev, buf, 2, -1);
}

static qm_err_t tcs_read_chip_id(tcs34083_t *tcs, uint8_t *id)
{
    uint8_t buf[1] = { TCS_ID };
    return bsp_i2c_master_read_quiet(tcs->dev, buf, 1, id, 1, -1);
}

static qm_err_t tcs_read(tcs34083_t *tcs, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t buf[1] = { reg };
    return bsp_i2c_master_read(tcs->dev, buf, 1, data, len, -1);
}


static qm_err_t tcs34083_init(tcs34083_t *tcs)
{
    uint8_t id = 0;

    if (tcs_read_chip_id(tcs, &id) != QM_EOK) {
        return -QM_EINIT;
    }

    if (id != TCS_CHIP_ID) {
        QM_LOGI(LOG_TAG, "chip id invalid: 0x%02x", id);
        return -QM_EINIT;
    }

    // 上电
    tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 设置积分时间 50ms
    tcs->atime = 0xEE;
    tcs_write(tcs, TCS_ATIME, 29);

    // 设置增益 1x
    tcs->gain = 0x03;
    tcs_write(tcs, TCS_GAIN, tcs->gain);

    // 启用 ALS
    tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN);

    return QM_EOK;
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
    tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN);
}

static float tcs34083_calculate_lux(tcs34083_t *tcs,
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

    float ir = (float)(r + g + b - c) / 2.0f;
    if (ir < 0) {
        ir = 0;
    }

    float r2 = r - ir;
    if (r2 < 0) {
        r2 = 0;
    }

    float g2 = g - ir;
    if (g2 < 0) {
        g2 = 0;
    }

    float b2 = b - ir;
    if (b2 < 0) {
        b2 = 0;
    }

    float integration_ms = (256 - tcs->atime) * 2.78f;
    float gain_multiplier;

    switch (tcs->gain)
    {
        case 0: gain_multiplier = 0.5f; break;
        case 1: gain_multiplier = 1.0f; break;
        case 2: gain_multiplier = 2.0f; break;
        case 3: gain_multiplier = 4.0f; break;
        case 4: gain_multiplier = 8.0f; break;
        default: gain_multiplier = pow(2, tcs->gain - 1); break;
    }

    float cpl = (integration_ms * gain_multiplier) / 155.0f;

    if (cpl <= 0.01f) {
        return 0;
    }

    float lux = (0.136f * r2 + 1.000f * g2 - 0.444f * b2) / cpl;

    if (lux < 0) {
        lux = 0;
    }

    return lux;
}

static void tcs34083_auto_gain(tcs34083_t *tcs, uint16_t c)
{
    if (c > 25000 && tcs->gain > 0) {
        tcs->gain--;
        tcs_write(tcs, TCS_GAIN, tcs->gain);
    } else if (c < 10000 && tcs->gain < 5) {
        tcs->gain++;
        tcs_write(tcs, TCS_GAIN, tcs->gain);
    }

    return;
}

static void light_read_task(void *arg)
{
    static float lux = 0;

    while (1) {
        bsp_sensor_light_read(&lux);

        // QM_LOGD(LOG_TAG, "lux: %.2f", lux);

        if (lux > LUX_HIGH_HINT_VALUE) {
            lux_low_cnt = 0;

            lux_high_cnt++;
            if (lux_high_cnt > 5) {
                QM_LOGI(LOG_TAG, "lux too high: %.2f", lux);

                lux_high_cnt = 0;

                lux_high_flag = 1;
                lux_low_flag = 0;
            }
        } else if (lux < LUX_LOW_HINT_VALUE) {
            lux_high_cnt = 0;

            lux_low_cnt++;
            if (lux_low_cnt > 5) {
                QM_LOGI(LOG_TAG, "lux too low: %.2f", lux);

                lux_low_cnt = 0;

                lux_low_flag = 1;
                lux_high_flag = 0;
            }
        } else {
            lux_high_cnt = 0;
            lux_low_cnt = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

qm_err_t bsp_sensor_light_init(void)
{
    if (bsp_i2c_init() != QM_EOK) {
        QM_LOGI(LOG_TAG, "I2C bus init failed, skip light sensor");
        return -QM_EINIT;
    }

    if (!bsp_i2c_probe_device(TCS34083_I2C_ADDR, 100)) {
        QM_LOGI(LOG_TAG, "TCS34083 not found, skip light sensor");
        return -QM_EINIT;
    }

    if (bsp_i2c_add_device(&tcs_dev_config, &tcs_dev_handle) != QM_EOK) {
        QM_LOGI(LOG_TAG, "I2C add device failed, skip light sensor");
        return -QM_EINIT;
    }

    g_tcs_handle.dev = tcs_dev_handle;
    g_tcs_handle.int_gpio = light_int.port;
    g_tcs_handle.gain = 0;
    g_tcs_handle.atime = 0;

    if (tcs34083_init(&g_tcs_handle) != QM_EOK) {
        QM_LOGI(LOG_TAG, "TCS34083 init failed, skip light sensor");
        return -QM_EINIT;
    }

    g_light_sensor_ready = true;
    qm_task_new_to_core(&g_tcs_handle.light_task, "light_read_task", light_read_task, &g_tcs_handle, 2 * 1024, 10, 1);

    return QM_EOK;
}

qm_err_t bsp_sensor_light_read(float *lux)
{
    if (!g_light_sensor_ready || lux == NULL) {
        return -QM_ERROR;
    }

    uint16_t r,g,b,c,ir;

    tcs34083_read_rgb(&g_tcs_handle,&r,&g,&b,&c,&ir);

    tcs34083_auto_gain(&g_tcs_handle, c);

    *lux = tcs34083_calculate_lux(&g_tcs_handle,r,g,b,c);

    return QM_EOK;
}
