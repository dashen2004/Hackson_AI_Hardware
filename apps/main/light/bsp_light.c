#include "qm.h"
#include "qm_work.h"
#include "qm_kv.h"
#include "bsp_light.h"
#include "audio_main.h"
#include "xjiang_cloud.h"
#include "qm_color.h"

#define LOG_TAG "BSP_LIGHT"
static qm_work_t delay_off = {0};
static bsp_light_store_t g_light_store = {0};

static qm_err_t light_set_lightness(int lightness)
{
    bsp_light_bulb_set_cw(bsp_light_store_get()->cct, lightness);

    return QM_EOK;
}

static void light_set_rgb(uint8_t on_off)
{
    bsp_light_store_t *store = bsp_light_store_get();

    qm_color_rgb_t rgb;
    qm_color_hsv_t hsv;

    qm_color_code2rgb(store->rgb_val, &rgb);
    qm_color_rgb2hsv(&rgb, &hsv);

    if (on_off) {
        bsp_light_bulb_set_hsv(hsv.h, hsv.s, hsv.v);
    } else {
        bsp_light_bulb_set_hsv(0, 0, 0);
    }
}

qm_err_t bsp_light_store_update(void)
{
    return qm_kv_set(CONFIG_BSP_LIGHT_KV_TAG, &g_light_store, sizeof(bsp_light_store_t), QM_TRUE);
}

qm_err_t bsp_light_init(void)
{
    qm_err_t ret = QM_EOK;
    int crash_reset = 0;
    int store_len = 0;

    ret = qm_kv_get(CONFIG_BSP_LIGHT_KV_TAG, &g_light_store, &store_len);
    if (ret != QM_EOK || store_len == 0) {
        g_light_store.is_reset = QM_FALSE;
        g_light_store.on_off = QM_FALSE;
        g_light_store.brightness = CONFIG_BSP_LIGHT_DEFAULT_BRI;
        g_light_store.cct = CONFIG_BSP_LIGHT_CCT_NATURAL_VALUE;
        g_light_store.mode = BSP_LIGHT_MODE_NONE;
        g_light_store.rgb_on_off = QM_FALSE;
        g_light_store.rgb_sel = BSP_RGB_SELECT_RED;
        g_light_store.rgb_val = CONFIG_BSP_RGB_RED_DEFAULT_VALUE;
    }

    QM_LOGD(LOG_TAG,"[kv]:[%d]%d(%d,%d),%d,%d\n", g_light_store.is_reset, g_light_store.brightness, g_light_store.cct, 
                                                    g_light_store.mode, g_light_store.rgb_sel, g_light_store.rgb_val);

    bsp_light_bulb_init();

    if (g_light_store.is_reset) {
        bsp_light_blink();
    } else {
        crash_reset = qm_is_crash_reset();
        if (!crash_reset) {
            bsp_light_bri_on_off(QM_TRUE);
        } else {
            bsp_light_bri_on_off(g_light_store.on_off);
        }
    }

    // default close rgb
    bsp_light_rgb_on_off(QM_FALSE);

    g_light_store.is_reset = QM_FALSE;

    bsp_light_store_update();

    return ret;
}

//存储信息获取
bsp_light_store_t *bsp_light_store_get(void)
{
    return &g_light_store;
}

//开关
qm_err_t bsp_light_bri_on_off(bool_t on_off)
{
    bsp_light_store_t *store = bsp_light_store_get();

    if (on_off) {
        if (store->mode == BSP_LIGHT_MODE_SLEEP) {
            store->mode = BSP_LIGHT_MODE_NONE;
        }

        light_set_lightness(store->brightness);
    } else {
        qm_cancel_delayed_action(&delay_off);
        
        light_set_lightness(0);
    }

    store->on_off = on_off;
    
    return QM_EOK;
}

//模式
qm_err_t bsp_light_mode(bsp_light_mode_t mode)
{
    bsp_light_store_t *store = bsp_light_store_get();

    switch (mode)
    {
        case BSP_LIGHT_MODE_NIGHT:
            store->brightness = 8;
            store->on_off = QM_TRUE;

            light_set_lightness(store->brightness);
        break;

        case BSP_LIGHT_MODE_READ:
            store->brightness = 100;
            store->on_off = QM_TRUE;

            light_set_lightness(store->brightness);
        break;

        case BSP_LIGHT_MODE_IDLE:
            store->brightness = 50;
            store->on_off = QM_TRUE;

            light_set_lightness(store->brightness);
        break;

        case BSP_LIGHT_MODE_SLEEP:
            store->on_off = QM_FALSE;

            light_set_lightness(0);
        break;

        default:

        break;
    }

    store->mode = mode;

    return QM_EOK;
}

// rgb on off
qm_err_t bsp_light_rgb_on_off(bool_t on_off)
{
    bsp_light_store_t *store = bsp_light_store_get();

    light_set_rgb(on_off);

    store->rgb_on_off = on_off;
    
    return QM_EOK;
}

// rgb select
qm_err_t bsp_light_rgb_select(bsp_rgb_select_t select)
{
    if (select <= BSP_RGB_SELECT_NONE || select > BSP_RGB_SELECT_COLORFUL) {
        return QM_EINVAL;
    }

    bsp_light_store_t *store = bsp_light_store_get();

    store->rgb_sel = select;
    store->rgb_on_off = QM_TRUE;

    switch (select) 
    {
        case BSP_RGB_SELECT_RED:
            store->rgb_val = CONFIG_BSP_RGB_RED_DEFAULT_VALUE;
        break;

        case BSP_RGB_SELECT_ORANGE:
            store->rgb_val = CONFIG_BSP_RGB_ORANGE_DEFAULT_VALUE;
        break;

        case BSP_RGB_SELECT_GREEN:
            store->rgb_val = CONFIG_BSP_RGB_GREEN_DEFAULT_VALUE;
        break;

        case BSP_RGB_SELECT_BLUE:
            store->rgb_val = CONFIG_BSP_RGB_BLUE_DEFAULT_VALUE;
        break;

        case BSP_RGB_SELECT_PURPLE:
            store->rgb_val = CONFIG_BSP_RGB_PURPLE_DEFAULT_VALUE;
        break;

        case BSP_RGB_SELECT_COLORFUL:
            store->rgb_val = CONFIG_BSP_RGB_COLORFUL_DEFAULT_VALUE;
        break;

        default:
            break;
    }

    light_set_rgb(QM_TRUE);

    return QM_EOK;
}

// rgb value set
qm_err_t bsp_light_rgb_value_set(uint32_t val)
{
    if (val > 0x00FFFFFF) {
        return QM_EINVAL;
    }

    bsp_light_store_t *store = bsp_light_store_get();

    store->rgb_val = val;
    store->rgb_on_off = QM_TRUE;

    light_set_rgb(QM_TRUE);

    return QM_EOK;
}

//亮度  亮度范围值
qm_err_t bsp_light_bri_set(int bri)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->brightness = bri;
    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_set_lightness(store->brightness);  

    return QM_EOK;
}

//亮度+
qm_err_t bsp_light_bri_up(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->brightness += LIGHT_BRI_UNIT;

    if (store->brightness > CONFIG_BSP_LIGHT_MAX_LIGHTNESS) {
        store->brightness = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_set_lightness(store->brightness);    

    return QM_EOK;
}

//亮度-
qm_err_t bsp_light_bri_down(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->brightness -= LIGHT_BRI_UNIT;

    if (store->brightness < CONFIG_BSP_LIGHT_MIN_LIGHTNESS) {
        store->brightness = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    
    light_set_lightness(store->brightness);   

    return QM_EOK;
}

//亮度挡位+
qm_err_t bsp_light_bri_gear_up(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->brightness += LIGHT_AUDIO_BRI_UNIT;

    if (store->brightness > CONFIG_BSP_LIGHT_MAX_LIGHTNESS) {
        store->brightness = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_set_lightness(store->brightness);    

    return QM_EOK;
}

//亮度挡位-
qm_err_t bsp_light_bri_gear_down(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->brightness -= LIGHT_AUDIO_BRI_UNIT;

    if (store->brightness < CONFIG_BSP_LIGHT_MIN_LIGHTNESS) {
        store->brightness = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    
    light_set_lightness(store->brightness);   

    return QM_EOK;
}

//亮度渐变
qm_err_t bsp_light_bri_grad(bsp_light_bri_grad_t grad)
{

    bsp_light_store_t *store = bsp_light_store_get();

    switch (grad)
    {
        case BSP_LIGHT_BRI_GRAD_UP:
            bsp_light_bri_gear_up();
        break;

        case BSP_LIGHT_BRI_GRAD_DOWN:
            bsp_light_bri_gear_down();
        break;

        default:
            break;
    }

    return QM_EOK;
}

//变光
qm_err_t bsp_light_switch1(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    // 变色温

    return QM_EOK;
}

//全亮
qm_err_t bsp_light_all(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->on_off = QM_TRUE;
    store->brightness = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_set_lightness(store->brightness);

    return QM_EOK;
}

//定时关灯
static void delay_off_action(void *arg)
{
    bsp_light_store_t *store = bsp_light_store_get();
    delay_off_complete_fn complete_fn = (delay_off_complete_fn)arg;

    if(store->on_off){
        bsp_light_bri_on_off(QM_FALSE);
        if(complete_fn){
            complete_fn();
        }
    }
}

qm_err_t bsp_light_delay_off_cancel(void)
{
    return qm_cancel_delayed_action(&delay_off);
}

qm_err_t bsp_light_delay_off(uint8_t delay_timer, delay_off_complete_fn fn)
{
    qm_cancel_delayed_action(&delay_off);

    return qm_post_delayed_action(&delay_off, delay_off_action, fn, DLEAY_OFF_UNIT(delay_timer));
}

//重置
qm_err_t bsp_light_reset(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->is_reset = QM_TRUE;
    store->on_off = QM_FALSE;
    store->brightness = CONFIG_BSP_LIGHT_DEFAULT_BRI;
    store->mode = BSP_LIGHT_MODE_NONE;
    store->cct = CONFIG_BSP_LIGHT_CCT_NATURAL_VALUE;
    store->rgb_on_off = QM_FALSE;
    store->rgb_sel = BSP_RGB_SELECT_RED;
    store->rgb_val = CONFIG_BSP_RGB_RED_DEFAULT_VALUE;

    return QM_EOK;
}

//灯光闪烁
qm_err_t bsp_light_blink(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->on_off = QM_TRUE;

    return QM_EOK;
}

qm_err_t voice_corpus_request(uint8_t number)
{
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    qm_spec_property_t *property_pre = NULL;
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return -QM_ENOMEM;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_CORPUS_PIID);
    qm_spec_property_pack_number(property_pre, number);
    property_pre->did = qm_iot_did_get();

    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_REPORT, property_operation);
}

qm_err_t voice_abort_request(uint8_t number)
{
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    qm_spec_property_t *property_pre = NULL;
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return -QM_ENOMEM;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_VOICE_CONTROL_SIID, CONFIG_AUDIO_VOICE_ABORT_PIID);
    qm_spec_property_pack_number(property_pre, number);
    property_pre->did = qm_iot_did_get();

    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_REPORT, property_operation);
}

qm_err_t voice_rtc_login_info_get(void)
{
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    qm_spec_property_t *property_pre = NULL;
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return -QM_ENOMEM;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_RTC_LOGIN_SIID, 0);
    property_pre->did = qm_iot_did_get();

    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_GET_REQ, property_operation);
}
