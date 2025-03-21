/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _QM_AUDIO_H_
#define _QM_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#if 0
#include "qm_spec_api.h"
#endif

#ifndef CONFIG_QM_AUDIO_SSID_MAX_LEN
#define CONFIG_QM_AUDIO_SSID_MAX_LEN              64
#endif

#ifndef CONFIG_QM_AUDIO_PWD_MAX_LEN
#define CONFIG_QM_AUDIO_PWD_MAX_LEN               64
#endif


/**
 * @brief 
 */
typedef enum {
    
    QM_AUDIO_RECV_NONE,

    QM_AUDIO_RECV_NET_NOTIFY,            //网络状态通知

    QM_AUDIO_RECV_WAKEUP_NOTIFY,         //唤醒通知

    QM_AUDIO_RECV_SR_TIMEOUT_NOTIFY,     //超时通知

    QM_AUDIO_RECV_NETCONFIG_NOTIFY,      //配网通知

    QM_AUDIO_RECV_OFFLINE_CMD_NOTIFY,    //离线控制指令下发

    QM_AUDIO_RECV_MAX

} qm_audio_recv_type_t;

typedef enum {
    QM_AUDIO_NET_CONNECT,       //已连接平台
    QM_AUDIO_NET_DISCONNECT,    //断开平台
} qm_audio_net_status_t;

/**
 * @brief 模块识别唤醒时, 通知用户的报文类型
 */
typedef struct {
    qm_audio_net_status_t net_status;
} qm_audio_net_notify_t;

/**
 * @brief 模块识别唤醒时, 通知用户的报文类型
 */
typedef struct {
    uint8_t reserved;   //保留
} qm_audio_wakeup_notify_t;

/**
 * @brief 模块识别语音数据超时
 */
typedef struct {
    uint8_t reserved;   //保留
} qm_audio_sr_timout_notify_t;

/**
 * @brief 模块识别类型
 */
typedef enum {
    QM_AUDIO_REC_CMD_NONE,
    QM_AUDIO_REC_CMD_WAKEUP,           //唤醒词
    QM_AUDIO_REC_CMD_COMMAND,          //识别词
    QM_AUDIO_REC_CMD_NETCONFIG,        //配网词
    QM_AUDIO_REC_CMD_VOLUME_UP,        //音量+
    QM_AUDIO_REC_CMD_VOLUME_DOWN,      //音量-
    QM_AUDIO_REC_CMD_VOLUME_MAX,       //音量MAX
    QM_AUDIO_REC_CMD_VOLUME_MIN,       //音量MIN
    QM_AUDIO_REC_CMD_MAX
} qm_audio_rec_cmd_type_t;

typedef struct 
{
    qm_audio_rec_cmd_type_t rec_type;       //识别类型,如识别为唤醒词/识别词
    uint32_t                rec_id;         //识别的ID号,ID号唯一
    char                  *rec_str;         //识别字符串, 比如乐鑫、思必驰的识别字符串都为拼音
    char                  *rec_priv;        //用户自定义参数,可用于说明
    char                  *reply_uri;       //回复音频
}qm_audio_cmd_table_t;

/**
 * @brief 模块识别语音数据时, 通知用户的报文类型
 */
typedef struct {
    qm_audio_rec_cmd_type_t rec_type;   //模块识别类型
    uint32_t                sr_id;      //识别词的ID号
    char                    *sr_str;    //识别词字符串, 比如乐鑫、思必驰的识别字符串都为拼音
    float                   threshold;  //识别阈值
} qm_audio_offline_cmd_notify_t;

#if 0
/**
 * @brief 模块识别语音数据时, 通知用户的报文类型
 */
typedef struct {
    qm_spec_property_t  *property; //
} qm_audio_online_cmd_notify_t;
#endif

/**
 * @brief dev模块收到从网络上来的报文时, 通知用户的报文内容
 */
typedef struct {
    qm_audio_recv_type_t recv_type;
    union {
        qm_audio_net_notify_t         net_notify;
        qm_audio_wakeup_notify_t      wakeup_notify;
        qm_audio_sr_timout_notify_t   timout_notify;
    #if 0
        qm_audio_online_cmd_notify_t  online_cmd_notify;
    #endif
        qm_audio_offline_cmd_notify_t offline_cmd_notify;
    } data;
} qm_audio_recv_t;

typedef void* qm_audio_t;

typedef void (*qm_audio_recv_handler_t)(qm_audio_t handle, qm_audio_recv_t *packet, void *arg);

typedef struct 
{
    uint8_t mic_num;

    uint32_t timeout;

    uint32_t pid;
    char *product_secret;

    uint8_t enable_prov;
    uint8_t enable_wakeup;
    uint8_t enable_welcome;
    uint8_t enable_reply_ind;

    const char *appid;
    const char *room;
    const char * uid;
    const char * token;

    int     table_num;
    qm_audio_cmd_table_t  *table;
    

    void *user_data;
    qm_audio_recv_handler_t recv_fn;

}qm_audio_cfg_t;

/**
 * @brief AUDIO模块初始化
 *
 * @return qm_err_t
 */
qm_audio_t qm_audio_open(qm_audio_cfg_t *cfg);

/**
 * @brief 模块初始化
 *
 * @param[in] params 初始化结构体
 *
 * @return qm_err_t
 */
qm_err_t qm_audio_start(qm_audio_t handle);

/**
 * @brief 模块连接AI平台
 *
 * @param[in] params 初始化结构体
 *
 * @return qm_err_t
 */
qm_err_t qm_audio_connect_ai(qm_audio_t handle);

/**
 * @brief 模块断连AI平台
 *
 * @param[in] params 初始化结构体
 *
 * @return qm_err_t
 */
qm_err_t qm_audio_disconnect_ai(qm_audio_t handle);

/**
 * @brief 模块音频播放
 *
 * @param[in] params 初始化结构体
 *
 * @return qm_err_t
 */
qm_err_t qm_audio_player(qm_audio_t handle, int voice_type, int voice_id);


#ifdef __cplusplus
}
#endif

#endif
