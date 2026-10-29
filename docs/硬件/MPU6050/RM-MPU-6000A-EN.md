# MPU-6000 / MPU-6050 寄存器手册

> 在线来源与配件对应：[赛道配件清单与实物图](../赛道配件清单与实物图.md)；原件核对／补图日期：2026-10-29。

用于查询寄存器地址、位字段、采样配置、中断、传感器读数、FIFO 和电源管理。位字段按 Bit7～Bit0 展开，`-` 表示资料中的保留位。正文保留英文技术术语和章节编号，可通过阅读导航定位。

> 来源：InvenSense，`RM-MPU-6000A-EN.pdf`，Revision 3.2，2011-11-14。未进行芯片、驱动或实物验证；实际模块版本与项目配置待确认。

相关资料：[产品规格书](PS-MPU-6000A-EN.md) · [模块原理图](MPU6050-V1-SCH.md) · [版本兼容说明](关于MPU6050模块新版本兼容问题的解决方案2.md)。

## 阅读导航

| 主题 | 入口 |
| --- | --- |
| 全部寄存器地址与位定义 | [3 Register Map](#section-3) |
| 采样率与数字低通滤波 | [4.2 Register 25 – Sample Rate Divider](#section-4-2) |
| 陀螺仪量程与自检 | [4.4 Register 27 – Gyroscope Configuration](#section-4-4) |
| 加速度计量程、高通滤波与自检 | [4.5 Register 28 – Accelerometer Configuration](#section-4-5) |
| 中断使能与状态 | [4.21 Register 56 – Interrupt Enable](#section-4-21) |
| 加速度、温度与陀螺仪测量值 | [4.23 Registers 59 to 64 – Accelerometer Measurements](#section-4-23) |
| 电源管理与唤醒 | [4.36 Register 107 – Power Management 1](#section-4-36) |
| FIFO 数据读写 | [4.38 Register 114 and 115 – FIFO Count Registers](#section-4-38) |
| 设备标识 WHO_AM_I | [4.40 Register 117 – Who Am I](#section-4-40) |

### 章节目录

| 章节 | 内容 |
| --- | --- |
| 1 | [Revision History](#section-1) |
| 2 | [Purpose and Scope](#section-2) |
| 3 | [Register Map](#section-3) |
| 4 | [Register Descriptions](#section-4) |
| 4.1 | [Register 1 – Auxiliary I²C Supply Selection](#section-4-1) |
| 4.2 | [Register 25 – Sample Rate Divider](#section-4-2) |
| 4.3 | [Register 26 – Configuration](#section-4-3) |
| 4.4 | [Register 27 – Gyroscope Configuration](#section-4-4) |
| 4.5 | [Register 28 – Accelerometer Configuration](#section-4-5) |
| 4.6 | [Register 29 – Free Fall Acceleration Threshold](#section-4-6) |
| 4.7 | [Register 30 – Free Fall Duration](#section-4-7) |
| 4.8 | [Register 31 – Motion Detection Threshold](#section-4-8) |
| 4.9 | [Register 32 – Motion Detection Duration](#section-4-9) |
| 4.10 | [Register 33 – Zero Motion Detection Threshold](#section-4-10) |
| 4.11 | [Register 34 – Zero Motion Detection Duration](#section-4-11) |
| 4.12 | [Register 35 – FIFO Enable](#section-4-12) |
| 4.13 | [Register 36 – I²C Master Control](#section-4-13) |
| 4.14 | [Registers 37 to 39 – I²C Slave 0 Control](#section-4-14) |
| 4.15 | [Registers 40 to 42 – I²C Slave 1 Control](#section-4-15) |
| 4.16 | [Registers 43 to 45 – I²C Slave 2 Control](#section-4-16) |
| 4.17 | [Registers 46 to 48 – I²C Slave 3 Control](#section-4-17) |
| 4.18 | [Registers 49 to 53 – I²C Slave 4 Control](#section-4-18) |
| 4.19 | [Register 54 – I²C Master Status](#section-4-19) |
| 4.20 | [Register 55 – INT Pin / Bypass Enable Configuration](#section-4-20) |
| 4.21 | [Register 56 – Interrupt Enable](#section-4-21) |
| 4.22 | [Register 58 – Interrupt Status](#section-4-22) |
| 4.23 | [Registers 59 to 64 – Accelerometer Measurements](#section-4-23) |
| 4.24 | [Registers 65 and 66 – Temperature Measurement](#section-4-24) |
| 4.25 | [Registers 67 to 72 – Gyroscope Measurements](#section-4-25) |
| 4.26 | [Registers 73 to 96 – External Sensor Data](#section-4-26) |
| 4.27 | [Register 97 – Motion Detection Status](#section-4-27) |
| 4.28 | [Register 99 – I²C Slave 0 Data Out](#section-4-28) |
| 4.29 | [Register 100 – I²C Slave 1 Data Out](#section-4-29) |
| 4.30 | [Register 101 – I²C Slave 2 Data Out](#section-4-30) |
| 4.31 | [Register 102 – I²C Slave 3 Data Out](#section-4-31) |
| 4.32 | [Register 103 – I²C Master Delay Control](#section-4-32) |
| 4.33 | [Register 104 – Signal Path Reset](#section-4-33) |
| 4.34 | [Register 105 – Motion Detection Control](#section-4-34) |
| 4.35 | [Register 106 – User Control](#section-4-35) |
| 4.36 | [Register 107 – Power Management 1](#section-4-36) |
| 4.37 | [Register 108 – Power Management 2](#section-4-37) |
| 4.38 | [Register 114 and 115 – FIFO Count Registers](#section-4-38) |
| 4.39 | [Register 116 – FIFO Read Write](#section-4-39) |
| 4.40 | [Register 117 – Who Am I](#section-4-40) |

<a id="section-1"></a>

## 1 Revision History

|Revision<br>Date|Revision|Description|
|---|---|---|
|11/29/2010|1.0|Initial Release|
|04/20/2011|1.1|Updated register map and descriptions to reflect enhanced register functionality.|
|05/19/2011|2.0|Updates for Rev C silicon:<br>Edits for readability (section 2.1)<br>Edits for changes in functionality (section 3, 4.4, 4.6, 4.7, 4.8, 4.21, 4.22, 4.23,<br>4.37)|
|10/07/2011|3.0|Updates for Rev D silicon:<br>Updated accelerometer sensitivity specifications (sections 4.6, 4.8, 4.10, 4.23)|
|10/24/2011|3.1|Edits for clarity|
|11/14/2011|3.2|Updated reset value for register 107 (section 3)<br>Updated register 27 with gyro self-test bits (section 4.4)<br>Provided gyro self-test instructions and register bits (section 4.4)<br> <br>Provided accel self-test instructions (section 4.5)|

<a id="section-2"></a>

## 2 Purpose and Scope

This document provides preliminary information regarding the register map and descriptions for the Motion Processing Units™ MPU-6000™ and MPU-6050™, collectively called the MPU-60X0™ or MPU™.

The MPU devices provide the world’s first integrated 6-axis motion processor solution that eliminates the package-level gyroscope and accelerometer cross-axis misalignment associated with discrete solutions. The devices combine a 3-axis gyroscope and a 3-axis accelerometer on the same silicon die together with an onboard Digital Motion Processor™ (DMP™) capable of processing complex 9-axis sensor fusion algorithms using the field-proven and proprietary MotionFusion™ engine.

The MPU-6000 and MPU-6050’s integrated 9-axis MotionFusion algorithms access external magnetometers or other sensors through an auxiliary master I²C bus, allowing the devices to gather a full set of sensor data without intervention from the system processor. The devices are offered in the same 4x4x0.9 mm QFN footprint and pinout as the current MPU-3000™ family of integrated 3-axis gyroscopes, providing a simple upgrade path and facilitating placement on already space constrained circuit boards.

For precision tracking of both fast and slow motions, the MPU-60X0 features a user-programmable gyroscope full-scale range of ±250, ±500, ±1000, and ±2000°/sec (dps). The parts also have a userprogrammable accelerometer full-scale range of ±2 _g_, ±4 _g_, ±8 _g_, and ±16 _g_ .

The MPU-6000 family is comprised of two parts, the MPU-6000 and MPU-6050. These parts are identical to each other with two exceptions. The MPU-6050 supports I²C communications at up to 400kHz and has a VLOGIC pin that defines its interface voltage levels; the MPU-6000 supports SPI at up to 20MHz in addition to I²C, and has a single supply pin, VDD, which is both the device’s logic reference supply and the analog supply for the part.

For more detailed information for the MPU-60X0 devices, please refer to the “MPU-6000 and MPU-6050 Product Specification”.

<a id="section-3"></a>

## 3 Register Map

The register map for the MPU-60X0 is listed below.

|Addr<br>(Hex)|Addr<br>(Dec.)|Register Name|Serial<br>I/F|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|---|---|
|01|1|AUX_VDDIO|R/W|AUX_VDDIO|-|-|-|-|-|-|-|
|19|25|SMPLRT_DIV|R/W|SMPLRT_DIV[7]|SMPLRT_DIV[6]|SMPLRT_DIV[5]|SMPLRT_DIV[4]|SMPLRT_DIV[3]|SMPLRT_DIV[2]|SMPLRT_DIV[1]|SMPLRT_DIV[0]|
|1A|26|CONFIG|R/W|-|-|EXT_SYNC_SET[2]|EXT_SYNC_SET[1]|EXT_SYNC_SET[0]|DLPF_CFG[2]|DLPF_CFG[1]|DLPF_CFG[0]|
|1B|27|GYRO_CONFIG|R/W|-|-|-|FS_SEL[1]|FS_SEL[0]|-|-|-|
|1C|28|ACCEL_CONFIG|R/W|XA_ST|YA_ST|ZA_ST|AFS_SEL[1]|AFS_SEL[0]|ACCEL_HPF[2]|ACCEL_HPF[1]|ACCEL_HPF[0]|
|1D|29|FF_THR|R/W|FF_THR[7]|FF_THR[6]|FF_THR[5]|FF_THR[4]|FF_THR[3]|FF_THR[2]|FF_THR[1]|FF_THR[0]|
|1E|30|FF_DUR|R/W|FF_DUR[7]|FF_DUR[6]|FF_DUR[5]|FF_DUR[4]|FF_DUR[3]|FF_DUR[2]|FF_DUR[1]|FF_DUR[0]|
|1F|31|MOT_THR|R/W|MOT_THR[7]|MOT_THR[6]|MOT_THR[5]|MOT_THR[4]|MOT_THR[3]|MOT_THR[2]|MOT_THR[1]|MOT_THR[0]|
|20|32|MOT_DUR|R/W|MOT_DUR[7]|MOT_DUR[6]|MOT_DUR[5]|MOT_DUR[4]|MOT_DUR[3]|MOT_DUR[2]|MOT_DUR[1]|MOT_DUR[0]|
|21|33|ZRMOT_THR|R/W|ZRMOT_THR[7]|ZRMOT_THR[6]|ZRMOT_THR[5]|ZRMOT_THR[4]|ZRMOT_THR[3]|ZRMOT_THR[2]|ZRMOT_THR[1]|ZRMOT_THR[0]|
|22|34|ZRMOT_DUR|R/W|ZRMOT_DUR[7]|ZRMOT_DUR[6]|ZRMOT_DUR[5]|ZRMOT_DUR[4]|ZRMOT_DUR[3]|ZRMOT_DUR[2]|ZRMOT_DUR[1]|ZRMOT_DUR[0]|
|23|35|FIFO_EN|R/W|TEMP_FIFO_EN|XG_FIFO_EN|YG_FIFO_EN|ZG_FIFO_EN|ACCEL_FIFO_EN|SLV2_FIFO_EN|SLV1_FIFO_EN|SLV0_FIFO_EN|
|24|36|I2C_MST_CTRL|R/W|MULT_MST_EN|WAIT_FOR_ES|SLV_3_FIFO_EN|I2C_MST_P_NSR|I2C_MST_CLK[3]|I2C_MST_CLK[2]|I2C_MST_CLK[1]|I2C_MST_CLK[0]|
|25|37|I2C_SLV0_ADDR|R/W|I2C_SLV0_RW|I2C_SLV0_ADDR[6]|I2C_SLV0_ADDR[5]|I2C_SLV0_ADDR[4]|I2C_SLV0_ADDR[3]|I2C_SLV0_ADDR[2]|I2C_SLV0_ADDR[1]|I2C_SLV0_ADDR[0]|
|26|38|I2C_SLV0_REG|R/W|I2C_SLV0_REG[7]|I2C_SLV0_REG[6]|I2C_SLV0_REG[5]|I2C_SLV0_REG[4]|I2C_SLV0_REG[3]|I2C_SLV0_REG[2]|I2C_SLV0_REG[1]|I2C_SLV0_REG[0]|
|27|39|I2C_SLV0_CTRL|R/W|I2C_SLV0_EN|I2C_SLV0_BYTE_SW|I2C_SLV0_REG_DIS|I2C_SLV0_GRP|I2C_SLV0_LEN[3]|I2C_SLV0_LEN[2]|I2C_SLV0_LEN[1]|I2C_SLV0_LEN[0]|
|28|40|I2C_SLV1_ADDR|R/W|I2C_SLV1_RW|I2C_SLV1_ADDR[6]|I2C_SLV1_ADDR[5]|I2C_SLV1_ADDR[4]|I2C_SLV1_ADDR[3]|I2C_SLV1_ADDR[2]|I2C_SLV1_ADDR[1]|I2C_SLV1_ADDR[0]|
|29|41|I2C_SLV1_REG|R/W|I2C_SLV1_REG[7]|I2C_SLV1_REG[6]|I2C_SLV1_REG[5]|I2C_SLV1_REG[4]|I2C_SLV1_REG[3]|I2C_SLV1_REG[2]|I2C_SLV1_REG[1]|I2C_SLV1_REG[0]|
|2A|42|I2C_SLV1_CTRL|R/W|I2C_SLV1_EN|I2C_SLV1_BYTE_SW|I2C_SLV1_REG_DIS|I2C_SLV1_GRP|I2C_SLV1_LEN[3]|I2C_SLV1_LEN[2]|I2C_SLV1_LEN[1]|I2C_SLV1_LEN[0]|
|2B|43|I2C_SLV2_ADDR|R/W|I2C_SLV2_RW|I2C_SLV2_ADDR[6]|I2C_SLV2_ADDR[5]|I2C_SLV2_ADDR[4]|I2C_SLV2_ADDR[3]|I2C_SLV2_ADDR[2]|I2C_SLV2_ADDR[1]|I2C_SLV2_ADDR[0]|
|2C|44|I2C_SLV2_REG|R/W|I2C_SLV2_REG[7]|I2C_SLV2_REG[6]|I2C_SLV2_REG[5]|I2C_SLV2_REG[4]|I2C_SLV2_REG[3]|I2C_SLV2_REG[2]|I2C_SLV2_REG[1]|I2C_SLV2_REG[0]|
|2D|45|I2C_SLV2_CTRL|R/W|I2C_SLV2_EN|I2C_SLV2_BYTE_SW|I2C_SLV2_REG_DIS|I2C_SLV2_GRP|I2C_SLV2_LEN[3]|I2C_SLV2_LEN[2]|I2C_SLV2_LEN[1]|I2C_SLV2_LEN[0]|
|2E|46|I2C_SLV3_ADDR|R/W|I2C_SLV3_RW|I2C_SLV3_ADDR[6]|I2C_SLV3_ADDR[5]|I2C_SLV3_ADDR[4]|I2C_SLV3_ADDR[3]|I2C_SLV3_ADDR[2]|I2C_SLV3_ADDR[1]|I2C_SLV3_ADDR[0]|
|2F|47|I2C_SLV3_REG|R/W|I2C_SLV3_REG[7]|I2C_SLV3_REG[6]|I2C_SLV3_REG[5]|I2C_SLV3_REG[4]|I2C_SLV3_REG[3]|I2C_SLV3_REG[2]|I2C_SLV3_REG[1]|I2C_SLV3_REG[0]|
|30|48|I2C_SLV3_CTRL|R/W|I2C_SLV3_EN|I2C_SLV3_BYTE_SW|I2C_SLV3_REG_DIS|I2C_SLV3_GRP|I2C_SLV3_LEN[3]|I2C_SLV3_LEN[2]|I2C_SLV3_LEN[1]|I2C_SLV3_LEN[0]|
|31|49|I2C_SLV4_ADDR|R/W|I2C_SLV4_RW|I2C_SLV4_ADDR[6]|I2C_SLV4_ADDR[5]|I2C_SLV4_ADDR[4]|I2C_SLV4_ADDR[3]|I2C_SLV4_ADDR[2]|I2C_SLV4_ADDR[1]|I2C_SLV4_ADDR[0]|
|32|50|I2C_SLV4_REG|R/W|I2C_SLV4_REG[7]|I2C_SLV4_REG[6]|I2C_SLV4_REG[5]|I2C_SLV4_REG[4]|I2C_SLV4_REG[3]|I2C_SLV4_REG[2]|I2C_SLV4_REG[1]|I2C_SLV4_REG[0]|
|33|51|I2C_SLV4_DO|R/W|I2C_SLV4_DO[7]|I2C_SLV4_DO[6]|I2C_SLV4_DO[5]|I2C_SLV4_DO[4]|I2C_SLV4_DO[3]|I2C_SLV4_DO[2]|I2C_SLV4_DO[1]|I2C_SLV4_DO[0]|
|34|52|I2C_SLV4_CTRL|R/W|I2C_SLV4_EN|I2C_SLV4_INT_EN|I2C_SLV4_REG_DIS|I2C_MST_DLY[4]|I2C_MST_DLY[3]|I2C_MST_DLY[2]|I2C_MST_DLY[1]|I2C_MST_DLY[0]|
|35|53|I2C_SLV4_DI|R|I2C_SLV4_DI[7]|I2C_SLV4_DI[6]|I2C_SLV4_DI[5]|I2C_SLV4_DI[4]|I2C_SLV4_DI[3]|I2C_SLV4_DI[2]|I2C_SLV4_DI[1]|I2C_SLV4_DI[0]|
|36|54|I2C_MST_STATUS|R|PASS_THROUGH|I2C_SLV4_DONE|I2C_LOST_ARB|I2C_SLV4_NACK|I2C_SLV3_NACK|I2C_SLV2_NACK|I2C_SLV1_NACK|I2C_SLV0_NACK|
|37|55|INT_PIN_CFG|R/W|INT_LEVEL|INT_OPEN|LATCH_INT_EN|INT_RD_CLEAR|FSYNC_INT_LEVEL|FSYNC_INT_EN|I2C_BYPASS_EN|CLKOUT_EN|
|38|56|INT_ENABLE|R/W|FF_EN|MOT_EN|ZMOT_EN|FIFO_OFLOW_EN|I2C_MST_INT_EN|-|-|DATA_RDY_EN|
|3A|58|INT_STATUS|R|FF_INT|MOT_INT|ZMOT_INT|FIFO_OFLOW_INT|I2C_MST_INT|-|-|DATA_RDY_INT|
|3B|59|ACCEL_XOUT_H|R|ACCEL_XOUT[15]|ACCEL_XOUT[14]|ACCEL_XOUT[13]|ACCEL_XOUT[12]|ACCEL_XOUT[11]|ACCEL_XOUT[10]|ACCEL_XOUT[9]|ACCEL_XOUT[8]|
|3C|60|ACCEL_XOUT_L|R|ACCEL_XOUT[7]|ACCEL_XOUT[6]|ACCEL_XOUT[5]|ACCEL_XOUT[4]|ACCEL_XOUT[3]|ACCEL_XOUT[2]|ACCEL_XOUT[1]|ACCEL_XOUT[0]|
|3D|61|ACCEL_YOUT_H|R|ACCEL_YOUT[15]|ACCEL_YOUT[14]|ACCEL_YOUT[13]|ACCEL_YOUT[12]|ACCEL_YOUT[11]|ACCEL_YOUT[10]|ACCEL_YOUT[9]|ACCEL_YOUT[8]|
|3E|62|ACCEL_YOUT_L|R|ACCEL_YOUT[7]|ACCEL_YOUT[6]|ACCEL_YOUT[5]|ACCEL_YOUT[4]|ACCEL_YOUT[3]|ACCEL_YOUT[2]|ACCEL_YOUT[1]|ACCEL_YOUT[0]|
|3F|63|ACCEL_ZOUT_H|R|ACCEL_ZOUT[15]|ACCEL_ZOUT[14]|ACCEL_ZOUT[13]|ACCEL_ZOUT[12]|ACCEL_ZOUT[11]|ACCEL_ZOUT[10]|ACCEL_ZOUT[9]|ACCEL_ZOUT[8]|
|40|64|ACCEL_ZOUT_L|R|ACCEL_ZOUT[7]|ACCEL_ZOUT[6]|ACCEL_ZOUT[5]|ACCEL_ZOUT[4]|ACCEL_ZOUT[3]|ACCEL_ZOUT[2]|ACCEL_ZOUT[1]|ACCEL_ZOUT[0]|
|41|65|TEMP_OUT_H|R|TEMP_OUT[15]|TEMP_OUT[14]|TEMP_OUT[13]|TEMP_OUT[12]|TEMP_OUT[11]|TEMP_OUT[10]|TEMP_OUT[9]|TEMP_OUT[8]|
|42|66|TEMP_OUT_L|R|TEMP_OUT[7]|TEMP_OUT[6]|TEMP_OUT[5]|TEMP_OUT[4]|TEMP_OUT[3]|TEMP_OUT[2]|TEMP_OUT[1]|TEMP_OUT[0]|
|43|67|GYRO_XOUT_H|R|GYRO_XOUT[15]|GYRO_XOUT[14]|GYRO_XOUT[13]|GYRO_XOUT[12]|GYRO_XOUT[11]|GYRO_XOUT[10]|GYRO_XOUT[9]|GYRO_XOUT[8]|
|44|68|GYRO_XOUT_L|R|GYRO_XOUT[7]|GYRO_XOUT[6]|GYRO_XOUT[5]|GYRO_XOUT[4]|GYRO_XOUT[3]|GYRO_XOUT[2]|GYRO_XOUT[1]|GYRO_XOUT[0]|
|45|69|GYRO_YOUT_H|R|GYRO_YOUT[15]|GYRO_YOUT[14]|GYRO_YOUT[13]|GYRO_YOUT[12]|GYRO_YOUT[11]|GYRO_YOUT[10]|GYRO_YOUT[9]|GYRO_YOUT[8]|
|46|70|GYRO_YOUT_L|R|GYRO_YOUT[7]|GYRO_YOUT[6]|GYRO_YOUT[5]|GYRO_YOUT[4]|GYRO_YOUT[3]|GYRO_YOUT[2]|GYRO_YOUT[1]|GYRO_YOUT[0]|
|47|71|GYRO_ZOUT_H|R|GYRO_ZOUT[15]|GYRO_ZOUT[14]|GYRO_ZOUT[13]|GYRO_ZOUT[12]|GYRO_ZOUT[11]|GYRO_ZOUT[10]|GYRO_ZOUT[9]|GYRO_ZOUT[8]|
|48|72|GYRO_ZOUT_L|R|GYRO_ZOUT[7]|GYRO_ZOUT[6]|GYRO_ZOUT[5]|GYRO_ZOUT[4]|GYRO_ZOUT[3]|GYRO_ZOUT[2]|GYRO_ZOUT[1]|GYRO_ZOUT[0]|
|49|73|EXT_SENS_DATA_00|R|EXT_SENS_DATA_00[7]|EXT_SENS_DATA_00[6]|EXT_SENS_DATA_00[5]|EXT_SENS_DATA_00[4]|EXT_SENS_DATA_00[3]|EXT_SENS_DATA_00[2]|EXT_SENS_DATA_00[1]|EXT_SENS_DATA_00[0]|
|4A|74|EXT_SENS_DATA_01|R|EXT_SENS_DATA_01[7]|EXT_SENS_DATA_01[6]|EXT_SENS_DATA_01[5]|EXT_SENS_DATA_01[4]|EXT_SENS_DATA_01[3]|EXT_SENS_DATA_01[2]|EXT_SENS_DATA_01[1]|EXT_SENS_DATA_01[0]|
|4B|75|EXT_SENS_DATA_02|R|EXT_SENS_DATA_02[7]|EXT_SENS_DATA_02[6]|EXT_SENS_DATA_02[5]|EXT_SENS_DATA_02[4]|EXT_SENS_DATA_02[3]|EXT_SENS_DATA_02[2]|EXT_SENS_DATA_02[1]|EXT_SENS_DATA_02[0]|
|4C|76|EXT_SENS_DATA_03|R|EXT_SENS_DATA_03[7]|EXT_SENS_DATA_03[6]|EXT_SENS_DATA_03[5]|EXT_SENS_DATA_03[4]|EXT_SENS_DATA_03[3]|EXT_SENS_DATA_03[2]|EXT_SENS_DATA_03[1]|EXT_SENS_DATA_03[0]|
|4D|77|EXT_SENS_DATA_04|R|EXT_SENS_DATA_04[7]|EXT_SENS_DATA_04[6]|EXT_SENS_DATA_04[5]|EXT_SENS_DATA_04[4]|EXT_SENS_DATA_04[3]|EXT_SENS_DATA_04[2]|EXT_SENS_DATA_04[1]|EXT_SENS_DATA_04[0]|
|4E|78|EXT_SENS_DATA_05|R|EXT_SENS_DATA_05[7]|EXT_SENS_DATA_05[6]|EXT_SENS_DATA_05[5]|EXT_SENS_DATA_05[4]|EXT_SENS_DATA_05[3]|EXT_SENS_DATA_05[2]|EXT_SENS_DATA_05[1]|EXT_SENS_DATA_05[0]|
|4F|79|EXT_SENS_DATA_06|R|EXT_SENS_DATA_06[7]|EXT_SENS_DATA_06[6]|EXT_SENS_DATA_06[5]|EXT_SENS_DATA_06[4]|EXT_SENS_DATA_06[3]|EXT_SENS_DATA_06[2]|EXT_SENS_DATA_06[1]|EXT_SENS_DATA_06[0]|
|50|80|EXT_SENS_DATA_07|R|EXT_SENS_DATA_07[7]|EXT_SENS_DATA_07[6]|EXT_SENS_DATA_07[5]|EXT_SENS_DATA_07[4]|EXT_SENS_DATA_07[3]|EXT_SENS_DATA_07[2]|EXT_SENS_DATA_07[1]|EXT_SENS_DATA_07[0]|
|51|81|EXT_SENS_DATA_08|R|EXT_SENS_DATA_08[7]|EXT_SENS_DATA_08[6]|EXT_SENS_DATA_08[5]|EXT_SENS_DATA_08[4]|EXT_SENS_DATA_08[3]|EXT_SENS_DATA_08[2]|EXT_SENS_DATA_08[1]|EXT_SENS_DATA_08[0]|
|52|82|EXT_SENS_DATA_09|R|EXT_SENS_DATA_09[7]|EXT_SENS_DATA_09[6]|EXT_SENS_DATA_09[5]|EXT_SENS_DATA_09[4]|EXT_SENS_DATA_09[3]|EXT_SENS_DATA_09[2]|EXT_SENS_DATA_09[1]|EXT_SENS_DATA_09[0]|
|53|83|EXT_SENS_DATA_10|R|EXT_SENS_DATA_10[7]|EXT_SENS_DATA_10[6]|EXT_SENS_DATA_10[5]|EXT_SENS_DATA_10[4]|EXT_SENS_DATA_10[3]|EXT_SENS_DATA_10[2]|EXT_SENS_DATA_10[1]|EXT_SENS_DATA_10[0]|
|54|84|EXT_SENS_DATA_11|R|EXT_SENS_DATA_11[7]|EXT_SENS_DATA_11[6]|EXT_SENS_DATA_11[5]|EXT_SENS_DATA_11[4]|EXT_SENS_DATA_11[3]|EXT_SENS_DATA_11[2]|EXT_SENS_DATA_11[1]|EXT_SENS_DATA_11[0]|
|55|85|EXT_SENS_DATA_12|R|EXT_SENS_DATA_12[7]|EXT_SENS_DATA_12[6]|EXT_SENS_DATA_12[5]|EXT_SENS_DATA_12[4]|EXT_SENS_DATA_12[3]|EXT_SENS_DATA_12[2]|EXT_SENS_DATA_12[1]|EXT_SENS_DATA_12[0]|
|56|86|EXT_SENS_DATA_13|R|EXT_SENS_DATA_13[7]|EXT_SENS_DATA_13[6]|EXT_SENS_DATA_13[5]|EXT_SENS_DATA_13[4]|EXT_SENS_DATA_13[3]|EXT_SENS_DATA_13[2]|EXT_SENS_DATA_13[1]|EXT_SENS_DATA_13[0]|
|57|87|EXT_SENS_DATA_14|R|EXT_SENS_DATA_14[7]|EXT_SENS_DATA_14[6]|EXT_SENS_DATA_14[5]|EXT_SENS_DATA_14[4]|EXT_SENS_DATA_14[3]|EXT_SENS_DATA_14[2]|EXT_SENS_DATA_14[1]|EXT_SENS_DATA_14[0]|
|58|88|EXT_SENS_DATA_15|R|EXT_SENS_DATA_15[7]|EXT_SENS_DATA_15[6]|EXT_SENS_DATA_15[5]|EXT_SENS_DATA_15[4]|EXT_SENS_DATA_15[3]|EXT_SENS_DATA_15[2]|EXT_SENS_DATA_15[1]|EXT_SENS_DATA_15[0]|
|59|89|EXT_SENS_DATA_16|R|EXT_SENS_DATA_16[7]|EXT_SENS_DATA_16[6]|EXT_SENS_DATA_16[5]|EXT_SENS_DATA_16[4]|EXT_SENS_DATA_16[3]|EXT_SENS_DATA_16[2]|EXT_SENS_DATA_16[1]|EXT_SENS_DATA_16[0]|
|5A|90|EXT_SENS_DATA_17|R|EXT_SENS_DATA_17[7]|EXT_SENS_DATA_17[6]|EXT_SENS_DATA_17[5]|EXT_SENS_DATA_17[4]|EXT_SENS_DATA_17[3]|EXT_SENS_DATA_17[2]|EXT_SENS_DATA_17[1]|EXT_SENS_DATA_17[0]|
|5B|91|EXT_SENS_DATA_18|R|EXT_SENS_DATA_18[7]|EXT_SENS_DATA_18[6]|EXT_SENS_DATA_18[5]|EXT_SENS_DATA_18[4]|EXT_SENS_DATA_18[3]|EXT_SENS_DATA_18[2]|EXT_SENS_DATA_18[1]|EXT_SENS_DATA_18[0]|
|5C|92|EXT_SENS_DATA_19|R|EXT_SENS_DATA_19[7]|EXT_SENS_DATA_19[6]|EXT_SENS_DATA_19[5]|EXT_SENS_DATA_19[4]|EXT_SENS_DATA_19[3]|EXT_SENS_DATA_19[2]|EXT_SENS_DATA_19[1]|EXT_SENS_DATA_19[0]|
|5D|93|EXT_SENS_DATA_20|R|EXT_SENS_DATA_20[7]|EXT_SENS_DATA_20[6]|EXT_SENS_DATA_20[5]|EXT_SENS_DATA_20[4]|EXT_SENS_DATA_20[3]|EXT_SENS_DATA_20[2]|EXT_SENS_DATA_20[1]|EXT_SENS_DATA_20[0]|
|5E|94|EXT_SENS_DATA_21|R|EXT_SENS_DATA_21[7]|EXT_SENS_DATA_21[6]|EXT_SENS_DATA_21[5]|EXT_SENS_DATA_21[4]|EXT_SENS_DATA_21[3]|EXT_SENS_DATA_21[2]|EXT_SENS_DATA_21[1]|EXT_SENS_DATA_21[0]|
|5F|95|EXT_SENS_DATA_22|R|EXT_SENS_DATA_22[7]|EXT_SENS_DATA_22[6]|EXT_SENS_DATA_22[5]|EXT_SENS_DATA_22[4]|EXT_SENS_DATA_22[3]|EXT_SENS_DATA_22[2]|EXT_SENS_DATA_22[1]|EXT_SENS_DATA_22[0]|
|60|96|EXT_SENS_DATA_23|R|EXT_SENS_DATA_23[7]|EXT_SENS_DATA_23[6]|EXT_SENS_DATA_23[5]|EXT_SENS_DATA_23[4]|EXT_SENS_DATA_23[3]|EXT_SENS_DATA_23[2]|EXT_SENS_DATA_23[1]|EXT_SENS_DATA_23[0]|
|61|97|MOT_DETECT_STATUS|R|MOT_XNEG|MOT_XPOS|MOT_YNEG|MOT_YPOS|MOT_ZNEG|MOT_ZPOS|-|MOT_ZRMOT|
|63|99|I2C_SLV0_DO|R/W|I2C_SLV0_DO[7]|I2C_SLV0_DO[6]|I2C_SLV0_DO[5]|I2C_SLV0_DO[4]|I2C_SLV0_DO[3]|I2C_SLV0_DO[2]|I2C_SLV0_DO[1]|I2C_SLV0_DO[0]|
|64|100|I2C_SLV1_DO|R/W|I2C_SLV1_DO[7]|I2C_SLV1_DO[6]|I2C_SLV1_DO[5]|I2C_SLV1_DO[4]|I2C_SLV1_DO[3]|I2C_SLV1_DO[2]|I2C_SLV1_DO[1]|I2C_SLV1_DO[0]|
|65|101|I2C_SLV2_DO|R/W|I2C_SLV2_DO[7]|I2C_SLV2_DO[6]|I2C_SLV2_DO[5]|I2C_SLV2_DO[4]|I2C_SLV2_DO[3]|I2C_SLV2_DO[2]|I2C_SLV2_DO[1]|I2C_SLV2_DO[0]|
|66|102|I2C_SLV3_DO|R/W|I2C_SLV3_DO[7]|I2C_SLV3_DO[6]|I2C_SLV3_DO[5]|I2C_SLV3_DO[4]|I2C_SLV3_DO[3]|I2C_SLV3_DO[2]|I2C_SLV3_DO[1]|I2C_SLV3_DO[0]|
|67|103|I2C_MST_DELAY_CTRL|R/W|DELAY_ES_SHADOW|-|-|I2C_SLV4_DLY_EN|I2C_SLV3_DLY_EN|I2C_SLV2_DLY_EN|I2C_SLV1_DLY_EN|I2C_SLV0_DLY_EN|
|68|104|SIGNAL_PATH_RESET|R/W|-|-|-|-|-|GYRO_RESET|ACCEL_RESET|TEMP_RESET|
|69|105|MOT_DETECT_CTRL|R/W|-|-|ACCEL_ON_DELAY[1]|ACCEL_ON_DELAY[0]|FF_COUNT[1]|FF_COUNT[0]|MOT_COUNT[1]|MOT_COUNT[0]|
|6A|106|USER_CTRL|R/W|-|FIFO_EN|I2C_MST_EN|I2C_IF_DIS|-|FIFO_RESET|I2C_MST_RESET|SIG_COND_RESET|
|6B|107|PWR_MGMT_1|R/W|DEVICE_RESET|SLEEP|CYCLE|-|TEMP_DIS|CLKSEL[2]|CLKSEL[1]|CLKSEL[0]|
|6C|108|PWR_MGMT_2|R/W|LP_WAKE_CTRL[1]|LP_WAKE_CTRL[0]|STBY_XA|STBY_YA|STBY_ZA|STBY_XG|STBY_YG|STBY_ZG|
|72|114|FIFO_COUNTH|R/W|FIFO_COUNT[15]|FIFO_COUNT[14]|FIFO_COUNT[13]|FIFO_COUNT[12]|FIFO_COUNT[11]|FIFO_COUNT[10]|FIFO_COUNT[9]|FIFO_COUNT[8]|
|73|115|FIFO_COUNTL|R/W|FIFO_COUNT[7]|FIFO_COUNT[6]|FIFO_COUNT[5]|FIFO_COUNT[4]|FIFO_COUNT[3]|FIFO_COUNT[2]|FIFO_COUNT[1]|FIFO_COUNT[0]|
|74|116|FIFO_R_W|R/W|FIFO_DATA[7]|FIFO_DATA[6]|FIFO_DATA[5]|FIFO_DATA[4]|FIFO_DATA[3]|FIFO_DATA[2]|FIFO_DATA[1]|FIFO_DATA[0]|
|75|117|WHO_AM_I|R|-|WHO_AM_I[6]|WHO_AM_I[5]|WHO_AM_I[4]|WHO_AM_I[3]|WHO_AM_I[2]|WHO_AM_I[1]|-|

<u>Note:</u> Register Names ending in _H and _L contain the high and low bytes, respectively, of an internal register value.

In the detailed register tables that follow, register names are in capital letters, while register values are in capital letters and italicized. For example, the ACCEL_XOUT_H register (Register 59) contains the 8 most significant bits, `ACCEL_XOUT` [15:8], of the 16-bit X-Axis accelerometer measurement, `ACCEL_XOUT` .

The reset value is 0x00 for all registers other than the registers below.

- Register 107: 0x40.

- Register 117: 0x68.

<a id="section-4"></a>

## 4 Register Descriptions

This section describes the function and contents of each register within the MPU-60X0.

<u>Note: The device will come up in sleep mode upon power-up.</u>

<a id="section-4-1"></a>

### 4.1 Register 1 – Auxiliary I²C Supply Selection

**AUX_VDDIO**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|01|1|AUX_VDDIO|-|-|-|-|-|-|-|

#### Description

This register specifies the auxiliary I²C supply voltage level.

For MPU-6050: `AUX_VDDIO` configures the high logic level of the auxiliary I²C bus to be either VLOGIC or VDD.

For MPU-6000: `AUX_VDDIO` should be written as 0.

Bits 6 through 0 are reserved.

#### Parameters

`AUX_VDDIO` MPU-6050: When set to 1, the auxiliary I²C bus high logic level is VDD. When cleared to 0, the auxiliary I²C bus high logic level is VLOGIC

MPU-6000: Write 0 for `AUX_VDDIO` .

<a id="section-4-2"></a>

### 4.2 Register 25 – Sample Rate Divider

**SMPRT_DIV**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|19|25|SMPLRT_DIV[7]|SMPLRT_DIV[6]|SMPLRT_DIV[5]|SMPLRT_DIV[4]|SMPLRT_DIV[3]|SMPLRT_DIV[2]|SMPLRT_DIV[1]|SMPLRT_DIV[0]|

#### Description

This register specifies the divider from the gyroscope output rate used to generate the Sample Rate for the MPU-60X0.

The sensor register output, FIFO output, DMP sampling, Motion detection, Zero Motion detection, and Free Fall detection are all based on the Sample Rate.

The Sample Rate is generated by dividing the gyroscope output rate by `SMPLRT_DIV` :

Sample Rate = Gyroscope Output Rate / (1 + `SMPLRT_DIV` )

where Gyroscope Output Rate = 8kHz when the DLPF is disabled ( `DLPF_CFG` = 0 or 7), and 1kHz when the DLPF is enabled (see Register 26).

<u>Note:</u> <u>The accelerometer output rate is 1kHz.</u> This means that for a Sample Rate greater than 1kHz, the same accelerometer sample may be output to the FIFO, DMP, and sensor registers more than once.

For a diagram of the gyroscope and accelerometer signal paths, see Section 8 of the MPU 6000/MPU-6050 Product Specification document.

#### Parameters

`SMPLRT_DIV` 8-bit unsigned value. The Sample Rate is determined by dividing the gyroscope output rate by this value.

<a id="section-4-3"></a>

### 4.3 Register 26 – Configuration

**CONFIG**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|1A|26|-|-|EXT_SYNC_SET[2]|EXT_SYNC_SET[1]|EXT_SYNC_SET[0]|DLPF_CFG[2]|DLPF_CFG[1]|DLPF_CFG[0]|

#### Description

This register configures the external Frame Synchronization (FSYNC) pin sampling and the Digital Low Pass Filter (DLPF) setting for both the gyroscopes and accelerometers.

An external signal connected to the FSYNC pin can be sampled by configuring `EXT_SYNC_SET` .

Signal changes to the FSYNC pin are latched so that short strobes may be captured. The latched FSYNC signal will be sampled at the Sampling Rate, as defined in register 25. After sampling, the latch will reset to the current FSYNC signal state.

The sampled value will be reported in place of the least significant bit in a sensor data register determined by the value of `EXT_SYNC_SET` according to the following table.

|EXT_SYNC_SET|FSYNC Bit Location|
|---|---|
|0|Input disabled|
|1|TEMP_OUT_L[0]|
|2|GYRO_XOUT_L[0]|
|3|GYRO_YOUT_L[0]|
|4|GYRO_ZOUT_L[0]|
|5|ACCEL_XOUT_L[0]|
|6|ACCEL_YOUT_L[0]|
|7|ACCEL_ZOUT_L[0]|

The DLPF is configured by `DLPF_CFG` . The accelerometer and gyroscope are filtered according to the value of `DLPF_CFG` as shown in the table below.

|DLPF_CFG|Accelerometer bandwidth (Hz), Fs = 1 kHz|Accelerometer delay (ms)|Gyroscope bandwidth (Hz)|Gyroscope delay (ms)|Gyroscope Fs (kHz)|
|---|---|---|---|---|---|
|0|260|0|256|0.98|8|
|1|184|2.0|188|1.9|1|
|2|94|3.0|98|2.8|1|
|3|44|4.9|42|4.8|1|
|4|21|8.5|20|8.3|1|
|5|10|13.8|10|13.4|1|
|6|5|19.0|5|18.6|1|
|7|RESERVED|RESERVED|RESERVED|RESERVED|8|

Bit 7 and bit 6 are reserved.

#### Parameters

`EXT_SYNC_SET` 3-bit unsigned value. Configures the FSYNC pin sampling. `DLPF_CFG` 3-bit unsigned value. Configures the DLPF setting.

<a id="section-4-4"></a>

### 4.4 Register 27 – Gyroscope Configuration

**GYRO_CONFIG**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|1B|27|XG_ST|YG_ST|ZG_ST|FS_SEL[1]|FS_SEL[0]|-|-|-|

#### Description

This register is used to trigger gyroscope self-test and configure the gyroscopes’ full scale range.

Gyroscope self-test permits users to test the mechanical and electrical portions of the gyroscope. The self-test for each gyroscope axis can be activated by controlling the `XG_ST`, `YG_ST`, and `ZG_ST` bits of this register. Self-test for each axis may be performed independently or all at the same time.

When self-test is activated, the on-board electronics will actuate the appropriate sensor. This actuation will move the sensor’s proof masses over a distance equivalent to a pre-defined Coriolis force. This proof mass displacement results in a change in the sensor output, which is reflected in the output signal. The output signal is used to observe the self-test response.

The self-test response is defined as follows:

Self-test response = Sensor output with self-test enabled – Sensor output without selftest enabled

The self-test limits for each gyroscope axis is provided in the electrical characteristics tables of the MPU-6000/MPU-6050 Product Specification document. When the value of the self-test response is within the min/max limits of the product specification, the part has passed self test. When the self-test response exceeds the min/max values specified in the document, the part is deemed to have failed self-test.

`FS_SEL` selects the full scale range of the gyroscope outputs according to the following table.

Bits 2 through 0 are reserved.

#### Parameters

|FS_SEL|Full Scale Range|
|---|---|
|0|± 250 °/s|
|1|± 500 °/s|
|2|± 1000 °/s|
|3|± 2000 °/s|

`XG_ST` Setting this bit causes the X axis gyroscope to perform self test.

`YG_ST` Setting this bit causes the Y axis gyroscope to perform self test.

`ZG_ST` Setting this bit causes the Z axis gyroscope to perform self test.

`FS_SEL` 2-bit unsigned value. Selects the full scale range of gyroscopes.

<a id="section-4-5"></a>

### 4.5 Register 28 – Accelerometer Configuration

**ACCEL_CONFIG**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|1C|28|XA_ST|YA_ST|ZA_ST|AFS_SEL[1]|AFS_SEL[0]|ACCEL_HPF[2]|ACCEL_HPF[1]|ACCEL_HPF[0]|

#### Description

This register is used to trigger accelerometer self test and configure the accelerometer full scale range. This register also configures the Digital High Pass Filter (DHPF).

Accelerometer self-test permits users to test the mechanical and electrical portions of the accelerometer. The self-test for each accelerometer axis can be activated by controlling the `XA_ST`, `YA_ST`, and `ZA_ST` bits of this register. Self-test for each axis may be performed independently or all at the same time.

When self-test is activated, the on-board electronics will actuate the appropriate sensor. This actuation simulates an external force. The actuated sensor, in turn, will produce a corresponding output signal. The output signal is used to observe the self-test response.

The self-test response is defined as follows:

Self-test response = Sensor output with self-test enabled – Sensor output without self-test enabled

The self-test limits for each accelerometer axis is provided in the electrical characteristics tables of the MPU-6000/MPU-6050 Product Specification document. When the value of the self-test response is within the min/max limits of the product specification, the part has passed self test. When the selftest response exceeds the min/max values specified in the document, the part is deemed to have failed self-test.

`AFS_SEL` selects the full scale range of the accelerometer outputs according to the following table.

|AFS_SEL|Full Scale Range|
|---|---|
|0|± 2 _g_|
|1|± 4 _g_|
|2|± 8 _g_|
|3|± 16 _g_|

`ACCEL_HPF` configures the DHPF available in the path leading to motion detectors (Free Fall, Motion threshold, and Zero Motion). The high pass filter output is not available to the data registers (see Figure in Section 8 of the MPU-6000/MPU-6050 Product Specification document).

The high pass filter has three modes:

- Reset: The filter output settles to zero within one sample. This effectively disables the high pass filter. This mode may be toggled to quickly settle the filter.

- On: The high pass filter will pass signals above the cut off frequency.

- Hold: When triggered, the filter holds the present sample. The filter output will be the difference between the input sample and the held sample.

|ACCEL_HPF|Filter Mode|Cut-off Frequency|
|---|---|---|
|0|Reset|None|
|1|On|5Hz|
|2|On|2.5Hz|
|3|On|1.25Hz|
|4|On|0.63Hz|
|7|Hold|None|

#### Parameters

`XA_ST` When set to 1, the X- Axis accelerometer performs self test.

`YA_ST` When set to 1, the Y- Axis accelerometer performs self test.

`ZA_ST` When set to 1, the Z- Axis accelerometer performs self test.

`ACCEL_FS_SEL` 2-bit unsigned value. Selects the full scale range of accelerometers.

`ACCEL_HPF` 3-bit unsigned value. Selects the Digital High Pass Filter configuration.

<a id="section-4-6"></a>

### 4.6 Register 29 – Free Fall Acceleration Threshold

**FF_THR**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|1D|29|FF_THR[7]|FF_THR[6]|FF_THR[5]|FF_THR[4]|FF_THR[3]|FF_THR[2]|FF_THR[1]|FF_THR[0]|

#### Description

This register configures the detection threshold for Free Fall event detection. The m _g_ per LSB increment for `FF_THR` can be found in the Electrical Specifications table of the MPU-6000/MPU6050 Product Specification document.

Free Fall is detected when the absolute value of the accelerometer measurements for the three axes are each less than the detection threshold. This condition increments the Free Fall duration counter (Register 30). The Free Fall interrupt is triggered when the Free Fall duration counter reaches the time specified in `FF_DUR` (Register 30).

For more details on the Free Fall detection interrupt, see Section 8.2 of the MPU-6000/MPU-6050 Product Specification document as well as Registers 56 and 58 of this document.

#### Parameters

`FF_THR` 8-bit unsigned value specifying the Free Fall detection threshold.

<a id="section-4-7"></a>

### 4.7 Register 30 – Free Fall Duration

**FF_DUR**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|1E|30|FF_DUR[7]|FF_DUR[6]|FF_DUR[5]|FF_DUR[4]|FF_DUR[3]|FF_DUR[2]|FF_DUR[1]|FF_DUR[0]|

#### Description

This register configures the duration counter threshold for Free Fall event detection. The duration counter ticks at 1kHz, therefore `FF_DUR` has a unit of 1 LSB = 1 ms.

The Free Fall duration counter increments while the absolute value of the accelerometer measurements are each less than the detection threshold (Register 29). The Free Fall interrupt is triggered when the Free Fall duration counter reaches the time specified in this register.

For more details on the Free Fall detection interrupt, see Section 8.2 of the MPU-6000/MPU-6050 Product Specification document as well as Registers 56 and 58 of this document.

#### Parameters

`FF_DUR` 8-bit unsigned value. Specifies the duration counter threshold. Unit of LSB = 1ms.

<a id="section-4-8"></a>

### 4.8 Register 31 – Motion Detection Threshold

**MOT_THR**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|1F|31|MOT_THR[7]|MOT_THR[6]|MOT_THR[5]|MOT_THR[4]|MOT_THR[3]|MOT_THR[2]|MOT_THR[1]|MOT_THR[0]|

#### Description

This register configures the detection threshold for Motion interrupt generation. The m _g_ per LSB increment for `MOT_THR` can be found in the Electrical Specifications table of the MPU-6000/MPU6050 Product Specification document.

Motion is detected when the absolute value of any of the accelerometer measurements exceeds this Motion detection threshold. This condition increments the Motion detection duration counter (Register 32). The Motion detection interrupt is triggered when the Motion Detection counter reaches the time count specified in `MOT_DUR` (Register 32).

The Motion interrupt will indicate the axis and polarity of detected motion in MOT_DETECT _STATUS (Register 97).

For more details on the Motion detection interrupt, see Section 8.3 of the MPU-6000/MPU-6050 Product Specification document as well as Registers 56 and 58 of this document.

#### Parameters

`MOT_THR` 8-bit unsigned value. Specifies the Motion detection threshold.

<a id="section-4-9"></a>

### 4.9 Register 32 – Motion Detection Duration

**MOT_DUR**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|20|32|MOT_DUR[7]|MOT_DUR[6]|MOT_DUR[5]|MOT_DUR[4]|MOT_DUR[3]|MOT_DUR[2]|MOT_DUR[1]|MOT_DUR[0]|

#### Description

This register configures the duration counter threshold for Motion interrupt generation. The duration counter ticks at 1 kHz, therefore `MOT_DUR` has a unit of 1 LSB = 1 ms.

The Motion detection duration counter increments when the absolute value of any of the accelerometer measurements exceeds the Motion detection threshold (Register 31). The Motion detection interrupt is triggered when the Motion detection counter reaches the time count specified in this register.

For more details on the Motion detection interrupt, see Section 8.3 of the MPU-6000/MPU-6050 Product Specification document.

#### Parameters

`MOT_DUR` 8-bit unsigned value. Specifies the duration counter threshold. Unit of 1 LSB = 1ms.

<a id="section-4-10"></a>

### 4.10 Register 33 – Zero Motion Detection Threshold

**ZRMOT_THR**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|21|33|ZRMOT_THR[7]|ZRMOT_THR[6]|ZRMOT_THR[5]|ZRMOT_THR[4]|ZRMOT_THR[3]|ZRMOT_THR[2]|ZRMOT_THR[1]|ZRMOT_THR[0]|

#### Description

This register configures the detection threshold for Zero Motion interrupt generation. The m _g_ per LSB increment for `ZRMOT_THR` can be found in the Electrical Specifications table of the MPU6000/MPU-6050 Product Specification document.

Zero Motion is detected when the absolute value of the accelerometer measurements for the 3 axes are each less than the detection threshold. This condition increments the Zero Motion duration counter (Register 34). The Zero Motion interrupt is triggered when the Zero Motion duration counter reaches the time count specified in `ZRMOT_DUR` (Register 34).

Unlike Free Fall or Motion detection, Zero Motion detection triggers an interrupt both when Zero Motion is first detected and when Zero Motion is no longer detected.

When a zero motion event is detected, a Zero Motion Status will be indicated in the MOT_DETECT_STATUS register (Register 97). When a motion-to-zero-motion condition is detected, the status bit is set to 1. When a zero-motion-to-motion condition is detected, the status bit is set to 0.

For more details on the Zero Motion detection interrupt, see Section 8.4 of the MPU-6000/MPU-6050 Product Specification document as well as Registers 56 and 58 of this document.

#### Parameters

`ZRMOT_THR` 8-bit unsigned value. Specifies the Zero Motion detection threshold.

<a id="section-4-11"></a>

### 4.11 Register 34 – Zero Motion Detection Duration

**ZRMOT_DUR**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|22|34|ZRMOT_DUR[7]|ZRMOT_DUR[6]|ZRMOT_DUR[5]|ZRMOT_DUR[4]|ZRMOT_DUR[3]|ZRMOT_DUR[2]|ZRMOT_DUR[1]|ZRMOT_DUR[0]|

#### Description

This register configures the duration counter threshold for Zero Motion interrupt generation. The duration counter ticks at 16 Hz, therefore `ZRMOT_DUR` has a unit of 1 LSB = 64 ms.

The Zero Motion duration counter increments while the absolute value of the accelerometer measurements are each less than the detection threshold (Register 33). The Zero Motion interrupt is triggered when the Zero Motion duration counter reaches the time count specified in this register.

For more details on the Zero Motion detection interrupt, see Section 8.4 of the MPU-6000/MPU-6050 Product Specification document, as well as Registers 56 and 58 of this document.

#### Parameters

`ZRMOT_DUR` 8-bit unsigned value. Specifies the duration counter threshold. Unit of 1 LSB = 64ms.

<a id="section-4-12"></a>

### 4.12 Register 35 – FIFO Enable

**FIFO_EN**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|23|35|TEMP_FIFO_EN|XG_FIFO_EN|YG_FIFO_EN|ZG_FIFO_EN|ACCEL_FIFO_EN|SLV2_FIFO_EN|SLV1_FIFO_EN|SLV0_FIFO_EN|

#### Description

This register determines which sensor measurements are loaded into the FIFO buffer.

Data stored inside the sensor data registers (Registers 59 to 96) will be loaded into the FIFO buffer if a sensor’s respective FIFO_EN bit is set to 1 in this register.

When a sensor’s FIFO_EN bit is enabled in this register, data from the sensor data registers will be loaded into the FIFO buffer. The sensors are sampled at the Sample Rate as defined in Register 25. For further information regarding sensor data registers, please refer to Registers 59 to 96

When an external Slave’s corresponding FIFO_EN bit ( _SLVx_FIFO_EN,_ where x=0, 1, or 2) is set to 1, the data stored in its corresponding data registers (EXT_SENS_DATA registers, Registers 73 to 96) will be written into the FIFO buffer at the Sample Rate. EXT_SENS_DATA register association with I²C Slaves is determined by the I2C_SLVx_CTRL registers (where x=0, 1, or 2; Registers 39,

42, and 45). For information regarding EXT_SENS_DATA registers, please refer to Registers 73 to 96.

Note that the corresponding FIFO_EN bit ( _SLV3_FIFO_EN)_ is found in I2C_MST_CTRL (Register

36). Also note that Slave 4 behaves in a different manner compared to Slaves 0-3. Please refer to Registers 49 to 53 for further information regarding Slave 4 usage.

#### Parameters

`TEMP_FIFO_EN` When set to 1, this bit enables TEMP_OUT_H and TEMP_OUT_L (Registers 65 and 66) to be written into the FIFO buffer.

`XG` FIFO_EN_ When set to 1, this bit enables GYRO_XOUT_H and GYRO_XOUT_L (Registers 67 and 68) to be written into the FIFO buffer.

`YG` FIFO_EN_ When set to 1, this bit enables GYRO_YOUT_H and GYRO_YOUT_L (Registers 69 and 70) to be written into the FIFO buffer.

`ZG` FIFO_EN_ When set to 1, this bit enables GYRO_ZOUT_H and GYRO_ZOUT_L (Registers 71 and 72) to be written into the FIFO buffer.

`ACCEL` FIFO_EN_ When set to 1, this bit enables ACCEL_XOUT_H, ACCEL_XOUT_L, ACCEL_YOUT_H, ACCEL_YOUT_L, ACCEL_ZOUT_H, and ACCEL_ZOUT_L (Registers 59 to 64) to be written into the FIFO buffer.

`SLV2` FIFO_EN_ When set to 1, this bit enables EXT_SENS_DATA registers (Registers 73 to 96) associated with Slave 2 to be written into the FIFO buffer.

`SLV1` FIFO_EN_ When set to 1, this bit enables EXT_SENS_DATA registers (Registers 73 to 96) associated with Slave 1 to be written into the FIFO buffer.

`SLV0` FIFO_EN_ When set to 1, this bit enables EXT_SENS_DATA registers (Registers 73 to 96) associated with Slave 0 to be written into the FIFO buffer.

<u>Note: For further information regarding the association of EXT_SENS_DATA registers to particular</u> slave devices, please refer to Registers 73 to 96.

<a id="section-4-13"></a>

### 4.13 Register 36 – I²C Master Control

**I2C_MST_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|24|36|MULT_MST_EN|WAIT_FOR_ES|SLV_3_FIFO_EN|I2C_MST_P_NSR|I2C_MST_CLK[3]|I2C_MST_CLK[2]|I2C_MST_CLK[1]|I2C_MST_CLK[0]|

#### Description

This register configures the auxiliary I²C bus for single-master or multi-master control. In addition, the register is used to delay the Data Ready interrupt, and also enables the writing of Slave 3 data into the FIFO buffer. The register also configures the auxiliary I²C Master’s transition from one slave read to the next, as well as the MPU-60X0’s 8MHz internal clock.

Multi-master capability allows multiple I²C masters to operate on the same bus. In circuits where multi-master capability is required, set `MULT_MST_EN` to 1. This will increase current drawn by approximately 30µA.

In circuits where multi-master capability is required, the state of the I²C bus must always be monitored by each separate I²C Master. Before an I²C Master can assume arbitration of the bus, it must first confirm that no other I²C Master has arbitration of the bus. When `MULT_MST_EN` is set to 1, the MPU-60X0’s bus arbitration detection logic is turned on, enabling it to detect when the bus is available.

When the `WAIT_FOR_ES` bit is set to 1, the Data Ready interrupt will be delayed until External Sensor data from the Slave Devices are loaded into the EXT_SENS_DATA registers. This is used to ensure that both the internal sensor data (i.e. from gyro and accel) and external sensor data have been loaded to their respective data registers (i.e. the data is synced) when the Data Ready interrupt is triggered.

When the Slave 3 FIFO enable bit ( `SLV_3_FIFO_EN` ) is set to 1, Slave 3 sensor measurement data will be loaded into the FIFO buffer each time. EXT_SENS_DATA register association with I²C Slaves is determined by I2C_SLV3_CTRL (Register 48).

For further information regarding EXT_SENS_DATA registers, please refer to Registers 73 to 96.

The corresponding FIFO_EN bits for Slave 0, Slave 1, and Slave 2 can be found in Register 35.

The `I2C_MST_P_NSR` bit configures the I²C Master’s transition from one slave read to the next slave read. If the bit equals 0, there will be a restart between reads. If the bit equals 1, there will be a stop followed by a start of the following read. When a write transaction follows a read transaction, the stop followed by a start of the successive write will be always used.

`I2C_MST_CLK` is a 4 bit unsigned value which configures a divider on the MPU-60X0 internal 8MHz clock. It sets the I²C master clock speed according to the following table:

|I2C_MST_CLK|I2C Master Clock<br>Speed|8MHz Clock<br>Divider|
|---|---|---|
|0|348 kHz|23|
|1|333 kHz|24|
|2|320 kHz|25|
|3|308 kHz|26|
|4|296 kHz|27|
|5|286 kHz|28|
|6|276 kHz|29|
|7|267 kHz|30|
|8|258 kHz|31|
|9|500 kHz|16|
|10|471 kHz|17|
|11|444 kHz|18|
|12|421 kHz|19|
|13|400 kHz|20|
|14|381 kHz|21|
|15|364 kHz|22|

#### Parameters

`MUL_MST_EN` When set to 1, this bit enables multi-master capability.

`WAIT_FOR_ES` When set to 1, this bit delays the Data Ready interrupt until External Sensor data from the Slave devices have been loaded into the EXT_SENS_DATA registers.

`SLV3_FIFO_EN` When set to 1, this bit enables EXT_SENS_DATA registers associated with Slave 3 to be written into the FIFO. The corresponding bits for Slaves 0-2 can be found in Register 35.

`I2C_MST_P_NSR` Controls the I²C Master’s transition from one slave read to the next slave read. When this bit equals 0, there is a restart between reads. When this bit equals 1, there is a stop and start marking the beginning of the next read.

When a write follows a read, a stop and start is always enforced.

`I2C_MST_CLK` 4 bit unsigned value. Configures the I²C master clock speed divider.

<u>Note: For further information regarding the association of EXT_SENS_DATA registers to particular</u> slave devices, please refer to Registers 73 to 96.

<a id="section-4-14"></a>

### 4.14 Registers 37 to 39 – I²C Slave 0 Control

**I2C_SLV0_ADDR, I2C_SLV0_REG, and I2C_SLV0_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|25|37|I2C_SLV0_RW|I2C_SLV0_ADDR[6]|I2C_SLV0_ADDR[5]|I2C_SLV0_ADDR[4]|I2C_SLV0_ADDR[3]|I2C_SLV0_ADDR[2]|I2C_SLV0_ADDR[1]|I2C_SLV0_ADDR[0]|
|26|38|I2C_SLV0_REG[7]|I2C_SLV0_REG[6]|I2C_SLV0_REG[5]|I2C_SLV0_REG[4]|I2C_SLV0_REG[3]|I2C_SLV0_REG[2]|I2C_SLV0_REG[1]|I2C_SLV0_REG[0]|
|27|39|I2C_SLV0_EN|I2C_SLV0_BYTE_SW|I2C_SLV0_REG_DIS|I2C_SLV0_GRP|I2C_SLV0_LEN[3]|I2C_SLV0_LEN[2]|I2C_SLV0_LEN[1]|I2C_SLV0_LEN[0]|

#### Description

These registers configure the data transfer sequence for Slave 0. Slaves 1, 2, and 3 also behave in a similar manner to Slave 0. However, Slave 4’s characteristics differ greatly from those of Slaves 0-3. For further information regarding Slave 4, please refer to registers 49 to 53.

I²C slave data transactions between the MPU-60X0 and Slave 0 are set as either read or write operations by the `I2C_SLV0_RW` bit. When this bit is 1, the transfer is a read operation. When the bit is 0, the transfer is a write operation.

`I2C_SLV0_ADDR` is used to specify the I²C slave address of Slave 0 _._

Data transfer starts at an internal register within Slave 0. This address of this register is specified by `I2C_SLV0_REG` .

The number of bytes transferred is specified by _I2C_SLV0_LEN._ When more than 1 byte is transferred ( `I2C_SLV0_LEN`     - 1), data is read from (written to) sequential addresses starting from

`I2C_SLV0_REG` .

In read mode, the result of the read is placed in the lowest available EXT_SENS_DATA register. For further information regarding the allocation of read results, please refer to the EXT_SENS_DATA register description (Registers 73 – 96).

In write mode, the contents of `I2C_SLV0_DO` (Register 99) will be written to the slave device.

`I2C_SLV0_EN` enables Slave 0 for I²C data transaction. A data transaction is performed only if more than zero bytes are to be transferred ( `I2C_SLV0_LEN`     - 0) between an enabled slave device ( `I2C_SLV0_EN` = 1).

`I2C_SLV0_BYTE_SW` configures byte swapping of word pairs. When byte swapping is enabled, the high and low bytes of a word pair are swapped. Please refer to `I2C_SLV0_GRP` for the pairing convention of the word pairs. When this bit is cleared to 0, bytes transferred to and from Slave 0 will be written to EXT_SENS_DATA registers in the order they were transferred.

When `I2C_SLV0_REG_DIS` is set to 1, the transaction will read or write data only. When cleared to 0, the transaction will write a register address prior to reading or writing data. This bit should equal 0 when specifying the register address within the Slave device to/from which the ensuing data transaction will take place.

`I2C_SLV0_GRP` specifies the grouping order of word pairs received from registers. When cleared to 0, bytes from register addresses 0 and 1, 2 and 3, etc (even, then odd register addresses) are paired to form a word. When set to 1, bytes from register addresses are paired 1 and 2, 3 and 4, etc. (odd, then even register addresses) are paired to form a word.

I²C data transactions are performed at the Sample Rate, as defined in Register 25. The user is responsible for ensuring that I²C data transactions to and from each enabled Slave can be completed within a single period of the Sample Rate.

The I²C slave access rate can be reduced relative to the Sample Rate. This reduced access rate is determined by `I2C_MST_DLY` (Register 52). Whether a slave’s access rate is reduced relative to the Sample Rate is determined by I2C_MST_DELAY_CTRL (Register 103).

The processing order for the slaves is fixed. The sequence followed for processing the slaves is Slave 0, Slave 1, Slave 2, Slave 3 and Slave 4. If a particular Slave is disabled it will be skipped.

Each slave can either be accessed at the sample rate or at a reduced sample rate. In a case where some slaves are accessed at the Sample Rate and some slaves are accessed at the reduced rate, the sequence of accessing the slaves (Slave 0 to Slave 4) is still followed. However, the reduced rate slaves will be skipped if their access rate dictates that they should not be accessed during that particular cycle. For further information regarding the reduced access rate, please refer to Register 52. Whether a slave is accessed at the Sample Rate or at the reduced rate is determined by the Delay Enable bits in Register 103.

#### Parameters

`I2C_SLV0_RW` When set to 1, this bit configures the data transfer as a read operation. When cleared to 0, this bit configures the data transfer as a write operation. `I2C_SLV0_ADDR` 7-bit I²C address of Slave 0.

`I2C_SLV0_REG` 8-bit address of the Slave 0 register to/from which data transfer starts.

`I2C_SLV0_EN` When set to 1, this bit enables Slave 0 for data transfer operations. When cleared to 0, this bit disables Slave 0 from data transfer operations.

`I2C_SLV0_BYTE_SW` When set to 1, this bit enables byte swapping. When byte swapping is enabled, the high and low bytes of a word pair are swapped. Please refer to `I2C_SLV0_GRP` for the pairing convention of the word pairs.

When cleared to 0, bytes transferred to and from Slave 0 will be written to EXT_SENS_DATA registers in the order they were transferred.

`I2C_SLV0_REG_DIS` When set to 1, the transaction will read or write data only. When cleared to 0, the transaction will write a register address prior to reading or writing data.

`I2C_SLV0_GRP` 1-bit value specifying the grouping order of word pairs received from registers. When cleared to 0, bytes from register addresses 0 and 1, 2 and 3, etc (even, then odd register addresses) are paired to form a word. When set to 1, bytes from register addresses are paired 1 and 2, 3 and 4, etc. (odd, then even register addresses) are paired to form a word.

`I2C_SLV0_LEN` 4-bit unsigned value. Specifies the number of bytes transferred to and from Slave 0. Clearing this bit to 0 is equivalent to disabling the register by writing 0 to `I2C_SLV0_EN` .

**<u>Byte Swapping Example</u>** The following example demonstrates byte swapping for `I2C_SLV0_BYTE_SW` = 1, `I2C_SLV0_GRP` = 0, `I2C_SLV0_REG` = 0x01, and `I2C_SLV0_LEN` = 0x4:

1. The first byte, read from Slave 0 register 0x01, will be stored at EXT_SENS_DATA_00. Because `I2C_SLV0_GRP` = 0, bytes from even, then odd register addresses will be paired together as word pairs. Since the read operation started from an odd register address instead of an even address, only one byte is read.
2. The second and third bytes will be swapped, since `I2C_SLV0_BYTE_SW` = 1 and _I2C_SLV0_REG[0]_ = 1. The data read from 0x02 will be stored at EXT_SENS_DATA_02, and the data read from 0x03 will be stored at EXT_SENS_DATA_01.
3. The last byte, read from address 0x04, will be stored at EXT_SENS_DATA_03. Because there is only one byte remaining in the read operation, byte swapping will not occur.

**<u>Slave Access Example</u>** Slave 0 is accessed at the Sample Rate, while Slave 1 is accessed at half the Sample Rate. The other slaves are disabled. In the first cycle, both Slave 0 and Slave 1 will be accessed. However, in the second cycle, only Slave 0 will be accessed. In the third cycle, both Slave 0 and Slave 1 will be accessed. In the fourth cycle, only Slave 0 will be accessed. This pattern continues.

<a id="section-4-15"></a>

### 4.15 Registers 40 to 42 – I²C Slave 1 Control

**I2C_SLV1_ADDR, I2C_SLV1_REG, and I2C_SLV1_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|28|40|I2C_SLV1_RW|I2C_SLV1_ADDR[6]|I2C_SLV1_ADDR[5]|I2C_SLV1_ADDR[4]|I2C_SLV1_ADDR[3]|I2C_SLV1_ADDR[2]|I2C_SLV1_ADDR[1]|I2C_SLV1_ADDR[0]|
|29|41|I2C_SLV1_REG[7]|I2C_SLV1_REG[6]|I2C_SLV1_REG[5]|I2C_SLV1_REG[4]|I2C_SLV1_REG[3]|I2C_SLV1_REG[2]|I2C_SLV1_REG[1]|I2C_SLV1_REG[0]|
|2A|42|I2C_SLV1_EN|I2C_SLV1_BYTE_SW|I2C_SLV1_REG_DIS|I2C_SLV1_GRP|I2C_SLV1_LEN[3]|I2C_SLV1_LEN[2]|I2C_SLV1_LEN[1]|I2C_SLV1_LEN[0]|

#### Description

These registers describe the data transfer sequence for Slave 1. Their functions correspond to those described for the Slave 0 registers (Registers 37 to 39).

<a id="section-4-16"></a>

### 4.16 Registers 43 to 45 – I²C Slave 2 Control

**I2C_SLV2_ADDR, I2C_SLV2_REG, and I2C_SLV2_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|2B|43|I2C_SLV2_RW|I2C_SLV2_ADDR[6]|I2C_SLV2_ADDR[5]|I2C_SLV2_ADDR[4]|I2C_SLV2_ADDR[3]|I2C_SLV2_ADDR[2]|I2C_SLV2_ADDR[1]|I2C_SLV2_ADDR[0]|
|2C|44|I2C_SLV2_REG[7]|I2C_SLV2_REG[6]|I2C_SLV2_REG[5]|I2C_SLV2_REG[4]|I2C_SLV2_REG[3]|I2C_SLV2_REG[2]|I2C_SLV2_REG[1]|I2C_SLV2_REG[0]|
|2D|45|I2C_SLV2_EN|I2C_SLV2_BYTE_SW|I2C_SLV2_REG_DIS|I2C_SLV2_GRP|I2C_SLV2_LEN[3]|I2C_SLV2_LEN[2]|I2C_SLV2_LEN[1]|I2C_SLV2_LEN[0]|

#### Description

These registers describe the data transfer sequence for Slave 2. Their functions correspond to those described for the Slave 0 registers (Registers 37 to 39).

<a id="section-4-17"></a>

### 4.17 Registers 46 to 48 – I²C Slave 3 Control

**I2C_SLV3_ADDR, I2C_SLV3_REG, and I2C_SLV3_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|2E|46|I2C_SLV3_RW|I2C_SLV3_ADDR[6]|I2C_SLV3_ADDR[5]|I2C_SLV3_ADDR[4]|I2C_SLV3_ADDR[3]|I2C_SLV3_ADDR[2]|I2C_SLV3_ADDR[1]|I2C_SLV3_ADDR[0]|
|2F|47|I2C_SLV3_REG[7]|I2C_SLV3_REG[6]|I2C_SLV3_REG[5]|I2C_SLV3_REG[4]|I2C_SLV3_REG[3]|I2C_SLV3_REG[2]|I2C_SLV3_REG[1]|I2C_SLV3_REG[0]|
|30|48|I2C_SLV3_EN|I2C_SLV3_BYTE_SW|I2C_SLV3_REG_DIS|I2C_SLV3_GRP|I2C_SLV3_LEN[3]|I2C_SLV3_LEN[2]|I2C_SLV3_LEN[1]|I2C_SLV3_LEN[0]|

#### Description

These registers describe the data transfer sequence for Slave 3. Their functions correspond to those described for the Slave 0 registers (Registers 37 to 39).

<a id="section-4-18"></a>

### 4.18 Registers 49 to 53 – I²C Slave 4 Control

**I2C_SLV4_ADDR, I2C_SLV4_REG, I2C_SLV4_DO, I2C_SLV4_CTRL, and I2C_SLV4_DI**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|31|49|I2C_SLV4_RW|I2C_SLV4_ADDR[6]|I2C_SLV4_ADDR[5]|I2C_SLV4_ADDR[4]|I2C_SLV4_ADDR[3]|I2C_SLV4_ADDR[2]|I2C_SLV4_ADDR[1]|I2C_SLV4_ADDR[0]|
|32|50|I2C_SLV4_REG[7]|I2C_SLV4_REG[6]|I2C_SLV4_REG[5]|I2C_SLV4_REG[4]|I2C_SLV4_REG[3]|I2C_SLV4_REG[2]|I2C_SLV4_REG[1]|I2C_SLV4_REG[0]|
|33|51|I2C_SLV4_DO[7]|I2C_SLV4_DO[6]|I2C_SLV4_DO[5]|I2C_SLV4_DO[4]|I2C_SLV4_DO[3]|I2C_SLV4_DO[2]|I2C_SLV4_DO[1]|I2C_SLV4_DO[0]|
|34|52|I2C_SLV4_EN|I2C_SLV4_INT_EN|I2C_SLV4_REG_DIS|I2C_MST_DLY[4]|I2C_MST_DLY[3]|I2C_MST_DLY[2]|I2C_MST_DLY[1]|I2C_MST_DLY[0]|
|35|53|I2C_SLV4_DI[7]|I2C_SLV4_DI[6]|I2C_SLV4_DI[5]|I2C_SLV4_DI[4]|I2C_SLV4_DI[3]|I2C_SLV4_DI[2]|I2C_SLV4_DI[1]|I2C_SLV4_DI[0]|

#### Description

These registers describe the data transfer sequence for Slave 4. The characteristics of Slave 4 differ greatly from those of Slaves 0-3. For further information regarding the characteristics of Slaves 0-3, please refer to Registers 37 to 48.

I²C slave data transactions between the MPU-60X0 and Slave 4 are set as either read or write operations by the `I2C_SLV4_RW` bit. When this bit is 1, the transfer is a read operation. When the bit is 0, the transfer is a write operation.

`I2C_SLV4_ADDR` is used to specify the I²C slave address of Slave 4 _._

Data transfer starts at an internal register within Slave 4. This register address is specified by `I2C_SLV4_REG` .

In read mode, the result of the read will be available in `I2C_SLV4_DI` . In write mode, the contents of `I2C_SLV4_DO` will be written into the slave device.

A data transaction is performed only if the `I2C_SLV4_EN` bit is set to 1. The data transaction should be enabled once its parameters are configured in the _ADDR and _REG registers. For write, the _DO register is also required. `I2C_SLV4_EN` will be cleared after the transaction is performed once.

An interrupt is triggered at the completion of a Slave 4 data transaction if the interrupt is enabled . The status of this interrupt can be observed in Register 54.

When `I2C_SLV4_REG_DIS` is set to 1, the transaction will read or write data instead of writing a register address. This bit should equal 0 when specifying the register address within the Slave device to/from which the ensuing data transaction will take place.

`I2C_MST_DLY` configures the reduced access rate of I²C slaves relative to the Sample Rate. When a slave’s access rate is decreased relative to the Sample Rate, the slave is accessed every

1 / (1 + `I2C_MST_DLY` ) samples

This base Sample Rate in turn is determined by `SMPLRT_DIV` (register 25) and `DLPF_CFG` (register 26). Whether a slave’s access rate is reduced relative to the Sample Rate is determined by I2C_MST_DELAY_CTRL (register 103).

For further information regarding the Sample Rate, please refer to register 25.

Slave 4 transactions are performed after Slave 0, 1, 2 and 3 transactions have been completed. Thus the maximum rate for Slave 4 transactions is determined by the Sample Rate as defined in Register 25.

#### Parameters

`I2C_SLV4_RW` When set to 1, this bit configures the data transfer as a read operation. When cleared to 0, this bit configures the data transfer as a write operation.

`I2C_SLV4_ADDR` 7-bit I²C address for Slave 4.

`I2C_SLV4_REG` 8-bit address of the Slave 4 register to/from which data transfer starts.

`I2C_SLV4_DO` This register stores the data to be written into the Slave 4. If `I2C_SLV4_RW` is set 1 (set to read), this register has no effect.

`I2C_SLV4_EN` When set to 1, this bit enables Slave 4 for data transfer operations.

When cleared to 0, this bit disables Slave 4 from data transfer operations.

`I2C_SLV4_INT_EN` When set to 1, this bit enables the generation of an interrupt signal upon completion of a Slave 4 transaction.

When cleared to 0, this bit disables the generation of an interrupt signal upon completion of a Slave 4 transaction.

The interrupt status can be observed in Register 54.

`I2C_SLV4_REG_DIS` When set to 1, the transaction will read or write data. When cleared to 0, the transaction will read or write a register address.

`I2C_MST_DLY` Configures the decreased access rate of slave devices relative to the Sample Rate.

`I2C_SLV4_DI` This register stores the data read from Slave 4.

This field is populated after a read transaction.

<a id="section-4-19"></a>

### 4.19 Register 54 – I²C Master Status

**I2C_MST_STATUS**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|36|54|PASS_THROUGH|I2C_SLV4_DONE|I2C_LOST_ARB|I2C_SLV4_NACK|I2C_SLV3_NACK|I2C_SLV2_NACK|I2C_SLV1_NACK|I2C_SLV0_NACK|

#### Description

This register shows the status of the interrupt generating signals in the I²C Master within the MPU60X0. This register also communicates the status of the FSYNC interrupt to the host processor.

Reading this register will clear all the status bits in the register.

#### Parameters

`PASS_THROUGH` This bit reflects the status of the FSYNC interrupt from an external device into the MPU-60X0. This is used as a way to pass an external interrupt through the MPU-60X0 to the host application processor. When set to 1, this bit will cause an interrupt if `FSYNC_INT_EN` is asserted in INT_PIN_CFG (Register 55).

`I2C_SLV4_DONE` Automatically sets to 1 when a Slave 4 transaction has completed. This triggers an interrupt if the `I2C_MST_INT_EN` bit in the INT_ENABLE register (Register 56) is asserted and if the `SLV_4_DONE_INT` bit is asserted in the I2C_SLV4_CTRL register (Register 52).

`I2C_LOST_ARB` This bit automatically sets to 1 when the I²C Master has lost arbitration of the auxiliary I²C bus (an error condition). This triggers an interrupt if the `I2C_MST_INT_EN` bit in the INT_ENABLE register (Register 56) is asserted.

`I2C_SLV4_NACK` This bit automatically sets to 1 when the I²C Master receives a NACK in a transaction with Slave 4. This triggers an interrupt if the `I2C_MST_INT_EN` bit in the INT_ENABLE register (Register 56) is asserted.

`I2C_SLV3_NACK` This bit automatically sets to 1 when the I²C Master receives a NACK in a transaction with Slave 3. This triggers an interrupt if the `I2C_MST_INT_EN` bit in the INT_ENABLE register (Register 56) is asserted.

`I2C_SLV2_NACK` This bit automatically sets to 1 when the I²C Master receives a NACK in a transaction with Slave 2. This triggers an interrupt if the `I2C_MST_INT_EN` bit in the INT_ENABLE register (Register 56) is asserted.

`I2C_SLV1_NACK` This bit automatically sets to 1 when the I²C Master receives a NACK in a transaction with Slave 1. This triggers an interrupt if the `I2C_MST_INT_EN` bit in the INT_ENABLE register (Register 56) is asserted.

`I2C_SLV0_NACK` This bit automatically sets to 1 when the I²C Master receives a NACK in a transaction with Slave 0. This triggers an interrupt if the `I2C_MST_INT_EN` bit in the INT_ENABLE register (Register 56) is asserted.

<a id="section-4-20"></a>

### 4.20 Register 55 – INT Pin / Bypass Enable Configuration

**INT_PIN_CFG**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|37|55|INT_LEVEL|INT_OPEN|LATCH_INT_EN|INT_RD_CLEAR|FSYNC_INT_LEVEL|FSYNC_INT_EN|I2C_BYPASS_EN|CLKOUT_EN|

#### Description

This register configures the behavior of the interrupt signals at the INT pins. This register is also used to enable the FSYNC Pin to be used as an interrupt to the host application processor, as well as to enable Bypass Mode on the I²C Master. This bit also enables the clock output.

`FSYNC_INT_EN` enables the FSYNC pin to be used as an interrupt to the host application processor. A transition to the active level specified in `FSYNC_INT_LEVEL` will trigger an interrupt. The status of this interrupt is read from the `PASS_THROUGH` bit in the I²C Master Status Register (Register 54).

When `I2C_BYPASS_EN` is equal to 1 and `I2C_MST_EN` (Register 106 bit[5]) is equal to 0, the host application processor will be able to directly access the auxiliary I²C bus of the MPU-60X0. When this bit is equal to 0, the host application processor will not be able to directly access the auxiliary I²C bus of the MPU-60X0 regardless of the state of _I2C_MST_EN._

For further information regarding Bypass Mode, please refer to Section 7.11 and 7.13 of the MPU6000/MPU-6050 Product Specification document.

#### Parameters

`INT_LEVEL` When this bit is equal to 0, the logic level for the INT pin is active high. When this bit is equal to 1, the logic level for the INT pin is active low.

`INT_OPEN` When this bit is equal to 0, the INT pin is configured as push-pull. When this bit is equal to 1, the INT pin is configured as open drain.

`LATCH_INT_EN` When this bit is equal to 0, the INT pin emits a 50us long pulse. When this bit is equal to 1, the INT pin is held high until the interrupt is cleared.

`INT_RD_CLEAR` When this bit is equal to 0, interrupt status bits are cleared only by reading INT_STATUS (Register 58)

When this bit is equal to 1, interrupt status bits are cleared on any read operation.

`FSYNC_INT_LEVEL` When this bit is equal to 0, the logic level for the FSYNC pin (when used as an interrupt to the host processor) is active high. When this bit is equal to 1, the logic level for the FSYNC pin (when used as an interrupt to the host processor) is active low.

`FSYNC_INT_EN` When equal to 0, this bit disables the FSYNC pin from causing an interrupt to the host processor.

When equal to 1, this bit enables the FSYNC pin to be used as an interrupt to the host processor.

`I2C_BYPASS_EN` When this bit is equal to 1 and `I2C_MST_EN` (Register 106 bit[5]) is equal to 0, the host application processor will be able to directly access the auxiliary I²C bus of the MPU-60X0. When this bit is equal to 0, the host application processor will not be able to directly access the auxiliary I²C bus of the MPU-60X0 regardless of the state of _I2C_MST_EN (_ Register 106 bit[5]).

`CLKOUT_EN` When this bit is equal to 1, a reference clock output is provided at the CLKOUT pin. When this bit is equal to 0, the clock output is disabled. For further information regarding CLKOUT, please refer to the MPU-60X0 Product Specification document.

<a id="section-4-21"></a>

### 4.21 Register 56 – Interrupt Enable

**INT_ENABLE**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|38|56|FF_EN|MOT_EN|ZMOT_EN|FIFO_OFLOW_EN|I2C_MST_INT_EN|-|-|DATA_RDY_EN|

#### Description

This register enables interrupt generation by interrupt sources.

For information regarding Free Fall detection, Motion detection, and Zero Motion detection, please refer to Registers 29 to 34

For information regarding the interrupt status for of each interrupt generation source, please refer to Register 58. Further information regarding I²C Master interrupt generation can be found in Register 54.

Bits 2 and 1 are reserved.

#### Parameters

`FF_EN` When set to 1, this bit enables Free Fall detection to generate an interrupt.

`MOT_EN` When set to 1, this bit enables Motion detection to generate an interrupt.

`ZMOT_EN` When set to 1, this bit enables Zero Motion detection to generate an interrupt.

`FIFO_OFLOW_EN` When set to 1, this bit enables a FIFO buffer overflow to generate an interrupt.

`I2C_MST_INT_EN` When set to 1, this bit enables any of the I²C Master interrupt sources to generate an interrupt.

`DATA_RDY_EN` When set to 1, this bit enables the Data Ready interrupt, which occurs each time a write operation to all of the sensor registers has been completed.

<a id="section-4-22"></a>

### 4.22 Register 58 – Interrupt Status

**INT_STATUS**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|3A|58|FF_INT|MOT_INT|ZMOT_INT|FIFO_OFLOW_INT|I2C_MST_INT|-|-|DATA_RDY_INT|

#### Description

This register shows the interrupt status of each interrupt generation source. Each bit will clear after the register is read.

For information regarding the corresponding interrupt enable bits, please refer to Register 56.

For a list of I²C Master interrupts, please refer to Register 54.

Bits 2 and 1 are reserved.

#### Parameters

`FF_INT` This bit automatically sets to 1 when a Free Fall interrupt has been generated.

The bit clears to 0 after the register has been read.

`MOT_INT` This bit automatically sets to 1 when a Motion Detection interrupt has been generated.

The bit clears to 0 after the register has been read.

`ZMOT_INT` This bit automatically sets to 1 when a Zero Motion Detection interrupt has been generated.

The bit clears to 0 after the register has been read.

`FIFO_OFLOW_INT` This bit automatically sets to 1 when a FIFO buffer overflow interrupt has been generated.

The bit clears to 0 after the register has been read.

`I2C_MST_INT` This bit automatically sets to 1 when an I²C Master interrupt has been generated. For a list of I²C Master interrupts, please refer to Register 54.

The bit clears to 0 after the register has been read.

`DATA_RDY_INT` This bit automatically sets to 1 when a Data Ready interrupt is generated.

The bit clears to 0 after the register has been read.

<a id="section-4-23"></a>

### 4.23 Registers 59 to 64 – Accelerometer Measurements

**ACCEL_XOUT_H, ACCEL_XOUT_L, ACCEL_YOUT_H, ACCEL_YOUT_L, ACCEL_ZOUT_H, and** **ACCEL_ZOUT_L**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|3B|59|ACCEL_XOUT[15]|ACCEL_XOUT[14]|ACCEL_XOUT[13]|ACCEL_XOUT[12]|ACCEL_XOUT[11]|ACCEL_XOUT[10]|ACCEL_XOUT[9]|ACCEL_XOUT[8]|
|3C|60|ACCEL_XOUT[7]|ACCEL_XOUT[6]|ACCEL_XOUT[5]|ACCEL_XOUT[4]|ACCEL_XOUT[3]|ACCEL_XOUT[2]|ACCEL_XOUT[1]|ACCEL_XOUT[0]|
|3D|61|ACCEL_YOUT[15]|ACCEL_YOUT[14]|ACCEL_YOUT[13]|ACCEL_YOUT[12]|ACCEL_YOUT[11]|ACCEL_YOUT[10]|ACCEL_YOUT[9]|ACCEL_YOUT[8]|
|3E|62|ACCEL_YOUT[7]|ACCEL_YOUT[6]|ACCEL_YOUT[5]|ACCEL_YOUT[4]|ACCEL_YOUT[3]|ACCEL_YOUT[2]|ACCEL_YOUT[1]|ACCEL_YOUT[0]|
|3F|63|ACCEL_ZOUT[15]|ACCEL_ZOUT[14]|ACCEL_ZOUT[13]|ACCEL_ZOUT[12]|ACCEL_ZOUT[11]|ACCEL_ZOUT[10]|ACCEL_ZOUT[9]|ACCEL_ZOUT[8]|
|40|64|ACCEL_ZOUT[7]|ACCEL_ZOUT[6]|ACCEL_ZOUT[5]|ACCEL_ZOUT[4]|ACCEL_ZOUT[3]|ACCEL_ZOUT[2]|ACCEL_ZOUT[1]|ACCEL_ZOUT[0]|

#### Description

These registers store the most recent accelerometer measurements.

Accelerometer measurements are written to these registers at the Sample Rate as defined in

Register 25.

The accelerometer measurement registers, along with the temperature measurement registers, gyroscope measurement registers, and external sensor data registers, are composed of two sets of registers: an internal register set and a user-facing read register set.

The data within the accelerometer sensors’ internal register set is always updated at the Sample Rate. Meanwhile, the user-facing read register set duplicates the internal register set’s data values whenever the serial interface is idle. This guarantees that a burst read of sensor registers will read measurements from the same sampling instant. Note that if burst reads are not used, the user is responsible for ensuring a set of single byte reads correspond to a single sampling instant by checking the Data Ready interrupt.

Each 16-bit accelerometer measurement has a full scale defined in `ACCEL_FS` (Register 28). For each full scale setting, the accelerometers’ sensitivity per LSB in _ACCEL_xOUT_ is shown in the table below.

#### Parameters

|AFS_SEL|Full Scale Range|LSB Sensitivity|
|---|---|---|
|0|±2_g_|16384 LSB/m_g_|
|1|±4_g_|8192 LSB/m_g_|
|2|±8_g_|4096 LSB/m_g_|
|3|±16_g_|2048 LSB/m_g_|

`ACCEL_XOUT` 16-bit 2’s complement value.

Stores the most recent X axis accelerometer measurement.

`ACCEL_YOUT` 16-bit 2’s complement value.

Stores the most recent Y axis accelerometer measurement.

`ACCEL_ZOUT` 16-bit 2’s complement value.

Stores the most recent Z axis accelerometer measurement.

<a id="section-4-24"></a>

### 4.24 Registers 65 and 66 – Temperature Measurement

**TEMP_OUT_H and TEMP_OUT_L**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|41|65|TEMP_OUT[15]|TEMP_OUT[14]|TEMP_OUT[13]|TEMP_OUT[12]|TEMP_OUT[11]|TEMP_OUT[10]|TEMP_OUT[9]|TEMP_OUT[8]|
|42|66|TEMP_OUT[7]|TEMP_OUT[6]|TEMP_OUT[5]|TEMP_OUT[4]|TEMP_OUT[3]|TEMP_OUT[2]|TEMP_OUT[1]|TEMP_OUT[0]|

#### Description

These registers store the most recent temperature sensor measurement.

Temperature measurements are written to these registers at the Sample Rate as defined in Register 25.

These temperature measurement registers, along with the accelerometer measurement registers, gyroscope measurement registers, and external sensor data registers, are composed of two sets of registers: an internal register set and a user-facing read register set.

The data within the temperature sensor’s internal register set is always updated at the Sample Rate. Meanwhile, the user-facing read register set duplicates the internal register set’s data values whenever the serial interface is idle. This guarantees that a burst read of sensor registers will read measurements from the same sampling instant. Note that if burst reads are not used, the user is responsible for ensuring a set of single byte reads correspond to a single sampling instant by checking the Data Ready interrupt.

The scale factor and offset for the temperature sensor are given in the [product specification](PS-MPU-6000A-EN.md#section-6-3). The register description cites Section 6.4; in Revision 3.2, the temperature sensor characteristics are listed in Section 6.3.

#### Parameters

`TEMP_OUT` is a 16-bit signed value that stores the most recent temperature sensor measurement.

<a id="section-4-25"></a>

### 4.25 Registers 67 to 72 – Gyroscope Measurements

**GYRO_XOUT_H, GYRO_XOUT_L, GYRO_YOUT_H, GYRO_YOUT_L, GYRO_ZOUT_H, and** **GYRO_ZOUT_L**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|43|67|GYRO_XOUT[15]|GYRO_XOUT[14]|GYRO_XOUT[13]|GYRO_XOUT[12]|GYRO_XOUT[11]|GYRO_XOUT[10]|GYRO_XOUT[9]|GYRO_XOUT[8]|
|44|68|GYRO_XOUT[7]|GYRO_XOUT[6]|GYRO_XOUT[5]|GYRO_XOUT[4]|GYRO_XOUT[3]|GYRO_XOUT[2]|GYRO_XOUT[1]|GYRO_XOUT[0]|
|45|69|GYRO_YOUT[15]|GYRO_YOUT[14]|GYRO_YOUT[13]|GYRO_YOUT[12]|GYRO_YOUT[11]|GYRO_YOUT[10]|GYRO_YOUT[9]|GYRO_YOUT[8]|
|46|70|GYRO_YOUT[7]|GYRO_YOUT[6]|GYRO_YOUT[5]|GYRO_YOUT[4]|GYRO_YOUT[3]|GYRO_YOUT[2]|GYRO_YOUT[1]|GYRO_YOUT[0]|
|47|71|GYRO_ZOUT[15]|GYRO_ZOUT[14]|GYRO_ZOUT[13]|GYRO_ZOUT[12]|GYRO_ZOUT[11]|GYRO_ZOUT[10]|GYRO_ZOUT[9]|GYRO_ZOUT[8]|
|48|72|GYRO_ZOUT[7]|GYRO_ZOUT[6]|GYRO_ZOUT[5]|GYRO_ZOUT[4]|GYRO_ZOUT[3]|GYRO_ZOUT[2]|GYRO_ZOUT[1]|GYRO_ZOUT[0]|

#### Description

These registers store the most recent gyroscope measurements.

Gyroscope measurements are written to these registers at the Sample Rate as defined in Register 25.

These gyroscope measurement registers, along with the accelerometer measurement registers, temperature measurement registers, and external sensor data registers, are composed of two sets of registers: an internal register set and a user-facing read register set.

The data within the gyroscope sensors’ internal register set is always updated at the Sample Rate. Meanwhile, the user-facing read register set duplicates the internal register set’s data values whenever the serial interface is idle. This guarantees that a burst read of sensor registers will read measurements from the same sampling instant. Note that if burst reads are not used, the user is responsible for ensuring a set of single byte reads correspond to a single sampling instant by checking the Data Ready interrupt.

Each 16-bit gyroscope measurement has a full scale defined in `FS_SEL` (Register 27). For each full scale setting, the gyroscopes’ sensitivity per LSB in _GYRO_xOUT_ is shown in the table below:

#### Parameters

|FS_SEL|Full Scale Range|LSB Sensitivity|
|---|---|---|
|0|± 250 °/s|131 LSB/°/s|
|1|± 500 °/s|65.5 LSB/°/s|
|2|± 1000 °/s|32.8 LSB/°/s|
|3|± 2000 °/s|16.4 LSB/°/s|

`GYRO_XOUT` 16-bit 2’s complement value.

Stores the most recent X axis gyroscope measurement.

`GYRO_YOUT` 16-bit 2’s complement value.

Stores the most recent Y axis gyroscope measurement.

`GYRO_ZOUT` 16-bit 2’s complement value.

Stores the most recent Z axis gyroscope measurement.

<a id="section-4-26"></a>

### 4.26 Registers 73 to 96 – External Sensor Data

**EXT_SENS_DATA_00 through EXT_SENS_DATA_23**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|49|73|EXT_SENS_DATA_00[7]|EXT_SENS_DATA_00[6]|EXT_SENS_DATA_00[5]|EXT_SENS_DATA_00[4]|EXT_SENS_DATA_00[3]|EXT_SENS_DATA_00[2]|EXT_SENS_DATA_00[1]|EXT_SENS_DATA_00[0]|
|4A|74|EXT_SENS_DATA_01[7]|EXT_SENS_DATA_01[6]|EXT_SENS_DATA_01[5]|EXT_SENS_DATA_01[4]|EXT_SENS_DATA_01[3]|EXT_SENS_DATA_01[2]|EXT_SENS_DATA_01[1]|EXT_SENS_DATA_01[0]|
|4B|75|EXT_SENS_DATA_02[7]|EXT_SENS_DATA_02[6]|EXT_SENS_DATA_02[5]|EXT_SENS_DATA_02[4]|EXT_SENS_DATA_02[3]|EXT_SENS_DATA_02[2]|EXT_SENS_DATA_02[1]|EXT_SENS_DATA_02[0]|
|4C|76|EXT_SENS_DATA_03[7]|EXT_SENS_DATA_03[6]|EXT_SENS_DATA_03[5]|EXT_SENS_DATA_03[4]|EXT_SENS_DATA_03[3]|EXT_SENS_DATA_03[2]|EXT_SENS_DATA_03[1]|EXT_SENS_DATA_03[0]|
|4D|77|EXT_SENS_DATA_04[7]|EXT_SENS_DATA_04[6]|EXT_SENS_DATA_04[5]|EXT_SENS_DATA_04[4]|EXT_SENS_DATA_04[3]|EXT_SENS_DATA_04[2]|EXT_SENS_DATA_04[1]|EXT_SENS_DATA_04[0]|
|4E|78|EXT_SENS_DATA_05[7]|EXT_SENS_DATA_05[6]|EXT_SENS_DATA_05[5]|EXT_SENS_DATA_05[4]|EXT_SENS_DATA_05[3]|EXT_SENS_DATA_05[2]|EXT_SENS_DATA_05[1]|EXT_SENS_DATA_05[0]|
|4F|79|EXT_SENS_DATA_06[7]|EXT_SENS_DATA_06[6]|EXT_SENS_DATA_06[5]|EXT_SENS_DATA_06[4]|EXT_SENS_DATA_06[3]|EXT_SENS_DATA_06[2]|EXT_SENS_DATA_06[1]|EXT_SENS_DATA_06[0]|
|50|80|EXT_SENS_DATA_07[7]|EXT_SENS_DATA_07[6]|EXT_SENS_DATA_07[5]|EXT_SENS_DATA_07[4]|EXT_SENS_DATA_07[3]|EXT_SENS_DATA_07[2]|EXT_SENS_DATA_07[1]|EXT_SENS_DATA_07[0]|
|51|81|EXT_SENS_DATA_08[7]|EXT_SENS_DATA_08[6]|EXT_SENS_DATA_08[5]|EXT_SENS_DATA_08[4]|EXT_SENS_DATA_08[3]|EXT_SENS_DATA_08[2]|EXT_SENS_DATA_08[1]|EXT_SENS_DATA_08[0]|
|52|82|EXT_SENS_DATA_09[7]|EXT_SENS_DATA_09[6]|EXT_SENS_DATA_09[5]|EXT_SENS_DATA_09[4]|EXT_SENS_DATA_09[3]|EXT_SENS_DATA_09[2]|EXT_SENS_DATA_09[1]|EXT_SENS_DATA_09[0]|
|53|83|EXT_SENS_DATA_10[7]|EXT_SENS_DATA_10[6]|EXT_SENS_DATA_10[5]|EXT_SENS_DATA_10[4]|EXT_SENS_DATA_10[3]|EXT_SENS_DATA_10[2]|EXT_SENS_DATA_10[1]|EXT_SENS_DATA_10[0]|
|54|84|EXT_SENS_DATA_11[7]|EXT_SENS_DATA_11[6]|EXT_SENS_DATA_11[5]|EXT_SENS_DATA_11[4]|EXT_SENS_DATA_11[3]|EXT_SENS_DATA_11[2]|EXT_SENS_DATA_11[1]|EXT_SENS_DATA_11[0]|
|55|85|EXT_SENS_DATA_12[7]|EXT_SENS_DATA_12[6]|EXT_SENS_DATA_12[5]|EXT_SENS_DATA_12[4]|EXT_SENS_DATA_12[3]|EXT_SENS_DATA_12[2]|EXT_SENS_DATA_12[1]|EXT_SENS_DATA_12[0]|
|56|86|EXT_SENS_DATA_13[7]|EXT_SENS_DATA_13[6]|EXT_SENS_DATA_13[5]|EXT_SENS_DATA_13[4]|EXT_SENS_DATA_13[3]|EXT_SENS_DATA_13[2]|EXT_SENS_DATA_13[1]|EXT_SENS_DATA_13[0]|
|57|87|EXT_SENS_DATA_14[7]|EXT_SENS_DATA_14[6]|EXT_SENS_DATA_14[5]|EXT_SENS_DATA_14[4]|EXT_SENS_DATA_14[3]|EXT_SENS_DATA_14[2]|EXT_SENS_DATA_14[1]|EXT_SENS_DATA_14[0]|
|58|88|EXT_SENS_DATA_15[7]|EXT_SENS_DATA_15[6]|EXT_SENS_DATA_15[5]|EXT_SENS_DATA_15[4]|EXT_SENS_DATA_15[3]|EXT_SENS_DATA_15[2]|EXT_SENS_DATA_15[1]|EXT_SENS_DATA_15[0]|
|59|89|EXT_SENS_DATA_16[7]|EXT_SENS_DATA_16[6]|EXT_SENS_DATA_16[5]|EXT_SENS_DATA_16[4]|EXT_SENS_DATA_16[3]|EXT_SENS_DATA_16[2]|EXT_SENS_DATA_16[1]|EXT_SENS_DATA_16[0]|
|5A|90|EXT_SENS_DATA_17[7]|EXT_SENS_DATA_17[6]|EXT_SENS_DATA_17[5]|EXT_SENS_DATA_17[4]|EXT_SENS_DATA_17[3]|EXT_SENS_DATA_17[2]|EXT_SENS_DATA_17[1]|EXT_SENS_DATA_17[0]|
|5B|91|EXT_SENS_DATA_18[7]|EXT_SENS_DATA_18[6]|EXT_SENS_DATA_18[5]|EXT_SENS_DATA_18[4]|EXT_SENS_DATA_18[3]|EXT_SENS_DATA_18[2]|EXT_SENS_DATA_18[1]|EXT_SENS_DATA_18[0]|
|5C|92|EXT_SENS_DATA_19[7]|EXT_SENS_DATA_19[6]|EXT_SENS_DATA_19[5]|EXT_SENS_DATA_19[4]|EXT_SENS_DATA_19[3]|EXT_SENS_DATA_19[2]|EXT_SENS_DATA_19[1]|EXT_SENS_DATA_19[0]|
|5D|93|EXT_SENS_DATA_20[7]|EXT_SENS_DATA_20[6]|EXT_SENS_DATA_20[5]|EXT_SENS_DATA_20[4]|EXT_SENS_DATA_20[3]|EXT_SENS_DATA_20[2]|EXT_SENS_DATA_20[1]|EXT_SENS_DATA_20[0]|
|5E|94|EXT_SENS_DATA_21[7]|EXT_SENS_DATA_21[6]|EXT_SENS_DATA_21[5]|EXT_SENS_DATA_21[4]|EXT_SENS_DATA_21[3]|EXT_SENS_DATA_21[2]|EXT_SENS_DATA_21[1]|EXT_SENS_DATA_21[0]|
|5F|95|EXT_SENS_DATA_22[7]|EXT_SENS_DATA_22[6]|EXT_SENS_DATA_22[5]|EXT_SENS_DATA_22[4]|EXT_SENS_DATA_22[3]|EXT_SENS_DATA_22[2]|EXT_SENS_DATA_22[1]|EXT_SENS_DATA_22[0]|
|60|96|EXT_SENS_DATA_23[7]|EXT_SENS_DATA_23[6]|EXT_SENS_DATA_23[5]|EXT_SENS_DATA_23[4]|EXT_SENS_DATA_23[3]|EXT_SENS_DATA_23[2]|EXT_SENS_DATA_23[1]|EXT_SENS_DATA_23[0]|

#### Description

These registers store data read from external sensors by the Slave 0, 1, 2, and 3 on the auxiliary I²C interface. Data read by Slave 4 is stored in I2C_SLV4_DI (Register 53).

External sensor data is written to these registers at the Sample Rate as defined in Register 25. This access rate can be reduced by using the Slave Delay Enable registers (Register 103).

External sensor data registers, along with the gyroscope measurement registers, accelerometer measurement registers, and temperature measurement registers, are composed of two sets of registers: an internal register set and a user-facing read register set.

The data within the external sensors’ internal register set is always updated at the Sample Rate (or the reduced access rate) whenever the serial interface is idle. This guarantees that a burst read of sensor registers will read measurements from the same sampling instant. Note that if burst reads are not used, the user is responsible for ensuring a set of single byte reads correspond to a single sampling instant by checking the Data Ready interrupt.

Data is placed in these external sensor data registers according to I2C_SLV0_CTRL, I2C_SLV1_CTRL, I2C_SLV2_CTRL, and I2C_SLV3_CTRL (Registers 39, 42, 45, and 48). When more than zero bytes are read ( _I2C_SLVx_LEN_     - 0) from an enabled slave ( _I2C_SLVx_EN_ = 1), the slave is read at the Sample Rate (as defined in Register 25) or delayed rate (if specified in Register 52 and 103). During each Sample cycle, slave reads are performed in order of Slave number. If all slaves are enabled with more than zero bytes to be read, the order will be Slave 0, followed by Slave 1, Slave 2, and Slave 3.

Each enabled slave will have EXT_SENS_DATA registers associated with it by number of bytes read ( _I2C_SLVx_LEN)_ in order of slave number, starting from EXT_SENS_DATA_00. Note that this means enabling or disabling a slave may change the higher numbered slaves’ associated registers. Furthermore, if fewer total bytes are being read from the external sensors as a result of such a change, then the data remaining in the registers which no longer have an associated slave device (i.e. high numbered registers) will remain in these previously allocated registers unless reset.

If the sum of the read lengths of all SLVx transactions exceed the number of available EXT_SENS_DATA registers, the excess bytes will be dropped. There are 24 EXT_SENS_DATA registers and hence the total read lengths between all the slaves cannot be greater than 24 or some bytes will be lost.

<u>Note: Slave 4’s behavior is distinct from that of Slaves 0-3. For further information regarding the</u> characteristics of Slave 4, please refer to Registers 49 to 53.

**<u>Example:</u>**

Suppose that Slave 0 is enabled with 4 bytes to be read ( `I2C_SLV0_EN` = 1 and `I2C_SLV0_LEN` = 4) while Slave 1 is enabled with 2 bytes to be read, ( _I2C_SLV1_EN=_ 1 and `I2C_SLV1_LEN` = 2). In such a situation, EXT_SENS_DATA _00 through _03 will be associated with Slave 0, while EXT_SENS_DATA _04 and 05 will be associated with Slave 1.

If Slave 2 is enabled as well, registers starting from EXT_SENS_DATA_06 will be allocated to Slave 2.

If Slave 2 is disabled while Slave 3 is enabled in this same situation, then registers starting from EXT_SENS_DATA_06 will be allocated to Slave 3 instead.

**<u>Register Allocation for Dynamic Disable vs. Normal Disable</u>**

If a slave is disabled at any time, the space initially allocated to the slave in the EXT_SENS_DATA register, will remain associated with that slave. This is to avoid dynamic adjustment of the register allocation.

The allocation of the EXT_SENS_DATA registers is recomputed only when (1) all slaves are disabled, or (2) the `I2C_MST_RST` bit is set (Register 106) .

This above is also true if one of the slaves gets NACKed and stops functioning.

<a id="section-4-27"></a>

### 4.27 Register 97 – Motion Detection Status

**MOT_DETECT_STATUS**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|61|97|MOT_XNEG|MOT_XPOS|MOT_YNEG|MOT_YPOS|MOT_ZNEG|MOT_ZPOS|-|MOT_ZRMOT|

#### Description

This register reports the status of Motion detection and Zero Motion detection.

The Motion detection bits, `MOT_XNEG`, `MOT_XPOS`, `MOT_YNEG`, `MOT_YPOS`, `MOT_ZNEG`, and `MOT_ZPOS`, report the axis and polarity of motion which generated a Motion Detection interrupt.

The `MOT_ZRMOT` bit is set to 1 when Zero Motion has been detected.

Reading this register clears the Motion detection bits. However, the `MOT_ZRMOT` bit does not clear until Zero Motion is no longer detected.

For more information regarding Motion detection and Zero Motion detection, please refer to Registers 31 to 34 in this document, as well as Sections 8.3 and 8.4 of the MPU-6000/MPU-6050 Product Specification document.

Bit 1 is reserved.

#### Parameters

`MOT_XNEG`

This bit automatically sets to 1 when motion in the negative X axis has generated a Motion detection interrupt.

`MOT_XPOS` This bit automatically sets to 1 when motion in the positive X axis has generated a Motion detection interrupt.

`MOT_YNEG` This bit automatically sets to 1 when motion in the negative Y axis has generated a Motion detection interrupt.

`MOT_YPOS` This bit automatically sets to 1 when motion in the positive Y axis has generated a Motion detection interrupt.

`MOT_ZNEG` This bit automatically sets to 1 when motion in the negative Z axis has generated a Motion detection interrupt.

`MOT_ZPOS` This bit automatically sets to 1 when motion in the positive Z axis has generated a Motion detection interrupt.

`MOT_ZRMOT` This bit automatically sets to 1 when Zero Motion detection interrupt is generated.

<a id="section-4-28"></a>

### 4.28 Register 99 – I²C Slave 0 Data Out

**I2C_SLV0_DO**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|63|99|I2C_SLV0_DO[7]|I2C_SLV0_DO[6]|I2C_SLV0_DO[5]|I2C_SLV0_DO[4]|I2C_SLV0_DO[3]|I2C_SLV0_DO[2]|I2C_SLV0_DO[1]|I2C_SLV0_DO[0]|

#### Description

This register holds the output data written into Slave 0 when Slave 0 is set to write mode.

For further information regarding Slave 0 control, please refer to Registers 37 to 39.

#### Parameters

`I2C_SLV0_DO` 8 bit unsigned value that is written into Slave 0 when Slave 0 is set to write mode.

<a id="section-4-29"></a>

### 4.29 Register 100 – I²C Slave 1 Data Out

**I2C_SLV1_DO**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|64|100|I2C_SLV1_DO[7]|I2C_SLV1_DO[6]|I2C_SLV1_DO[5]|I2C_SLV1_DO[4]|I2C_SLV1_DO[3]|I2C_SLV1_DO[2]|I2C_SLV1_DO[1]|I2C_SLV1_DO[0]|

#### Description

This register holds the output data written into Slave 1 when Slave 1 is set to write mode.

For further information regarding Slave 1 control, please refer to Registers 40 to 42.

#### Parameters

`I2C_SLV1_DO` 8 bit unsigned value that is written into Slave 1 when Slave 1 is set to write mode.

<a id="section-4-30"></a>

### 4.30 Register 101 – I²C Slave 2 Data Out

**I2C_SLV2_DO**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|65|101|I2C_SLV2_DO[7]|I2C_SLV2_DO[6]|I2C_SLV2_DO[5]|I2C_SLV2_DO[4]|I2C_SLV2_DO[3]|I2C_SLV2_DO[2]|I2C_SLV2_DO[1]|I2C_SLV2_DO[0]|

#### Description

This register holds the output data written into Slave 2 when Slave 2 is set to write mode.

For further information regarding Slave 2 control, please refer to Registers 43 to 45.

#### Parameters

`I2C_SLV2_DO` 8 bit unsigned value that is written into Slave 2 when Slave 2 is set to write mode.

<a id="section-4-31"></a>

### 4.31 Register 102 – I²C Slave 3 Data Out

**I2C_SLV3_DO**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|66|102|I2C_SLV3_DO[7]|I2C_SLV3_DO[6]|I2C_SLV3_DO[5]|I2C_SLV3_DO[4]|I2C_SLV3_DO[3]|I2C_SLV3_DO[2]|I2C_SLV3_DO[1]|I2C_SLV3_DO[0]|

#### Description

This register holds the output data written into Slave 3 when Slave 3 is set to write mode.

For further information regarding Slave 3 control, please refer to Registers 46 to 48.

#### Parameters

`I2C_SLV3_DO` 8 bit unsigned value that is written into Slave 3 when Slave 3 is set to write mode.

<a id="section-4-32"></a>

### 4.32 Register 103 – I²C Master Delay Control

**I2C_MST_DELAY_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|67|103|DELAY_ES_SHADOW|-|-|I2C_SLV4_DLY_EN|I2C_SLV3_DLY_EN|I2C_SLV2_DLY_EN|I2C_SLV1_DLY_EN|I2C_SLV0_DLY_EN|

#### Description

This register is used to specify the timing of external sensor data shadowing. The register is also used to decrease the access rate of slave devices relative to the Sample Rate.

When `DELAY_ES_SHADOW` is set to 1, shadowing of external sensor data is delayed until all data has been received.

When _I2C_SLV4_DLY_EN, I2C_SLV3_DLY_EN, I2C_SLV2_DLY_EN, I2C_SLV1_DLY_EN,_ and `I2C_SLV0_DLY_EN` are enabled, the rate of access for the corresponding slave devices is reduced.

When a slave’s access rate is decreased relative to the Sample Rate, the slave is accessed every

1 / (1 + `I2C_MST_DLY` ) samples.

This base Sample Rate in turn is determined by `SMPLRT_DIV` (register 25) and `DLPF_CFG` (register 26).

For further information regarding `I2C_MST_DLY`, please refer to register 52.

For further information regarding the Sample Rate, please refer to register 25.

Bits 6 and 5 are reserved.

#### Parameters

`DELAY_ES_SHADOW` When set, delays shadowing of external sensor data until all data has been received.

`I2C_SLV4_DLY_EN` When enabled, slave 4 will only be accessed at a decreased rate.

`I2C_SLV3_DLY_EN` When enabled, slave 3 will only be accessed at a decreased rate.

`I2C_SLV2_DLY_EN` When enabled, slave 2 will only be accessed at a decreased rate.

`I2C_SLV1_DLY_EN` When enabled, slave 1 will only be accessed at a decreased rate.

`I2C_SLV0_DLY_EN` When enabled, slave 0 will only be accessed at a decreased rate.

<a id="section-4-33"></a>

### 4.33 Register 104 – Signal Path Reset

**SIGNAL_PATH_RESET**

**Type: Write Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|68|104|-|-|-|-|-|GYRO_RESET|ACCEL_RESET|TEMP_RESET|

#### Description

This register is used to reset the analog and digital signal paths of the gyroscope, accelerometer, and temperature sensors.

The reset will revert the signal path analog to digital converters and filters to their power up configurations.

<u>Note: This register does not clear the sensor registers.</u>

Bits 7 to 3 are reserved.

#### Parameters

`GYRO_RESET` When set to 1, this bit resets the gyroscope analog and digital signal paths.

`ACCEL_RESET` When set to 1, this bit resets the accelerometer analog and digital signal paths.

`TEMP_RESET` When set to 1, this bit resets the temperature sensor analog and digital signal paths.

<a id="section-4-34"></a>

### 4.34 Register 105 – Motion Detection Control

**MOT_DETECT_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|69|105|-|-|ACCEL_ON_DELAY[1]|ACCEL_ON_DELAY[0]|FF_COUNT[1]|FF_COUNT[0]|MOT_COUNT[1]|MOT_COUNT[0]|

#### Description

This register is used to add delay to the accelerometer power on time. It is also used to configure the Free Fall and Motion detection decrement rate.

The accelerometer data path provides samples to the sensor registers, Motion detection, Zero Motion detection, and Free Fall detection modules. The signal path contains filters which must be flushed on wake-up with new samples before the detection modules begin operations. The default wake-up delay, of 4ms can be lengthened by up to 3ms. This additional delay is specified in `ACCEL_ON_DELAY` in units of 1 LSB = 1 ms. The user may select any value above zero unless instructed otherwise by InvenSense. Please refer to Section 8 of the MPU-6000/MPU-6050 Product Specification document for further information regarding the detection modules.

Detection is registered by the Free Fall detection module or the Motion detection module after accelerometer measurements meet their respective threshold conditions over a specified number of samples. When the threshold conditions are met, the corresponding detection counter increments by 1. The user may control the rate at which the detection counter decrements when the threshold condition is not met by configuring `FF_COUNT` and `MOT_COUNT` . The decrement rate can be set according to the following table:

|FF_COUNT or<br>MOT_COUNT|Counter<br>Decrement|
|---|---|
|0|Reset|
|1|1|
|2|2|
|3|4|

When `FF_COUNT` or `MOT_COUNT` are configured to 0 (reset), any non-qualifying sample will reset the corresponding counter to 0. For further information on Free Fall detection and Motion detection, please refer to Registers 29 to 32.

Bits 7 and 6 are reserved.

#### Parameters

`ACCEL_ON_DELAY` 2-bit unsigned value. Specifies the additional power-on delay applied to accelerometer data path modules.

Unit of 1 LSB = 1 ms.

`FF_COUNT` 2-bit unsigned value. Configures the Free Fall detection counter decrement rate.

`MOT_COUNT` 2-bit unsigned value. Configures the Motion detection counter decrement rate.

<a id="section-4-35"></a>

### 4.35 Register 106 – User Control

**USER_CTRL**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|6A|106|-|FIFO_EN|I2C_MST_EN|I2C_IF_DIS|-|FIFO_RESET|I2C_MST_RESET|SIG_COND_RESET|

#### Description

This register allows the user to enable and disable the FIFO buffer, I²C Master Mode **,** and primary I²C interface. The FIFO buffer, I²C Master, sensor signal paths and sensor registers can also be reset using this register.

When `I2C_MST_EN` is set to 1, I²C Master Mode is enabled. In this mode, the MPU-60X0 acts as the I²C Master to the external sensor slave devices on the auxiliary I²C bus. When this bit is cleared to 0, the auxiliary I2C bus lines (AUX_DA and AUX_CL) are logically driven by the primary I2C bus

(SDA and SCL). This is a precondition to enabling Bypass Mode. For further information regarding Bypass Mode, please refer to Register 55.

<u>MPU-6000: The primary SPI interface will be enabled in place of the disabled primary I</u> <sup>2</sup> C interface when `I2C_IF_DIS` is set to 1.

<u>MPU-6050: Always write 0 to</u> `I2C_IF_DIS` .

When the reset bits ( _FIFO_RESET, I2C_MST_RESET, and SIG_COND_RESET_ ) are set to 1, these reset bits will trigger a reset and then clear to 0.

Bits 7 and 3 are reserved.

#### Parameters

`FIFO_EN` When set to 1, this bit enables FIFO operations.

When this bit is cleared to 0, the FIFO buffer is disabled. The FIFO buffer cannot be written to or read from while disabled.

The FIFO buffer’s state does not change unless the MPU-60X0 is power cycled.

`I2C_MST_EN` When set to 1, this bit enables I²C Master Mode.

When this bit is cleared to 0, the auxiliary I²C bus lines (AUX_DA and AUX_CL) are logically driven by the primary I²C bus (SDA and SCL).

`I2C_IF_DIS` <u>MPU-6000:</u> When set to 1, this bit disables the primary I²C interface and enables the SPI interface instead.

<u>MPU-6050: Always write this bit as zero.</u>

`FIFO_RESET` This bit resets the FIFO buffer _w_ hen set to 1 while `FIFO_EN` equals 0. This bit automatically clears to 0 after the reset has been triggered.

`I2C_MST_RESET` This bit resets the I²C Master when set to 1 while `I2C_MST_EN` equals 0. This bit automatically clears to 0 after the reset has been triggered.

`SIG_COND_RESET` When set to 1, this bit resets the signal paths for all sensors (gyroscopes, accelerometers, and temperature sensor). This operation will also clear the sensor registers. This bit automatically clears to 0 after the reset has been triggered.

When resetting only the signal path (and not the sensor registers), please use Register 104, SIGNAL_PATH_RESET.

<a id="section-4-36"></a>

### 4.36 Register 107 – Power Management 1

**PWR_MGMT_1**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|6B|107|DEVICE_RESET|SLEEP|CYCLE|-|TEMP_DIS|CLKSEL[2]|CLKSEL[1]|CLKSEL[0]|

#### Description

This register allows the user to configure the power mode and clock source. It also provides a bit for resetting the entire device, and a bit for disabling the temperature sensor.

By setting `SLEEP` to 1, the MPU-60X0 can be put into low power sleep mode. When `CYCLE` is set to 1 while `SLEEP` is disabled, the MPU-60X0 will be put into Cycle Mode. In Cycle Mode, the device cycles between sleep mode and waking up to take a single sample of data from active sensors at a rate determined by `LP_WAKE_CTRL` (register 108). To configure the wake frequency, use `LP_WAKE_CTRL` within the Power Management 2 register (Register 108).

An internal 8MHz oscillator, gyroscope based clock, or external sources can be selected as the MPU-60X0 clock source. When the internal 8 MHz oscillator or an external source is chosen as the clock source, the MPU-60X0 can operate in low power modes with the gyroscopes disabled.

Upon power up, the MPU-60X0 clock source defaults to the internal oscillator. However, it is highly recommended that the device be configured to use one of the gyroscopes (or an external clock source) as the clock reference for improved stability. The clock source can be selected according to the following table.

|CLKSEL|Clock Source|
|---|---|
|0|Internal 8MHz oscillator|
|1|PLL with X axis gyroscope reference|
|2|PLL with Y axis gyroscope reference|
|3|PLL with Z axis gyroscope reference|
|4|PLL with external 32.768kHz reference|
|5|PLL with external 19.2MHz reference|
|6|Reserved|
|7|Stops the clock and keeps the timing generator<br>in reset|

For further information regarding the MPU-60X0 clock source, please refer to the MPU-6000/MPU6050 Product Specification document.

Bit 4 is reserved.

#### Parameters

`DEVICE_RESET` When set to 1, this bit resets all internal registers to their default values.

The bit automatically clears to 0 once the reset is done.

The default values for each register can be found in Section 3.

`SLEEP` When set to 1, this bit puts the MPU-60X0 into sleep mode.

`CYCLE` When this bit is set to 1 and `SLEEP` is disabled, the MPU-60X0 will cycle between sleep mode and waking up to take a single sample of data from active sensors at a rate determined by `LP_WAKE_CTRL` (register 108).

`TEMP_DIS` When set to 1, this bit disables the temperature sensor.

`CLKSEL` 3-bit unsigned value. Specifies the clock source of the device.

<a id="section-4-37"></a>

### 4.37 Register 108 – Power Management 2

**PWR_MGMT_2**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|6C|108|LP_WAKE_CTRL[1]|LP_WAKE_CTRL[0]|STBY_XA|STBY_YA|STBY_ZA|STBY_XG|STBY_YG|STBY_ZG|

#### Description

This register allows the user to configure the frequency of wake-ups in Accelerometer Only Low Power Mode. This register also allows the user to put individual axes of the accelerometer and gyroscope into standby mode.

The MPU-9150 can be put into Accerlerometer Only Low Power Mode by setting `PWRSEL` to 1 in the Power Management 1 register (Register 107). In this mode, the device will power off all devices except for the primary I²C interface, waking only the accelerometer at fixed intervals to take a single measurement. The frequency of wake-ups can be configured with `LP_WAKE_CTRL` as shown below.

|LP_WAKE_CTRL|Wake-up Frequency|
|---|---|
|0|1.25 Hz|
|1|2.5 Hz|
|2|5 Hz|
|3|10 Hz|

For further information regarding the MPU-9150’s power modes, please refer to Register 107.

The user can put individual accelerometer and gyroscopes axes into standby mode by using this register. If the device is using a gyroscope axis as the clock source and this axis is put into standby mode, the clock source will automatically be changed to the internal 8MHz oscillator.

#### Parameters

`LP_WAKE_CTRL` 2-bit unsigned value.

Specifies the frequency of wake-ups during Accelerometer Only Low Power Mode.

`STBY_XA` When set to 1, this bit puts the X axis accelerometer into standby mode.

`STBY_YA` When set to 1, this bit puts the Y axis accelerometer into standby mode.

`STBY_ZA` When set to 1, this bit puts the Z axis accelerometer into standby mode.

`STBY_XG` When set to 1, this bit puts the X axis gyroscope into standby mode.

`STBY_YG` When set to 1, this bit puts the Y axis gyroscope into standby mode.

`STBY_ZG` When set to 1, this bit puts the Z axis gyroscope into standby mode.

<a id="section-4-38"></a>

### 4.38 Register 114 and 115 – FIFO Count Registers

**FIFO_COUNT_H and FIFO_COUNT_L**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|72|114|FIFO_COUNT[15]|FIFO_COUNT[14]|FIFO_COUNT[13]|FIFO_COUNT[12]|FIFO_COUNT[11]|FIFO_COUNT[10]|FIFO_COUNT[9]|FIFO_COUNT[8]|
|73|115|FIFO_COUNT[7]|FIFO_COUNT[6]|FIFO_COUNT[5]|FIFO_COUNT[4]|FIFO_COUNT[3]|FIFO_COUNT[2]|FIFO_COUNT[1]|FIFO_COUNT[0]|

#### Description

These registers keep track of the number of samples currently in the FIFO buffer.

These registers shadow the FIFO Count value. Both registers are loaded with the current sample count when FIFO_COUNT_H (Register 72) is read.

Note: Reading only FIFO_COUNT_L will not update the registers to the current sample count. FIFO_COUNT_H must be accessed first to update the contents of both these registers.

`FIFO_COUNT` should always be read in high-low order in order to guarantee that the most current FIFO Count value is read.

#### Parameters

`FIFO_COUNT` 16-bit unsigned value. Indicates the number of bytes stored in the FIFO buffer. This number is in turn the number of bytes that can be read from the FIFO buffer and it is directly proportional to the number of samples available given the set of sensor data bound to be stored in the FIFO (register 35 and 36).

<a id="section-4-39"></a>

### 4.39 Register 116 – FIFO Read Write

**FIFO_R_W**

**Type: Read/Write**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|74|116|FIFO_DATA[7]|FIFO_DATA[6]|FIFO_DATA[5]|FIFO_DATA[4]|FIFO_DATA[3]|FIFO_DATA[2]|FIFO_DATA[1]|FIFO_DATA[0]|

#### Description

This register is used to read and write data from the FIFO buffer.

Data is written to the FIFO in order of register number (from lowest to highest). If all the FIFO enable flags (see below) are enabled and all External Sensor Data registers (Registers 73 to 96) are associated with a Slave device, the contents of registers 59 through 96 will be written in order at the Sample Rate.

The contents of the sensor data registers (Registers 59 to 96) are written into the FIFO buffer when their corresponding FIFO enable flags are set to 1 in FIFO_EN (Register 35). An additional flag for the sensor data registers associated with I²C Slave 3 can be found in I2C_MST_CTRL (Register 36).

If the FIFO buffer has overflowed, the status bit `FIFO_OFLOW_INT` is automatically set to 1. This bit is located in INT_STATUS (Register 58). When the FIFO buffer has overflowed, the oldest data will be lost and new data will be written to the FIFO.

If the FIFO buffer is empty, reading this register will return the last byte that was previously read from the FIFO until new data is available. The user should check `FIFO_COUNT` to ensure that the FIFO buffer is not read when empty.

#### Parameters

`FIFO_DATA` 8-bit data transferred to and from the FIFO buffer.

<a id="section-4-40"></a>

### 4.40 Register 117 – Who Am I

**WHO_AM_I**

**Type: Read Only**

|Register<br>(Hex)|Register<br>(Decimal)|Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0|
|---|---|---|---|---|---|---|---|---|---|
|75|117|-|WHO_AM_I[6]|WHO_AM_I[5]|WHO_AM_I[4]|WHO_AM_I[3]|WHO_AM_I[2]|WHO_AM_I[1]|-|

#### Description

This register is used to verify the identity of the device. The contents of `WHO_AM_I` are the upper 6 bits of the MPU-60X0’s 7-bit I²C address. The least significant bit of the MPU-60X0’s I²C address is determined by the value of the AD0 pin. The value of the AD0 pin is not reflected in this register.

The default value of the register is 0x68.

Bits 0 and 7 are reserved. (Hard coded to 0)

#### Parameters

`WHO_AM_I` Contains the 6-bit I²C address of the MPU-60X0.

The Power-On-Reset value of Bit6:Bit1 is 110 100.

## Legal notice

This information furnished by InvenSense is believed to be accurate and reliable. However, no responsibility is assumed by InvenSense for its use, or for any infringements of patents or other rights of third parties that may result from its use. Specifications are subject to change without notice. InvenSense reserves the right to make changes to this product, including its circuits and software, in order to improve its design and/or performance, without prior notice. InvenSense makes no warranties, neither expressed nor implied, regarding the information and specifications contained in this document. InvenSense assumes no responsibility for any claims or damages arising from information contained in this document, or from the use of products and services detailed therein. This includes, but is not limited to, claims or damages based on the infringement of patents, copyrights, mask work and/or other intellectual property rights.

Certain intellectual property owned by InvenSense and described in this document is patent protected. No license is granted by implication or otherwise under any patent or patent rights of InvenSense. This publication supersedes and replaces all information previously supplied. Trademarks that are registered trademarks are the property of their respective companies. InvenSense sensors should not be used or sold in the development, storage, production or utilization of any conventional or mass-destructive weapons or for any other weapons or life threatening applications, as well as in any other life critical applications such as medical equipment, transportation, aerospace and nuclear instruments, undersea equipment, power plant equipment, disaster prevention and crime prevention equipment.

InvenSense® is a registered trademark of InvenSense, Inc. MPU <sup>TM</sup>, MPU-6000 <sup>TM</sup>, MPU-6050 <sup>TM</sup>, MPU-60X0 <sup>TM</sup>, Digital Motion Processor <sup>™</sup>, DMP <sup>™</sup>, Motion Processing Unit™, MotionFusion™, and MotionApps™ are trademarks of InvenSense, Inc.

©2011 InvenSense, Inc. All rights reserved.

返回：[文档索引](../../README.md)。
