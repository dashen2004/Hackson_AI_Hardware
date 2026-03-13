#ifndef _QM_NW_SWITCH_H
#define _QM_NW_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"

#define CONFIG_NETWORK_SIID     (8)

// 平台下发WIFI账号
#define CONFIG_WIFI_SSID_PIID   (2)
// 平台下发WIFI密码
#define CONFIG_WIFI_PWD_PIID    (3) 
// 平台下发网络切换
#define CONFIG_NET_SWITCH_PIID  (1) 

typedef enum {
    // 回复平台wifi账号
    QM_MW_REPORT_SSID_RSP = 0x0,
    // 回复平台wifi密码
    QM_MW_REPORT_PWD_RSP,
    // 回复平台网络切换
    QM_NW_REPORT_NET_SWITCH,
    QM_NW_REPORT_MAX,
} network_report_t;

qm_err_t bsp_nw_cmd_action(qm_spec_property_t *spec_prop);

int qm_nw_switch_init(void *iot_handle);
qm_err_t qm_nw_switch_4g(void);
int qm_nw_get_net_type(void);


#ifdef __cplusplus
}
#endif

#endif


