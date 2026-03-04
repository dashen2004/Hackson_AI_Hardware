# NDS03 SDK User Guide 

- [NDS03 SDK User Guide](#nds03-sdk-user-guide)
  - [1. 介绍 Introduction](#1-介绍-introduction)
  - [2. 平台接口实现 User Platform API Implementation](#2-平台接口实现-user-platform-api-implementation)
  - [3. I2C通讯 I2C Communication](#3-i2c通讯-i2c-communication)
  - [4. NDS03 初始化与测距流程 NDS03 Initialization And Ranging](#4-nds03-初始化与测距流程-nds03-initialization-and-ranging)
  - [5. 标定 Calibration](#5-标定-calibration)
    - [5.1. Offset标定 Offset Calibration](#51-offset标定-offset-calibration)
    - [5.2. 串扰标定 Crosstalk Calibration](#52-串扰标定-crosstalk-calibration)
  - [6. 低功耗 Lower Power](#6-低功耗-lower-power)
    - [6.1. 低功耗模式的进入与唤醒 Sleep and Wake up](#61-低功耗模式的进入与唤醒-sleep-and-wake-up)
    - [6.2. 深度休眠模式的进入与唤醒 Deep Sleep and Wake up](#62-深度休眠模式的进入与唤醒-deep-sleep-and-wake-up)
    
    

## 1. 介绍 Introduction

本文档对NDS03 SDK的使用流程做基本介绍，如需更详细的信息可查看源码注释或者联系支持人员

- SDK 代码目录结构

    ```python
    |--NDS03_SDK
        |-- inc 头文件
         	|-- nds03_calib.h
         	|-- nds03_comm.h
         	|-- nds03_data.h
         	|-- nds03_def.h
         	|-- nds03_dev.h
        |-- src 源码
         	|-- nds03_calib.c
         	|-- nds03_comm.c
         	|-- nds03_data.c
         	|-- nds03_dev.c
        |-- example
            |-- NDS03_SingleRanging_Example.c
            |-- NDS03_ContiunousRanging_Example.c
            |-- nds03_platform.c
            |-- nds03_platform.h  
    ```



## 2. 平台接口实现 User Platform API Implementation

用户需要实现 ```example/nds03_platform.c```中的接口函数，以适配目标平台，详情见```example/nds03_platform.h```。

```c
int8_t nds03_platform_init(NDS03_Platform_t *pdev, void *arg);				// NDS03使用平台初始化
int8_t nds03_platform_uninit(NDS03_Platform_t *pdev, void *arg);			// NDS03使用平台释放
int8_t nds03_i2c_read_nbytes(NDS03_Platform_t *pDev,  uint8_t reg_addr, uint8_t *read_data, uint16_t len);	// i2c连续读len个字节
int8_t nds03_i2c_write_nbytes(NDS03_Platform_t *pDev,  uint8_t reg_addr, uint8_t *write_data, uint16_t len); // i2c连续写len个字节
int8_t nds03_delay_1ms(NDS03_Platform_t *pDev, uint32_t wait_ms);		// 延时wait_ms毫秒
int8_t nds03_delay_10us(NDS03_Platform_t *pDev, uint32_t wait_10us);	// 延时10*wait_10us微秒
int8_t nds03_set_xshut_pin_level(NDS03_Platform_t *pDev, int8_t level);	// 设置nds03 xshut引脚电平
```



## 3. I2C通讯 I2C Communication

- **默认设备地址**：

    ```c
    0x5C
    ```

- **修改I2C地址方式**：

  NDS03支持修改i2c设备地址，但由于模组没有对修改后的地址进行保存，重新上电则自动恢复默认i2c地址0x5C.
  **Note**: NDS03设备的i2c地址后，本地同时要更新新的地址.

    ```c
    NDS03_Error NDS03_SetDevAddr(NDS03_Dev_t *pNxDevice, uint8_t addr);
    ```



## 4. NDS03 初始化与测距流程 NDS03 Initialization And Ranging

完整测距流程可参考```example/NDS03_SingleRanging_Example.c```

- **初始化平台**

  先对平台进行初始化，包括I2C初始化。

  ```c
  int8_t nds03_platform_init(NDS03_Platform_t *pdev, void *arg)
  ```

- **启动 Boot Up**

  初始化NDS03之前，首先需要调用`NDS03_WaitDeviceBootUp()`函数启动模组；

  ```c
  NDS03_Error NDS03_WaitDeviceBootUp(NDS03_Dev_t *pNxDevice)
  ```

- **初始化模组 Initialization**

    ```c
    NDS03_Error NDS03_InitDevice(NDS03_Dev_t *pNxDevice)
    ```

- **单次测距 Ranging**

  使用`NDS03_GetSingleRangingData`获取测距数据

  ```c
  NDS03_Error NDS03_GetSingleRangingData(NDS03_Dev_t *pNxDevice)
  ```


- **连续测距 Ranging**

- 使用`NDS03_GetContinuousRangingData`获取测距数据

  ```c
  NDS03_Error NDS03_GetContinuousRangingData(NDS03_Dev_t *pNxDevice)
  ```



## 5. 标定 Calibration

通常，NDS03需要进行两次标定，详见**《NDS03标定手册》**

### 5.1. Offset标定 Offset Calibration 

模组焊接到PCB后，需要进行一次offset标定，详见**《NDS03标定手册》**

- **默认标定距离 Calibration at default position**

  在默认offset标定环境下(模组放在距90%反射板500 mm处)，调用`NDS03_OffsetCalibration`接口，标定完成后标定数据会保存在模组内部flash中，开机自动读取flash中的标定数据。

  ```c
  NDS03_Error NDS03_OffsetCalibration(NDS03_Dev_t *pNxDevice)
  ```

* **指定标定距离 Calibration at custom position**

  如果用户有在其他距离处进行offset标定的需求，则需要使用`NDS03_OffsetCalibrationAtDepth`接口，参数`calib_depth_mm`用来指定模组到目标处距离。

  ```c
  NDS03_Error NDS03_OffsetCalibrationAtDepth(NDS03_Dev_t *pNxDevice, uint16_t calib_depth_mm)
  ```

### 5.2. 串扰标定 Crosstalk Calibration 

如需在NDS03模组上方增加盖板，则需要进行串扰标定，详见**《NDS03标定手册》**

- **默认串扰标定距离 Crosstalk calibration at default distance** 

	在默认串扰标定的环境下(模组放在距90%反射板500 mm处)，调用`NDS03_XTalkCalibration`接口，标定完成后标定数据会保存在模组内部flash中，开机自动读取flash中的标定数据。

    ```c
    NDS03_Error NDS03_XTalkCalibration(NDS03_Dev_t *pNxDevice)
    ```

- **串扰值获取 Crosstalk Vaule** 

	调用 `NDS03_GetXTalkValue` 函数接口，可获取串扰值，如果该值比较大或者返回错误，请联系支持人员协助。

    ```c
    NDS03_Error NDS03_GetXTalkValue(NDS03_Dev_t *pNxDevice, uint32_t *xtalk_value)
    ```



## 6. 低功耗 Lower Power

### 6.1. 低功耗模式的进入与唤醒 Sleep and Wake up 

- #### 进入低功耗 Sleep

  调用`NDS03_Sleep`函数，NDS03模组即可进入低功耗模式。

  ```c
  NDS03_Error NDS03_Sleep(NDS03_Dev_t *pNxDevice)
  ```

- #### 低功耗唤醒 Wake up

  调用`NDS03_Wakeup`函数，NDS03模组即可从低功耗模式中唤醒。

  ```c
  NDS03_Error NDS03_Wakeup(NDS03_Dev_t *pNxDevice)
  ```
