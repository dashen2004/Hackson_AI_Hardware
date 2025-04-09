#ifndef _VOLC_TOKEN_H_
#define _VOLC_TOKEN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

/**
 * @brief dev模块收到从网络上来的报文时, 通知用户的报文内容
 */
typedef enum {
    VOLC_RTC_CODE_SUCCESS,
    VOLC_RTC_CODE_FAILED,
    VOLC_RTC_CODE_TIMEOUT,
} volc_rtc_code_t;

typedef void (*volc_rtc_token_request_fn)(volc_rtc_code_t code, void *arg);

/**
 * @brief dev模块收到从网络上来的报文时, 通知用户的报文内容
 */
typedef enum {
    VOLC_RTC_CODE_SUCCESS,
    VOLC_RTC_CODE_FAILED,
    VOLC_RTC_CODE_TIMEOUT,
} volc_rtc_code_t;

/**
 * @brief AUDIO模块初始化
 *
 * @return qm_err_t
 */
qm_err_t volc_rtc_token_request(volc_rtc_token_request_fn *cfg, uint32_t timeout);


#ifdef __cplusplus
}
#endif

#endif
