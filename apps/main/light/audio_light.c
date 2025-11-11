
#include "qm.h"
#include "qm_work.h"
#include "vars.h"

#include "audio_main.h"
#include "audio_light.h"
#include "screens.h"

#include "bsp_light.h"
#include "bsp_audio.h"

#include "xjiang_cloud.h"
#include "qm_agora_gen.h"

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
static qm_err_t grad_aciton(qm_spec_property_t *property);
static qm_err_t cct_action(qm_spec_property_t *property);
static qm_err_t delay_off_aciton(qm_spec_property_t *property);
static qm_err_t reset_aciton(qm_spec_property_t *property);
static qm_err_t volume_set_action(qm_spec_property_t *property);
static qm_err_t volume_mute_action(qm_spec_property_t *property);
static qm_err_t volume_cmd_action(qm_spec_property_t *property);
static qm_err_t chat_state_action(qm_spec_property_t *property);
static qm_err_t voice_corpus_action(qm_spec_property_t *property);
static qm_err_t voice_abort_action(qm_spec_property_t *property);
static qm_err_t voice_emotion_action(qm_spec_property_t *property);
static qm_err_t rtc_login_info_uid(qm_spec_property_t *property);
static qm_err_t rtc_login_info_appid(qm_spec_property_t *property);
static qm_err_t rtc_login_info_channel_name(qm_spec_property_t *property);
static qm_err_t rtc_login_info_token(qm_spec_property_t *property);
static qm_err_t rtc_login_info_license_value(qm_spec_property_t *property);
static qm_err_t rgb_onoff_aciton(qm_spec_property_t *property);
static qm_err_t rgb_value_aciton(qm_spec_property_t *property);

static qm_err_t onoff_report(qm_spec_property_t *property);
static qm_err_t bri_report(qm_spec_property_t *property);
static qm_err_t mode_report(qm_spec_property_t *property);
static qm_err_t switch_report(qm_spec_property_t *property);
static qm_err_t grad_report(qm_spec_property_t *property);
static qm_err_t cct_report(qm_spec_property_t *property);
static qm_err_t delay_off_report(qm_spec_property_t *property);
static qm_err_t reset_report(qm_spec_property_t *property);
static qm_err_t volume_set_report(qm_spec_property_t *property);
static qm_err_t volume_mute_report(qm_spec_property_t *property);
static qm_err_t volume_cmd_report(qm_spec_property_t *property);
static qm_err_t chat_state_report(qm_spec_property_t *property);
static qm_err_t voice_corpus_report(qm_spec_property_t *property);
static qm_err_t voice_abort_report(qm_spec_property_t *property);
static qm_err_t voice_emotion_report(qm_spec_property_t *property);
static qm_err_t rtc_login_info_uid_report(qm_spec_property_t *property);
static qm_err_t rtc_login_info_appid_report(qm_spec_property_t *property);
static qm_err_t rtc_login_info_channel_name_report(qm_spec_property_t *property);
static qm_err_t rtc_login_info_token_report(qm_spec_property_t *property);
static qm_err_t rtc_login_info_license_value_report(qm_spec_property_t *property);
static qm_err_t rgb_onoff_report(qm_spec_property_t *property);
static qm_err_t rgb_value_report(qm_spec_property_t *property);

static audio_light_table_t g_light_table[] =
{
    {CONFIG_AUDIO_LIGHT_ONOFF_SIID,     CONFIG_AUDIO_LIGHT_ONOFF_PIID,      onoff_aciton,  onoff_report},
    {CONFIG_AUDIO_LIGHT_BRI_SIID,       CONFIG_AUDIO_LIGHT_BRI_PIID,        bri_aciton, bri_report},
    {CONFIG_AUDIO_LIGHT_MODE_SIID,      CONFIG_AUDIO_LIGHT_MODE_PIID,       mode_aciton,  mode_report},
    {CONFIG_AUDIO_LIGHT_SWITCH_SIID,    CONFIG_AUDIO_LIGHT_SWITCH_PIID,     switch_aciton, switch_report},
    {CONFIG_AUDIO_LIGHT_GRAD_SIID,      CONFIG_AUDIO_LIGHT_GRAD_PIID,       grad_aciton,  grad_report},
    {CONFIG_AUDIO_LIGHT_CCT_SIID,       CONFIG_AUDIO_LIGHT_CCT_PIID,        cct_action, cct_report},
    {CONFIG_AUDIO_LIGHT_DELAY_OFF_SIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_PIID,  delay_off_aciton,  delay_off_report},
    {CONFIG_AUDIO_LIGHT_RESET_SIID,     CONFIG_AUDIO_LIGHT_RESET_PIID,      reset_aciton,  reset_report},
    {CONFIG_AUDIO_VOLUME_SIID,          CONFIG_AUDIO_VOLUME_PIID,           volume_set_action,  volume_set_report},
    {CONFIG_AUDIO_VOLUME_SIID,          CONFIG_AUDIO_VOLUME_CMD_PIID,       volume_cmd_action,  volume_cmd_report},
    {CONFIG_AUDIO_VOLUME_SIID,          CONFIG_AUDIO_VOLUME_MUTE_PIID,      volume_mute_action,  volume_mute_report},
    {CONFIG_AUDIO_EXIT_CHAT_SIID,       CONFIG_AUDIO_CHAT_STATE_PIID,       chat_state_action,  chat_state_report},
    {CONFIG_AUDIO_VOICE_CONTROL_SIID,   CONFIG_AUDIO_VOICE_CORPUS_PIID,     voice_corpus_action,voice_corpus_report},
    {CONFIG_AUDIO_VOICE_CONTROL_SIID,   CONFIG_AUDIO_VOICE_ABORT_PIID,      voice_abort_action,  voice_abort_report},
    {CONFIG_AUDIO_VOICE_CONTROL_SIID,   CONFIG_AUDIO_VOICE_EMOTION_PIID,    voice_emotion_action,  voice_emotion_report},
    //RTC登录信息UID
    {CONFIG_AUDIO_RTC_LOGIN_SIID,       CONFIG_AUDIO_RTC_UID_PIID,          rtc_login_info_uid, rtc_login_info_uid_report},
    //RTC登录信息APPID
    {CONFIG_AUDIO_RTC_LOGIN_SIID,       CONFIG_AUDIO_RTC_APPID_PIID,        rtc_login_info_appid, rtc_login_info_appid_report},
    //RTC登录信息channal name
    {CONFIG_AUDIO_RTC_LOGIN_SIID,       CONFIG_AUDIO_RTC_CHANNAL_NAME_PIID, rtc_login_info_channel_name, rtc_login_info_channel_name_report},
    //RTC登录信息token
    {CONFIG_AUDIO_RTC_LOGIN_SIID,       CONFIG_AUDIO_RTC_TOKEN_PIID,        rtc_login_info_token, rtc_login_info_token_report},
#if CONFIG_AGORA_USE_LICENSE
    //RTC登录信息license value
    {CONFIG_AUDIO_RTC_LOGIN_SIID,       CONFIG_AUDIO_RTC_LICENSE_VALUE_PIID, rtc_login_info_license_value, rtc_login_info_license_value_report},
#endif
    {CONFIG_AUDIO_RGB_ONOFF_SIID,       CONFIG_AUDIO_RGB_ONOFF_PIID,        rgb_onoff_aciton,  rgb_onoff_report},
    {CONFIG_AUDIO_RGB_VALUE_SIID,       CONFIG_AUDIO_RGB_VALUE_PIID,        rgb_value_aciton,  rgb_value_report},
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
        CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, (int)(store->brightness));
//模式
    audio_light_add(property_operation, 
        CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, (int)(store->mode));
//cct
    audio_light_add(property_operation, 
        CONFIG_AUDIO_LIGHT_CCT_SIID, CONFIG_AUDIO_LIGHT_CCT_PIID, CONFIG_AUDIO_LIGHT_CCT_TYPE, (int)(store->cct));

//rgb on_off
    audio_light_add(property_operation, 
        CONFIG_AUDIO_RGB_ONOFF_SIID, CONFIG_AUDIO_RGB_ONOFF_PIID, CONFIG_AUDIO_RGB_ONOFF_TYPE, (int)(store->rgb_on_off));

//rgb value
    audio_light_add(property_operation, 
        CONFIG_AUDIO_RGB_VALUE_SIID, CONFIG_AUDIO_RGB_VALUE_PIID, CONFIG_AUDIO_RGB_VALUE_TYPE, (int)(store->rgb_val));

    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_AWS_REPORT, property_operation);
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
    qm_msleep(600);
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

// cct
static qm_err_t cct_action(qm_spec_property_t *property)
{
    int cct = 0;
    qm_spec_property_unpack_number(property, &cct);

    bsp_light_cct_set(cct);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t cct_report(qm_spec_property_t *property)
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

//定时关灯
static qm_err_t delay_off_report(qm_spec_property_t *property)
{
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
    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_AWS_REPORT, property_operation);
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
    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_AWS_REPORT, property_operation);
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
    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_AWS_REPORT, property_operation);
}

static qm_err_t chat_state_action(qm_spec_property_t *property)
{
    int number = 0;
    qm_spec_property_unpack_number(property, &number);
    wakeup_quit_immediately_trigger(number);
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
    return xjiang_cloud_multi_report(QM_IOT_CORE_MSG_TYPE_AWS_REPORT, property_operation);
}

static qm_err_t voice_corpus_action(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t voice_corpus_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t voice_abort_action(qm_spec_property_t *property)
{
    int number = 0;
    qm_spec_property_unpack_number(property, &number);

    return QM_EOK;
}

static qm_err_t voice_abort_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

extern void bsp_audio_llm_emotion_set(char *emotion);
static qm_err_t voice_emotion_action(qm_spec_property_t *property)
{
    char data[64] = {0};
    int len = 0;
    if(property == NULL) {
        return QM_ERROR;
    }
    
    qm_spec_property_unpack_string(property, data, &len);
    bsp_audio_llm_emotion_set(data);
    return QM_EOK;
}

static qm_err_t voice_emotion_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t rtc_login_info_uid(qm_spec_property_t *property)
{
    int value = 0;
    if(property == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_number(property, &value);
    qm_agora_gen_set_data(QM_AGORA_GEN_TYPE_UID, &value, 0);
    return QM_EOK;
}

static qm_err_t rtc_login_info_uid_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t rtc_login_info_appid(qm_spec_property_t *property)
{
    char *data = NULL;
    int len = 0;
    if(property == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, NULL, &len);
    if(len == 0) {
        return QM_ERROR;
    }
    data = (char *)qm_malloc(len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, data, &len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_agora_gen_set_data(QM_AGORA_GEN_TYPE_APPID, data, len - 1);
    return QM_EOK;
}

static qm_err_t rtc_login_info_appid_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t rtc_login_info_channel_name(qm_spec_property_t *property)
{
    char *data = NULL;
    int len = 0;
    if(property == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, NULL, &len);
    if(len == 0) {
        return QM_ERROR;
    }
    data = (char *)qm_malloc(len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, data, &len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_agora_gen_set_data(QM_AGORA_GEN_TYPE_CHANNAL_NAME, data, len - 1);
    return QM_EOK;
}

static qm_err_t rtc_login_info_channel_name_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t rtc_login_info_token(qm_spec_property_t *property)
{
    char *data = NULL;
    int len = 0;
    if(property == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, NULL, &len);
    if(len == 0) {
        return QM_ERROR;
    }
    data = (char *)qm_malloc(len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, data, &len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_agora_gen_set_data(QM_AGORA_GEN_TYPE_TOKEN, data, len - 1);
    return QM_EOK;
}

static qm_err_t rtc_login_info_token_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

static qm_err_t rtc_login_info_license_value(qm_spec_property_t *property)
{
    char *data = NULL;
    int len = 0;
    if(property == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, NULL, &len);
    if(len == 0) {
        return QM_ERROR;
    }
    data = (char *)qm_malloc(len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, data, &len);
    if(data == NULL) {
        return QM_ERROR;
    }
    qm_agora_gen_set_data(QM_AGORA_GEN_TYPE_LICENSE_VALUE, data, len - 1);
    return QM_EOK;
}

static qm_err_t rtc_login_info_license_value_report(qm_spec_property_t *property)
{
    return QM_EOK;
}

// rgb on off
static qm_err_t rgb_onoff_aciton(qm_spec_property_t *property)
{
    qm_msleep(600);
    bool_t onoff = QM_FALSE;
    qm_spec_property_unpack_bool(property, &onoff);

    bsp_light_rgb_on_off(onoff);

    bsp_light_store_update();

    return QM_EOK;
}

static qm_err_t rgb_onoff_report(qm_spec_property_t *property)
{
   return audio_light_snapshot_report();
}

// rgb value
static qm_err_t rgb_value_aciton(qm_spec_property_t *property)
{
    char *data = NULL;
    int len = 0;
    if (property == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, NULL, &len);
    if (len == 0) {
        return QM_ERROR;
    }
    data = (char *)qm_malloc(len);
    if (data == NULL) {
        return QM_ERROR;
    }
    qm_spec_property_unpack_string(property, data, &len);
    if (data == NULL) {
        return QM_ERROR;
    }

    uint32_t rgb_val = 0;

    if (data[0] == 'r' && data[1] == 'g' && data[2] == 'b') {
        uint16_t split_offset_1 = 0, split_offset_2 = 0;

        for (int32_t i = 0; data[i] != '\0'; i++) {
            if (data[i] == ',') {
                if (split_offset_1 == 0) {
                    split_offset_1 = i;
                } else {
                    split_offset_2 = i;
                }
            }
        }

        char *red_start = &data[4], *red_end = &data[split_offset_1 - 1];
        char *green_start = &data[split_offset_1 + 2], *green_end = &data[split_offset_2 - 1];
        char *blue_start = &data[split_offset_2 + 2], *blue_end = &data[strlen(data) - 2];

        uint32_t red_value = strtod(red_start, &red_end);
        uint32_t green_value = strtod(green_start, &green_end);
        uint32_t blue_value = strtod(blue_start, &blue_end);

        rgb_val = (red_value << 16) | (green_value << 8) | blue_value;
    }

    bsp_light_rgb_select(BSP_RGB_SELECT_COLORFUL);
    bsp_light_rgb_value_set(rgb_val);

    bsp_light_store_update();

    tick_screen_page_rgb_value();

    return QM_EOK;
}

static qm_err_t rgb_value_report(qm_spec_property_t *property)
{
    return audio_light_snapshot_report();
}
