/**
 * @file NDS03_SingleRanging_Example.c
 * @author tongsheng.tang
 * @brief 单次测量示例
 * @version 2.x.x
 * @date 2025-06
 * 
 * @copyright Copyright (c) 2025, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 * @license BSD 3-Clause License
 *          This file is part of the NDS03 SDK and is licensed under the BSD 3-Clause License.
 *          You may obtain a copy of the license in the project root directory, in the file named LICENSE.
 * 
 */

#include <stdio.h>
#include "nds03_platform.h"
#include "nds03_comm.h"
#include "nds03_dev.h"
#include "nds03_data.h"
#include "nds03_calib.h"
#include "nds03_def.h"

/** 模组设备 */
NDS03_Dev_t g_nds03_device;

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
    CHECK_RET(NDS03_GetSingleRangingData(pNxDevice));

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
int32_t main(void)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    NDS03_Dev_t     *pNxDevice = &g_nds03_device;
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

