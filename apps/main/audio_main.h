/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _AUDIO_MAIN_H_
#define _AUDIO_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "qm_spec_api.h"

// #define CONFIG_NETWORK_URL       "ws://aiagentplatform.xjiangiot.com:8000/xiaozhi/v1/" //老服务

#define CONFIG_NETWORK_URL       "ws://aiagentplatform.xjiangiot.com:8000/xiaojiang/v1/"    //正式服
// #define CONFIG_NETWORK_URL       "ws://aiagentplatform.xjiangiot.com:28000/xiaojiang/v1/"  //测试服


#define CONFIG_NETWORK_DELAY_MS         (1500)

#define CONFIG_DISABLE_SR_FUNC          (0)
#define CONFIG_SUPPORT_AEC              (1)

#define CONFIG_HARDWARE_VERSION         (21)
#define CONFIG_XJIANG_VERSION           (14)
#define CONFIG_XJIANG_CLOUD_PID         (10290)
#define CONFIG_XJIANG_CLOUD_SECERT      "91B47743DC334240947B79CB80253852"

#define  CONFIG_AUDIO_LIGHT_HW_VERSION  "LIGHT.0059.G.101_FW_WB74_0.1.4_0.1.4"

#define CONFIG_SUPPORT_ONLINE_OPEN_WAV   (1)
#define CONFIG_SUPPORT_ONLINE_CLOSE_WAV  (1)

#define CONFIG_AUDIO_BOARD_USER_BUTTON              (0)
#define CONFIG_AUDIO_BOARD_USER_STATUS_LED          (0)

#define CONFIG_AUDIO_BOARD_SUPPORT_BUTTON_SWITCH    (0)

//语音事件
#define CONFIG_AUDIO_WAKEUPT_TIMEOUT         (60 * 1000)
// #define CONFIG_AUDIO_WAKEUPT_TIMEOUT         (20 * 60 * 1000)

#define CONFIG_AUDIO_CONFIG_IND_TIMEOUT      (3 * 60 * 1000)  //配网提示间隔

#define CONFIG_AUDIO_CONN_ERROR_TIMEOUT      (2 * 60 * 1000)  //联网错误超时

#define CONFIG_AUDIO_CONN_SWITCH_TIMEOUT     (10  * 1000)  //切换模式超时

#define CONFIG_AUDIO_MAIN_EVENT_ID           (0xFF00)

#define CONFIG_AUDIO_SUB_EVENT_CONFIG           (0x0001)   //配网模式
#define CONFIG_AUDIO_SUB_EVENT_CONFIG_TIMEOUT   (0x0002)   //配网超时
#define CONFIG_AUDIO_SUB_EVENT_CEART_WS         (0x0003)   //创建Websocket
#define CONFIG_AUDIO_SUB_EVENT_CONN_WS          (0x0004)   //连接Websocket成功
#define CONFIG_AUDIO_SUB_EVENT_NET_ERROR        (0x0005)   //网络异常断开2分钟
#define CONFIG_AUDIO_USB_EVENT_CONNING_LOCAL    (0x0006)   // 连接路由器

#define CONFIG_AUDIO_SUB_EVENT_WAKEUP_ID            (0x0106)
#define CONFIG_AUDIO_SUB_EVENT_TIEMOUT_ID           (0x0107)
#define CONFIG_AUDIO_SUB_EVENT_CONTROL_ID           (0x0108)
#define CONFIG_AUDIO_SUB_EVENT_SWITCH_MODE_ID       (0x0109)
#define CONFIG_AUDIO_SUB_EVENT_SWITCH_COMPLETE_ID   (0x010A)

#define CONFIG_AUDIO_SUB_EVENT_OTA_START            (0x010B)
#define CONFIG_AUDIO_SUB_EVENT_OTA_SUCCESS          (0x010C)
#define CONFIG_AUDIO_SUB_EVENT_OTA_FAILED           (0x010D)

#define CONFIG_AUDIO_ID_LIGHT_WAKEUP         (1)
#define CONFIG_AUDIO_ID_LIGHT_NETCONFIG      (2)
#define CONFIG_AUDIO_ID_LIGHT_ON             (3)             //开灯/打开灯光
#define CONFIG_AUDIO_ID_LIGHT_OFF            (4)             //关灯/关闭灯光
#define CONFIG_AUDIO_ID_LIGHT_UP             (5)             //亮一点/调亮一点
#define CONFIG_AUDIO_ID_LIGHT_DOWN           (6)             //暗一点/调暗一点
#define CONFIG_AUDIO_ID_LIGHT_MAX            (7)             //最大亮度
#define CONFIG_AUDIO_ID_LIGHT_MIN            (8)             //最小亮度
#define CONFIG_AUDIO_ID_LIGHT_MOD            (9)             //中等亮度
#define CONFIG_AUDIO_ID_LIGHT_ALL            (10)            //全亮
#define CONFIG_AUDIO_ID_LIGHT_NEUTER         (11)            //中性光
#define CONFIG_AUDIO_ID_LIGHT_NATURE         (12)            //自然光
#define CONFIG_AUDIO_ID_LIGHT_SLEEP          (13)            //睡觉
#define CONFIG_AUDIO_ID_LIGHT_NIGHT          (14)            //小夜灯
#define CONFIG_AUDIO_ID_LIGHT_CHANGE         (15)            //变光
#define CONFIG_AUDIO_ID_LIGHT_READ1          (16)            //阅读模式
#define CONFIG_AUDIO_ID_LIGHT_READ2          (17)            //看书
#define CONFIG_AUDIO_ID_LIGHT_IDLE           (18)            //休闲模式
#define CONFIG_AUDIO_ID_LIGHT_TOP            (19)            //上光
#define CONFIG_AUDIO_ID_LIGHT_LOWER          (20)            //下光
#define CONFIG_AUDIO_ID_LIGHT_TIMER          (21)            //定时模式
#define CONFIG_AUDIO_ID_LIGHT_RESET          (22)            //恢复出厂设置


//遥控器事件
#define CONFIG_AUDIO_BEACON_HOLD_TIME           (2 * 100)
#define CONFIG_AUDIO_BEACON_POWER_ON_TIMEOUT    (6 * 1000)

//对码
#define CONFIG_AUDIO_BEACON_PAIR            (0x86)
//开灯
#define CONFIG_AUDIO_BEACON_ON              (0x01)
//关灯
#define CONFIG_AUDIO_BEACON_OFF             (0x02)
//亮度+
#define CONFIG_AUDIO_BEACON_BRI_UP          (0x10)
#define CONFIG_AUDIO_BEACON_BRI_UP_HOLD     (0x82)
#define CONFIG_AUDIO_BEACON_BRI_UP_CANCEL   (0xC2)
//亮度-
#define CONFIG_AUDIO_BEACON_BRI_DOWN        (0x11)
#define CONFIG_AUDIO_BEACON_BRI_DOWN_HOLD   (0x83)
#define CONFIG_AUDIO_BEACON_BRI_DOWN_CANCEL (0xC3)
//上发光变亮
#define CONFIG_AUDIO_BEACON_WARM_UP         (0x12)
#define CONFIG_AUDIO_BEACON_WARM_UP_HOLD    (0x84)
#define CONFIG_AUDIO_BEACON_WARM_UP_CANCEL  (0xC4)

//下发光变亮
#define CONFIG_AUDIO_BEACON_WARM_DOWN           (0x13)
#define CONFIG_AUDIO_BEACON_WARM_DOWN_HOLD      (0x85)
#define CONFIG_AUDIO_BEACON_WARM_DOWN_CANCEL    (0xC5)

//变光
#define CONFIG_AUDIO_BEACON_SWITCH1         (0x04)
//全/半亮
#define CONFIG_AUDIO_BEACON_SWITCH2         (0x05)
//小夜灯
#define CONFIG_AUDIO_BEACON_NIGHT           (0x07)
//延时关灯： 延时60s关灯
#define CONFIG_AUDIO_BEACON_DELAY_OFF       (0x08)
//清码：通电5S后执行
#define CONFIG_AUDIO_BEACON_RESET           (0x80)

//spec事件
//开关灯
#define CONFIG_AUDIO_LIGHT_ONOFF_SIID       (2)
#define CONFIG_AUDIO_LIGHT_ONOFF_PIID       (1)
#define CONFIG_AUDIO_LIGHT_ONOFF_TYPE       (QM_SPEC_PROPERTY_FORMAT_BOOL)

//亮度
#define CONFIG_AUDIO_LIGHT_BRI_SIID         (2)
#define CONFIG_AUDIO_LIGHT_BRI_PIID         (2)
#define CONFIG_AUDIO_LIGHT_BRI_TYPE         (QM_SPEC_PROPERTY_FORMAT_NUMBER)
//模式
#define CONFIG_AUDIO_LIGHT_MODE_SIID        (2)
#define CONFIG_AUDIO_LIGHT_MODE_PIID        (3)
#define CONFIG_AUDIO_LIGHT_MODE_TYPE        (QM_SPEC_PROPERTY_FORMAT_NUMBER)
//变光
#define CONFIG_AUDIO_LIGHT_SWITCH_SIID      (2)
#define CONFIG_AUDIO_LIGHT_SWITCH_PIID      (4)
#define CONFIG_AUDIO_LIGHT_SWITCH_TYPE       (QM_SPEC_PROPERTY_FORMAT_NUMBER)
//灯源选择
#define CONFIG_AUDIO_LIGHT_SEL_SIID         (2)
#define CONFIG_AUDIO_LIGHT_SEL_PIID         (5)
#define CONFIG_AUDIO_LIGHT_SEL_TYPE         (QM_SPEC_PROPERTY_FORMAT_NUMBER)
//亮度配比
#define CONFIG_AUDIO_LIGHT_RATIO_SIID       (2)
#define CONFIG_AUDIO_LIGHT_RATIO_PIID       (6)
#define CONFIG_AUDIO_LIGHT_RATIO_TYPE       (QM_SPEC_PROPERTY_FORMAT_NUMBER)
//亮度渐变:亮一点，暗一点
#define CONFIG_AUDIO_LIGHT_GRAD_SIID        (2)
#define CONFIG_AUDIO_LIGHT_GRAD_PIID        (7)
#define CONFIG_AUDIO_LIGHT_GRAD_TYPE        (QM_SPEC_PROPERTY_FORMAT_NUMBER)
//定时
#define CONFIG_AUDIO_LIGHT_DELAY_OFF_SIID   (3)
#define CONFIG_AUDIO_LIGHT_DELAY_OFF_PIID   (1)
#define CONFIG_AUDIO_LIGHT_DELAY_OFF_TYPE   (QM_SPEC_PROPERTY_FORMAT_NUMBER)
//重置设备
#define CONFIG_AUDIO_LIGHT_RESET_SIID       (3)
#define CONFIG_AUDIO_LIGHT_RESET_PIID       (2)
#define CONFIG_AUDIO_LIGHT_RESET_TYPE       (QM_SPEC_PROPERTY_FORMAT_NUMBER)

#ifdef __cplusplus
}
#endif

#endif
