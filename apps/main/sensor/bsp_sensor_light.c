#include "bsp_sensor_light.h"
#include "driver/i2c_master.h"
#include "qm_gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "bsp_i2c.h"
#include "qm_task_psram.h"

#define LOG_TAG "TCS34083"
#define LUX_HIGH_HINT_VALUE BSP_SENSOR_LUX_HIGH_HINT
#define LUX_LOW_HINT_VALUE  BSP_SENSOR_LUX_LOW_HINT
#define TCS34083_I2C_HZ             (100000)
#define TCS34083_READ_INTERVAL_MS   (10000)

typedef struct {
    i2c_master_dev_handle_t dev;
    gpio_num_t int_gpio;

    uint8_t gain;
    uint8_t atime;
    uint16_t astep;

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
    .device_address = TCS34083_I2C_ADDR,
    .scl_speed_hz = TCS34083_I2C_HZ,
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

static void tcs34083_disable(tcs34083_t *tcs)
{
    if (tcs != NULL && tcs->dev != NULL) {
        tcs_write(tcs, TCS_ENABLE, 0);
    }
}

static void tcs34083_stop(const char *reason)
{
    g_light_sensor_ready = false;
    tcs34083_disable(&g_tcs_handle);
    QM_LOGI(LOG_TAG, "stop: %s", reason);
}

static qm_err_t tcs_read(tcs34083_t *tcs, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t buf[1] = { reg };
    return bsp_i2c_master_read_quiet(tcs->dev, buf, 1, data, len, -1);
}

static qm_err_t tcs_read_chip_id(tcs34083_t *tcs, uint8_t *id)
{
    return tcs_read(tcs, TCS_ID, id, 1);
}

static float tcs34083_again_multiplier(uint8_t again)
{
    static const float table[] = {
        0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f,
        128.0f, 256.0f, 512.0f, 1024.0f, 2048.0f,
    };

    if (again >= (sizeof(table) / sizeof(table[0]))) {
        return 1.0f;
    }

    return table[again];
}

static uint32_t tcs34083_adc_fullscale(const tcs34083_t *tcs)
{
    return (uint32_t)(tcs->atime + 1) * (tcs->astep + 1);
}

static qm_err_t tcs34083_set_gain(tcs34083_t *tcs, uint8_t gain)
{
    uint8_t cfg1 = 0;

    if (gain > TCS34083_AGC_GAIN_MAX) {
        gain = TCS34083_AGC_GAIN_MAX;
    }

    if (tcs_read(tcs, TCS_CFG1, &cfg1, 1) == QM_EOK) {
        cfg1 = (cfg1 & 0xE0) | (gain & 0x1F);
    } else {
        cfg1 = gain;
    }

    if (tcs_write(tcs, TCS_CFG1, cfg1) != QM_EOK) {
        return -QM_ERROR;
    }

    tcs->gain = gain;
    return QM_EOK;
}

static bool tcs34083_is_saturated(uint8_t astatus, uint16_t clear, uint32_t fullscale)
{
    if (astatus & TCS_ASTATUS_ASAT_STATUS) {
        return true;
    }

    if (fullscale == 0) {
        return false;
    }

    return clear >= (fullscale * TCS34083_AGC_HIGH_PCT / 1000);
}

static void tcs34083_auto_gain_for_next(tcs34083_t *tcs, uint16_t clear, uint32_t fullscale)
{
    uint32_t lo = fullscale * TCS34083_AGC_LOW_PCT / 1000;

    if (clear < lo && tcs->gain < TCS34083_AGC_GAIN_MAX) {
        tcs34083_set_gain(tcs, tcs->gain + 1);
    }
}

static uint32_t tcs34083_integration_ms(const tcs34083_t *tcs)
{
    return tcs34083_adc_fullscale(tcs) / 360;
}

static void tcs34083_wait_integration(const tcs34083_t *tcs)
{
    vTaskDelay(pdMS_TO_TICKS(tcs34083_integration_ms(tcs) + 5));
}

static void tcs34083_dump_regs(tcs34083_t *tcs, const char *tag)
{
    uint8_t enable = 0;
    uint8_t atime = 0;
    uint8_t astep_l = 0;
    uint8_t astep_h = 0;
    uint8_t cfg1 = 0;
    uint8_t status2 = 0;

    tcs_read(tcs, TCS_ENABLE, &enable, 1);
    tcs_read(tcs, TCS_ATIME, &atime, 1);
    tcs_read(tcs, TCS_ASTEP_L, &astep_l, 1);
    tcs_read(tcs, TCS_ASTEP_H, &astep_h, 1);
    tcs_read(tcs, TCS_CFG1, &cfg1, 1);
    tcs_read(tcs, TCS_STATUS2, &status2, 1);

    QM_LOGI(LOG_TAG,
            "%s: EN=0x%02x ATIME=0x%02x ASTEP=0x%02x%02x CFG1=0x%02x STATUS2=0x%02x",
            tag, enable, atime, astep_h, astep_l, cfg1, status2);
}

static qm_err_t tcs34083_wait_avalid(tcs34083_t *tcs, int timeout_ms)
{
    uint8_t status = 0;
    int elapsed = 0;
    int read_err = 0;

    while (elapsed < timeout_ms) {
        if (tcs_read(tcs, TCS_STATUS2, &status, 1) != QM_EOK) {
            read_err++;
            vTaskDelay(pdMS_TO_TICKS(5));
            elapsed += 5;
            continue;
        }
        if (status & TCS_STATUS2_AVALID) {
            return QM_EOK;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
        elapsed += 5;
    }

    if (read_err > 0) {
        QM_LOGD(LOG_TAG, "STATUS2 read err=%d last=0x%02x", read_err, status);
    }

    return -QM_ETIMEOUT;
}

static qm_err_t tcs34083_init(tcs34083_t *tcs)
{
    uint8_t buf[TCS34083_ALS_DATA_BYTES];
    uint8_t cfg1 = 0;
    uint16_t clear, red, green, blue;

    /* Configure ASTEP/ATIME before PON, per datasheet recommended 50ms integration. */
    tcs->astep = TCS34083_ASTEP_RECOMMEND;
    tcs->atime = TCS34083_ATIME_RECOMMEND;
    tcs->gain = TCS34083_AGAIN_RECOMMEND;

    if (tcs_write(tcs, TCS_ASTEP_L, (uint8_t)(tcs->astep & 0xFF)) != QM_EOK) {
        return -QM_EINIT;
    }
    if (tcs_write(tcs, TCS_ASTEP_H, (uint8_t)(tcs->astep >> 8)) != QM_EOK) {
        return -QM_EINIT;
    }
    if (tcs_write(tcs, TCS_ATIME, tcs->atime) != QM_EOK) {
        return -QM_EINIT;
    }

    if (tcs_read(tcs, TCS_CFG1, &cfg1, 1) == QM_EOK) {
        cfg1 = (cfg1 & 0xE0) | (tcs->gain & 0x1F);
    } else {
        cfg1 = tcs->gain;
    }
    if (tcs_write(tcs, TCS_CFG1, cfg1) != QM_EOK) {
        return -QM_EINIT;
    }

    if (tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON) != QM_EOK) {
        return -QM_EINIT;
    }
    vTaskDelay(pdMS_TO_TICKS(3));

    if (tcs_write(tcs, TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN) != QM_EOK) {
        return -QM_EINIT;
    }

    if (tcs34083_wait_avalid(tcs, 120) != QM_EOK) {
        QM_LOGD(LOG_TAG, "AVALID not set, wait integration %ums",
                (unsigned)tcs34083_integration_ms(tcs));
        tcs34083_wait_integration(tcs);
    }

    if (tcs_read(tcs, TCS_ASTATUS, buf, sizeof(buf)) != QM_EOK) {
        tcs34083_dump_regs(tcs, "init als read fail");
        tcs34083_disable(tcs);
        return -QM_EINIT;
    }

    clear = (uint16_t)buf[2] << 8 | buf[1];
    red = (uint16_t)buf[4] << 8 | buf[3];
    green = (uint16_t)buf[6] << 8 | buf[5];
    blue = (uint16_t)buf[8] << 8 | buf[7];
    tcs->gain = buf[0] & 0x0F;

    if (clear == 0 && red == 0 && green == 0 && blue == 0) {
        tcs34083_dump_regs(tcs, "init als all zero");
        tcs34083_disable(tcs);
        return -QM_EINIT;
    }

    QM_LOGI(LOG_TAG, "init ok: clear=%u again=%u tint=%ums",
            clear, tcs->gain, (unsigned)tcs34083_integration_ms(tcs));

    return QM_EOK;
}

static qm_err_t tcs34083_read_als(tcs34083_t *tcs,
                                uint16_t *clear,
                                uint16_t *red,
                                uint16_t *green,
                                uint16_t *blue,
                                uint16_t *wideband,
                                uint8_t *astatus)
{
    uint8_t buf[TCS34083_ALS_DATA_BYTES];

    if (tcs34083_wait_avalid(tcs, 120) != QM_EOK) {
        tcs34083_wait_integration(tcs);
    }

    /* Read ASTATUS first to latch ADATA0..5, then 13 bytes 0x94..0xA0. */
    if (tcs_read(tcs, TCS_ASTATUS, buf, sizeof(buf)) != QM_EOK) {
        return -QM_ERROR;
    }

    if (astatus != NULL) {
        *astatus = buf[0];
    }

    *clear = (uint16_t)buf[2] << 8 | buf[1];
    *red = (uint16_t)buf[4] << 8 | buf[3];
    *green = (uint16_t)buf[6] << 8 | buf[5];
    *blue = (uint16_t)buf[8] << 8 | buf[7];
    *wideband = (uint16_t)buf[10] << 8 | buf[9];
    tcs->gain = buf[0] & 0x0F;

    return QM_EOK;
}

static float tcs34083_calculate_lux(const tcs34083_t *tcs,
                                    uint8_t latched_gain,
                                    uint16_t red,
                                    uint16_t green,
                                    uint16_t blue,
                                    uint16_t clear)
{
    if (clear == 0) {
        return 0;
    }

    float integration_ms = (float)tcs34083_integration_ms(tcs);
    float gain_multiplier = tcs34083_again_multiplier(latched_gain);
    float cpl = (integration_ms * gain_multiplier) / TCS34083_LUX_DGF;

    if (cpl <= 0.01f) {
        return 0;
    }

    float ir = (float)(red + green + blue - clear) / 2.0f;
    if (ir < 0) {
        ir = 0;
    }

    float r2 = (float)red - ir;
    float g2 = (float)green - ir;
    float b2 = (float)blue - ir;

    if (r2 < 0) {
        r2 = 0;
    }
    if (g2 < 0) {
        g2 = 0;
    }
    if (b2 < 0) {
        b2 = 0;
    }

    float lux = (0.136f * r2 + 1.000f * g2 - 0.444f * b2) / cpl;
    if (lux < 0) {
        lux = 0;
    }

    return lux;
}

static void light_read_task(void *arg)
{
    static float lux = 0;
    uint8_t fail_cnt = 0;

    while (g_light_sensor_ready) {
        if (bsp_sensor_light_read(&lux) != QM_EOK) {
            fail_cnt++;
            if (fail_cnt >= 3) {
                tcs34083_stop("als read failed");
                break;
            }
        } else {
            fail_cnt = 0;

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
        }

        vTaskDelay(pdMS_TO_TICKS(TCS34083_READ_INTERVAL_MS));
    }
}

qm_err_t bsp_sensor_light_init(void)
{
    if (bsp_i2c_init() != QM_EOK) {
        QM_LOGI(LOG_TAG, "I2C bus init failed, skip light sensor");
        return -QM_EINIT;
    }

    bsp_i2c_scan_bus();

    bool probed = bsp_i2c_probe_device(TCS34083_I2C_ADDR, 100);
    QM_LOGI(LOG_TAG, "probe 0x%02x: %s", TCS34083_I2C_ADDR, probed ? "ACK" : "NACK");
    if (!probed) {
        QM_LOGI(LOG_TAG, "TCS34083 not found, skip light sensor");
        return -QM_EINIT;
    }

    if (bsp_i2c_add_device(&tcs_dev_config, &tcs_dev_handle) != QM_EOK) {
        QM_LOGI(LOG_TAG, "I2C add device failed, skip light sensor");
        return -QM_EINIT;
    }

    g_tcs_handle.dev = tcs_dev_handle;
    g_tcs_handle.int_gpio = light_int.port;

    {
        uint8_t id = 0;

        if (tcs_read_chip_id(&g_tcs_handle, &id) != QM_EOK) {
            QM_LOGI(LOG_TAG, "chip id read failed at 0x%02x", TCS34083_I2C_ADDR);
            return -QM_EINIT;
        }

        QM_LOGI(LOG_TAG, "chip id at 0x%02x: 0x%02x (expect 0x%02x)",
                TCS34083_I2C_ADDR, id, TCS_CHIP_ID);
        if (id != TCS_CHIP_ID) {
            QM_LOGI(LOG_TAG, "TCS34083 chip id mismatch, skip light sensor");
            return -QM_EINIT;
        }
    }

    if (tcs34083_init(&g_tcs_handle) != QM_EOK) {
        QM_LOGI(LOG_TAG, "TCS34083 init failed, skip light sensor");
        return -QM_EINIT;
    }

    g_light_sensor_ready = true;
    TaskHandle_t light_th = NULL;
    if (qm_task_create_psram_to_core(light_read_task, "light_read_task",
                                     qm_task_stack_bytes_to_words(2 * 1024),
                                     &g_tcs_handle, 10, 1, &light_th) != pdPASS) {
        QM_LOGI(LOG_TAG, "light_read_task create failed");
        tcs34083_stop("task create failed");
        return -QM_EINIT;
    }
    g_tcs_handle.light_task.hdl = light_th;

    return QM_EOK;
}

qm_err_t bsp_sensor_light_read(float *lux)
{
    uint16_t red, green, blue, clear, wideband;
    uint8_t astatus = 0;
    uint8_t latched_gain = 0;
    uint32_t fullscale;
    bool saturated = false;
    int retry;

    if (!g_light_sensor_ready || lux == NULL) {
        return -QM_ERROR;
    }

    fullscale = tcs34083_adc_fullscale(&g_tcs_handle);

    for (retry = 0; retry < 8; retry++) {
        if (tcs34083_read_als(&g_tcs_handle, &clear, &red, &green, &blue, &wideband,
                              &astatus) != QM_EOK) {
            return -QM_ERROR;
        }

        latched_gain = astatus & 0x0F;
        saturated = tcs34083_is_saturated(astatus, clear, fullscale);

        if (saturated && latched_gain > 0) {
            QM_LOGI(LOG_TAG, "als saturated C=%u again=%u, reduce gain",
                    clear, latched_gain);
            if (tcs34083_set_gain(&g_tcs_handle, latched_gain - 1) != QM_EOK) {
                return -QM_ERROR;
            }
            tcs34083_wait_integration(&g_tcs_handle);
            continue;
        }

        break;
    }

    if (saturated) {
        QM_LOGI(LOG_TAG, "als saturated at min gain, C=%u", clear);
    }

    *lux = tcs34083_calculate_lux(&g_tcs_handle, latched_gain, red, green, blue, clear);

    if (!saturated) {
        tcs34083_auto_gain_for_next(&g_tcs_handle, clear, fullscale);
    }

    QM_LOGI(LOG_TAG,
            "als C=%u R=%u G=%u B=%u WB=%u again=%u tint=%ums lux=%.2f%s",
            clear, red, green, blue, wideband,
            latched_gain,
            (unsigned)tcs34083_integration_ms(&g_tcs_handle),
            *lux,
            saturated ? " SAT" : "");

    return QM_EOK;
}
