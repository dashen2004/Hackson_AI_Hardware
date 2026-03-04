/**
 * @file NDS03_ContiunousRanging_Example.c
 * @author tongsheng.tang
 * @brief 连续测量示例
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

/**
 * @brief 单次测量接口
 * 
 * @param pNxDevice 模组设备
 */
NDS03_Error SG_RangingTest(NDS03_Dev_t *pNxDevice)
{
    NDS03_Error ret = NDS03_ERROR_NONE;
    /* 获取测量数据 */
    ret = NDS03_GetContinuousRangingData(pNxDevice);
    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("NDS03_GetContinuousRangingData error!!\r\n");
    }
    else{
        NX_PRINTF("depth:%d mm, confi:%d, count:%d\r\n", pNxDevice->ranging_data[0].depth, pNxDevice->ranging_data[0].confi, pNxDevice->ranging_data[0].count);
    }
    
    return ret;
}

/**
 * @brief NDS03连续测量主函数示例
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
            printf("nds03_platform init error\n");
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
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 初始化模组设备 */
        ret = NDS03_InitDevice(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_InitDevice error!!\r\n");
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 连续模式开启测量 */
        ret = NDS03_StartContinuousMeasurement(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_StartContinuousMeasurement error!!\r\n");
        }
    }
    /* 循环测量 */
    while((cnt--) && (NDS03_ERROR_NONE == ret)) {
        ret = SG_RangingTest(pNxDevice);
    }
    if(NDS03_ERROR_NONE == ret){
        /* 连续模式关闭测量 */
        ret = NDS03_StopContinuousMeasurement(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_StopContinuousMeasurement error!!\r\n");
        }
    }

    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("NDS03_ContiunousRanging_Example failed, error code: %d\r\n", ret);
    }

    /* 释放平台 */
    ret = nds03_platform_uninit(&pNxDevice->platform, NULL);
    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("nds03_platform uninit error\r\n");
    }

    return ret;
}

