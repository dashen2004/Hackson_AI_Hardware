#ifndef _BSP_LIGHT_H
#define _BSP_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define CONFIG_BSP_LIGHT_KV_TAG "bsp_light"

#define CONFIG_BSP_LIGHT_FADE_TIME          (1000)

#define CONFIG_BSP_LIGHT_FREQ               (4000)

//延时关灯计算单元
#define DLEAY_OFF_UNIT(X)   (X * 60 * 1000)

//灯光闪烁
#define LIGHT_BLINK_INTERVAL        (500)
#define LIGHT_BLINK_COUNT           (1)
#define LIGHT_BLINK_BEGIN_DUTY      (50)

//项目灯光总亮度计算公式
#define LIGHT_LIGHTNESS_UNIT(X)          (X * 1)
#define LIGHT_TOTAL_LIGHTNESS_UNIT(X)    (X / 1)

//灯光亮度单元
#define LIGHT_BRI_UNIT            (LIGHT_LIGHTNESS_UNIT(5))
#define LIGHT_AUDIO_BRI_UNIT      (LIGHT_LIGHTNESS_UNIT(20))

//最小亮度
#define CONFIG_BSP_LIGHT_MIN            (8)
//最大亮度
#define CONFIG_BSP_LIGHT_MAX            (100)

#define CONFIG_BSP_LIGHT_DEFAULT_BRI      (100)
#define CONFIG_BSP_LIGHT_MIN_LIGHTNESS    (LIGHT_LIGHTNESS_UNIT(CONFIG_BSP_LIGHT_MIN))   //8%
#define CONFIG_BSP_LIGHT_MAX_LIGHTNESS    (LIGHT_LIGHTNESS_UNIT(CONFIG_BSP_LIGHT_MAX)) //100%

// rgb default value
#define CONFIG_BSP_RGB_RED_DEFAULT_VALUE        (0x00FD5B3F)
#define CONFIG_BSP_RGB_ORANGE_DEFAULT_VALUE     (0x00FD9B3F)
#define CONFIG_BSP_RGB_BLUE_DEFAULT_VALUE       (0x003FB7FD)
#define CONFIG_BSP_RGB_GREEN_DEFAULT_VALUE      (0x0060FD5A)
#define CONFIG_BSP_RGB_PURPLE_DEFAULT_VALUE     (0x008B3FFC)
#define CONFIG_BSP_RGB_COLORFUL_DEFAULT_VALUE   (0x003F71FD)
#define CONFIG_BSP_RGB_SHUTDOWN_VALUE           (0x00E0E0E0)

#define CONFIG_BSP_RGB_RED_START_VALUE          (0x00FD3F3F)
#define CONFIG_BSP_RGB_RED_END_VALUE            (0x00FD743F)
#define CONFIG_BSP_RGB_ORANGE_START_VALUE       (0x00FD753F)
#define CONFIG_BSP_RGB_ORANGE_END_VALUE         (0x00FDD340)
#define CONFIG_BSP_RGB_GREEN_START_VALUE        (0x0040FD6F)
#define CONFIG_BSP_RGB_GREEN_END_VALUE          (0x0070D53A)
#define CONFIG_BSP_RGB_BLUE_START_VALUE         (0x003F71FD)
#define CONFIG_BSP_RGB_BLUE_END_VALUE           (0x0040FDFD)
#define CONFIG_BSP_RGB_PURPLE_START_VALUE       (0x00603FFB)
#define CONFIG_BSP_RGB_PURPLE_END_VALUE         (0x00AB3FFB)
#define CONFIG_BSP_RGB_COLORFUL_START_VALUE     (0x00FD3F3F)
#define CONFIG_BSP_RGB_COLORFUL_END_VALUE       (0x00FD3FC3)

// cct
#define CONFIG_BSP_LIGHT_CCT_MAX_VALUE          (5700)
#define CONFIG_BSP_LIGHT_CCT_MIN_VALUE          (2700)
#define CONFIG_BSP_LIGHT_CCT_WARM_VALUE         (2700)
#define CONFIG_BSP_LIGHT_CCT_NATURAL_VALUE      (3500)
#define CONFIG_BSP_LIGHT_CCT_COOL_VALUE         (5500)

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
} bsp_light_mode_t;

typedef enum 
{
    BSP_LIGHT_BRI_GRAD_NONE     = 0,
    BSP_LIGHT_BRI_GRAD_UP       = 1,     //亮一点
    BSP_LIGHT_BRI_GRAD_DOWN     = 2,     //暗一点
} bsp_light_bri_grad_t;

typedef enum 
{
    BSP_RGB_SELECT_NONE     = 0,
    BSP_RGB_SELECT_RED      = 1,
    BSP_RGB_SELECT_ORANGE   = 2,
    BSP_RGB_SELECT_GREEN    = 3,
    BSP_RGB_SELECT_BLUE     = 4,
    BSP_RGB_SELECT_PURPLE   = 5,
    BSP_RGB_SELECT_COLORFUL = 6,
} bsp_rgb_select_t;

typedef struct  {
    uint8_t is_reset;

    uint8_t on_off;
    uint8_t brightness;
    uint16_t cct;
    bsp_light_mode_t mode;

    uint8_t rgb_on_off;
    uint32_t rgb_val;
    bsp_rgb_select_t rgb_sel;
} bsp_light_store_t;

qm_err_t bsp_light_init(void);

typedef void (*delay_off_complete_fn)(void);

//存储信息获取
bsp_light_store_t *bsp_light_store_get(void);

//更新存储信息
qm_err_t bsp_light_store_update(void);

//开关
qm_err_t bsp_light_bri_on_off(bool_t on_off);

// 色温
qm_err_t bsp_light_cct_set(uint32_t cct_val);

// rgb on off
qm_err_t bsp_light_rgb_on_off(bool_t on_off);

//模式
qm_err_t bsp_light_mode(bsp_light_mode_t mode);

// rgb select
qm_err_t bsp_light_rgb_select(bsp_rgb_select_t select);

//亮度  亮度范围值 0 ~ 100
qm_err_t bsp_light_bri_set(int bri);

// rgb value set
qm_err_t bsp_light_rgb_value_set(uint32_t value);

//上光亮度  亮度范围值 0 ~ 100
qm_err_t bsp_light_bri_up_set(int bri);

//下光亮度  亮度范围值 0 ~ 100
qm_err_t bsp_light_bri_down_set(int bri);

//亮度+
qm_err_t bsp_light_bri_up(void);

//亮度-
qm_err_t bsp_light_bri_down(void);

//亮度渐变
qm_err_t bsp_light_bri_grad(bsp_light_bri_grad_t grad);

//变光
qm_err_t bsp_light_switch1(void);

//定时关灯
qm_err_t bsp_light_delay_off_cancel(void);
qm_err_t bsp_light_delay_off(uint8_t delay_timer, delay_off_complete_fn fn);

//重置
qm_err_t bsp_light_reset(void);

//灯光闪烁
qm_err_t bsp_light_blink(void);

//全亮
qm_err_t bsp_light_all(void);

qm_err_t voice_corpus_request(uint8_t number);

qm_err_t voice_abort_request(uint8_t number);

void bsp_light_bulb_init(void);
void bsp_light_bulb_set_cw(uint16_t cct, uint8_t brightness);
void bsp_light_bulb_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value);

#ifdef __cplusplus
}
#endif
#endif
