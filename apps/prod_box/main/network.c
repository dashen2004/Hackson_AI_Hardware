#include "network.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "sys_notify.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "user_debug.h"

#define network_log(format, ...)  custom_log("network", format, ##__VA_ARGS__)

/* The examples use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "wells"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       4

#define EXAMPLE_ESP_MAXIMUM_RETRY  5

static int s_retry_num = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    tcpip_adapter_ip_info_t ip;

    switch(event->event_id) {

    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        network_log("got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        s_retry_num = 0;

        network_status_changed(NETWORK_STATUS_UP);
        
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                network_log("retry to connect to the AP");
            }

            network_status_changed(NETWORK_STATUS_DOWN);
            network_log("connect to the AP fail\n");
            break;
        }

    case SYSTEM_EVENT_AP_STACONNECTED:
        network_log("station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        network_log("station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_ETH_CONNECTED:
        network_log("Ethernet Link Up");
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        network_log("Ethernet Link Down");
        break;
    case SYSTEM_EVENT_ETH_START:
        network_log("Ethernet Started");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
        ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip));
        network_log("Ethernet Got IP Addr");
        network_log("~~~~~~~~~~~");
        network_log("ETHIP:" IPSTR, IP2STR(&ip.ip));
        network_log("ETHMASK:" IPSTR, IP2STR(&ip.netmask));
        network_log("ETHGW:" IPSTR, IP2STR(&ip.gw));
        network_log("~~~~~~~~~~~");
        break;
    case SYSTEM_EVENT_ETH_STOP:
        network_log("Ethernet Stopped");
        break;

    default:
        break;
    }
    return ESP_OK;
}

static void wifi_softap_init(void)
{
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    network_log("wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

}

static void wifi_sta_init(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "xjiang",
            .password = "asdf062516",
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    network_log("wifi_init_sta finished.");
}


static void wifi_init(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    ESP_ERROR_CHECK(esp_wifi_start() );
}

void network_init(void)
{
    wifi_init();

    wifi_sta_init();

}