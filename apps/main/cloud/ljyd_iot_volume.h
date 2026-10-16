#ifndef LJYD_IOT_VOLUME_H
#define LJYD_IOT_VOLUME_H

#include "qm_spec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LJYD_IOT_VOLUME_SIID      4
#define LJYD_IOT_VOLUME_PIID      1 /* 绝对音量 10..100 */
#define LJYD_IOT_VOLUME_PIID_ADJ  2 /* 1增大 2减小 3最大 4最小 */

/* 读 KV。音量等到芯片就绪后再写。 */
int ljyd_iot_volume_init(void *iot_handle);

/* 由 ljyd_iot_prop 表调用。 */
int ljyd_iot_volume_set_value(qm_spec_property_t *property);
int ljyd_iot_volume_set_adjust(qm_spec_property_t *property);

#ifdef __cplusplus
}
#endif

#endif
