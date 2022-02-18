#ifndef __PRODTST_CMD_H__
#define __PRODTST_CMD_H__

#include "prodtst_core.h"

#define PRODTST_FW_VER_KEY             "fw_ver"
#define PRODTST_FW_VER_MAX_LEN           16
#define PRODTST_FW_NAME_KEY             "fw_name"
#define PRODTST_FW_NAME_MAX_LEN          16
#define PRODTST_WIFI_STA_MAC_KEY         "wifi_sta_mac"
#define PRODTST_WIFI_AP_MAC_KEY         "wifi_ap_mac"
#define PRODTST_MAC_MAX_LEN              24
#define PRODTST_AP_RSSI_KEY             "ap_rssi"
#define PRODTST_HW_CONFIG_KEY           "hw_config"
#define PRODTST_HW_CONFIG_MAX_LEN        4096
#define PRODTST_HW_TOKEN_KEY            "hw_token"
#define PRODTST_HW_TOKEN_MAX_LEN         196
#define PRODTST_NFC_KEY                 "nfc"
#define PRODTST_NFC_MAX_LEN              128



extern prodtst_cmd_t prodtst_wifi_generic_cmd[];
extern prodtst_cmd_t prodtst_hw_wifi_specific_cmd[];
extern prodtst_cmd_t prodtst_hw_wifi_special_cmd[];





#endif



