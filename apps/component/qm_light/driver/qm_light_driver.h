/**
 * @file light_effect_api.h
 * @author Seven (gan0ling@126.com)
 * @brief 灯效控制API，包括初始化、设置亮度、设置灯效、停止灯效等。
 * @version 0.1
 * @date 2024-03-14
 * 
 * @copyright Copyright (c) 2024
 * 
**/
#ifndef _QM_LIGHT_DRIVER_H_
#define _QM_LIGHT_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define QM_LIGHT_TIME_INFINITE    0xFFFFFFFF

typedef void* qm_light_driver_handle_t;

typedef enum {
    QM_LIGHT_DRIVER_LEVEL_LOW = 0,            /**< pass this param to qm_light_driver_init if the light is on when control-gpio level is low */
    QM_LIGHT_DRIVER_LEVEL_HIGH = 1,           /**< pass this param to qm_light_driver_init if the light is on when control-gpio level is high */
} qm_light_driver_level_t;

typedef enum
{
    QM_LIGHT_DRIVER_EVNET_COMPLETE_LIGHTNESS_LINEAR,
    QM_LIGHT_DRIVER_EVNET_COMPLETE_BLINK,
    QM_LIGHT_DRIVER_EVNET_COMPLETE_BREATH,
    QM_LIGHT_DRIVER_EVNET_COMPLETE_STOP,
    QM_LIGHT_DRIVER_EVNET_UNKNOWN,
} qm_light_driver_event_type_t;

typedef void (*qm_light_driver_event_cb)(qm_light_driver_handle_t handle, qm_light_driver_event_type_t event_type, void* arg);

typedef struct {
    int port;               //外设端口
    int pin;                //外设引脚
    uint32_t freq;                   //频率
    uint16_t lightness;         //默认亮度值，有效范围[0,65535]
    qm_light_driver_level_t level;     //行为电平
    qm_light_driver_event_cb cb;
    void *arg;
} qm_light_driver_param_t;

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
qm_light_driver_handle_t qm_light_driver_init(qm_light_driver_param_t *param);

/**
 * @brief deinitialize light controller
 */
qm_err_t qm_light_driver_deinit(qm_light_driver_handle_t handle);

/**
 * @brief set lightness gradual change
 * @param[in] light: light channel
 * @param[in] lightness: target lightness
 * @param[in] time: gradual change time
 * @param[in] cb: light change done callback function
 */
qm_err_t qm_light_driver_set_lightness_linear(qm_light_driver_handle_t handle, uint16_t lightness, uint32_t time);

/**
 * @brief blink light
 * @param[in] light: light channel
 * @param[in] lightness_begin: blink start lightness
 * @param[in] lightness_end: blink end lightness
 * @param[in] interval: blink interval, the unit is ms
 * @param[in] begin_duty: start lightness duty in total interval, value range is 0-100
 *            for example, intreval is 1000ms, duty is 60, then start lightness will lighten
 *            600ms, end lightness will lighten 400ms
 * @param[in] cb: light change done callback function
 * @param[in] times: blink times
 */
qm_err_t qm_light_driver_blink(qm_light_driver_handle_t handle, uint16_t lightness_begin, uint16_t lightness_end,
                 uint32_t interval, uint8_t begin_duty, uint32_t times);

/**
 * @brief breath light
 * @param[in] light: light channel
 * @param[in] lightness_begin: breath start lightness
 * @param[in] lightness_end: breath end lightness
 * @param[in] interval: breath interval, the unit is ms
 * @param[in] forward_duty: start lightness duty in total interval, value range is 0-100
 *            for example, intreval is 1000ms, duty is 60, then start lightness will lighten
 *            600ms, end lightness will lighten 400ms
 * @param[in] times: breath times
 * @param[in] half_breath_end: whether the last breath is half or not, if TURE, lightness will stay
 *            on lightness_end, otherwise lightness till stay on lightness_begin
 * @param[in] cb: light change done callback function
 */
qm_err_t qm_light_driver_breath(qm_light_driver_handle_t handle, uint16_t lightness_begin, uint16_t lightness_end,
                    uint32_t interval, uint8_t forward_duty, uint32_t times, bool_t half_breath_end);

/**
 * @brief stop light action
 * @param[in] light: light channel
 */
qm_err_t qm_light_driver_stop(qm_light_driver_handle_t handle);

/**
 * @brief check whether light is busy or not
 * @retval TRUE: busy
 * @retval FALSE: idle
 */
bool_t qm_light_driver_is_busy(qm_light_driver_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
