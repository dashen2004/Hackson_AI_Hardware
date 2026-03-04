/**
 * @file NDS03_Calibration_Example.c
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

/**
 * @brief NDS03标定函数示例
 *        按照NDS03标定手册进行标定，以手册为主
 *        注意：以下标定示例需要将设备处于500mm处
 * @return int32_t 
 */
int32_t main(void)
{
    NDS03_Error     ret;
    NDS03_Dev_t     *pNxDevice = &g_nds03_device;
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