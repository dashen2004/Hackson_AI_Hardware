#include "qm.h"
#include "qm_work.h"

#include "bsp_audio.h"
#include "audio_main.h"
#include "audio_cloud.h"
#include "xjiang_cloud.h"
#include "bsp_light.h"
#include "audio_light.h"


static qm_work_t reset_post = {0};

static bsp_audio_cmd_table_t cmd_table[] = 
{    //唤醒
    {BSP_AUDIO_WAKEUP_CMD,   CONFIG_AUDIO_ID_LIGHT_WAKEUP,    "ni hao xiao jian",   "你好小箭" , DEAFALUT_REPLY_WAV, 0.1},
    {BSP_AUDIO_WAKEUP_CMD,   CONFIG_AUDIO_ID_LIGHT_WAKEUP,    "xiao jian xiao jian","小箭小箭" , DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_NETCONFIG, "pei wang mo shi",    "配网模式" , DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_ON,        "kai deng",           "开灯"     , DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_ON,        "da kai deng guang",  "打开灯光" , DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_OFF,       "guan deng",          "关灯",     DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_OFF,       "guan bi deng guang", "关闭灯光", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_UP,        "liang yi dian",      "亮一点",   DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_UP,        "tiao liang yi dian", "调亮一点", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_DOWN,      "an yi dian",         "暗一点",   DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_DOWN,      "tiao an yi dian",    "调暗一点", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_MAX,      "zui da liang du",      "最大亮度", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_MIN,      "zui xiao liang du",    "最小亮度", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_MOD,      "zhong deng liang du",  "中等亮度", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_ALL,      "quan liang",           "全亮",    DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_NEUTER,   "zhong xing guang",     "中性光",  DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_NATURE,   "zi ran guang",         "自然光",  DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_SLEEP,    "shui jiao",            "睡觉",    DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_NIGHT,    "xiao ye deng",         "小夜灯",  DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_CHANGE,   "bian guang",           "变光",    DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_READ1,    "yue du mo shi",        "阅读模式", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_READ1,    "kan shu",              "看书",     DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_IDLE,     "xiu xian mo shi",      "休闲模式", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_TOP,      "shang guang",           "上光",    DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_LOWER,    "xia guang",             "下光",    DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_TIMER,    "ding shi mo shi",       "定时模式", DEAFALUT_REPLY_WAV, 0.1},
    // {BSP_AUDIO_COMM_CMD,     CONFIG_AUDIO_ID_LIGHT_RESET,    "hui fu chu chang she zhi", "恢复出厂设置", DEAFALUT_REPLY_WAV, 0.1},
};

static void audio_action_and_report(qm_spec_property_t *spec, uint8_t siid, uint16_t piid, int type, int value, uint8_t only_report)
{
    qm_spec_property_pack_xiid(spec, siid, piid);
    switch (type)
    {
        case QM_SPEC_PROPERTY_FORMAT_BOOL:
            qm_spec_property_pack_bool(spec, (bool_t)value);
            break;

        case QM_SPEC_PROPERTY_FORMAT_NUMBER:
            qm_spec_property_pack_number(spec, value);
            break;
        
        default:
            break;
    }

    if(only_report){
        audio_light_report(spec);
    }else{
        audio_light_action_and_report(spec);
    }
}

static void reset_action(void *arg)
{
    xjiang_cloud_reset();   
}

void audio_cloud_event_post(int audio_event)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();

    switch (audio_event)
    {
        case CONFIG_AUDIO_ID_LIGHT_NETCONFIG:
            qm_post_delayed_action(&reset_post, reset_action, NULL, CONFIG_NETWORK_DELAY_MS);
        break;
        case CONFIG_AUDIO_ID_LIGHT_ON:
            audio_action_and_report(&spec, 
                    CONFIG_AUDIO_LIGHT_ONOFF_SIID, CONFIG_AUDIO_LIGHT_ONOFF_PIID, CONFIG_AUDIO_LIGHT_ONOFF_TYPE, QM_TRUE, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_OFF:

            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_ONOFF_SIID, CONFIG_AUDIO_LIGHT_ONOFF_PIID, CONFIG_AUDIO_LIGHT_ONOFF_TYPE, QM_FALSE, QM_FALSE); 
        break;

        case CONFIG_AUDIO_ID_LIGHT_UP:                 //亮一点/调亮一点
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_UP, QM_FALSE); 
        break;

        case CONFIG_AUDIO_ID_LIGHT_DOWN:               //暗一点/调暗一点
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_DOWN, QM_FALSE); 
        
        break;

        case CONFIG_AUDIO_ID_LIGHT_MAX:                //最大亮度
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, CONFIG_BSP_LIGHT_MAX, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_MIN:                //最小亮度
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, CONFIG_BSP_LIGHT_MIN, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_MOD:                //中等亮度
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, 50, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_ALL:                //全亮
            bsp_light_all();
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_NEUTER:             //中性光
            bsp_light_all();
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_NATURE:             //自然光
            bsp_light_all();
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_SLEEP:              //睡觉
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_SLEEP, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_NIGHT:              //小夜灯
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_NIGHT, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_CHANGE:             //变光      
            bsp_light_switch1();

            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_SWITCH_SIID, CONFIG_AUDIO_LIGHT_SWITCH_PIID, CONFIG_AUDIO_LIGHT_SWITCH_TYPE, store->total_bri, QM_TRUE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_READ1:               //阅读模式
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_READ, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_READ2:               //看书

        break;

        case CONFIG_AUDIO_ID_LIGHT_IDLE:               //休闲模式
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_NIGHT, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_TOP:                //上光

            bsp_light_select(BSP_LIGHT_SEL_UP);
            bsp_light_bri_set(CONFIG_BSP_LIGHT_MAX);
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);

        break;

        case CONFIG_AUDIO_ID_LIGHT_LOWER:              //下光
            bsp_light_select(BSP_LIGHT_SEL_DOWN);
            bsp_light_bri_set(CONFIG_BSP_LIGHT_MAX);
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_TIMER:              //定时模式
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_DELAY_OFF_SIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_PIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_TYPE, 30, QM_FALSE);
        break;

        case CONFIG_AUDIO_ID_LIGHT_RESET:              //恢复出厂设置
            bsp_light_reset();
            xjiang_cloud_reset();
            bsp_light_store_update();
        break;
        
        default:
            break;
    }
}

qm_err_t audio_cloud_init(void)
{
    bsp_audio_init(cmd_table, QM_ARRAY_SIZE(cmd_table));

    return QM_EOK;
}
