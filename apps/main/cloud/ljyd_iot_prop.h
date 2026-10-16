#ifndef LJYD_IOT_PROP_H
#define LJYD_IOT_PROP_H

#include "qm_spec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 各物模型模块在这里初始化。云连接只调这一次。 */
int ljyd_iot_prop_init(void *iot_handle);

/* 云端设属性。按 siid/piid 查表调回调，没有匹配项则返回。 */
void ljyd_iot_prop_on_set(qm_spec_property_t *property);

#ifdef __cplusplus
}
#endif

#endif
