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
    .dev_pwr_state = NDS03_ENABLE,
    .config = {
        .range_frame_time_us = 500 * 1000,
    },
    .chip_info = {
        .fw_version = 0x01,
    },
    .data_cnt = 1,
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

    // printf("ret: %d\n", NDS03_OffsetCalibration(&nds_dev));
    // 需要注释掉NDS03_GetSingleRangingData函数中的 CHECK_RET(NDS03_WaitforDataVal(pNxDevice, NDS03_DEPTH_DATA_FLAG, timeout_ms));

    // while (1) {
    //     NDS03_GetSingleRangingData(&nds_dev);

    //     QM_LOGI("1", "dist: %d %d %d %d", nds_dev.ranging_data[0].depth, nds_dev.ranging_data[1].depth,
    //                                         nds_dev.ranging_data[2].depth, nds_dev.ranging_data[3].depth);

    //     qm_msleep(1000);
    // }

    return QM_EOK;
}

qm_err_t bsp_sensor_calib(void)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    NDS03_Dev_t     *pNxDevice = &nds_dev;
    /* 测距次数为20次 */
    int32_t     i,  cnt = 20;
    int32_t     depth_sum, depth_aver;

    // 初始值
    pNxDevice->platform.i2c_dev_addr = NDS03_DEFAULT_SLAVE_ADDR;
    pNxDevice->platform.xshut_pin = 0;
    
    if(NDS03_ERROR_NONE == ret){
        /* 初始化平台，包含I2C初始化 */
        ret = nds03_platform_init(&pNxDevice->platform, NULL);
        if(NDS03_ERROR_NONE != ret){
            printf("nds03_platform init error\n");
            return ret;
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 初始化设备变量的I2C地址 */
        pNxDevice->platform.i2c_dev_addr = NDS03_DEFAULT_SLAVE_ADDR;
        /* 循环等待设备启动, 若模组或者I2C读写函数有问题则会报错 */
        ret = NDS03_WaitDeviceBootUp(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_WaitDeviceBootUp error\r\n");
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 初始化模组设备 */
        ret = NDS03_InitDevice(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_InitDevice error!!\r\n");
            return NDS03_ERROR_INIT;
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 将设备处于500mm处 */
        /* 串扰标定 */
        ret = NDS03_XtalkCalibration(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            if(NDS03_CALIB_ERROR_XTALK_EXCESSIVE == ret){
                NX_PRINTF("[Warning] NDS03_XtalkCalibration xtalk excessive!!\r\n");
            }
            else{
                NX_PRINTF("NDS03_XtalkCalibration error!!\r\n");
            }
        }
    }
    
    if(NDS03_ERROR_NONE == ret){
        /* Offset标定 */
        ret = NDS03_OffsetCalibrationAtDepth(pNxDevice, 500);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_OffsetCalibrationAtDepth error!!\r\n");
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 获取平均值 */
        depth_sum = 0;
        for(i = 0; i < cnt; i++) {
            if(NDS03_ERROR_NONE != NDS03_GetSingleRangingData(pNxDevice)){
                NX_PRINTF("NDS03_GetSingleRangingData error!!\r\n");
                break;
            }
            depth_sum = depth_sum + (int32_t)(uint32_t)pNxDevice->ranging_data[0].depth;
        }
        if(NDS03_ERROR_NONE == ret){
            depth_aver = depth_sum / cnt;
            if(depth_aver < 480 || depth_aver > 520){
                NX_PRINTF("NDS03 calibration fail!!\r\n");
            }
            else{
                NX_PRINTF("NDS03 calibration success\r\n");
            }
        }
    }

    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("NDS03_Calibration_Example failed, error code: %d\r\n", ret);
    }

    /* 释放平台 */
    ret = nds03_platform_uninit(&pNxDevice->platform, NULL);
    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("nds03_platform uninit error\r\n");
    }

    return ret;
}


//定义打印模式
typedef enum {
    PRINT_SINGLE,    // 打印单个目标
    PRINT_ALL         // 打印所有有效目标
} PrintMode_t;

/**
 * @brief 单次测量接口
 * 
 * @param pNxDevice 模组设备
 * @param printMode  指定输出结果的模式，可以是 PRINT_SINGLE 或 PRINT_ALL
 */
NDS03_Error SG_RangingTest(NDS03_Dev_t *pNxDevice, PrintMode_t printMode) {
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint8_t         targetNum = 0; // 配置的目标数量
    uint8_t         confiThreshold = 10; // 有效目标的置信度阈值
    uint8_t         printed = 0; // 已打印信息

    ret = NDS03_GetTargetNum(pNxDevice, &targetNum);
    if (ret != NDS03_ERROR_NONE) {
        NX_PRINTF("Failed to get number of targets, error code: %d\r\n", ret);
        return ret;
    }
    // 获取测量数据
    // CHECK_RET(NDS03_GetSingleRangingData(pNxDevice));
    NDS03_GetSingleRangingData(pNxDevice);

    // 根据打印模式筛选并打印目标
    switch (printMode) {
        case PRINT_SINGLE:
            NX_PRINTF("Depth:%5d mm,Confi:%3d,Count:%5d\r\n",
                        pNxDevice->ranging_data[0].depth,
                        pNxDevice->ranging_data[0].confi,
                        pNxDevice->ranging_data[0].count);
            break;
        case PRINT_ALL:
            for (uint8_t i = 0; i < targetNum; ++i) {
                if (pNxDevice->ranging_data[i].confi > confiThreshold) {
                    NX_PRINTF("Target%2d->Depth:%5dmm,Confi:%3d,Count:%5d\r\n",
                            i + 1,
                            pNxDevice->ranging_data[i].depth,
                            pNxDevice->ranging_data[i].confi,
                            pNxDevice->ranging_data[i].count);
                    printed = 1; // 已打印
                }
            }
            if (!printed) {
                NX_PRINTF("No targets with confidence above threshold.\r\n");
            }
            break;
        default:
            NX_PRINTF("Please configure the print mode.\r\n");
            break;
    }
    return ret;
}

/**
 * @brief NDS03单次测量示例主函数
 * 
 * @return int32_t 
 */
qm_err_t bsp_sensor_single_measure(void)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    NDS03_Dev_t     *pNxDevice = &nds_dev;
    /* 循环测距次数为100次 */
    int32_t cnt = 100;
    
    // 初始值
    pNxDevice->platform.i2c_dev_addr = NDS03_DEFAULT_SLAVE_ADDR;
    pNxDevice->platform.xshut_pin = 0;

    if(NDS03_ERROR_NONE == ret){
        /* 初始化平台，包含I2C初始化 */
        ret = nds03_platform_init(&pNxDevice->platform, NULL);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("nds03_platform init error\n");
            return ret;
        }
    }
    
    if(NDS03_ERROR_NONE == ret){
        /* 初始化设备变量的IIC地址 */
        pNxDevice->platform.i2c_dev_addr = NDS03_DEFAULT_SLAVE_ADDR;
        /* 循环等待设备启动, 若模组或者IIC读写函数有问题则会报错 */
        ret = NDS03_WaitDeviceBootUp(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_WaitDeviceBootUp error\r\n");
            ret = NDS03_ERROR_BOOT;
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 初始化模组设备 */
        if(NDS03_ERROR_NONE != NDS03_InitDevice(pNxDevice)){
            NX_PRINTF("NDS03_InitDevice error!!\r\n");
            ret = NDS03_ERROR_INIT;
        }
    }

    /* 循环测量 */
    while((cnt--) && (NDS03_ERROR_NONE == ret)) {
        //指定输出结果的模式，可以是 PRINT_SINGLE 或 PRINT_ALL
        PrintMode_t currentMode = PRINT_ALL;
        ret = SG_RangingTest(pNxDevice,currentMode);
    }

    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("NDS03_SingleRanging_Example failed, error code: %d\r\n", ret);
    }

    /* 释放平台 */
    ret = nds03_platform_uninit(&pNxDevice->platform, NULL);
    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("nds03_platform uninit error\n");
    }

    return ret;
}
