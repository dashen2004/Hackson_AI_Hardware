#include "ljyd_iot_prop.h"
#include "ljyd_iot_volume.h"

#include "qm.h"

/*
 * 物模型下行表。新增属性加一行，回调在各自模块里。
 * 对齐 ljyd_ai_lamp 的 siid / piid / action。
 */

typedef int (*ljyd_iot_prop_fn)(qm_spec_property_t *property);

typedef struct {
    uint8_t siid;
    uint16_t piid;
    ljyd_iot_prop_fn action;
} ljyd_iot_prop_entry_t;

static const ljyd_iot_prop_entry_t s_table[] = {
    {LJYD_IOT_VOLUME_SIID, LJYD_IOT_VOLUME_PIID, ljyd_iot_volume_set_value},
    {LJYD_IOT_VOLUME_SIID, LJYD_IOT_VOLUME_PIID_ADJ, ljyd_iot_volume_set_adjust},
};

int ljyd_iot_prop_init(void *iot_handle)
{
    int ret;

    if (iot_handle == NULL) {
        return -QM_EINVAL;
    }

    ret = ljyd_iot_volume_init(iot_handle);
    if (ret != QM_EOK) {
        return ret;
    }
    return QM_EOK;
}

void ljyd_iot_prop_on_set(qm_spec_property_t *property)
{
    size_t i;

    if (property == NULL) {
        return;
    }
    for (i = 0; i < QM_ARRAY_SIZE(s_table); i++) {
        if (s_table[i].siid != property->siid ||
            s_table[i].piid != property->piid) {
            continue;
        }
        if (s_table[i].action != NULL) {
            (void)s_table[i].action(property);
        }
        return;
    }
}
