#ifndef __QM_AUDIO_PROV_H__
#define __QM_AUDIO_PROV_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

typedef void* qm_audio_prov_handle_t;

/**
 * @brief provision模块收到app配网信息, 通知用户的报文内容
 */
typedef struct {
    char *ssid; 
    char *password;
    int8_t timezone;
} qm_audio_prov_recv_t;

/**
 * @brief provision模块收到从网络上来的报文时, 通知用户所调用的数据回调函数
 *
 * @param[in] handle provision会话句柄
 * @param[in] packet provision消息结构体, 存放收到的报文内容
 * @param[in] userdata 用户上下文
 *
 * @return void
 */
typedef void (* qm_audio_prov_recv_handler_t)(void *handle, const qm_audio_prov_recv_t *packet, void *userdata);

/**
 * @brief 
 */
typedef struct {
    uint32_t pid;
    char *product_secret;
    void *userdata;
    qm_audio_prov_recv_handler_t recv_fn;
} qm_audio_prov_config_t;

/**
 * @brief 创建provision会话实例, 并以默认值配置会话参数
 *
 * @return void *
 * @retval 非NULL provision实例的句柄
 * @retval NULL   初始化失败, 一般是内存分配失败导致
 * 
 */
qm_audio_prov_handle_t qm_audio_prov_open(qm_audio_prov_config_t *config);

/**
 * @brief 启动provision流程
 *
 * @param handle provision会话句柄
 *
 * @return qm_err_t
 * @retval <QM_EOK  数据接收失败
 * @retval >=QM_EOK 数据接收成功
 */

qm_err_t qm_audio_prov_start(qm_audio_prov_handle_t handle);

/**
 * @brief 结束provision会话, 销毁实例并回收资源
 *
 * @param[in] handle 指向provision会话句柄的指针
 *
 * @return int32_t
 * @retval <QM_EOK  执行失败
 * @retval >=QM_EOK 执行成功
 * 
 */
qm_err_t qm_audio_prov_close(qm_audio_prov_handle_t handle);


#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_PROVISION_H__ */