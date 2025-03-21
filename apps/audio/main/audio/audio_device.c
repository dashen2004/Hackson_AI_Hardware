#include "qm.h"
#include "qm_audio.h"
#if 0
#include "bsp_board.h"
#include "bsp_light.h"

#include "audio_light.h"
#include "xjiang_cloud.h"
#include "ble_remoter.h"
#endif

#include "qm_work.h"

#include "audio_main.h"
#include "audio_device.h"




#define LOG_TAG "audio_dev"

#define CONFIG_NETWORK_DELAY_MS         (1500)
// #define CONFIG_NETWORK_URL       "ws://59.110.92.240:9097/voicePlatform/api/ws"
// #define CONFIG_NETWORK_URL       "ws://192.168.1.188:9096/voicePlatform/api/ws"

static qm_work_t reset_post = {0};
static qm_audio_t audio_handle = NULL;

#if 0
static qm_record_store_info_t g_record_cmd_table[] =
{
    //恢复出厂设置
    // {QM_RECORD_TYPE_REPLY_IND, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_IND,      QM_RECORD_VOICE_TYPE_MP3},
    //唤醒
    {QM_RECORD_TYPE_WAKEUP,  CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_WAKEUP,     QM_RECORD_VOICE_TYPE_MP3},
    //超时
    {QM_RECORD_TYPE_TIMEOUT, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_TIMEOUT,    QM_RECORD_VOICE_TYPE_MP3},
    //配网
    {QM_RECORD_TYPE_NETCONFIG,CONFIG_AUDIO_DEV_LOCAL_ROLE_ID, AUDIO_ID_LIGHT_NETCONFIG,  QM_RECORD_VOICE_TYPE_MP3},
    //开灯
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_ON,         QM_RECORD_VOICE_TYPE_MP3},
    //关灯
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_OFF,        QM_RECORD_VOICE_TYPE_MP3},
    //调亮一点
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_UP,         QM_RECORD_VOICE_TYPE_MP3},
    //调暗一点
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_DOWN,       QM_RECORD_VOICE_TYPE_MP3},
    //最大亮度
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_MAX,        QM_RECORD_VOICE_TYPE_MP3},
    //最小亮度
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_MIN,        QM_RECORD_VOICE_TYPE_MP3},
    //中等亮度
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_MOD,        QM_RECORD_VOICE_TYPE_MP3},
    //全亮
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_ALL,        QM_RECORD_VOICE_TYPE_MP3},
    //中性光
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_NEUTER,        QM_RECORD_VOICE_TYPE_MP3},
    //自然光
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_NEUTER,        QM_RECORD_VOICE_TYPE_MP3},
    //睡眠
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_SLEEP,      QM_RECORD_VOICE_TYPE_MP3},
    //小夜灯
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_NIGHT,      QM_RECORD_VOICE_TYPE_MP3},
    //变光
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_CHANGE,     QM_RECORD_VOICE_TYPE_MP3},
    //阅读模式
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_READ1,       QM_RECORD_VOICE_TYPE_MP3},
    //看书
    // {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_READ2,       QM_RECORD_VOICE_TYPE_MP3},

    //休闲模式
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_IDLE,       QM_RECORD_VOICE_TYPE_MP3},
    //上光
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_TOP,        QM_RECORD_VOICE_TYPE_MP3},
    //下光
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_LOWER,      QM_RECORD_VOICE_TYPE_MP3},
    //定时
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_TIMER,      QM_RECORD_VOICE_TYPE_MP3},
    //恢复出厂设置
    {QM_RECORD_TYPE_CONTROL, CONFIG_AUDIO_DEV_LOCAL_ROLE_ID,  AUDIO_ID_LIGHT_RESET,      QM_RECORD_VOICE_TYPE_MP3},
};

static qm_sr_cmd_t g_sr_cmd_table[] = 
{
    {QM_SR_CMD_WAKEUP, AUDIO_ID_LIGHT_WAKEUP,    "ni hao xiao jian",    "你好小箭"},
    {QM_SR_CMD_NETCONFIG,AUDIO_ID_LIGHT_NETCONFIG, "pei wang mo shi",    "配网模式"},
#if CONFIG_QM_AUDIO_OFFLINE_CMD_SUPPORT
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_ON,        "kai deng",           "开灯"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_ON,        "da kai deng guang",  "打开灯光"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_OFF,       "guan deng",          "关灯"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_OFF,       "guan bi deng guang", "关闭灯光"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_UP,        "liang yi dian",      "亮一点"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_UP,        "tiao liang yi dian", "调亮一点"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_DOWN,      "an yi dian",         "暗一点"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_DOWN,      "tiao an yi dian",    "调暗一点"},

    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_MAX,      "zui da liang du",      "最大亮度"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_MIN,      "zui xiao liang du",    "最小亮度"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_MOD,      "zhong deng liang du",  "中等亮度"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_ALL,      "quan liang",           "全亮"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_NEUTER,   "zhong xing guang",     "中性光"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_NATURE,   "zi ran guang",         "自然光"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_SLEEP,    "shui jiao",            "睡觉"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_NIGHT,    "xiao ye deng",         "小夜灯"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_CHANGE,   "bian guang",           "变光"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_READ1,    "yue du mo shi",        "阅读模式"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_READ1,    "kan shu",              "看书"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_IDLE,     "xiu xian mo shi",      "休闲模式"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_TOP,      "shang guang",           "上光"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_LOWER,    "xia guang",             "下光"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_TIMER,    "ding shi mo shi",       "定时模式"},
    {QM_SR_CMD_SR,     AUDIO_ID_LIGHT_RESET,    "hui fu chu chang she zhi", "恢复出厂设置"},
#endif
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

static void qm_audio_online_notify(qm_audio_t handle, void *notify, void *arg)
{
    qm_audio_online_cmd_notify_t *online_notify = (qm_audio_online_cmd_notify_t *)notify;
    audio_light_action_and_report(online_notify->property);
}

static void qm_audio_offline_notify(qm_audio_t handle, void *notify, void *arg)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    qm_audio_offline_cmd_notify_t *offline_notify = (qm_audio_offline_cmd_notify_t *)notify;

    switch (offline_notify->sr_id)
    {
        case AUDIO_ID_LIGHT_ON:
            audio_action_and_report(&spec, 
                    CONFIG_AUDIO_LIGHT_ONOFF_SIID, CONFIG_AUDIO_LIGHT_ONOFF_PIID, CONFIG_AUDIO_LIGHT_ONOFF_TYPE, QM_TRUE, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_OFF:

            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_ONOFF_SIID, CONFIG_AUDIO_LIGHT_ONOFF_PIID, CONFIG_AUDIO_LIGHT_ONOFF_TYPE, QM_FALSE, QM_FALSE); 
        break;

        case AUDIO_ID_LIGHT_UP:                 //亮一点/调亮一点
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_UP, QM_FALSE); 
        break;

        case AUDIO_ID_LIGHT_DOWN:               //暗一点/调暗一点
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_DOWN, QM_FALSE); 
        
        break;

        case AUDIO_ID_LIGHT_MAX:                //最大亮度
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, CONFIG_BSP_LIGHT_MAX, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_MIN:                //最小亮度
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, CONFIG_BSP_LIGHT_MIN, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_MOD:                //中等亮度
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, 50, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_ALL:                //全亮
            bsp_light_all();
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case AUDIO_ID_LIGHT_NEUTER:             //中性光
            bsp_light_all();
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case AUDIO_ID_LIGHT_NATURE:             //自然光
            bsp_light_all();
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case AUDIO_ID_LIGHT_SLEEP:              //睡觉
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_SLEEP, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_NIGHT:              //小夜灯
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_NIGHT, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_CHANGE:             //变光      
            bsp_light_switch1();

            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_SWITCH_SIID, CONFIG_AUDIO_LIGHT_SWITCH_PIID, CONFIG_AUDIO_LIGHT_SWITCH_TYPE, store->total_bri, QM_TRUE);
        break;

        case AUDIO_ID_LIGHT_READ1:               //阅读模式
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_READ, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_READ2:               //看书

        break;

        case AUDIO_ID_LIGHT_IDLE:               //休闲模式
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_NIGHT, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_TOP:                //上光

            bsp_light_select(BSP_LIGHT_SEL_UP);
            bsp_light_bri_set(CONFIG_BSP_LIGHT_MAX);
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);

        break;

        case AUDIO_ID_LIGHT_LOWER:              //下光
            bsp_light_select(BSP_LIGHT_SEL_DOWN);
            bsp_light_bri_set(CONFIG_BSP_LIGHT_MAX);
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE);
        break;

        case AUDIO_ID_LIGHT_TIMER:              //定时模式
            audio_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_DELAY_OFF_SIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_PIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_TYPE, 30, QM_FALSE);
        break;

        case AUDIO_ID_LIGHT_RESET:              //恢复出厂设置
            ble_remoter_reset();
            bsp_light_reset();
            xjiang_cloud_reset();
            bsp_light_store_update();
        break;
        
        default:
            break;
    }
}

static void qm_audio_recv_handler(qm_audio_t handle, qm_audio_recv_t *packet, void *arg)
{
    switch (packet->recv_type)
    {
        case QM_AUDIO_RECV_NETCONFIG_NOTIFY:
            qm_post_delayed_action(&reset_post, reset_action, NULL, CONFIG_NETWORK_DELAY_MS);
        break;
    
        case QM_AUDIO_RECV_ONLINE_CMD_NOTIFY:
            qm_audio_online_notify(handle, &packet->data.online_cmd_notify, arg);
        break;

        case QM_AUDIO_RECV_OFFLINE_CMD_NOTIFY:
            qm_audio_offline_notify(handle, &packet->data.offline_cmd_notify, arg);
        break;

        default:

        break;
    }
}
#endif

static qm_audio_cmd_table_t g_record_cmd_table[] =
{
    {QM_AUDIO_REC_CMD_WAKEUP,    AUDIO_ID_LIGHT_WAKEUP,     "ni hao xiao jian",    "你好小箭", NULL},
    {QM_AUDIO_REC_CMD_WAKEUP,    AUDIO_ID_LIGHT_WAKEUP2,    "xiao jian xiao jian", "小箭小箭", NULL},
    {QM_AUDIO_REC_CMD_NETCONFIG, AUDIO_ID_LIGHT_NETCONFIG,  "pei wang mo shi",     "配网模式", NULL},
};


qm_err_t audio_device_init(void)
{
    qm_audio_cfg_t cfg = {
        .mic_num = 1,

        .pid = CONFIG_XJIANG_CLOUD_PID,
        .product_secret = CONFIG_XJIANG_CLOUD_SECERT,
        
        .timeout = CONFIG_AUDIO_DEVICE_TIMEOUT,
        
        .enable_prov = QM_TRUE,
        .enable_wakeup = QM_TRUE,
        
        .appid = DEFAULT_APPID,
        .room = DEFAULT_ROOMID,
        .uid = DEFAULT_UID,
        .token = DEFAULT_TOKEN,

        .table = g_record_cmd_table,
        .table_num = QM_ARRAY_SIZE(g_record_cmd_table),
    };

    audio_handle = qm_audio_open(&cfg);
    if(audio_handle == NULL){
        return -QM_EIO;
    }
    qm_audio_start(audio_handle);

    return QM_EOK;
#if 0
    qm_sr_cmd_info_t sr_info = {0};

    qm_record_store_t record_info = {0};
    uint32_t pid = CONFIG_XJIANG_CLOUD_PID;

    uint32_t default_id = CONFIG_AUDIO_DEV_LOCAL_ROLE_ID;
    int enable_wakeup = QM_TRUE;
    int enable_welcome = QM_FALSE;
    uint32_t sr_timeout = 10 * 1000;

    qm_audio_t audio_handle = qm_audio_init();

    if(audio_handle == NULL){
        return -QM_ERROR;
    }

    record_info.info = g_record_cmd_table;
    record_info.num = QM_ARRAY_SIZE(g_record_cmd_table);

    sr_info.sr_cmd = g_sr_cmd_table;
    sr_info.cmd_num = QM_ARRAY_SIZE(g_sr_cmd_table);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_SR_CMD, &sr_info);

    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_PRODUCT_ID, &pid);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_URL, CONFIG_NETWORK_URL);

    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_I2S_READ, bsp_i2s_read);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_STORE_INFO, &record_info);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_DEFAULT_ROLE, &default_id);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_SR_TIMEOUT, &sr_timeout);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_ENABLE_WAKEUP, &enable_wakeup);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_ENABLE_WELCOME, &enable_welcome);
    // qm_audio_setopt(audio_handle, QM_AUDIO_OPT_ENABLE_REPLY_IND, &enable_wakeup);
    qm_audio_setopt(audio_handle, QM_AUDIO_OPT_RECV_HANDLER, qm_audio_recv_handler);

    return qm_audio_start(audio_handle);
#endif
}