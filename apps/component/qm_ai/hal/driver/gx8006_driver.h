
#ifndef __QM_AI_DRIVER_GX8006_H__
#define __QM_AI_DRIVER_GX8006_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"
#include "qm_ai_driver.h"

extern qm_ai_driver_param_t QM_AI_DRIVER_GX8006_DEFAULT_HANDLE;

/**
 * @brief  打开会话
 * 
 * @details 
 * 
 */
int gx8006_driver_open(void);


/**
 * @brief 写入数据流
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_write(void *data, int len, void *arg);

/**
 * @brief 获取数据流
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_read(void *data, int len, uint32_t timeout, void *arg);

/**
 * @brief 设置消音状态
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_set_mute(bool_t mute);

/**
 * @brief 获取消音状态
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_get_mute(bool_t *mute);

/**
 * @brief 设置音量
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_set_volume(uint32_t volume);

/**
 * @brief 获取音量
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_get_volume(uint32_t *volume);

/**
 * @brief 使能PA
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_enable_pa(bool_t enable);

/**
 * @brief 关闭会话
 * 
 * @param[in] handle 会话句柄
 * 
 * @details 
 * 
 */
int32_t gx8006_driver_close(void);


#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_COMMON_H__ */
