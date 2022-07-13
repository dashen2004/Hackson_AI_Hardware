/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_system.h"
#include "nvs_flash.h"
#include "network.h"
#include "cloud.h"
#include "prodtst_app.h"
#include "drivers.h"
#include "qm.h"

int user_debug_enabled = 1;


#define HEX_LOG(name, p_data, len) \
    do{\
        uint32_t i; \
        printf("[%s:%d]",__FUNCTION__,__LINE__);\
        printf("%s ",name);  \
        for(i=0;i<len;i++)\
        {\
          printf("%02X ", *(p_data+i));\
        }\
        printf("\n");\
      }while(0)


void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    //drivers_init();

    qm_main();

    //extern void miot_qrcode_init(void);
    //miot_qrcode_init();

    // network_init();

    // prodtst_app_init();

    // xjiang_cloud_init();
    #include "totp.h"

    totp_t totp = {0};
    totp_t last_totp = {0};
    uint32_t key = 0;
    
    char *msg = "012345";

    char *secret_key = "123456";
    char hmac[40] = {0};
    
    #include "qm_utils_hmac.h"

    qm_utils_hmac_sha1(msg, 6, (char*)hmac, (char*)secret_key, 6);

    printf("digest: %s\n", hmac);

    uint8_t mac[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

    totp_get_key(mac, 1657711869, &key);

    printf("passwd1: %u\n",key);

    totp_get_keys(1800, mac, 1657711869, &totp, &last_totp);
    #if 1
    for(int i = 0; i < 8; i++){
        printf("passwd: %u\n", totp[i]);   
    }

    for(int i = 0; i < 8; i++){
        printf("last passwd: %u\n", last_totp[i]);   
    }
    #endif

}
