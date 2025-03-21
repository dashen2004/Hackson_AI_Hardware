#ifndef _AUDIO_DEVICE_H_
#define _AUDIO_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define CONFIG_AUDIO_DEV_LOCAL_ROLE_ID  (1)

typedef enum
{
    AUDIO_ID_LIGHT_WAKEUP = 1,         //唤醒1
    AUDIO_ID_LIGHT_WAKEUP2,            //唤醒2
    AUDIO_ID_LIGHT_ON  ,               //开灯/打开灯光
    AUDIO_ID_LIGHT_OFF,                //关灯/关闭灯光
    AUDIO_ID_LIGHT_UP,                 //亮一点/调亮一点
    AUDIO_ID_LIGHT_DOWN,               //暗一点/调暗一点
    AUDIO_ID_LIGHT_MAX,                //最大亮度
    AUDIO_ID_LIGHT_MIN,                //最小亮度
    AUDIO_ID_LIGHT_MOD,                //中等亮度
    AUDIO_ID_LIGHT_ALL,                //全亮
    AUDIO_ID_LIGHT_NEUTER,             //中性光
    AUDIO_ID_LIGHT_NATURE,             //自然光
    AUDIO_ID_LIGHT_SLEEP,              //睡觉
    AUDIO_ID_LIGHT_NIGHT,              //小夜灯
    AUDIO_ID_LIGHT_CHANGE,             //变光
    AUDIO_ID_LIGHT_READ1,               //阅读模式
    AUDIO_ID_LIGHT_READ2,               //看书
    AUDIO_ID_LIGHT_IDLE,               //休闲模式
    AUDIO_ID_LIGHT_TOP,                //上光
    AUDIO_ID_LIGHT_LOWER,              //下光
    AUDIO_ID_LIGHT_TIMER,              //定时模式
    AUDIO_ID_LIGHT_RESET,              //恢复出厂设置

    AUDIO_ID_LIGHT_TIMEOUT      = 100, //超时
    AUDIO_ID_LIGHT_NETCONFIG    = 101, //配网模式

    AUDIO_ID_LIGHT_IND    = 102, //提示音
    
    AUDIO_ID_WELCOME    = 103, //提示音

}audio_player_audio_id_t;


qm_err_t audio_device_init(void);


#ifdef __cplusplus
}
#endif

#endif