/* volc rtc example code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
 
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/idf_additions.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "audio_sys.h"
#include "audio_thread.h"

#include "qm.h"

#include "audio_mem.h"

#define ENABLE_TASK_MONITOR

static char *TAG = "main";

#if defined(ENABLE_TASK_MONITOR)
static void monitor_task(void *arg)
{
    while (1) {
        audio_sys_get_real_time_stats();
        AUDIO_MEM_SHOW(TAG);
        vTaskDelay(10000 / portTICK_RATE_MS);
    }
}
#endif


void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

#if defined(ENABLE_TASK_MONITOR)
    audio_thread_create(NULL, "monitor_task", monitor_task, NULL, 4 * 1024, 1, true, 1);
#endif

    // init byte rtc engine
    qm_main();
}
