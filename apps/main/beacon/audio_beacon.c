#include "qm.h"
#include "qm_work.h"
#include "qm_utils_timer.h"
#include "ble_remoter.h"

#include "bsp_light.h"

#include "audio_main.h"
#include "audio_light.h"

#include "xjiang_cloud.h"

static uint8_t power_on_flag = 0;
static qm_work_t post_work = {0};

static void pair_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void on_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void off_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void bri_up_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void bri_down_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void warm_up_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void warm_down_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void switch1_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void switch2_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void night_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void delay_off_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void reset_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);

static void bri_up_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void bri_up_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);

static void bri_down_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void bri_down_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);

static void warm_up_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void warm_up_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);

static void warm_down_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);
static void warm_down_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload);

static ble_remoter_cmd_table_t g_cmd_table[] = {
    {BLE_REMOTE_CMD_TYPE_PAIR, CONFIG_AUDIO_BEACON_PAIR,        pair_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_ON,          on_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_OFF,         off_recv},    

    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_BRI_UP,          bri_up_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_BRI_UP_HOLD,     bri_up_hold_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_BRI_UP_CANCEL,   bri_up_cancel_recv},

    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_BRI_DOWN,         bri_down_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_BRI_DOWN_HOLD,    bri_down_hold_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_BRI_DOWN_CANCEL,  bri_down_cancel_recv},

    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_WARM_UP,          warm_up_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_WARM_UP_HOLD,     warm_up_hold_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_WARM_UP_CANCEL,   warm_up_cancel_recv},
    
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_WARM_DOWN,        warm_down_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_WARM_DOWN_HOLD,   warm_down_hold_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_WARM_DOWN_CANCEL, warm_down_cancel_recv},

    
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_SWITCH1,     switch1_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_SWITCH2,     switch2_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_NIGHT,       night_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_DELAY_OFF,   delay_off_recv},
    {BLE_REMOTE_CMD_TYPE_COMM, CONFIG_AUDIO_BEACON_RESET,       reset_recv},
};

static void beacon_action_and_report(qm_spec_property_t *spec, uint8_t siid, uint16_t piid, int type, int value, uint8_t only_report)
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

static void pair_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    QM_LOGD("1", "pair_recv !!");
    bsp_light_blink();
}
//开灯
static void on_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};

    QM_LOGD("1", "on_recv !!");

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_ONOFF_SIID, CONFIG_AUDIO_LIGHT_ONOFF_PIID, CONFIG_AUDIO_LIGHT_ONOFF_TYPE, QM_TRUE, QM_FALSE);
}
//关灯
static void off_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};

    QM_LOGD("1", "off_recv !!");

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_ONOFF_SIID, CONFIG_AUDIO_LIGHT_ONOFF_PIID, CONFIG_AUDIO_LIGHT_ONOFF_TYPE, QM_FALSE, QM_FALSE); 

}
//亮度+ --> 亮度渐变:亮一点
static void bri_up_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "bri_up_recv !!");

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_UP, QM_FALSE); 

}
//亮度- --> 亮度渐变:暗一点
static void bri_down_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }
    
    QM_LOGD("1", "bri_up_recv !!");

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_DOWN, QM_FALSE); 

}
//上发光变亮
static void warm_up_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "warm_up_recv !!");

    bsp_light_warm_up();

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE); 
}
//下发光变亮
static void warm_down_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "warm_down_recv !!");
    
    bsp_light_warm_down();

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE); 
}
//变光
static void switch1_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "switch1_recv !!");
    
    bsp_light_switch1();

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_SWITCH_SIID, CONFIG_AUDIO_LIGHT_SWITCH_PIID, CONFIG_AUDIO_LIGHT_SWITCH_TYPE, store->total_bri, QM_TRUE);
}
//全/半亮
static void switch2_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "switch2_recv !!");
    
    bsp_light_switch2();

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_SWITCH_SIID, CONFIG_AUDIO_LIGHT_SWITCH_PIID, CONFIG_AUDIO_LIGHT_SWITCH_TYPE, store->total_bri, QM_TRUE);
}
//小夜灯
static void night_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "night_recv !!");

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_MODE_SIID, CONFIG_AUDIO_LIGHT_MODE_PIID, CONFIG_AUDIO_LIGHT_MODE_TYPE, BSP_LIGHT_MODE_NIGHT, QM_FALSE);
}
//延时关灯： 延时60s关灯
static void delay_off_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "delay_off_recv !!");

    beacon_action_and_report(&spec, 
        CONFIG_AUDIO_LIGHT_DELAY_OFF_SIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_PIID, CONFIG_AUDIO_LIGHT_DELAY_OFF_TYPE, 1, QM_FALSE);
}
//清码：通电5S后执行
static void post_action(void *arg)
{
    int evnet = (int)arg;
    qm_spec_property_t spec = {0};
    bsp_light_store_t *store = bsp_light_store_get();

    switch (evnet)
    {
        case CONFIG_AUDIO_BEACON_RESET:
            power_on_flag = QM_FALSE;
        break;

        case CONFIG_AUDIO_BEACON_BRI_UP_HOLD:
            beacon_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_UP, QM_FALSE); 
            qm_post_delayed_action(&post_work, post_action, (void *)evnet, CONFIG_AUDIO_BEACON_HOLD_TIME);
        break;

        case CONFIG_AUDIO_BEACON_BRI_DOWN_HOLD:
            beacon_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_GRAD_SIID, CONFIG_AUDIO_LIGHT_GRAD_PIID, CONFIG_AUDIO_LIGHT_GRAD_TYPE, AUDIO_LIGHT_GRAD_DOWN, QM_FALSE); 
        
            qm_post_delayed_action(&post_work, post_action, (void *)evnet, CONFIG_AUDIO_BEACON_HOLD_TIME);
        break;

        case CONFIG_AUDIO_BEACON_WARM_UP_HOLD:
            bsp_light_warm_up();

            beacon_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE); 
            qm_post_delayed_action(&post_work, post_action, (void *)evnet, CONFIG_AUDIO_BEACON_HOLD_TIME);
        break;

        case CONFIG_AUDIO_BEACON_WARM_DOWN_HOLD:
            bsp_light_warm_down();

            beacon_action_and_report(&spec, 
                CONFIG_AUDIO_LIGHT_BRI_SIID, CONFIG_AUDIO_LIGHT_BRI_PIID, CONFIG_AUDIO_LIGHT_BRI_TYPE, store->total_bri, QM_TRUE); 
            
                qm_post_delayed_action(&post_work, post_action, (void *)evnet, CONFIG_AUDIO_BEACON_HOLD_TIME);
        break;

        default:
            break;
    }
}

static void reset_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    if(!power_on_flag){
        return ;
    }

    QM_LOGD("1", "reset_recv !!");
        
    ble_remoter_reset();
    bsp_light_reset();
    xjiang_cloud_reset();
    bsp_light_store_update();

}

void audio_beacon_init(void)
{
    ble_remoter_params_t params = {

        .comm_param = {
            .fmsk = 0,
            .pid = 0,
            .subtype = 1,
        },
        
        .pair_param = {
            .enable = QM_TRUE,
            .pair_window = CONFIG_AUDIO_BEACON_POWER_ON_TIMEOUT,
        },
    };

    ble_remoter_init(&params, g_cmd_table, QM_ARRAY_SIZE(g_cmd_table));

    power_on_flag = QM_TRUE;
    qm_post_delayed_action(&post_work, post_action, (void *)CONFIG_AUDIO_BEACON_RESET, CONFIG_AUDIO_BEACON_POWER_ON_TIMEOUT);
}

static void bri_up_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "bri_up_hold_recv !!");
    qm_cancel_delayed_action(&post_work);
    qm_post_delayed_action(&post_work, post_action, (void *)CONFIG_AUDIO_BEACON_BRI_UP_HOLD, CONFIG_AUDIO_BEACON_HOLD_TIME);
}

static void bri_up_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "bri_up_cancel_recv !!");
    qm_cancel_delayed_action(&post_work);
}

static void bri_down_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "bri_down_hold_recv !!");
    qm_cancel_delayed_action(&post_work);
    qm_post_delayed_action(&post_work, post_action, (void *)CONFIG_AUDIO_BEACON_BRI_DOWN_HOLD, CONFIG_AUDIO_BEACON_HOLD_TIME);
}

static void bri_down_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "bri_down_cancel_recv !!");
    qm_cancel_delayed_action(&post_work);
}

static void warm_up_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "warm_up_hold_recv !!");
    qm_cancel_delayed_action(&post_work);
    qm_post_delayed_action(&post_work, post_action, (void *)CONFIG_AUDIO_BEACON_WARM_UP_HOLD, CONFIG_AUDIO_BEACON_HOLD_TIME);
}

static void warm_up_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "warm_up_cancel_recv !!");
    qm_cancel_delayed_action(&post_work);
}

static void warm_down_hold_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "warm_down_hold_recv !!");
    qm_cancel_delayed_action(&post_work);
    qm_post_delayed_action(&post_work, post_action, (void *)CONFIG_AUDIO_BEACON_WARM_DOWN_HOLD, CONFIG_AUDIO_BEACON_HOLD_TIME);
}

static void warm_down_cancel_recv(ble_remoter_cmd_type_t cmd_type, uint8_t opcode, ble_remoter_payload_t *payload)
{
    bsp_light_store_t *store = bsp_light_store_get();
    if(!store->on_off){
        return;
    }

    QM_LOGD("1", "warm_down_cancel_recv !!");
    qm_cancel_delayed_action(&post_work);
}


