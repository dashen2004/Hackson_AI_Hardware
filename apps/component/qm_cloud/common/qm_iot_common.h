#ifndef __QM_IOT_COMMON_H__
#define __QM_IOT_COMMON_H__

#if defined(__cplusplus)
extern "C" {
#endif


#define QM_IOT_COMMON_DID_MAX_LEN       (16)
#define QM_IOT_COMMON_TOPIC_MAX_LEN     (64)
#define QM_IOT_COMMON_MSG_ID_MAX_NUM    (65535)


#define QM_IOT_COMMON_ID_KEY                "id"
#define QM_IOT_COMMON_PARAMS_KEY            "params"

#define QM_IOT_COMMON_PARAMS_SET_KEY        "set"
#define QM_IOT_COMMON_PARAMS_GET_KEY        "get"
#define QM_IOT_COMMON_PARAMS_REPORT_KEY     "report"


#define QM_IOT_COMMON_PARAMS_TYPE_KEY                    "type"
#define QM_IOT_COMMON_PARAMS_MAC_TYPE_KEY                "mac"
#define QM_IOT_COMMON_PARAMS_SSID_TYPE_KEY               "ssid"
#define QM_IOT_COMMON_PARAMS_BSSID_TYPE_KEY              "bssid"
#define QM_IOT_COMMON_PARAMS_CHANNEL_TYPE_KEY            "channel"
#define QM_IOT_COMMON_PARAMS_RSSI_TYPE_KEY               "rssi"
#define QM_IOT_COMMON_PARAMS_IP_TYPE_KEY                 "ip"
#define QM_IOT_COMMON_PARAMS_MODULE_TYPE_KEY             "moudle"

typedef enum {
    QM_IOT_CODE_SUCCESS = 0,
    QM_IOT_CODE_ERROR,
}qm_iot_code_t;


#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_COMMON_H__ */

