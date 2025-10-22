#ifndef _BSP_LIGHT_H
#define _BSP_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define CONFIG_BSP_LIGHT_KV_TAG "bsp_light"

#define CONFIG_BSP_LIGHT_NUM    (2)

#define CONFIG_BSP_LIGHT_FADE_TIME          (1000)

#define CONFIG_BSP_LIGHT_FREQ               (4000)

#define CONFIG_BSP_LIGHT_UP_INDEX           (0)
#define CONFIG_BSP_LIGHT_UP_PIN             (1)

#define CONFIG_BSP_LIGHT_DOWN_INDEX         (1)
#define CONFIG_BSP_LIGHT_DOWN_PIN           (2)


//延时关灯计算单元
#define DLEAY_OFF_UNIT(X)   (X * 60 * 1000)

//灯光闪烁
#define LIGHT_BLINK_INTERVAL        (500)
#define LIGHT_BLINK_COUNT           (1)
#define LIGHT_BLINK_BEGIN_DUTY      (50)

//项目灯光总亮度计算公式
#define LIGHT_LIGHTNESS_UNIT(X)          (X * 100)
#define LIGHT_TOTAL_LIGHTNESS_UNIT(X)    (X / 100)

//灯光亮度单元
#define LIGHT_BRI_UNIT            (LIGHT_LIGHTNESS_UNIT(5))
#define LIGHT_AUDIO_BRI_UNIT      (LIGHT_LIGHTNESS_UNIT(20))

#define LIGHT_TOTAL_LIGHTNESS(X,Y)      (((X * 75) + (Y * 25)) / 100)
#define LIGHT_UP_LIGHTNESS(X)           ((X * 100))
#define LIGHT_DOWN_LIGHTNESS(X)         ((X * 100))

//最小亮度
#define CONFIG_BSP_LIGHT_MIN            (8)
//最大亮度
#define CONFIG_BSP_LIGHT_MAX            (100)


#define CONFIG_BSP_LIGHT_TOTAL_DEFAULT_BRI      (100)
#define CONFIG_BSP_LIGHT_MIN_LIGHTNESS          (LIGHT_LIGHTNESS_UNIT(CONFIG_BSP_LIGHT_MIN))   //8%
#define CONFIG_BSP_LIGHT_MAX_LIGHTNESS          (LIGHT_LIGHTNESS_UNIT(CONFIG_BSP_LIGHT_MAX)) //100%
#define CONFIG_BSP_LIGHT_UP_DEFAULT_BRI         (LIGHT_LIGHTNESS_UNIT(100))
#define CONFIG_BSP_LIGHT_DOWN_DEFAULT_BRI       (LIGHT_LIGHTNESS_UNIT(100))


//

//灯光简易算法
#define LIGHT_CLAMP(x)  (x)  //server will do clamp, 或者调光曲线负责最低亮度的限制

#define LIGHTNESS_MAX                   (10000.0)  //亮度最大值(%)

#define LIGHTNESS_VALUE(X,Y)              (LIGHT_CLAMP(X)*65535.0/LIGHTNESS_MAX)

typedef enum 
{
    BSP_LIGHT_MODE_NONE     = 0,    //空模式
    BSP_LIGHT_MODE_NIGHT    = 1,    //小夜灯:上光 8%+下光 8%
    BSP_LIGHT_MODE_READ     = 2,    //阅读模式:上光100%+下光100%
    BSP_LIGHT_MODE_SLEEP    = 4,    //睡觉模式:关灯
    BSP_LIGHT_MODE_IDLE     = 3,    //休闲模式:上光100%+下光 50%
}bsp_light_mode_t;

typedef enum 
{
    BSP_LIGHT_SEL_NONE     = 0,
    BSP_LIGHT_SEL_DUAL     = 1,     //双光源
    BSP_LIGHT_SEL_DOWN     = 2,     //仅下光
    BSP_LIGHT_SEL_UP       = 3,     //仅上光
}bsp_light_sel_t;

typedef enum 
{
    BSP_LIGHT_BRI_RATIO_NONE        = 0,
    BSP_LIGHT_BRI_RATIO_DUAL        = 1,     //亮度均衡:   上光 50%+下光 50%
    BSP_LIGHT_BRI_RATIO_UP          = 2,     //上光源更亮: 上光100%+下光 8%
    BSP_LIGHT_BRI_RATIO_DOWN        = 3,     //下光源更亮: 上光 8%+下光 100%
}bsp_light_bri_ratio_t;

typedef enum 
{
    BSP_LIGHT_BRI_GRAD_NONE     = 0,
    BSP_LIGHT_BRI_GRAD_UP       = 1,     //亮一点
    BSP_LIGHT_BRI_GRAD_DOWN     = 2,     //暗一点
}bsp_light_bri_grad_t;

typedef struct  {

    uint8_t is_reset;

    uint8_t on_off;

    //上光*0.75+下光*0.25=总亮度
    uint16_t up_bri;  
    
    uint16_t down_bri;  

    uint8_t total_bri; 
    //

    bsp_light_sel_t sel;

    bsp_light_mode_t mode;

} bsp_light_store_t;

qm_err_t bsp_light_init(void);

typedef void (*delay_off_complete_fn)(void);

//存储信息获取
bsp_light_store_t *bsp_light_store_get(void);

//更新存储信息
qm_err_t bsp_light_store_update(void);

//开关
qm_err_t bsp_light_bri_on_off(bool_t on_off);

//模式
qm_err_t bsp_light_mode(bsp_light_mode_t mode);

//灯源选择
qm_err_t bsp_light_select(bsp_light_sel_t sel);

//亮度  亮度范围值 0 ~ 100
qm_err_t bsp_light_bri_set(int bri);

//上光亮度  亮度范围值 0 ~ 100
qm_err_t bsp_light_bri_up_set(int bri);

//下光亮度  亮度范围值 0 ~ 100
qm_err_t bsp_light_bri_down_set(int bri);

//亮度+
qm_err_t bsp_light_bri_up(void);

//亮度-
qm_err_t bsp_light_bri_down(void);

//亮度配比
qm_err_t bsp_light_bri_ratio(bsp_light_bri_ratio_t ratio);

//亮度渐变
qm_err_t bsp_light_bri_grad(bsp_light_bri_grad_t grad);

//上光开关
qm_err_t bsp_light_uplight_switch(bool_t on_off);

//下光开关
qm_err_t bsp_light_downlight_switch(bool_t on_off);

//上发光变亮
qm_err_t bsp_light_warm_up(void);

//下发光变亮
qm_err_t bsp_light_warm_down(void);

//变光
qm_err_t bsp_light_switch1(void);

//全亮->半亮
qm_err_t bsp_light_switch2(void);

//定时关灯
qm_err_t bsp_light_delay_off_cancel(void);
qm_err_t bsp_light_delay_off(uint8_t delay_timer, delay_off_complete_fn fn);

//重置
qm_err_t bsp_light_reset(void);

//灯光闪烁
qm_err_t bsp_light_blink(void);

//全亮
qm_err_t bsp_light_all(void);

//中性光
qm_err_t bsp_light_neuter(void);

//自然光
qm_err_t bsp_light_nature(void);


qm_err_t voice_corpus_request(uint8_t number);

qm_err_t voice_abort_request(uint8_t number);

#ifdef __cplusplus
}
#endif
#endif
