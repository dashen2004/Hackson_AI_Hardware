/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BSP_BAT_H_
#define _BSP_BAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"


qm_err_t bsp_bat_init(void);

qm_err_t bsp_bat_cancel_standby(void);
qm_err_t bsp_bat_enter_standby(int standby_time);



#ifdef __cplusplus
}
#endif

#endif
