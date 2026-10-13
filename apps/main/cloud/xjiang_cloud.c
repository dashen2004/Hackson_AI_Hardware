#include "xjiang_cloud.h"
#include "ai_app.h"

#include "qm_iot_api.h"
#include "qm_iot_dynreg.h"
#include "qm_kv.h"
#include "qm_provision.h"
#include "qm_wifi.h"
#include "qm_wifi_dev.h"
#include "qm_ble_gap.h"
#include "qm_work.h"
#include "qm_string.h"
#include "qm_gpio.h"

#include <string.h>

#define LOG_TAG "cloud"

#define CLOUD_KV_BIND      "cloud/bind"
#define CLOUD_KV_DID       "cloud/did"
#define CLOUD_KV_HOST      "cloud/host"
#define CLOUD_KV_PORT      "cloud/port"
#define CLOUD_KV_SRV_CA    "cloud/s_ca"
#define CLOUD_KV_CLI_CRT   "cloud/c_ca"
#define CLOUD_KV_CLI_KEY   "cloud/c_key"
#define CLOUD_KV_PUB_KEY   "cloud/p_key"
#define CLOUD_CERT_MAX     (8 * 1024)

#define CLOUD_WIFI_DELAY_MS 1000
#define CLOUD_IOT_TASK_STACK (16 * 1024)
#define CLOUD_IOT_TASK_PRIO  5

#define CLOUD_RESET_GPIO       14
#define CLOUD_RESET_HOLD_MS    3000
#define CLOUD_RESET_POLL_MS    100
#define CLOUD_RESET_TASK_STACK 2048
#define CLOUD_RESET_TASK_PRIO  5

static void *iot_handle;
static qm_provision_handle_t provision_handle;
static uint8_t cloud_started;
static uint8_t iot_task_running;
static uint8_t wifi_event_registered;
static qm_work_t wifi_delay_work;
static char pending_ssid[CONFIG_QM_WIFI_SSID_MAX_LEN + 1];
static char pending_password[CONFIG_QM_WIFI_PASSWD_MAX_LEN + 1];

static char dynreg_host[128];
static char dynreg_server_ca[CLOUD_CERT_MAX];
static char dynreg_client_crt[CLOUD_CERT_MAX];
static char dynreg_client_key[CLOUD_CERT_MAX];
static char dynreg_public_key[CLOUD_CERT_MAX];
static qm_iot_cloud_auth_t cloud_credential;
static char device_sn[13];

static void cloud_set_bound(uint8_t bound)
{
    (void)qm_kv_set(CLOUD_KV_BIND, &bound, (int)sizeof(bound), 1);
}

static int cloud_save_credential(const qm_iot_cloud_auth_t *credential)
{
    if (credential == NULL || credential->did == 0) {
        return -QM_EINVAL;
    }
    (void)qm_kv_set(CLOUD_KV_DID, (void *)&credential->did, (int)sizeof(credential->did), 1);
    (void)qm_kv_set(CLOUD_KV_HOST, credential->cloud_info.host,
                    (int)strlen(credential->cloud_info.host) + 1, 1);
    (void)qm_kv_set(CLOUD_KV_PORT, (void *)&credential->cloud_info.port,
                    (int)sizeof(credential->cloud_info.port), 1);
    if (credential->server_cert != NULL && credential->server_cert_len > 0) {
        (void)qm_kv_set(CLOUD_KV_SRV_CA, credential->server_cert,
                        (int)credential->server_cert_len, 1);
    }
    if (credential->client_crt != NULL && credential->client_crt_len > 0) {
        (void)qm_kv_set(CLOUD_KV_CLI_CRT, credential->client_crt,
                        (int)credential->client_crt_len, 1);
    }
    if (credential->client_key != NULL && credential->client_key_len > 0) {
        (void)qm_kv_set(CLOUD_KV_CLI_KEY, credential->client_key,
                        (int)credential->client_key_len, 1);
    }
    if (credential->ota_public_key != NULL && credential->ota_public_key_len > 0) {
        (void)qm_kv_set(CLOUD_KV_PUB_KEY, credential->ota_public_key,
                        (int)credential->ota_public_key_len, 1);
    }
    return QM_EOK;
}

static void refresh_device_sn(void)
{
    qm_bd_addr_t mac = {0};

    if (device_sn[0] != '\0') {
        return;
    }
    if (qm_ble_gap_get_local_addr(mac) != QM_EOK) {
        QM_LOGW(LOG_TAG, "get ble mac failed");
        return;
    }
    qm_snprintf(device_sn, sizeof(device_sn), "%02X%02X%02X%02X%02X%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void copy_len(char *dst, uint32_t dst_size, const char *src, uint32_t len)
{
    if (src == NULL || len == 0 || len >= dst_size) {
        return;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static void dynreg_recv_handler(void *handle, const qm_iot_dynreg_recv_t *packet, void *userdata)
{
    const qm_iot_dynreg_device_info_t *info;

    (void)handle;
    (void)userdata;
    if (packet == NULL || packet->type != QM_IOT_DYNREGRECV_DEVICE_INFO) {
        return;
    }
    info = &packet->data.device_info;
    memset(&cloud_credential, 0, sizeof(cloud_credential));
    cloud_credential.did = info->did;
    copy_len(dynreg_host, sizeof(dynreg_host), info->mqtt_host, info->mqtt_host_len);
    strncpy(cloud_credential.cloud_info.host, dynreg_host, sizeof(cloud_credential.cloud_info.host) - 1);
    cloud_credential.cloud_info.port = info->mqtt_port ? info->mqtt_port : 8883;
    QM_LOGI(LOG_TAG, "dynreg did=%u host=%s port=%u",
            (unsigned)cloud_credential.did,
            cloud_credential.cloud_info.host,
            (unsigned)cloud_credential.cloud_info.port);
    copy_len(dynreg_server_ca, sizeof(dynreg_server_ca), info->server_cert, info->server_cert_len);
    cloud_credential.server_cert = dynreg_server_ca;
    cloud_credential.server_cert_len = info->server_cert_len;
    copy_len(dynreg_client_crt, sizeof(dynreg_client_crt), info->client_crt, info->client_cert_len);
    cloud_credential.client_crt = dynreg_client_crt;
    cloud_credential.client_crt_len = info->client_cert_len;
    copy_len(dynreg_client_key, sizeof(dynreg_client_key), info->client_private_key, info->client_privkey_len);
    cloud_credential.client_key = dynreg_client_key;
    cloud_credential.client_key_len = info->client_privkey_len;
    copy_len(dynreg_public_key, sizeof(dynreg_public_key), info->public_key, info->public_key_len);
    cloud_credential.ota_public_key = dynreg_public_key;
    cloud_credential.ota_public_key_len = info->public_key_len;
}

static int cloud_run_dynreg(void)
{
    void *dynreg_handle;
    uint32_t product_id = CONFIG_XJIANG_CLOUD_PID;
    uint16_t port = CONFIG_QM_IOT_DYNREG_PORT;
    uint32_t retry_num = CONFIG_QM_IOT_DYNREG_RETRY_COUNT;
    uint32_t timeout = CONFIG_QM_IOT_DYNREG_RECV_TIMEOUT_MS;
    int ret = QM_EOK;

    refresh_device_sn();
    if (device_sn[0] == '\0') {
        return -QM_EINVAL;
    }

    dynreg_handle = qm_iot_dynreg_init();
    if (dynreg_handle == NULL) {
        return -QM_ENOMEM;
    }
    ret |= qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_PORT, &port);
    ret |= qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_PRODUCT_ID, &product_id);
    ret |= qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_PRODUCT_SECRET,
                                (void *)CONFIG_XJIANG_CLOUD_SECERT);
    ret |= qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_SN, device_sn);
    ret |= qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_MAX_RETRY_NUM, &retry_num);
    ret |= qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_RECV_TIMEOUT_MS, &timeout);
    ret |= qm_iot_dynreg_setopt(dynreg_handle, QM_IOT_DYNREGOPT_RECV_HANDLER, dynreg_recv_handler);
    if (ret == QM_EOK) {
        ret = qm_iot_dynreg_start(dynreg_handle, QM_IOT_NETWORK_TYPE_WIFI);
    }
    (void)qm_iot_dynreg_deinit(&dynreg_handle);
    if (ret != QM_EOK || cloud_credential.did == 0) {
        QM_LOGE(LOG_TAG, "dynreg failed ret=%d did=%u", ret, (unsigned)cloud_credential.did);
        return ret != QM_EOK ? ret : -QM_EINVAL;
    }
    ret = qm_iot_setopt(iot_handle, QM_IOT_OPT_CLOUD_CREDENTIAL, &cloud_credential);
    if (ret != QM_EOK) {
        QM_LOGE(LOG_TAG, "set credential failed ret=%d", ret);
        return ret;
    }
    return cloud_save_credential(&cloud_credential);
}

static void cloud_set_devinfo(void)
{
    qm_iot_devinfo_t info = {0};
    qm_wifi_ap_record_t ap = {0};
    uint8_t mac[6] = {0};

    info.type = QM_IOT_NETWORK_TYPE_WIFI;
    if (qm_wifi_get_mac(QM_WIFI_IF_STA, mac) == QM_EOK) {
        qm_snprintf(info.data.wifi.mac, sizeof(info.data.wifi.mac),
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    if (qm_wifi_sta_get_ap_info(&ap) == QM_EOK && ap.ssid_len > 0) {
        memcpy(info.data.wifi.ssid, ap.ssid, ap.ssid_len);
        qm_snprintf(info.data.wifi.bssid, sizeof(info.data.wifi.bssid),
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    ap.bssid[0], ap.bssid[1], ap.bssid[2],
                    ap.bssid[3], ap.bssid[4], ap.bssid[5]);
        info.data.wifi.rssi = ap.rssi;
        info.data.wifi.channel = ap.channel;
    }
    (void)qm_iot_setopt(iot_handle, QM_IOT_OPT_DEVINFO, &info);
}

/*
 * is_reset 非 0 时，MQTT 连上也不发 CLOUD_CONN，要等 bindDevNotify。
 * 本地标记会停在 1，云不一定再推绑定通知。配网并动态注册之后，
 * 启动前改成未复位，让连接成功事件发出来。
 */
static void cloud_migrate_reset_marker(void)
{
    qm_iot_reset_info_t reset_info;
    int ret;
    int was;

    if (iot_handle == NULL) {
        return;
    }
    memset(&reset_info, 0, sizeof(reset_info));
    ret = qm_iot_reset_info_get(iot_handle, &reset_info);
    was = (ret == QM_EOK) ? (int)reset_info.is_reset : -1;
    if (was == (int)QM_IOT_NOT_RESET) {
        QM_LOGI(LOG_TAG, "reset marker already clear");
        return;
    }
    reset_info.is_reset = QM_IOT_NOT_RESET;
    ret = qm_iot_reset_info_set(iot_handle, &reset_info);
    QM_LOGI(LOG_TAG, "reset marker force not-reset was=%d ret=%d", was, ret);
}

static int cloud_start_iot(void)
{
    int ret;

    if (iot_handle == NULL || cloud_started) {
        return QM_EOK;
    }
    /* 已存 WiFi 也每次重新注册，不用 KV 里的旧证书直接连。 */
    ret = cloud_run_dynreg();
    if (ret != QM_EOK) {
        return ret;
    }
    cloud_set_devinfo();
    cloud_migrate_reset_marker();
    ret = qm_iot_start(iot_handle);
    if (ret != QM_EOK) {
        QM_LOGE(LOG_TAG, "iot start failed ret=%d", ret);
        return ret;
    }
    cloud_started = 1;
    /* Session task only starts when started is already set and network is up. */
    (void)qm_iot_network_update(iot_handle, QM_IOT_NETWORK_UP);
    QM_LOGI(LOG_TAG, "iot start ok");
    return ret;
}

static void cloud_iot_task(void *arg)
{
    (void)arg;
    (void)cloud_start_iot();
    iot_task_running = 0;
    qm_task_exit(NULL);
}

static void cloud_schedule_iot(void)
{
    static qm_task_t task;

    if (cloud_started || iot_task_running) {
        return;
    }
    iot_task_running = 1;
    if (qm_task_new(&task, "cloud_iot", cloud_iot_task, NULL,
                    CLOUD_IOT_TASK_STACK, CLOUD_IOT_TASK_PRIO) != 0) {
        iot_task_running = 0;
        QM_LOGE(LOG_TAG, "iot task create failed");
    }
}

static int cloud_apply_wifi(const char *ssid, const char *password)
{
    qm_wifi_config_t config = {0};
    uint32_t ssid_len;
    uint32_t password_len;

    if (ssid == NULL || ssid[0] == '\0') {
        return -QM_EINVAL;
    }
    ssid_len = (uint32_t)strlen(ssid);
    password_len = (password != NULL) ? (uint32_t)strlen(password) : 0;
    if (ssid_len > CONFIG_QM_WIFI_SSID_MAX_LEN || password_len > CONFIG_QM_WIFI_PASSWD_MAX_LEN) {
        return -QM_EINVAL;
    }
    memcpy(config.sta.ssid, ssid, ssid_len);
    config.sta.ssid_len = (uint8_t)ssid_len;
    if (password_len > 0) {
        memcpy(config.sta.password, password, password_len);
        config.sta.password_len = (uint8_t)password_len;
    }
    return qm_wifi_set_config(QM_WIFI_IF_STA, &config);
}

static uint8_t provision_resetting;

static void cloud_clear_saved_provision(void)
{
    uint8_t bound = 0;

    (void)qm_kv_set(CLOUD_KV_BIND, &bound, (int)sizeof(bound), 1);
    (void)qm_kv_del("wifi_cfg");
}

static void cloud_reset_provision(void)
{
    if (provision_resetting) {
        return;
    }
    provision_resetting = 1;

    if (iot_handle != NULL && qm_iot_reset(iot_handle) == QM_EOK) {
        QM_LOGI(LOG_TAG, "gpio%d held %u ms, request unbind",
                CLOUD_RESET_GPIO, (unsigned)CLOUD_RESET_HOLD_MS);
        return;
    }

    cloud_clear_saved_provision();
    QM_LOGI(LOG_TAG, "gpio%d held %u ms, cloud not up, reboot",
            CLOUD_RESET_GPIO, (unsigned)CLOUD_RESET_HOLD_MS);
    qm_msleep(50);
    qm_reboot();
}

static void cloud_reset_task(void *arg)
{
    qm_gpio_dev_t gpio = {
        .port = CLOUD_RESET_GPIO,
        .config = {
            .mode = QM_GPIO_MODE_INPUT,
            .pull_en = QM_GPIO_PULLUP_ONLY,
        },
    };
    uint32_t low_ms = 0;

    (void)arg;
    if (qm_gpio_init(&gpio) != QM_EOK) {
        QM_LOGE(LOG_TAG, "reset gpio%d init fail", CLOUD_RESET_GPIO);
        qm_task_exit(NULL);
        return;
    }

    while (1) {
        if (qm_gpio_get_level(&gpio) == 0) {
            if (low_ms == 0) {
                QM_LOGI(LOG_TAG, "gpio%d low, hold %u ms to reset provision",
                        CLOUD_RESET_GPIO, (unsigned)CLOUD_RESET_HOLD_MS);
            }
            low_ms += CLOUD_RESET_POLL_MS;
            if (low_ms >= CLOUD_RESET_HOLD_MS) {
                cloud_reset_provision();
            }
        } else {
            low_ms = 0;
        }
        qm_msleep(CLOUD_RESET_POLL_MS);
    }
}

static void cloud_reset_start(void)
{
    static qm_task_t task;

    if (qm_task_new(&task, "prov_rst", cloud_reset_task, NULL,
                    CLOUD_RESET_TASK_STACK, CLOUD_RESET_TASK_PRIO) != 0) {
        QM_LOGE(LOG_TAG, "reset task create fail");
    }
}

static int cloud_has_saved_wifi(void)
{
    qm_wifi_cfg_t cfg;
    int len = (int)sizeof(cfg);

    memset(&cfg, 0, sizeof(cfg));
    if (qm_kv_get("wifi_cfg", &cfg, &len) != QM_EOK || len != (int)sizeof(cfg)) {
        return 0;
    }
    return cfg.config.sta.ssid_len > 0 && cfg.config.sta.ssid[0] != '\0';
}

static int cloud_start_wifi(void)
{
    int ret = qm_wifi_init();
    if (ret != QM_EOK) {
        return ret;
    }
    ret = qm_wifi_set_mode(QM_WIFI_MODE_STA);
    if (ret != QM_EOK) {
        return ret;
    }
    return qm_wifi_start();
}

static void cloud_wifi_connect_later(void *arg)
{
    int ret;

    (void)arg;
    ret = qm_wifi_init();
    if (ret != QM_EOK) {
        QM_LOGE(LOG_TAG, "wifi init ret=%d", ret);
        return;
    }
    ret = qm_wifi_set_mode(QM_WIFI_MODE_STA);
    if (ret != QM_EOK) {
        QM_LOGE(LOG_TAG, "wifi set mode ret=%d", ret);
        return;
    }
    ret = cloud_apply_wifi(pending_ssid, pending_password);
    if (ret != QM_EOK) {
        QM_LOGE(LOG_TAG, "save provision wifi failed ret=%d", ret);
        return;
    }
    ret = qm_wifi_start();
    QM_LOGI(LOG_TAG, "wifi start ssid=%s ret=%d", pending_ssid, ret);
}

static void cloud_provision_recv(qm_provision_handle_t handle,
                                 const qm_provision_recv_t *recv,
                                 void *userdata)
{
    (void)userdata;
    if (recv == NULL || recv->ssid[0] == '\0') {
        return;
    }
    QM_LOGI(LOG_TAG, "provision ssid=%s", recv->ssid);
    if (recv->env_type_valid) {
        (void)qm_iot_dynreg_env_set((qm_iot_dynreg_env_t)recv->env_type);
    }
    memset(pending_ssid, 0, sizeof(pending_ssid));
    memset(pending_password, 0, sizeof(pending_password));
    strncpy(pending_ssid, recv->ssid, sizeof(pending_ssid) - 1);
    if (recv->password[0] != '\0') {
        strncpy(pending_password, recv->password, sizeof(pending_password) - 1);
    }
    cloud_set_bound(1);
    if (handle != NULL) {
        qm_provision_destroy(handle);
        if (provision_handle == handle) {
            provision_handle = NULL;
        }
        QM_LOGI(LOG_TAG, "ble stopped");
    }
    (void)qm_post_delayed_action(&wifi_delay_work, cloud_wifi_connect_later,
                                 NULL, CLOUD_WIFI_DELAY_MS);
}

static int cloud_start_provision(void)
{
    qm_provision_config_t config = {
        .product_id = CONFIG_XJIANG_CLOUD_PID,
        .product_secret = CONFIG_XJIANG_CLOUD_SECERT,
        .adv_cid = CONFIG_QM_IOT_PROV_BLE_ADV_CID,
        .recv_handler = cloud_provision_recv,
        .userdata = NULL,
    };
    int ret;

    if (provision_handle == NULL) {
        provision_handle = qm_provision_create(&config);
        if (provision_handle == NULL) {
            return -QM_ENOMEM;
        }
    }
    ret = qm_provision_start(provision_handle);
    QM_LOGI(LOG_TAG, "ble provision start ret=%d", ret);
    return ret;
}

static void cloud_wifi_event_handler(void *arg, qm_event_base_t event_base,
                                     int32_t event_id, void *event_data,
                                     size_t event_data_size)
{
    (void)arg;
    (void)event_base;
    (void)event_data;
    (void)event_data_size;

    switch ((qm_wifi_event_t)event_id) {
        case QM_WIFI_EVENT_STA_START:
            QM_LOGI(LOG_TAG, "wifi sta start");
            (void)qm_wifi_connect();
            break;
        case QM_WIFI_EVENT_STA_GOT_IP:
            QM_LOGI(LOG_TAG, "wifi got ip");
            (void)qm_iot_network_update(iot_handle, QM_IOT_NETWORK_UP);
            cloud_schedule_iot();
            break;
        case QM_WIFI_EVENT_STA_DISCONNECTED:
            QM_LOGW(LOG_TAG, "wifi disconnected");
            (void)qm_iot_network_update(iot_handle, QM_IOT_NETWORK_DOWN);
            (void)qm_wifi_connect();
            break;
        default:
            break;
    }
}

static void cloud_iot_event_handler(void *handle, const qm_iot_event_t *event, void *userdata)
{
    (void)handle;
    (void)userdata;
    if (event == NULL) {
        return;
    }
    switch (event->type) {
        case QM_IOT_EVENT_CLOUD_CONN:
            QM_LOGI(LOG_TAG, "cloud connected");
            if (ai_on_cloud_connected() != QM_EOK) {
                QM_LOGE(LOG_TAG, "agent start failed");
            }
            break;
        case QM_IOT_EVENT_CLOUD_DISCONN:
            QM_LOGW(LOG_TAG, "cloud disconnected");
            ai_on_cloud_disconnected();
            break;
        case QM_IOT_EVENT_CONNING_CLOUD:
            QM_LOGI(LOG_TAG, "cloud connecting");
            break;
        case QM_IOT_EVENT_RESET:
            QM_LOGI(LOG_TAG, "cloud reset, clear provision and reboot");
            cloud_clear_saved_provision();
            qm_msleep(50);
            qm_reboot();
            break;
        default:
            QM_LOGD(LOG_TAG, "iot event %d", event->type);
            break;
    }
}

static void cloud_iot_recv_handler(void *handle, const qm_iot_recv_t *recv, void *userdata)
{
    (void)handle;
    (void)userdata;
    if (recv == NULL) {
        return;
    }
    QM_LOGD(LOG_TAG, "iot recv type=%d siid=%u piid=%u", recv->type, recv->siid, recv->piid);
}

int xjiang_cloud_init(void)
{
    uint32_t pid = CONFIG_XJIANG_CLOUD_PID;
    int ret;

    cloud_reset_start();

    iot_handle = qm_iot_init();
    if (iot_handle == NULL) {
        QM_LOGE(LOG_TAG, "qm_iot_init failed");
        return -QM_EIO;
    }
    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_ID, &pid);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_VERSION_STR, CONFIG_XJIANG_VERSION_STR);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_PRODUCT_SECRET, CONFIG_XJIANG_CLOUD_SECERT);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_RECV_HANDLER, cloud_iot_recv_handler);
    qm_iot_setopt(iot_handle, QM_IOT_OPT_EVENT_HANDLER, cloud_iot_event_handler);
    (void)qm_iot_dynreg_env_set(QM_IOT_DYNREG_ENV_TEST);

    if (!wifi_event_registered) {
        ret = qm_event_handler_register(QM_EVENT_WIFI, QM_EVENT_ANY_ID,
                                        cloud_wifi_event_handler, NULL);
        if (ret != QM_EOK) {
            QM_LOGE(LOG_TAG, "wifi event register failed ret=%d", ret);
            return ret;
        }
        wifi_event_registered = 1;
    }

    refresh_device_sn();
    if (device_sn[0] != '\0') {
        (void)qm_iot_setopt(iot_handle, QM_IOT_OPT_SN, device_sn);
    }

    ret = ai_app_bind_cloud(iot_handle);
    if (ret != QM_EOK) {
        QM_LOGE(LOG_TAG, "agent bind failed ret=%d", ret);
        return ret;
    }

    if (cloud_has_saved_wifi()) {
        QM_LOGI(LOG_TAG, "saved wifi, skip ble");
        return cloud_start_wifi();
    }

    QM_LOGI(LOG_TAG, "no saved wifi, start ble provision");
    return cloud_start_provision();
}
