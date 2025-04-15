#include "qm.h"
#include "qm_audio_prov.h"
#include "qm_utils_string.h"

#include "qm_ble_gap.h"
#include "qm_ble_core.h"
#include "qm_ble_export.h"
#include "qm_ble_common.h"
#include "qm_work.h"
#include "qm_spec_api.h"
#include "qm_spec_core.h"
#include "qm_spec_service.h"

#define LOG_TAG "PROV"

#define PROV_HANDLE_LIST    (2)

#define PRODUCT_ID_LEN      (32)
#define PRODUCT_SECRET_LEN  (32)
#define QM_BLE_ADV_NAME "xj-%u-%02x%02x"
#define QM_IOT_PROV_DEINIT_DELAY_MS     (500)

typedef struct 
{
    uint32_t pid;
    char *product_secret;
    uint8_t ble_connect;
    uint8_t need_deinit;
    void *userdata;
    char local_name[32];
    qm_audio_prov_recv_handler_t recv_handler;
    qm_work_t deinit_work;
}qm_audio_prov_t;

static void qm_apinfo_ready_cb(uint8_t *data, uint32_t len);
static void qm_ble_status_change_cb(qm_ble_event_t event);
static void qm_extinfo_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t len);
static void qm_ble_response_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t length);
static void qm_ble_set_dev_status_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t length);
static void qm_ble_get_dev_status_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t length);

static qm_audio_prov_t *g_ble_prov_handle = NULL;

static void qm_ble_status_change_cb(qm_ble_event_t event)
{
    if(g_ble_prov_handle == NULL){
        return ;
    }

    switch (event) {
        case QM_BLE_CONNECTED:
            QM_BLE_INFO("QM_BLE_CONNECTED");
            g_ble_prov_handle->ble_connect = QM_TRUE;
		break;
        case QM_BLE_DISCONNECTED:

            QM_BLE_INFO("QM_BLE_DISCONNECTED");
            g_ble_prov_handle->ble_connect = QM_FALSE;

            if(!g_ble_prov_handle->need_deinit){
                qm_ble_start_advertising(3, 1, 0, g_ble_prov_handle->local_name);
            }else{
                
                g_ble_prov_handle->need_deinit = QM_FALSE;

                qm_cancel_delayed_action(&g_ble_prov_handle->deinit_work);

                qm_ble_end();

                qm_free(g_ble_prov_handle);
                g_ble_prov_handle = NULL;
            }

        break;
        case QM_BLE_AUTHENTICATED:
            QM_BLE_INFO("QM_BLE_AUTHENTICATED");
            break;
        default:
        break;
    }
}

static void qm_ble_set_dev_status_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t length)
{

}

static void qm_ble_get_dev_status_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t length)
{

}

static void qm_ble_response_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t length)
{
 
}

static void qm_extinfo_callback(uint8_t msg_id, uint8_t cmd, uint8_t *buffer, uint32_t len)
{

}

static void qm_apinfo_ready_cb(uint8_t *data, uint32_t len)
{
    int string_len = 0;
    char *string_value = NULL;
    qm_spec_property_t *property = NULL;
    qm_audio_prov_recv_t prov_recv = {0};
    qm_spec_property_operation_t *operation = qm_spec_property_operation_creat();
    if(operation == NULL){
        return;
    }
    QM_HEX_LOGD(LOG_TAG, "apinfo_ready ", data, len);
    qm_spec_standard_service_unpack(QM_SPEC_DATA_TYPE_HEX, operation, data, len);

    property = qm_spec_property_find(operation, QM_SPEC_WIFI_SSID_SERVICE_SIID, QM_SPEC_WIFI_SSID_SERVICE_PIID);
    if(property == NULL){
        return;
    }
    qm_spec_property_unpack_string_direct(property, &string_value, &string_len);
    prov_recv.ssid = string_value;
    QM_LOGD(LOG_TAG, "pro siid %d piid %d, payload %s", property->siid, property->piid, prov_recv.ssid);

    property = qm_spec_property_find(operation, QM_SPEC_WIFI_PASSWORD_SERVICE_SIID, QM_SPEC_WIFI_PASSWORD_SERVICE_PIID);
    if(property == NULL){
        return;
    }
    qm_spec_property_unpack_string_direct(property, &string_value, &string_len);
    prov_recv.password = string_value;
    QM_LOGD(LOG_TAG, "pro siid %d piid %d, payload %s", property->siid, property->piid, prov_recv.password);

    property = qm_spec_property_find(operation, QM_SPEC_NTP_TIMEZONE_SIID, QM_SPEC_NTP_TIMEZONE_PIID);
    if(property != NULL){
        qm_spec_property_unpack_int8(property, &prov_recv.timezone);
        QM_LOGD(LOG_TAG, "pro siid %d piid %d, payload %d", property->siid, property->piid, prov_recv.timezone);
    }

    if(g_ble_prov_handle && g_ble_prov_handle->recv_handler){
        g_ble_prov_handle->recv_handler(g_ble_prov_handle, &prov_recv, g_ble_prov_handle->userdata);
    }

    qm_spec_property_operation_delete(operation);
}   

/**
 * @brief 创建provision会话实例, 并以默认值配置会话参数
 *
 * @return void *
 * @retval 非NULL provision实例的句柄
 * @retval NULL   初始化失败, 一般是内存分配失败导致
 * 
 */
qm_audio_prov_handle_t qm_audio_prov_open(qm_audio_prov_config_t *config)
{
    if(g_ble_prov_handle){
        return NULL;
    }

    g_ble_prov_handle = (qm_audio_prov_t *)qm_calloc(1, sizeof(qm_audio_prov_t));
    if(g_ble_prov_handle == NULL){
        return NULL;
    }

    g_ble_prov_handle->pid = config->pid;
    g_ble_prov_handle->product_secret = config->product_secret;

    g_ble_prov_handle->userdata  = config->userdata;
    g_ble_prov_handle->recv_handler = config->recv_fn;

    return (qm_audio_prov_handle_t)g_ble_prov_handle;
}


/**
 * @brief 启动provision流程
 *
 * @param handle provision会话句柄
 *
 * @return int32_t
 * @retval <QM_EOK  数据接收失败
 * @retval >=QM_EOK 数据接收成功
 */

 qm_err_t qm_audio_prov_start(qm_audio_prov_handle_t handle)
{
    uint8_t mac[6] = {0};
    qm_ble_dev_info_t dinfo = {0};

    qm_audio_prov_t *prov_handle = (qm_audio_prov_t *)handle;
    if(prov_handle == NULL){
        return -QM_EINVAL;
    }

    dinfo.product_id = prov_handle->pid;
    
    dinfo.dev_adv_mac = mac;
    dinfo.product_secret = prov_handle->product_secret;

    qm_ble_init(&dinfo,
                 qm_ble_status_change_cb,
                 qm_ble_set_dev_status_callback,
                 qm_ble_get_dev_status_callback,
                 qm_ble_response_callback,
                 qm_extinfo_callback,
                 qm_apinfo_ready_cb,
                 NULL);

    qm_ble_gap_get_local_addr(mac);
    qm_ble_core_device_id_set(mac, 6);
    qm_snprintf(prov_handle->local_name, 32, QM_BLE_ADV_NAME, dinfo.product_id, mac[4], mac[5]);

    return qm_ble_start_advertising(3, 1, 0, prov_handle->local_name);
}

static void deinit_delay_work(void *arg)
{
    if(g_ble_prov_handle && g_ble_prov_handle->ble_connect){
        qm_ble_disconnect_ble();
    }
}

/**
 * @brief 结束provision会话, 销毁实例并回收资源
 *
 * @param[in] handle 指向provision会话句柄的指针
 *
 * @return int32_t
 * @retval <QM_EOK  执行失败
 * @retval >=QM_EOK 执行成功
 * 
 */
qm_err_t qm_audio_prov_close(qm_audio_prov_handle_t handle)
{
    qm_audio_prov_t *prov_handle = (qm_audio_prov_t *)handle;

    if(g_ble_prov_handle == NULL || prov_handle == NULL){
        return -QM_EINVAL;
    }

    prov_handle->need_deinit = QM_TRUE;

    if(!prov_handle->ble_connect){
        qm_ble_end();
        qm_free(prov_handle);
        g_ble_prov_handle = NULL;
    }else{
        qm_post_delayed_action(&g_ble_prov_handle->deinit_work, deinit_delay_work, NULL, QM_IOT_PROV_DEINIT_DELAY_MS);
    }

    return QM_EOK;
}
