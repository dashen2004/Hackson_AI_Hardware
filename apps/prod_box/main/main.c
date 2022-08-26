/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_system.h"
#include "nvs_flash.h"
#include "network.h"
//#include "cloud.h"
//#include "prodtst_app.h"
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

    extern void opple_serial_test(void);
    opple_serial_test();

    //extern void qm_serial_test(void);
    //qm_serial_test();

}
