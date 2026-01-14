#ifndef __QM_AI_DRIVER_H__
#define __QM_AI_DRIVER_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

typedef enum
{
    QM_AI_DRIVER_CTRL_NONE = 0,

    /**
     * @brief 设置消音状态
     *
     * @details
     *
     * 数据类型: (qm_ai_driver_read_handler_t)
     */
    QM_AI_DRIVER_CTRL_READ_CALLBACK,

    /**
     * @brief 设置消音状态
     *
     * @details
     *
     * 数据类型: (qm_ai_driver_event_handler_t)
     */
    QM_AI_DRIVER_CTRL_EVENT_CALLBACK,

    /**
     * @brief 设置消音状态
     *
     * @details
     *
     * 数据类型: (bool_t *)
     */
    QM_AI_DRIVER_CTRL_SET_MUTE,
    /**
     * @brief 获取消音状态
     *
     * @details
     *
     * 数据类型: (bool_t *)
     */
    QM_AI_DRIVER_CTRL_GET_MUTE,
    /**
     * @brief 设置音量
     *
     * @details
     *
     * 数据类型: (uint32_t *)
     */
    QM_AI_DRIVER_CTRL_SET_VOLUME,
    /**
     * @brief 获取音量
     *
     * @details
     *
     * 数据类型: (uint32_t *)
     */
    QM_AI_DRIVER_CTRL_GET_VOLUME,
    /**
     * @brief 使能 PA状态
     *
     * @details
     *
     * 数据类型: (bool_t *)
     */
    QM_AI_DRIVER_CTRL_ENABLE_PA,
}qm_ai_driver_ctrl_t;

typedef struct {
    int (*open)(void);
    int (*close)(void);
    int (*write)(void *data, int len);
    int (*read)(void *data, int len, uint32_t timeout);
    int (*ctrl) (qm_ai_driver_ctrl_t cmd, void *param);
} qm_ai_driver_param_t;

typedef enum
{
    QM_AI_DRIVER_EVENT_NONE = 0,

    /**
     * @brief 设置消音状态
     *
     * @details
     * 
     */
    QM_AI_DRIVER_EVENT_WAKEUP,

    /**
     * @brief 设置消音状态
     *
     * @details
     * 
     */
    QM_AI_DRIVER_EVENT_TIMEOUT,

}qm_ai_driver_event_type_t;

typedef union
{
    struct 
    {
        int reserved;
    }wakeup;

    struct 
    {
        int reserved;
    }timeout;
}qm_ai_driver_event_t;

typedef void (*qm_ai_driver_read_handler_t)(void *handle, uint8_t *data, int len);
typedef void (*qm_ai_driver_event_handler_t)(void *handle, qm_ai_driver_event_type_t type, qm_ai_driver_event_t *param);

#if defined(__cplusplus)
}
#endif

#endif