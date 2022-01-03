#ifndef _SYS_NOTIFY_H_
#define _SYS_NOTIFY_H_

/** @brief system defined notifications */ 
typedef enum{
 
    notify_WIFI_STATUS_CHANGED,        /**< Wlan connection status is changed, type: void (*function)(WiFiEvent status, void* arg)*/
    notify_WiFI_PARA_CHANGED,          /**< Wlan parameters has received (channel, BSSID, key...), called when connect to a wlan, type: void (*function)(apinfo_adv_t *ap_info, char *key, int key_len, void* arg)*/
    notify_DHCP_COMPLETED,             /**< ESP8266 has get the IP address from DHCP server, type: void (*function)(IPStatusTypedef *pnet, void* arg)*/
    notify_WIFICONFIG_STARTED,
    notify_WIFICONFIG_WPS_COMPLETED,     /**< wificonfig receive SSID, Key, type: void (*function)(network_InitTypeDef_st *nwkpara, void* arg)*/
    notify_WIFICONFIG_GET_EXTRA_DATA,    /**< wificonfig receive extra data, type: void (*function)(int datalen, char*data, void* arg)*/
    notify_TCP_CLIENT_CONNECTED,       /**< A tcp client has connected to TCP server, type: void (*function)(int fd, void* arg)*/
    notify_DNS_RESOLVE_COMPLETED,      /**< A DNS host address has resolved, type: void (*function)(uint8_t *hostname, uint32_t ip,  void* arg)*/
    notify_SYS_WILL_POWER_OFF,         /**< System power will be turned off, type: void (*function)(void* arg)*/
    notify_WIFI_CONNECT_FAILED,        /**< A wlan connection attemption is failed, type: void join_fail(OSStatus err, void* arg)*/
    notify_WIFI_Fatal_ERROR,           /**< A fatal error occured when communicating with wlan sub-system, type: void (*function)(void* arg)*/
    notify_Stack_Overflow_ERROR,       /**< A RTOS thread's stack is over-flowed, type: void (*function)(char *taskname, void* arg)*/
    notify_GPRS_STATUS_CHANGED,
    notify_NTP_UPDATE,
    notify_DISPLAY,
    notify_CONFIG,
    notify_POWERDOWN,
    notify_CLOUD_EVNET_CHANGED,
    notify_WIFI_AP_CHANGED,
    notify_SYSTEM_BUTTON_CLICKED_CHANGED,
    notify_SYSTEM_BUTTON_LONGPRESSED_CHANGED,
    notify_SYSTEM_BUTTON_DOUBLECLICKED_CHANGED,
    notify_PACKET_CONFIRM,
    notify_SERIAL_PACKET_CONFIRM,
    notify_SERIAL_CONFIG_NOTIFY,
    notify_NETWORK_STATUS_CHANGED,
    notify_CONFIG_UPDATE,
    notify_SUB_EVENT_CHANNED,
    notify_SERIAL_PACKET_REPORT,
    notify_IO_STATUS_CHANGED,
    notify_SERIAL_DATA_UPDATE,
    notify_MAX
    
}notify_types_t;


typedef enum {
    NOTIFY_GPRS_UP = 1,
    NOTIFY_GPRS_DOWN,
} notify_gprs_t; 

typedef enum {
    NOTIFY_STATION_UP = 1,
    NOTIFY_STATION_DOWN,

    NOTIFY_AP_UP,
    NOTIFY_AP_DOWN,
} notify_wlan_t; 

typedef notify_wlan_t WiFiEvent;

typedef enum{

    NETWORK_STATUS_UP = 1,
    NETWORK_STATUS_DOWN,
    NETWORK_STATUS_SOFTAP,
    
}network_status_t;

int system_notify_register(notify_types_t notify_type, void* functionAddress, void* arg);
int system_notify_remove(notify_types_t notify_type, void* functionAddress);
int mico_system_notify_remove_all(notify_types_t notify_type);



void network_status_changed(network_status_t status);


#endif

