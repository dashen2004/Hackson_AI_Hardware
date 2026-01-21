/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _XJIANG_CLOUD_H_
#define _XJIANG_CLOUD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "qm_spec_api.h"
#include "qm_iot_core.h"

int xjiang_cloud_init(void);

int xjiang_cloud_reset(void);
#if 0
void qm_iot_user_report(uint8_t pid, int value);
#endif
qm_err_t xjiang_cloud_report(qm_spec_property_t *property);
// qm_err_t xjiang_cloud_multi_report(qm_iot_core_msg_type_t type, qm_spec_property_operation_t *operation);
qm_err_t iot_voice_corpus_request(uint8_t number);

uint8_t get_ota_status_flag(void);
void *xjiang_cloud_get_handle(void);

#ifdef __cplusplus
}
#endif

#endif