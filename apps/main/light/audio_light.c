
#include "qm.h"
#include "qm_work.h"
#include "vars.h"

#include "audio_main.h"
#include "audio_light.h"

#include "bsp_light.h"
#include "bsp_audio.h"

#include "xjiang_cloud.h"

typedef struct 
{
    uint8_t siid;
    uint16_t piid;
    qm_err_t (*action)(qm_spec_property_t *property);
    qm_err_t (*report)(qm_spec_property_t *property);
}audio_light_table_t;

static qm_err_t onoff_aciton(qm_spec_property_t *property);
static qm_err_t bri_aciton(qm_spec_property_t *property);
static qm_err_t mode_aciton(qm_spec_property_t *property);
static qm_err_t switch_aciton(qm_spec_property_t *property);
static qm_err_t sel_aciton(qm_spec_property_t *property);
static qm_err_t ratio_aciton(qm_spec_property_t *property);
static qm_err_t grad_aciton(qm_spec_property_t *property);
static qm_err_t uplight_aciton(qm_spec_property_t *property);
static qm_err_t downlight_aciton(qm_spec_property_t *property);
static qm_err_t delay_off_aciton(qm_spec_property_t *property);
static qm_err_t reset_aciton(qm_spec_property_t *property);
static qm_err_t volume_set_action(qm_spec_property_t *property);
static qm_err_t volume_mute_action(qm_spec_property_t *property);
static qm_err_t volume_cmd_action(qm_spec_property_t *property);
static qm_err_t chat_state_action(qm_spec_property_t *property);

static qm_err_t onoff_report(qm_spec_property_t *property);
static qm_err_t bri_report(qm_spec_property_t *property);
static qm_err_t mode_report(qm_spec_property_t *property);
static qm_err_t switch_report(qm_spec_property_t *property);
static qm_err_t sel_report(qm_spec_property_t *property);
static qm_err_t ratio_report(qm_spec_property_t *property);
static qm_err_t grad_report(qm_spec_property_t *property);
static qm_err_t uplight_report(qm_spec_property_t *property);
static qm_err_t downlight_report(qm_spec_property_t *property);
static qm_err_t delay_off_report(qm_spec_property_t *property);
static qm_err_t reset_report(qm_spec_property_t *property);
static qm_err_t volume_set_report(qm_spec_property_t *property);
static qm_err_t volume_mute_report(qm_spec_property_t *property);
static qm_err_t volume_cmd_report(qm_spec_property_t *property);
static qm_err_t chat_state_report(qm_spec_property_t *property);

extern int audio_stream_EGV_set(uint8_t type);
extern int audio_stream_EGV_get(void);
extern int audio_stream_ALG_set(float type);
extern float audio_stream_ALG_get(void);
static qm_err_t EGV_aciton(qm_spec_property_t *property)
{
    char data[10] = {0};
    int len = 0;
    int value = 0;
    if(property == NULL) {
        return QM_ERROR;
    }

    qm_spec_property_unpack_string(property, data, &len);
    if(sscanf(data, "%d", &value) != 1) {
        return QM_ERROR;
    }

    audio_stream_EGV_set(value);
    return QM_EOK;
}

static qm_err_t EGV_report(qm_spec_property_t *property)
{
    qm_spec_property_t *property_pre = NULL;
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return QM_ERROR;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_EGV_SIID, CONFIG_AUDIO_EGV_PIID);
    char data[10] = {0};
    sprintf(data, "%d", audio_stream_EGV_get());
    qm_spec_property_pack_string(property_pre, data, strlen(data));
    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(property_operation);
}

static qm_err_t ALG_aciton(qm_spec_property_t *property)
{
    char data[10] = {0};
    int len = 0;
    float value = 0;
    if(property == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, data, &len);
    if(sscanf(data, "%1f", &value) != 1) {
        return QM_ERROR;
    }

    audio_stream_ALG_set(value);
    return QM_EOK;
}

static qm_err_t ALG_report(qm_spec_property_t *property)
{
    qm_spec_property_t *property_pre = NULL;
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return QM_ERROR;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_EGV_SIID, CONFIG_AUDIO_EGV_PIID);
    char data[10] = {0};
    sprintf(data, "%1f", audio_stream_ALG_get());
    qm_spec_property_pack_string(property_pre, data, strlen(data));
    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(property_operation);
}

static audio_light_table_t g_light_table[] =
{
    {CONFIG_AUDIO_LIGHT_ONOFF_SIID,     CONFIG_AUDIO_LIGHT_ONOFF_PIID,      onoff_aciton,  onoff_report},
    {CONFIG_AUDIO_LIGHT_BRI_SIID,       CONFIG_AUDIO_LIGHT_BRI_PIID,        bri_aciton, bri_report},
    {CONFIG_AUDIO_LIGHT_MODE_SIID,      CONFIG_AUDIO_LIGHT_MODE_PIID,       mode_aciton,  mode_report},
    {CONFIG_AUDIO_LIGHT_SWITCH_SIID,    CONFIG_AUDIO_LIGHT_SWITCH_PIID,     switch_aciton, switch_report},
    {CONFIG_AUDIO_LIGHT_SEL_SIID,       CONFIG_AUDIO_LIGHT_SEL_PIID,        sel_aciton,  sel_report},
    {CONFIG_AUDIO_LIGHT_RATIO_SIID,     CONFIG_AUDIO_LIGHT_RATIO_PIID,      ratio_aciton,  ratio_report},
    {CONFIG_AUDIO_LIGHT_GRAD_SIID,      CONFIG_AUDIO_LIGHT_GRAD_PIID,       grad_aciton,  grad_report},
    {CONFIG_AUDIO_LIGHT_UPLIGHT_SIID,   CONFIG_AUDIO_LIGHT_UPLIGHT_PIID,    uplight_aciton,  uplight_report},
    {CONFIG_AUDIO_LIGHT_DOWNLIGHT_SIID, CONFIG_AUDIO_LIGHT_DOWNLIGHT_PIID,  downlight_aciton,  downlight_report},
    {CONFIG_AUDIO_LIGHT_DELAY_OFF_SIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_PIID,  delay_off_aciton,  delay_off_report},
    {CONFIG_AUDIO_LIGHT_RESET_SIID,     CONFIG_AUDIO_LIGHT_RESET_PIID,      reset_aciton,  reset_report},
    {CONFIG_AUDIO_VOLUME_SIID,          CONFIG_AUDIO_VOLUME_PIID,           volume_set_action,  volume_set_report},
    {CONFIG_AUDIO_VOLUME_SIID,          CONFIG_AUDIO_VOLUME_CMD_PIID,       volume_cmd_action,  volume_cmd_report},
    {CONFIG_AUDIO_VOLUME_SIID,          CONFIG_AUDIO_VOLUME_MUTE_PIID,      volume_mute_action,  volume_mute_report},
    {CONFIG_AUDIO_EXIT_CHAT_SIID,       CONFIG_AUDIO_CHAT_STATE_PIID,       chat_state_action,  chat_state_report},
    {CONFIG_AUDIO_EGV_SIID,     CONFIG_AUDIO_EGV_PIID,      EGV_aciton,  EGV_report},
    {CONFIG_AUDIO_ALG_SIID,     CONFIG_AUDIO_ALG_PIID,      ALG_aciton,  ALG_report},
};

qm_err_t audio_light_init(void)
{
    return bsp_light_init();
}

static void audio_light_add(void *operation, uint8_t siid, uint16_t piid, int type, int value)
{
    qm_spec_property_t *property_pre = NULL;
    qm_spec_property_operation_t *property_operation = (qm_spec_property_operation_t *)operation;
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return ;
    }

    qm_spec_property_pack_xiid(property_pre, siid, piid);
    switch (type)
    {
        case QM_SPEC_PROPERTY_FORMAT_BOOL:
            qm_spec_property_pack_bool(property_pre, (bool_t)value);
            break;

        case QM_SPEC_PROPERTY_FORMAT_NUMBER:
            qm_spec_property_pack_number(property_pre, value);
            break;
        default:
            break;
    }

    qm_spec_property_add(property_operation, property_pre);
}

qm_err_t audio_light_snapshot_report(void)
{
    bsp_light_store_t *store = bsp_light_store_get();
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }

//开关灯
    audio_light_add(property_operation, 
        CONFIG_AUDIO_LIGHT_ONOFF_SIID, CONFIG_AUDIO_LIGHT_ONOFF_PIID, CONFIG_AUDIO_LIGHT_ONOFF_TYPE, (int)(store->on_off));
//亮度
    audio_light_add(property_operation, 
        CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, (int)(store->total_bri));
//模式
    audio_light_add(property_operation, 
        CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, (int)(store->mode));

//灯源选择
    audio_light_add(property_operation, 
        CONFIG_AUDIO_LIGHT_SEL_SIID, CONFIG_AUDIO_LIGHT_SEL_PIID, CONFIG_AUDIO_LIGHT_SEL_TYPE, (int)(store->sel));

    return xjiang_cloud_multi_report(property_operation);
}

qm_err_t audio_light_action(qm_spec_property_t *property)
{
    if(property == NULL){
        return -QM_EINVAL;
    }

    for (int i = 0; i < QM_ARRAY_SIZE(g_light_table); i++)
    {
        if (g_light_table[i].siid == property->siid && 
            g_light_table[i].piid == property->piid)
        {
            g_light_table[i].action(property);
            break;   
        }
    }
    
    return QM_EOK;
}

qm_err_t audio_light_report(qm_spec_property_t *property)
{
    if(property == NULL){
        return -QM_EINVAL;
    }

    for (int i = 0; i < QM_ARRAY_SIZE(g_light_table); i++)
    {
        if (g_light_table[i].siid == property->siid && 
            g_light_table[i].piid == property->piid)
        {
            g_light_table[i].report(property);
            break;   
        }
    }
    
    return QM_EOK;
}

qm_err_t audio_light_action_and_report(qm_spec_property_t *property)
{
    qm_err_t ret = QM_EOK;
    if(property == NULL){
        return -QM_EINVAL;
    }

    for (int i = 0; i < QM_ARRAY_SIZE(g_light_table); i++)
    {
        if (g_light_table[i].siid == property->siid && 
            g_light_table[i].piid == property->piid)
        {
            ret = g_light_table[i].action(property);
            if(ret != QM_EOK){
                break;
            }
            ret = g_light_table[i].report(property);
            break;   
        }
    }
    
    return ret;
}

//开关灯
static qm_err_t onoff_aciton(qm_spec_property_t *property)
{
    bool_t onoff = QM_FALSE;
    qm_spec_property_unpack_bool(property, &onoff);

    if(onoff == QM_FALSE){
        bsp_light_delay_off_cancel();
    }

    bsp_light_bri_on_off(onoff);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t onoff_report(qm_spec_property_t *property)
{
   return audio_light_snapshot_report();
}
//亮度
static qm_err_t bri_aciton(qm_spec_property_t *property)
{
    int bri = 0;
    qm_spec_property_unpack_number(property, &bri);

    bsp_light_bri_set(bri);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t bri_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}
//模式
static qm_err_t mode_aciton(qm_spec_property_t *property)
{
    int mode = 0;
    qm_spec_property_unpack_number(property, &mode);

    bsp_light_mode(mode);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t mode_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}

//变光
static qm_err_t switch_aciton(qm_spec_property_t *property)
{
    int sw = 0;
    qm_spec_property_unpack_number(property, &sw);

    if(sw == QM_TRUE){
        bsp_light_switch1();
    }

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t switch_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}

//灯源选择
static qm_err_t sel_aciton(qm_spec_property_t *property)
{
    int sel = 0;
    qm_spec_property_unpack_number(property, &sel);

    bsp_light_select(sel);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t sel_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}

//亮度配比
static qm_err_t ratio_aciton(qm_spec_property_t *property)
{
    int ratio = 0;
    qm_spec_property_unpack_number(property, &ratio);

    bsp_light_bri_ratio(ratio);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t ratio_report(qm_spec_property_t *property)
{
//亮度配比上报(节省资源复用同一个 property)
    xjiang_cloud_report(property);

    return audio_light_snapshot_report();
}

//亮度渐变:亮一点，暗一点
static qm_err_t grad_aciton(qm_spec_property_t *property)
{   
    int grad = 0;
    qm_spec_property_unpack_number(property, &grad);

    bsp_light_bri_grad(grad);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t grad_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}

//上光
static qm_err_t uplight_aciton(qm_spec_property_t *property)
{   
    int onoff = 0;
    qm_spec_property_unpack_number(property, &onoff);

    bsp_light_uplight_switch(onoff);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t uplight_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}

//下光
static qm_err_t downlight_aciton(qm_spec_property_t *property)
{   
    int onoff = 0;
    qm_spec_property_unpack_number(property, &onoff);

    bsp_light_downlight_switch(onoff);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t downlight_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}

//定时
static void delay_off_work(void)
{
    audio_light_snapshot_report();
}

static qm_err_t delay_off_aciton(qm_spec_property_t *property)
{
    int delay_off = 0;
    qm_spec_property_unpack_number(property, &delay_off);

    return bsp_light_delay_off(delay_off, delay_off_work);
}

static qm_err_t delay_off_report(qm_spec_property_t *property)
{
//定时关灯
    xjiang_cloud_report(property);
    return QM_EOK;
}

//重置设备
static qm_err_t reset_aciton(qm_spec_property_t *property)
{
    bsp_light_reset();
    return QM_EOK;
}

static qm_err_t reset_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t volume_set_action(qm_spec_property_t *property)
{
    int number = 0;

    qm_spec_property_unpack_number(property, &number);

    set_var_volume_value(number, true);
    return QM_EOK;
}

static qm_err_t volume_set_report(qm_spec_property_t *property)
{
    qm_spec_property_t *property_pre = NULL;
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return QM_ERROR;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_VOLUME_SIID, CONFIG_AUDIO_VOLUME_PIID);
    qm_spec_property_pack_number(property_pre, get_var_volume_value());
    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(property_operation);
}

static qm_err_t volume_mute_action(qm_spec_property_t *property)
{
    int number = 0;

    qm_spec_property_unpack_number(property, &number);
    
    switch (number)
    {
        case 1: //开启静音模式
            set_var_mute_value(true, true);
        break;

        case 2: //关闭静音模式
            set_var_mute_value(false, true);
        break;        
    
        default:
        break;
    }
    return QM_EOK;
}

static qm_err_t volume_mute_report(qm_spec_property_t *property)
{
    qm_spec_property_t *property_pre = NULL;
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return QM_ERROR;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_VOLUME_SIID, CONFIG_AUDIO_VOLUME_MUTE_PIID);
    if(get_var_mute_value() == 1) {
        qm_spec_property_pack_number(property_pre, 1);
    } else {
        qm_spec_property_pack_number(property_pre, 2);
    }
    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(property_operation);
}

static qm_err_t volume_cmd_action(qm_spec_property_t *property)
{
    int number = 0;
    int get = get_var_volume_value();
    qm_spec_property_unpack_number(property, &number);
    switch (number)
    {
        case 1: //音量增大
            get += CONFIG_AUDIP_VOLUME_DETLA;
            if(get >= CONFIG_AUDIP_MAX_VOLUME){
                get = CONFIG_AUDIP_MAX_VOLUME;
            }
        break;

        case 2: //音量减小
            get -= CONFIG_AUDIP_VOLUME_DETLA;
            if(get <= CONFIG_AUDIP_MIN_VOLUME){
                get = CONFIG_AUDIP_MIN_VOLUME;
            }
        break;        
    
        case 3: //最大音量
            get = CONFIG_AUDIP_MAX_VOLUME;
        break;    

        case 4: //最小音量
            get = CONFIG_AUDIP_MIN_VOLUME;
        break;  

        default:
        break;
    }
    set_var_volume_value(get, true);
    return QM_EOK;
}

static qm_err_t volume_cmd_report(qm_spec_property_t *property)
{
    qm_spec_property_t *property_pre = NULL;
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return QM_ERROR;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_VOLUME_SIID, CONFIG_AUDIO_VOLUME_PIID);
    qm_spec_property_pack_number(property_pre, get_var_volume_value());
    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(property_operation);
}

static qm_err_t chat_state_action(qm_spec_property_t *property)
{
    int number = 0;
    qm_spec_property_unpack_number(property, &number);
    wakeup_timerout_immediately_trigger(number);
    return QM_EOK;
}

extern uint8_t get_wakeup_timeout_flag(void);
static qm_err_t chat_state_report(qm_spec_property_t *property)
{   
    qm_spec_property_t *property_pre = NULL;
    qm_spec_property_operation_t *property_operation = qm_spec_property_operation_creat();
    if(property_operation == NULL){
        return -QM_ENOMEM;
    }
    property_pre = qm_spec_property_creat();
    if(property_pre == NULL){
        return QM_ERROR;
    }

    qm_spec_property_pack_xiid(property_pre, CONFIG_AUDIO_EXIT_CHAT_SIID, CONFIG_AUDIO_CHAT_STATE_PIID);
    qm_spec_property_pack_number(property_pre, get_wakeup_timeout_flag() + 1);
    qm_spec_property_add(property_operation, property_pre);
    return xjiang_cloud_multi_report(property_operation);
}

