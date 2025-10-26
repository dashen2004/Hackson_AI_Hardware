/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BSP_BUTTON_H_
#define _BSP_BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

qm_err_t bsp_button_init(void);

qm_err_t bsp_button_stop(void);

qm_err_t bsp_button_start(void);


#ifdef __cplusplus
}
#endif

#endif
