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
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
// #include "audio_sys.h"
// #include "audio_thread.h"
// #include "audio_mem.h"

#include "qm.h"
#include "ai_main.h"
#include "bsp_camera.h"

#include "esp_bt.h"
#include "esp_bt_main.h"

// #define ENABLE_TASK_MONITOR

static char *TAG = "main";

static void log_memory_once(const char *stage)
{
    ESP_LOGI(TAG,
             "mem@%s: internal total=%u free=%u largest=%u | psram total=%u free=%u largest=%u",
             stage,
             (unsigned)heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}

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
    if (bsp_camera_early_init() != QM_EOK) {
        ESP_LOGW(TAG, "camera early init failed, continue without camera");
    }
    if (bsp_camera_stream_prepare() != QM_EOK) {
        ESP_LOGW(TAG, "camera stream prepare failed, continue without camera");
    }
    ESP_ERROR_CHECK(esp_netif_init());
    log_memory_once("startup");

    esp_log_level_set("AUDIO_THREAD", ESP_LOG_ERROR);
    esp_log_level_set("i2c_bus_v2", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_HAL", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_ERROR);
    // esp_log_level_set("I2S_STREAM_IDF5.x", ESP_LOG_ERROR);
    esp_log_level_set("RSP_FILTER", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_EVT", ESP_LOG_ERROR);
    
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

#if defined(ENABLE_TASK_MONITOR)
    audio_thread_create(NULL, "monitor_task", monitor_task, NULL, 4 * 1024, 1, true, 1);
#endif

    // init byte rtc engine
    qm_main();
}
