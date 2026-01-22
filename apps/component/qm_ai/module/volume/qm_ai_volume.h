#ifndef __QM_MODULE_VOLUME_H__
#define __QM_MODULE_VOLUME_H__

#include <stdbool.h>
#include <stdint.h>
#include "qm.h"
#include "qm_ai_license.h"


#ifdef __cplusplus
extern "C" {
#endif

int32_t qm_ai_volume_init(qm_ai_license_config_t *config);
int32_t qm_ai_volume_deinit(void);
int32_t qm_ai_volume_start();
int32_t qm_ai_volume_stop();

#ifdef __cplusplus
}
#endif

#endif // __QM_MODULE_VOLUME_H__
