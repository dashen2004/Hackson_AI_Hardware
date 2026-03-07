#include "bsp_sensor_tof.h"
#include "qm_gpio.h"
#include "nds03_platform.h"
#include "nds03_calib.h"
#include "nds03_comm.h"
#include "nds03_data.h"
#include "nds03_dev.h"

static qm_gpio_dev_t tof_int_io = {
    .port = 12,
    .config = {
        .mode = QM_GPIO_MODE_INPUT,
        .pull_en = QM_GPIO_PULLUP_ONLY,
    },
};

static NDS03_Dev_t nds_dev = {
    .dev_pwr_state = NDS03_DISABLE,
    .config = {0},
    .chip_info = {1},
    .data_cnt = 0,
    .ranging_data = {{0}},
    .platform = {
        .i2c_dev_addr = 0x5C,
        .xshut_pin = 0,
    },
};


qm_err_t bsp_sensor_tof_init(void)
{
    // qm_gpio_init(&tof_int_io);
    // qm_gpio_enable_irq(&tof_int_io, QM_GPIO_INTR_NEGEDGE, tof_int_handler, NULL);
    nds03_platform_init(&nds_dev.platform, NULL);
    
    QM_LOGI("1", "Waiting for NDS03 boot up...");
    NDS03_WaitDeviceBootUp(&nds_dev);

    QM_LOGI("1", "NDS03 boot up success, fw version: 0x%08x", nds_dev.chip_info.fw_version);
    NDS03_InitDevice(&nds_dev);

    QM_LOGI("1", "NDS03 init success");

    // NDS03_OffsetCalibration(&nds_dev);

    // while (1) {
    //     NDS03_GetSingleRangingData(&nds_dev);

    //     QM_LOGI("1", "dist: %d", nds_dev.ranging_data[0].depth);

    //     qm_msleep(1000);
    // }
    

    return QM_EOK;
}