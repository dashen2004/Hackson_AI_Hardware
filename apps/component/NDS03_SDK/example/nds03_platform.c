/**
 * @file nds03_platform.c
 * @author tongsheng.tang
 * @brief This file list all the functions for user to implement.
 * @version 2.x.x
 * @date 2025-06
 * 
 * @copyright Copyright (c) 2025, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 * @license BSD 3-Clause License
 *          This file is part of the NDS05 SDK and is licensed under the BSD 3-Clause License.
 *          You may obtain a copy of the license in the project root directory, in the file named LICENSE.
 * 
 */
#include "nds03_def.h"
#include "nds03_platform.h"

#include "bsp_i2c.h"

static i2c_master_dev_handle_t nds_dev_handle;
static i2c_device_config_t nds_dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x5C,
    .scl_speed_hz = 400000,
};

/**
 * @brief NDS03使用平台初始化
 * 
 * @param   pDev        平台设备指针
 * @param   arg         参数指针
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_platform_init(NDS03_Platform_t *pdev, void *arg)
{
    bsp_i2c_init();
    bsp_i2c_add_device(&nds_dev_config, &nds_dev_handle);

    return 0;
}

/**
 * @brief NDS03使用平台释放
 * 
 * @param   pDev        平台设备指针
 * @param   arg         参数指针
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_platform_uninit(NDS03_Platform_t *pdev, void *arg)
{
    return 0;
}

/**
 * @brief i2c连续读len个字节
 * 
 * @param   pDev        平台设备指针
 * @param   reg_addr    读寄存器地址
 * @param   read_data   读数据
 * @param   len         读数据长度
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_i2c_read_nbytes(NDS03_Platform_t *pDev,  uint8_t reg_addr, uint8_t *read_data, uint16_t len)
{
    uint8_t i2c_addr = pDev->i2c_dev_addr;

    // 用户实现I2C读操作代码，注意restart之前没有stop位
    uint8_t cmd = reg_addr;
    qm_err_t ret = bsp_i2c_master_read(nds_dev_handle, &cmd, 1, read_data, len, 1000);

    return 0;
}

/**
 * @brief i2c连续写len个字节
 * 
 * @param   pDev        平台设备指针
 * @param   reg_addr    写寄存器地址
 * @param   write_data  写数据
 * @param   len         写数据长度
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_i2c_write_nbytes(NDS03_Platform_t *pDev,  uint8_t reg_addr, uint8_t *write_data, uint16_t len)
{
    uint8_t i2c_addr = pDev->i2c_dev_addr;

    // 用户实现I2C写操作代码
    uint8_t buf[len + 1];
    buf[0] = reg_addr;
    memcpy(&buf[1], write_data, len);

    qm_err_t ret = bsp_i2c_master_write(nds_dev_handle, buf, len + 1, 1000);

    return 0;
}

/**
 * @brief 获取i2c时钟频率
 * 
 * @param pDev 平台设备指针
 * @param clock_frequency 返回的时钟频率
 * @return 
 */
int8_t nds03_i2c_get_clock_frequency(NDS03_Platform_t *pDev, uint32_t *clock_frequency)
{
    //用户实现获取i2c时钟频率代码
    *clock_frequency = nds_dev_config.scl_speed_hz;

    return 0;
}

/**
 * @brief 设置i2c时钟频率
 * 
 * @param pDev 平台设备指针
 * @param clock_frequency 时钟频率
 * @return 
 */
int8_t nds03_i2c_set_clock_frequency(NDS03_Platform_t *pDev, uint32_t clock_frequency)
{
    //用户实现设置i2c时钟频率代码

    return 0;
}


/**
 * @brief 延时wait_ms毫秒
 *
 * @param   pDev        平台设备指针
 * @param   wait_ms     输入需要延时时长  
 * @return  int8_t  
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_delay_1ms(NDS03_Platform_t *pDev, uint32_t wait_ms)
{
    // 用户实现延迟1ms代码
    qm_msleep(wait_ms);

    return 0;
}

/**
 * @brief 延时10*wait_10us微秒
 *
 * @param   pDev        平台设备指针
 * @param   wait_10us   输入需要延时时长
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_delay_10us(NDS03_Platform_t *pDev, uint32_t wait_10us)
{
    // 用户实现延迟10us代码
    qm_usleep(wait_10us * 10);

    return 0;
}

/**
 * @brief 获取系统时钟时间，单位ms
 *
 * @param   pDev        平台设备指针
 * @param   time_ms     时间（ms）
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_get_system_clk_ms(NDS03_Platform_t *pDev, int32_t *time_ms)
{
    *time_ms = qm_now_ms();

    return 0;
}

/**
 * @brief 设置nds03 xshut引脚电平
 *
 * @param   pDev        平台设备指针
 * @param   level       引脚电平，0为低电平，1为高电平
 * @return  int8_t  
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_set_xshut_pin_level(NDS03_Platform_t *pDev, int8_t level)
{

    return 0;
}


