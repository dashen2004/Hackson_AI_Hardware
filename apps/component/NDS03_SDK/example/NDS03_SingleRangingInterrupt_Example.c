/**
 * @file NDS03_SingleRangingInterrupt_Example.c
 * @author tongsheng.tang
 * @brief 单次测量中断示例
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
NDS03_Error ret = NDS03_ERROR_NONE;
/* 循环测距次数为100次 */
int32_t cnt = 100;

/**
 * @brief 单次测量接口
 * 
 * @param pNxDevice 模组设备
 */
NDS03_Error SG_RangingTest(NDS03_Dev_t *pNxDevice)
{
    /* 获取测量数据 */
    CHECK_RET(NDS03_GetInterruptRangingData(pNxDevice));
    NX_PRINTF("depth:%d mm, confi:%d, count:%d\r\n", pNxDevice->ranging_data[0].depth, pNxDevice->ranging_data[0].confi, pNxDevice->ranging_data[0].count);
    CHECK_RET(NDS03_StartSingleMeasurement(pNxDevice));

    return ret;
}

/**
 * @brief 用户实现NDS03 GPIO1引脚的配置
 *        配置NDS03 GPIO1引脚为中断输入且是下降沿触发
 * 
 */
int8_t USER_GPIO1PinConfig(void){
    // 用户实现MCU中断配置代码
    return 0;
}

/**
 * @brief NDS03单次中断测量主函数示例
 * 
 * @return int32_t 
 */
int32_t main(void)
{
    NDS03_Dev_t     *pNxDevice = &g_nds03_device;

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
        /* 用户配置中断引脚代码 */
        ret = USER_GPIO1PinConfig();
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("USER_GPIO1PinConfig error!!\r\n");
        }
    }

    if(NDS03_ERROR_NONE == ret){
        /* 设置使用有新数据时触发中断,且低电平产生中断 */
        ret = NDS03_SetGpio1Config(pNxDevice, NDS03_GPIO1_NEW_MEASURE_READY, NDS03_GPIO1_POLARITY_LOW);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_SetGpio1Config error!!\r\n");
        }    
    }

    if(NDS03_ERROR_NONE == ret){
        /* 需要先测量一次 */
        ret = NDS03_StartSingleMeasurement(pNxDevice);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_StartSingleMeasurement error!!\r\n");
        }  
    }

    /* 循环测量 */
    while(cnt && (NDS03_ERROR_NONE == ret)) {
        ret = NDS03_Delay1ms(pNxDevice, 10);
    }
    
    if(NDS03_ERROR_NONE == ret){
        /* 关闭测距中断 */
        ret = NDS03_SetGpio1Config(pNxDevice, NDS03_GPIO1_FUNCTIONALITY_OFF, NDS03_GPIO1_POLARITY_LOW);
        if(NDS03_ERROR_NONE != ret){
            NX_PRINTF("NDS03_SetGpio1Config error!!\r\n");
        }  
    }

    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("NDS03_SingleRangingInterrupt_Example failed, error code: %d\r\n", ret);
    }

    /* 释放平台 */
    ret = nds03_platform_uninit(&pNxDevice->platform, NULL);
    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("nds03_platform uninit error\n");
    }

    return ret;
}

/* 中断服务函数，在中断中调用 */
void Interrupt_Handler(void){
    ret = SG_RangingTest(&g_nds03_device);
    if(NDS03_ERROR_NONE != ret){
        NX_PRINTF("SG_RangingTest error!!\r\n");
    }
    cnt--;
}

