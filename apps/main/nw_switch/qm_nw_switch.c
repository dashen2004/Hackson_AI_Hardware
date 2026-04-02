#include "qm_iot_api.h"
#include "qm_iot_config.h"
#include "qm_nw_switch.h"
#include "qm_work.h"

#include "qm_kv.h"

#define LOG_TAG "NW SWITCH"

typedef struct 
{
    uint8_t siid;
    uint16_t piid;
    void (*action)(qm_spec_property_t *property);
} nw_cloud_set_table_t;

typedef struct {
    uint32_t did;
    qm_iot_net_type_t net_type;
    void *iot_handle;
    qm_mutex_t lock;
    qm_work_t post_wifi_disconn;
    qm_iot_wifi_info_t wifi_info;
} qm_nw_switch_t;

static qm_nw_switch_t nw_switch;

extern void qm_example_ai_init();

static void wifi_disconn_acion(void *arg);
static void wifi_ssid_action(qm_spec_property_t *property);
static void wifi_pwd_action(qm_spec_property_t *property);
static void net_switch_action(qm_spec_property_t *property);
static qm_err_t respose_network_report(network_report_t report);

static nw_cloud_set_table_t set_table[] = 
{    
    {CONFIG_NETWORK_SIID, CONFIG_WIFI_SSID_PIID , wifi_ssid_action},

    {CONFIG_NETWORK_SIID, CONFIG_WIFI_PWD_PIID , wifi_pwd_action},

    {CONFIG_NETWORK_SIID, CONFIG_NET_SWITCH_PIID , net_switch_action},
};

// 物模型解析
qm_err_t bsp_nw_cmd_action(qm_spec_property_t *spec_prop)
{
    qm_mutex_lock(&nw_switch.lock, QM_WAIT_FOREVER);
    for (int i = 0; i < QM_ARRAY_SIZE(set_table); i++)
    {
        if(set_table[i].siid == spec_prop->siid &&set_table[i].piid == spec_prop->piid){
            if(set_table[i].action){
                set_table[i].action(spec_prop);
            }
            break;
        }
    }
    qm_mutex_unlock(&nw_switch.lock);
    return QM_EOK;
}

// 向平台以report方式回复消息
static qm_err_t respose_network_report(network_report_t report)
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

    property_pre->did = nw_switch.did;
    
    switch (report) {
        case QM_MW_REPORT_SSID_RSP:
            qm_spec_property_pack_xiid(property_pre, CONFIG_NETWORK_SIID, CONFIG_WIFI_SSID_PIID);
            qm_spec_property_pack_string(property_pre, nw_switch.wifi_info.ssid, strlen(nw_switch.wifi_info.ssid));
        break;
        case QM_MW_REPORT_PWD_RSP:
            qm_spec_property_pack_xiid(property_pre, CONFIG_NETWORK_SIID, CONFIG_WIFI_PWD_PIID);
            qm_spec_property_pack_string(property_pre, nw_switch.wifi_info.pwd, strlen(nw_switch.wifi_info.pwd));
        break;
        case QM_NW_REPORT_NET_SWITCH:
            qm_spec_property_pack_xiid(property_pre, CONFIG_NETWORK_SIID, CONFIG_NET_SWITCH_PIID);
            qm_spec_property_pack_number(property_pre, 2);
        break;
        default:
        break;
    }

    qm_spec_property_add(property_operation, property_pre);
    return qm_iot_property_report(nw_switch.iot_handle, property_operation);
}

// wifi断连
static void wifi_disconn_acion(void *arg)
{
    // 切换为4G连接
    nw_switch.net_type = QM_IOT_NET_TYPE_4G;
    qm_kv_set("net_type", &nw_switch.net_type, sizeof(qm_iot_net_type_t), 1);
    qm_reboot();
}

// 获取wifi ssid
static void wifi_ssid_action(qm_spec_property_t *property)
{
    int len = 0;
    if(property == NULL) {
        return ;
    }
    
    qm_spec_property_unpack_string(property, nw_switch.wifi_info.ssid, &len);
    QM_LOGD(LOG_TAG, "get wifi ssid from cloud: %s\n", nw_switch.wifi_info.ssid);
    // 回复平台wifi账号
    respose_network_report(QM_MW_REPORT_SSID_RSP);
}

// 获取wifi pwd
static void wifi_pwd_action(qm_spec_property_t *property)
{
    int len = 0;
    if(property == NULL) {
        return ;
    }
    
    qm_spec_property_unpack_string(property, nw_switch.wifi_info.pwd, &len);
    QM_LOGD(LOG_TAG, "get wifi pwd from cloud: %s\n", nw_switch.wifi_info.pwd);
    // 回复平台wifi密码
    respose_network_report(QM_MW_REPORT_PWD_RSP);
    // 保存wifi信息至本地
    qm_iot_wifi_info_set(&nw_switch.wifi_info);
}

// 接收切换WIFI信息
static void net_switch_action(qm_spec_property_t *property)
{
    if (property == NULL) {
        return ;
    }

    // 回复平台切换消息
    respose_network_report(QM_NW_REPORT_NET_SWITCH);

    if (nw_switch.net_type == QM_IOT_NET_TYPE_WIFI_COMBO) {
        QM_LOGD(LOG_TAG, "change nettype to 4G");
        nw_switch.net_type = QM_IOT_NET_TYPE_4G;
    } else {
        QM_LOGD(LOG_TAG, "change nettype to WIFI");
        nw_switch.net_type = QM_IOT_NET_TYPE_WIFI_COMBO;
    }

    // 更新本地联网方式信息并重启
    qm_kv_set("net_type", &nw_switch.net_type, sizeof(qm_iot_net_type_t), 1);
    qm_reboot();
    // extern qm_err_t bsp_bat_enable_4g(void);
    // bsp_bat_enable_4g();
    // qm_iot_change_net_type(nw_switch.iot_handle, nw_switch.net_type);
}

// 网络切换初始化
int qm_nw_switch_init(void *iot_handle)
{
    int kv_len = 1;
    if (iot_handle == NULL) {
        return QM_ENOMEM;
    }

    // 获取设备的连网方式
    qm_kv_get("net_type", &nw_switch.net_type, &kv_len);
    if (nw_switch.net_type != QM_IOT_NET_TYPE_WIFI_COMBO && nw_switch.net_type != QM_IOT_NET_TYPE_4G){
        // 如果不是4G或WIFI则默认WIFI
        nw_switch.net_type = QM_IOT_NET_TYPE_WIFI_COMBO;
        qm_kv_set("net_type", &nw_switch.net_type, sizeof(qm_iot_net_type_t), 1);
    }

    nw_switch.iot_handle = iot_handle;
    nw_switch.did = qm_iot_did_get();
    
    qm_mutex_new(&nw_switch.lock);
    // qm_iot_setopt(iot_handle, QM_IOT_OPT_EVENT_HANDLER, qm_iot_event_handler);

    extern qm_err_t qm_nw_switch_wifi(void);
    qm_nw_switch_wifi();

    return QM_EOK;
}

qm_err_t qm_nw_switch_4g(void)
{
    nw_switch.net_type = QM_IOT_NET_TYPE_4G;
    qm_kv_set("net_type", &nw_switch.net_type, sizeof(qm_iot_net_type_t), 1);
    qm_reboot();

    return QM_EOK;
}

qm_err_t qm_nw_switch_wifi(void)
{
    nw_switch.net_type = QM_IOT_NET_TYPE_WIFI_COMBO;
    qm_kv_set("net_type", &nw_switch.net_type, sizeof(qm_iot_net_type_t), 1);

    return QM_EOK;
}

int qm_nw_get_net_type(void)
{
    return nw_switch.net_type;
}
