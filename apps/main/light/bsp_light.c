#include "qm.h"
#include "qm_work.h"
#include "qm_kv.h"

#include "qm_light_driver.h"
#include "qm_light_alg.h"

#include "bsp_light.h"

#define LOG_TAG "BSP_LIGHT"

typedef struct  {

    qm_work_t  light_work; 

    qm_light_driver_handle_t handle;
    
    qm_light_driver_param_t base;

    uint16_t lightness;     //有效范围[0,65535]


    qm_light_alg_id_t alg_id;

} bsp_light_info_t;

static qm_work_t delay_off = {0};
static void qm_light_driver_event(qm_light_driver_handle_t handle, qm_light_driver_event_type_t event_type, void* arg);

static bsp_light_store_t g_light_store = {0};

static bsp_light_info_t g_light_info[CONFIG_BSP_LIGHT_NUM] = 
{
    //上光源：亮度 100%
    {
        .alg_id = QM_LIGHT_ALG_ID_MAIN,
        .base = {
            .port = CONFIG_BSP_LIGHT_UP_INDEX,
            .freq = CONFIG_BSP_LIGHT_FREQ,
            .pin = CONFIG_BSP_LIGHT_UP_PIN, 
            .level = QM_LIGHT_DRIVER_LEVEL_HIGH,
            .cb = qm_light_driver_event,
            .arg = (void *)&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX],
        },
    },

    //下光源：亮度 100%
    {
        .alg_id = QM_LIGHT_ALG_ID_AUX,
        .base = {
            .port = CONFIG_BSP_LIGHT_DOWN_INDEX,
            .freq = CONFIG_BSP_LIGHT_FREQ,
            .pin = CONFIG_BSP_LIGHT_DOWN_PIN, 
            .level = QM_LIGHT_DRIVER_LEVEL_HIGH,
            .cb = qm_light_driver_event,
            .arg = (void *)&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX],
        },
    }
};

static void qm_light_driver_event(qm_light_driver_handle_t handle, qm_light_driver_event_type_t event_type, void* arg)
{
    
}

static qm_err_t light_set_lightness(bsp_light_info_t *light_info, int lightness, int fade_time)
{
    int lightness_algo = qm_light_alg_get_exp(light_info->alg_id, lightness);
    int lightness_new = LIGHTNESS_VALUE((lightness), lightness_algo);
 
    if(lightness_new != light_info->lightness){

        QM_LOGD(LOG_TAG,"[lslt]:%d,(%d,%d)d,%d\n", light_info->alg_id, lightness_new, lightness_algo, fade_time);
        
        qm_light_driver_set_lightness_linear(light_info->handle, lightness_new, fade_time);

        light_info->lightness = lightness_new;
    }

    return QM_EOK;
}
static qm_err_t light_set_lightness_blink(bsp_light_info_t *light_info, int lightness)
{
    int lightness_algo = qm_light_alg_get_exp(light_info->alg_id, lightness);
    int lightness_new = LIGHTNESS_VALUE((lightness), lightness_algo);
 
    QM_LOGD(LOG_TAG,"[blink]:%d,(%d,%d)d\n", light_info->alg_id, lightness_new, lightness_algo);
        
    light_info->lightness = lightness_new;
    qm_light_driver_blink(light_info->handle, 0, lightness_new, LIGHT_BLINK_INTERVAL, LIGHT_BLINK_BEGIN_DUTY, LIGHT_BLINK_COUNT);

    return QM_EOK;
}


static void light_total_lightness_update(void)
{

    bsp_light_store_t *store = bsp_light_store_get();

    switch (store->sel)
    {
        case BSP_LIGHT_SEL_DUAL:
            store->total_bri = LIGHT_TOTAL_LIGHTNESS_UNIT(LIGHT_TOTAL_LIGHTNESS(store->up_bri, store->down_bri));
        break;

        case BSP_LIGHT_SEL_DOWN:
            store->total_bri = LIGHT_TOTAL_LIGHTNESS_UNIT(store->down_bri);
        break;

        case BSP_LIGHT_SEL_UP:
            store->total_bri = LIGHT_TOTAL_LIGHTNESS_UNIT(store->up_bri);
        break;
    
        default:
            break;
    }
    QM_LOGD(LOG_TAG,"[update]:%d,(%d,%d)\n", store->total_bri, store->up_bri, store->down_bri);
}

qm_err_t bsp_light_store_update(void)
{
    return qm_kv_set(CONFIG_BSP_LIGHT_KV_TAG, &g_light_store, sizeof(bsp_light_store_t), QM_TRUE);
}

qm_err_t bsp_light_init(void)
{
    qm_err_t ret = QM_EOK;
    int store_len = 0;

    qm_light_alg_table_init();

    ret = qm_kv_get(CONFIG_BSP_LIGHT_KV_TAG, &g_light_store, &store_len);
    if(ret != QM_EOK || store_len == 0){
        g_light_store.up_bri = CONFIG_BSP_LIGHT_UP_DEFAULT_BRI;
        g_light_store.down_bri = CONFIG_BSP_LIGHT_DOWN_DEFAULT_BRI;
        g_light_store.sel = BSP_LIGHT_SEL_DUAL;
        g_light_store.mode = BSP_LIGHT_MODE_NONE;
        g_light_store.is_reset = QM_FALSE;
        light_total_lightness_update();
    }

    QM_LOGD(LOG_TAG,"[kv]:[%d]%d(%d,%d),%d,%d\n", g_light_store.is_reset, g_light_store.total_bri, g_light_store.up_bri, g_light_store.down_bri, g_light_store.sel, g_light_store.mode);

    for (int i = 0; i < QM_ARRAY_SIZE(g_light_info); i++)
    {
        g_light_info[i].handle = qm_light_driver_init(&g_light_info[i].base);
        if(g_light_info[i].handle == NULL){
            return -QM_EINVAL;
        }
    }
    
    if(g_light_store.is_reset){
        bsp_light_blink();
    }else{
        bsp_light_bri_on_off(QM_TRUE);
    }

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

    if(on_off){

        if(store->mode == BSP_LIGHT_MODE_SLEEP){
            store->mode = BSP_LIGHT_MODE_NONE;
        }

        light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
        light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    
    }else{
        qm_cancel_delayed_action(&delay_off);
        
        light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], 0, CONFIG_BSP_LIGHT_FADE_TIME);
        light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], 0, CONFIG_BSP_LIGHT_FADE_TIME);
    
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

            store->up_bri = LIGHT_LIGHTNESS_UNIT(8);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(8);
            store->on_off = QM_TRUE;
            store->sel = BSP_LIGHT_SEL_DUAL;

            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);

        break;

        case BSP_LIGHT_MODE_READ:

            store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(100);
            store->on_off = QM_TRUE;
            store->sel = BSP_LIGHT_SEL_DUAL;

            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);
        break;

        case BSP_LIGHT_MODE_IDLE:

            store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(50);
            store->on_off = QM_TRUE;
            store->sel = BSP_LIGHT_SEL_DUAL;
            
            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);

        break;

        case BSP_LIGHT_MODE_SLEEP:

            store->on_off = QM_FALSE;
            
            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], 0, CONFIG_BSP_LIGHT_FADE_TIME);
            light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], 0, CONFIG_BSP_LIGHT_FADE_TIME);

        break;

        default:

        break;
    }

    store->mode = mode;
    light_total_lightness_update();

    return QM_EOK;
}

//灯源选择
qm_err_t bsp_light_select(bsp_light_sel_t sel)
{

    bsp_light_store_t *store = bsp_light_store_get();

    switch (sel)
    {
        case BSP_LIGHT_SEL_DUAL:
            store->up_bri = LIGHT_LIGHTNESS_UNIT(store->total_bri);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(store->total_bri);
        break;

        case BSP_LIGHT_SEL_DOWN:
            store->up_bri = LIGHT_LIGHTNESS_UNIT(0);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(store->total_bri);
        break;

        case BSP_LIGHT_SEL_UP:
            store->down_bri = LIGHT_LIGHTNESS_UNIT(0);
            store->up_bri = LIGHT_LIGHTNESS_UNIT(store->total_bri);
        break;
    
        default:
            break;
    }

    store->sel = sel;
    store->mode = BSP_LIGHT_MODE_NONE;
    light_total_lightness_update();

    if(store->on_off){
        light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
        light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);    
    }

    return QM_EOK;
}

//亮度  亮度范围值
qm_err_t bsp_light_bri_set(int bri)
{

    bsp_light_store_t *store = bsp_light_store_get();

    switch (store->sel)
    {
        case BSP_LIGHT_SEL_DUAL:
            store->up_bri = LIGHT_LIGHTNESS_UNIT(bri);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(bri);
        break;

        case BSP_LIGHT_SEL_DOWN:
            store->up_bri = LIGHT_LIGHTNESS_UNIT(0);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(bri);
        break;

        case BSP_LIGHT_SEL_UP:
            store->down_bri = LIGHT_LIGHTNESS_UNIT(0);
            store->up_bri = LIGHT_LIGHTNESS_UNIT(bri);
        break;
    
        default:
            break;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);    

    return QM_EOK;
}

//亮度配比
qm_err_t bsp_light_bri_ratio(bsp_light_bri_ratio_t ratio)
{

    bsp_light_store_t *store = bsp_light_store_get();

    switch (ratio)
    {
        case BSP_LIGHT_BRI_RATIO_DUAL:
            store->up_bri = LIGHT_LIGHTNESS_UNIT(50);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(50);
            store->sel = BSP_LIGHT_SEL_DUAL;

        break;

        case BSP_LIGHT_BRI_RATIO_UP:
            store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(8);
            store->sel = BSP_LIGHT_SEL_DUAL;

        break;

        case BSP_LIGHT_BRI_RATIO_DOWN:
            store->up_bri =  LIGHT_LIGHTNESS_UNIT(8);
            store->down_bri = LIGHT_LIGHTNESS_UNIT(100);
            store->sel = BSP_LIGHT_SEL_DUAL;

        break;
    
        default:
            break;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);    

    return QM_EOK;
}

//亮度+
qm_err_t bsp_light_bri_up(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    switch (store->sel)
    {
        case BSP_LIGHT_SEL_DUAL:
            store->up_bri += LIGHT_BRI_UNIT;
            store->down_bri += LIGHT_BRI_UNIT;
        break;

        case BSP_LIGHT_SEL_DOWN:
            store->down_bri += LIGHT_BRI_UNIT;
        break;

        case BSP_LIGHT_SEL_UP:
            store->up_bri += LIGHT_BRI_UNIT;
        break;
    
        default:
            break;
    }

    if(store->up_bri >= CONFIG_BSP_LIGHT_MAX_LIGHTNESS){
        store->up_bri = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    if(store->down_bri >= CONFIG_BSP_LIGHT_MAX_LIGHTNESS){
        store->down_bri = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);    

    return QM_EOK;
}

//亮度-
qm_err_t bsp_light_bri_down(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    switch (store->sel)
    {
        case BSP_LIGHT_SEL_DUAL:
            store->up_bri -= LIGHT_BRI_UNIT;
            store->down_bri -= LIGHT_BRI_UNIT;
            if(store->up_bri <= CONFIG_BSP_LIGHT_MIN_LIGHTNESS){
                store->up_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            }
        
            if(store->down_bri <= CONFIG_BSP_LIGHT_MIN_LIGHTNESS){
                store->down_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            }
        break;

        case BSP_LIGHT_SEL_DOWN:
            store->down_bri -= LIGHT_BRI_UNIT;
            if(store->down_bri <= CONFIG_BSP_LIGHT_MIN_LIGHTNESS){
                store->down_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            }
        break;

        case BSP_LIGHT_SEL_UP:
            store->up_bri -= LIGHT_BRI_UNIT;
            if(store->up_bri <= CONFIG_BSP_LIGHT_MIN_LIGHTNESS){
                store->up_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            }
        break;
    
        default:
            break;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);    

    return QM_EOK;
}

//亮度挡位+
qm_err_t bsp_light_bri_gear_up(void)
{
    bsp_light_store_t *store = bsp_light_store_get();
    switch (store->sel)
    {
        case BSP_LIGHT_SEL_DUAL:
            store->up_bri += LIGHT_AUDIO_BRI_UNIT;
            store->down_bri += LIGHT_AUDIO_BRI_UNIT;
        break;

        case BSP_LIGHT_SEL_DOWN:
            store->down_bri += LIGHT_AUDIO_BRI_UNIT;
        break;

        case BSP_LIGHT_SEL_UP:
            store->up_bri += LIGHT_AUDIO_BRI_UNIT;
        break;
    
        default:
            break;
    }

    if(store->up_bri >= CONFIG_BSP_LIGHT_MAX_LIGHTNESS){
        store->up_bri = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    if(store->down_bri >= CONFIG_BSP_LIGHT_MAX_LIGHTNESS){
        store->down_bri = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);    

    return QM_EOK;
}

//亮度挡位-
qm_err_t bsp_light_bri_gear_down(void)
{
    bsp_light_store_t *store = bsp_light_store_get();
    int up_bri_gear = store->up_bri / LIGHT_AUDIO_BRI_UNIT;
    int down_bri_gear = store->down_bri / LIGHT_AUDIO_BRI_UNIT;
    switch (store->sel)
    {
        case BSP_LIGHT_SEL_DUAL:
            if(up_bri_gear == 0) {
                store->up_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            } else {
                store->up_bri = (up_bri_gear - 1) * LIGHT_AUDIO_BRI_UNIT;
            }
            if(down_bri_gear == 0) {
                store->down_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            } else {
                store->down_bri = (down_bri_gear - 1) * LIGHT_AUDIO_BRI_UNIT;
            }
        break;

        case BSP_LIGHT_SEL_DOWN:
            if(down_bri_gear == 0) {
                store->down_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            } else {
                store->down_bri = (down_bri_gear - 1) * LIGHT_AUDIO_BRI_UNIT;
            }
        break;

        case BSP_LIGHT_SEL_UP:
            if(up_bri_gear == 0) {
                store->up_bri = CONFIG_BSP_LIGHT_MIN_LIGHTNESS;
            } else {
                store->up_bri = (up_bri_gear - 1) * LIGHT_AUDIO_BRI_UNIT;
            }
        break;
    
        default:
            break;
    }
    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);    

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

//上发光变亮
qm_err_t bsp_light_warm_up(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->up_bri += LIGHT_BRI_UNIT;
    if(store->up_bri >= CONFIG_BSP_LIGHT_MAX_LIGHTNESS){
        store->up_bri = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    if(store->down_bri){
        store->down_bri -= LIGHT_BRI_UNIT;
        if(store->down_bri < CONFIG_BSP_LIGHT_MIN_LIGHTNESS){
            store->down_bri = LIGHT_LIGHTNESS_UNIT(0);
        }
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;
    if(store->up_bri && store->down_bri){
        store->sel = BSP_LIGHT_SEL_DUAL;
    }else{
        store->sel = BSP_LIGHT_SEL_UP;
    }
    
    light_total_lightness_update();
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);  

    return QM_EOK;
}

qm_err_t bsp_light_bri_up_set(int bri)
{
    bsp_light_store_t *store = bsp_light_store_get();
    
    store->up_bri = LIGHT_LIGHTNESS_UNIT(bri);

    if(!store->up_bri && !store->down_bri){
        store->sel = BSP_LIGHT_SEL_UP;
        store->up_bri = LIGHT_LIGHTNESS_UNIT(1);
    }else if(!store->up_bri && store->down_bri){
        store->sel = BSP_LIGHT_SEL_DOWN;
    }else if(store->up_bri && store->down_bri){
        store->sel = BSP_LIGHT_SEL_DUAL;
    }
    
    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_total_lightness_update();
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);  

    return QM_EOK;
}

qm_err_t bsp_light_bri_down_set(int bri)
{
    bsp_light_store_t *store = bsp_light_store_get();
    
    store->down_bri = LIGHT_LIGHTNESS_UNIT(bri);

    if(!store->up_bri && !store->down_bri){
        store->sel = BSP_LIGHT_SEL_UP;
        store->down_bri = LIGHT_LIGHTNESS_UNIT(1);
    }else if(!store->up_bri && store->down_bri){
        store->sel = BSP_LIGHT_SEL_DOWN;
    }else if(store->up_bri && store->down_bri){
        store->sel = BSP_LIGHT_SEL_DUAL;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_total_lightness_update();
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);  

    return QM_EOK;
}

//下发光变亮
qm_err_t bsp_light_warm_down(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->down_bri += LIGHT_BRI_UNIT;
    if(store->down_bri >= CONFIG_BSP_LIGHT_MAX_LIGHTNESS){
        store->down_bri = CONFIG_BSP_LIGHT_MAX_LIGHTNESS;
    }

    if(store->up_bri){
        store->up_bri -= LIGHT_BRI_UNIT;
        if(store->up_bri < CONFIG_BSP_LIGHT_MIN_LIGHTNESS){
            store->up_bri = LIGHT_LIGHTNESS_UNIT(0);
        }
    }


    if(store->up_bri && store->down_bri){
        store->sel = BSP_LIGHT_SEL_DUAL;
    }else{
        store->sel = BSP_LIGHT_SEL_DOWN;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_total_lightness_update();
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);  

    return QM_EOK;
}

//变光
qm_err_t bsp_light_switch1(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->on_off = QM_TRUE;

    //下(100%亮度) -> 上 (100%亮度)
    if(store->down_bri == LIGHT_LIGHTNESS_UNIT(100) && !store->up_bri)
    {
        store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
        store->down_bri = LIGHT_LIGHTNESS_UNIT(0);
        store->sel = BSP_LIGHT_SEL_UP;
    }

    //上 (100%亮度) -> 上+下 (100%亮度)
    else if (store->up_bri == LIGHT_LIGHTNESS_UNIT(100) && 
             store->down_bri == LIGHT_LIGHTNESS_UNIT(0))
    {
        store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
        store->down_bri = LIGHT_LIGHTNESS_UNIT(100);
        store->sel = BSP_LIGHT_SEL_DUAL;
    }
   
    //上+下 (100%亮度) -> 小夜灯8%(上+下)
    else if (store->up_bri == LIGHT_LIGHTNESS_UNIT(100) && 
            store->down_bri == LIGHT_LIGHTNESS_UNIT(100))
    {
        store->up_bri = LIGHT_LIGHTNESS_UNIT(8);
        store->down_bri = LIGHT_LIGHTNESS_UNIT(8);
        store->sel = BSP_LIGHT_SEL_DUAL;
    }

    else{
        store->up_bri = LIGHT_LIGHTNESS_UNIT(0);
        store->down_bri = LIGHT_LIGHTNESS_UNIT(100);
        store->sel = BSP_LIGHT_SEL_DOWN;
    }

    store->on_off = QM_TRUE;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);

    return QM_EOK;
}

//全亮
qm_err_t bsp_light_all(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->on_off = QM_TRUE;

    store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
    store->down_bri = LIGHT_LIGHTNESS_UNIT(100);

    store->sel = BSP_LIGHT_SEL_DUAL;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);

    return QM_EOK;
}

//全/半亮
qm_err_t bsp_light_switch2(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    //上+下 (100%亮度) -> 上+下 (50%亮度)
    if(store->down_bri == LIGHT_LIGHTNESS_UNIT(100) && 
        store->up_bri == LIGHT_LIGHTNESS_UNIT(100))
    {
        store->up_bri = LIGHT_LIGHTNESS_UNIT(50);
        store->down_bri = LIGHT_LIGHTNESS_UNIT(50);
    }

    //上+下 (50%亮度) -> 上+下 (100%亮度)
    else if (store->up_bri == LIGHT_LIGHTNESS_UNIT(50) && 
            store->down_bri == LIGHT_LIGHTNESS_UNIT(50))
    {
        store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
        store->down_bri = LIGHT_LIGHTNESS_UNIT(100);
    }

    else{
        store->up_bri = LIGHT_LIGHTNESS_UNIT(100);
        store->down_bri = LIGHT_LIGHTNESS_UNIT(100);
    }

    store->on_off = QM_TRUE;
    store->sel = BSP_LIGHT_SEL_DUAL;
    store->mode = BSP_LIGHT_MODE_NONE;

    light_total_lightness_update();

    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri, CONFIG_BSP_LIGHT_FADE_TIME);
    light_set_lightness(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri, CONFIG_BSP_LIGHT_FADE_TIME);

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

    store->up_bri = CONFIG_BSP_LIGHT_UP_DEFAULT_BRI;
    store->down_bri = CONFIG_BSP_LIGHT_DOWN_DEFAULT_BRI;
    store->sel = BSP_LIGHT_SEL_DUAL;
    store->mode = BSP_LIGHT_MODE_NONE;
    store->is_reset = QM_TRUE;

    light_total_lightness_update();

    return QM_EOK;
}

//灯光闪烁
qm_err_t bsp_light_blink(void)
{
    bsp_light_store_t *store = bsp_light_store_get();

    store->on_off = QM_TRUE;

    light_set_lightness_blink(&g_light_info[CONFIG_BSP_LIGHT_UP_INDEX], store->up_bri);
    light_set_lightness_blink(&g_light_info[CONFIG_BSP_LIGHT_DOWN_INDEX], store->down_bri);

    return QM_EOK;
}
