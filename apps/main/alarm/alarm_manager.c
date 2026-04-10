#include "alarm_manager.h"
#include "qm_errno.h"
#include "qm_kv.h"
#include "qm_utils_crc32.h"

#define KV_TAG_ALARM            ("alarm")
#define KV_TAG_ALARM_BKP        ("alarm_bkp")
#define ALARM_STORE_MAGIC_NUM   (0x12345678)

#define LOG_TAG                 ("alarm_manager")

typedef struct {
    uint32_t magic;
    uint32_t count;
    alarm_t alarm_list[ALARM_LIST_MAX_COUNT];
    uint32_t crc;   // crc32 of alarm_list
} alarm_store_t;

typedef struct {
    uint8_t is_init;

    alarm_store_t *alarm_store;
} alarm_manager_handle_t;

static alarm_manager_handle_t alarm_manager_handle;

static uint32_t calc_alarm_store_crc(alarm_store_t *alarm_store)
{
    uint32_t crc = 0;
    uint8_t alarm_data[alarm_store->count * 4];

    for (int i = 0; i < alarm_store->count; i++) {
        alarm_data[i * 4 + 0] = alarm_store->alarm_list[i].hour;
        alarm_data[i * 4 + 1] = alarm_store->alarm_list[i].min;
        alarm_data[i * 4 + 2] = alarm_store->alarm_list[i].repeat_mask;
        alarm_data[i * 4 + 3] = alarm_store->alarm_list[i].enable;
    }

    crc = qm_utils_crc32(alarm_data, alarm_store->count * 4, ALARM_STORE_MAGIC_NUM);;

    return crc;
}

qm_err_t alarm_manager_init(void)
{
    alarm_manager_handle.alarm_store = (alarm_store_t *)malloc(sizeof(alarm_store_t));
    if (alarm_manager_handle.alarm_store == NULL) {
        return -QM_ENOMEM;
    }

    memset(alarm_manager_handle.alarm_store, 0, sizeof(alarm_store_t));

    // get from flash
    int len = sizeof(alarm_store_t);
    qm_kv_get(KV_TAG_ALARM, alarm_manager_handle.alarm_store, &len);

    // check data
    if (alarm_manager_handle.alarm_store->magic != ALARM_STORE_MAGIC_NUM || 
        alarm_manager_handle.alarm_store->crc != calc_alarm_store_crc(alarm_manager_handle.alarm_store)) {
        // backup data
        QM_LOGE(LOG_TAG, "alarm manager data error, try to get backup data");
        qm_kv_get(KV_TAG_ALARM_BKP, alarm_manager_handle.alarm_store, &len);

        // check backup data
        if (alarm_manager_handle.alarm_store->magic != ALARM_STORE_MAGIC_NUM ||
            alarm_manager_handle.alarm_store->crc != calc_alarm_store_crc(alarm_manager_handle.alarm_store)) {
            QM_LOGE(LOG_TAG, "alarm manager backup data error");

            memset(alarm_manager_handle.alarm_store, 0, sizeof(alarm_store_t));

            alarm_manager_handle.alarm_store->magic = ALARM_STORE_MAGIC_NUM;
            alarm_manager_handle.alarm_store->crc = calc_alarm_store_crc(alarm_manager_handle.alarm_store);

            qm_kv_set(KV_TAG_ALARM, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);
            qm_kv_set(KV_TAG_ALARM_BKP, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);
        }
    }

    QM_LOGI(LOG_TAG, "alarm manager init success, count: %d", alarm_manager_handle.alarm_store->count);
    alarm_manager_handle.is_init = 1;

    return QM_EOK;
}

qm_err_t alarm_manager_update(void)
{
    // update crc
    alarm_manager_handle.alarm_store->count = 0;

    qm_kv_set(KV_TAG_ALARM, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);
    qm_kv_set(KV_TAG_ALARM_BKP, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);

    return QM_EOK;
}

qm_err_t alarm_manager_get_count(int32_t *count)
{
    *count = 0;

    if (!alarm_manager_handle.is_init) {
        QM_LOGE(LOG_TAG, "alarm manager not init");
        return -QM_EINIT;
    }

    *count = alarm_manager_handle.alarm_store->count;

    return QM_EOK;
}

qm_err_t alarm_manager_get_alarm_list(alarm_t *alarm_list)
{
    if (!alarm_manager_handle.is_init) {
        QM_LOGE(LOG_TAG, "alarm manager not init");
        return -QM_EINIT;
    }

    if (alarm_list == NULL) {
        QM_LOGE(LOG_TAG, "alarm list is NULL");
        return -QM_EINVAL;
    }

    memcpy(alarm_list, alarm_manager_handle.alarm_store->alarm_list, sizeof(alarm_t) * alarm_manager_handle.alarm_store->count);

    return QM_EOK;
}

qm_err_t alarm_manager_get_alarm(alarm_t *alarm, uint32_t alarm_index)
{
    if (!alarm_manager_handle.is_init) {
        QM_LOGE(LOG_TAG, "alarm manager not init");
        return -QM_EINIT;
    }

    if (alarm_index >= alarm_manager_handle.alarm_store->count) {
        QM_LOGE(LOG_TAG, "alarm index %d out of range", alarm_index);
        return -QM_EINVAL;
    }

    memcpy(alarm, &alarm_manager_handle.alarm_store->alarm_list[alarm_index], sizeof(alarm_t));

    return QM_EOK;
}

qm_err_t alarm_manager_add_alarm(alarm_t alarm)
{
    if (!alarm_manager_handle.is_init) {
        QM_LOGE(LOG_TAG, "alarm manager not init");
        return -QM_EINIT;
    }

    if (alarm_manager_handle.alarm_store->count >= ALARM_LIST_MAX_COUNT) {
        QM_LOGE(LOG_TAG, "alarm list full");
        return -QM_EFULL;
    }

    alarm_store_t *store = alarm_manager_handle.alarm_store;

    memcpy(&store->alarm_list[store->count], &alarm, sizeof(alarm_t));
    store->count++;

    store->crc = calc_alarm_store_crc(store);

    qm_kv_set(KV_TAG_ALARM, store, sizeof(alarm_store_t), 1);
    qm_kv_set(KV_TAG_ALARM_BKP, store, sizeof(alarm_store_t), 1);

    return QM_EOK;
}

qm_err_t alarm_manager_del_alarm(int32_t alarm_index)
{
    if (!alarm_manager_handle.is_init) {
        QM_LOGE(LOG_TAG, "alarm manager not init");
        return -QM_EINIT;
    }

    if (alarm_index >= alarm_manager_handle.alarm_store->count) {
        QM_LOGE(LOG_TAG, "alarm index %d out of range", alarm_index);
        return -QM_EINVAL;
    }

    // shift alarm list
    for (int i = alarm_index; i < alarm_manager_handle.alarm_store->count - 1; i++) {
        memcpy(&alarm_manager_handle.alarm_store->alarm_list[i], &alarm_manager_handle.alarm_store->alarm_list[i + 1], sizeof(alarm_t));
    }

    alarm_manager_handle.alarm_store->count--;
    alarm_manager_handle.alarm_store->crc = calc_alarm_store_crc(alarm_manager_handle.alarm_store);

    qm_kv_set(KV_TAG_ALARM, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);
    qm_kv_set(KV_TAG_ALARM_BKP, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);

    return QM_EOK;
}

qm_err_t alarm_manager_mod_alarm(alarm_t alarm, int32_t alarm_index)
{
    if (!alarm_manager_handle.is_init) {
        QM_LOGE(LOG_TAG, "alarm manager not init");
        return -QM_EINIT;
    }

    if (alarm_index >= alarm_manager_handle.alarm_store->count) {
        QM_LOGE(LOG_TAG, "alarm index %d out of range", alarm_index);
        return -QM_EINVAL;
    }

    memcpy(&alarm_manager_handle.alarm_store->alarm_list[alarm_index], &alarm, sizeof(alarm_t));

    alarm_manager_handle.alarm_store->crc = calc_alarm_store_crc(alarm_manager_handle.alarm_store);

    qm_kv_set(KV_TAG_ALARM, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);
    qm_kv_set(KV_TAG_ALARM_BKP, alarm_manager_handle.alarm_store, sizeof(alarm_store_t), 1);

    return QM_EOK;
}
