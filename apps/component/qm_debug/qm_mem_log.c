#include "qm_mem_log.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#define LOG_TAG                "mem"
#define PERIODIC_INTERVAL_US   (10 * 1000 * 1000)

static esp_timer_handle_t s_periodic_timer;
static bool s_periodic_started;
static uint32_t s_internal_min;

static uint32_t mem_size(uint32_t caps)
{
    return (uint32_t)heap_caps_get_free_size(caps);
}

static uint32_t mem_total(uint32_t caps)
{
    return (uint32_t)heap_caps_get_total_size(caps);
}

static uint32_t mem_largest(uint32_t caps)
{
    return (uint32_t)heap_caps_get_largest_free_block(caps);
}

static void periodic_timer_cb(void *arg)
{
    (void)arg;
    qm_mem_log_print("periodic");
}

void qm_mem_log_init(void)
{
    if (s_periodic_started) {
        return;
    }

    const esp_timer_create_args_t args = {
        .callback = periodic_timer_cb,
        .name = "mem_log",
    };

    if (esp_timer_create(&args, &s_periodic_timer) != ESP_OK) {
        ESP_LOGW(LOG_TAG, "periodic timer create failed");
        return;
    }

    if (esp_timer_start_periodic(s_periodic_timer, PERIODIC_INTERVAL_US) != ESP_OK) {
        ESP_LOGW(LOG_TAG, "periodic timer start failed");
        return;
    }

    s_periodic_started = true;
}

void qm_mem_log_print(const char *stage)
{
    const char *tag = (stage != NULL) ? stage : "?";
    uint32_t internal_free = mem_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (s_internal_min == 0 || internal_free < s_internal_min) {
        s_internal_min = internal_free;
    }

    ESP_LOGI(LOG_TAG,
             "[%s] heap free=%u min=%u | "
             "internal total=%u free=%u min=%u largest=%u | "
             "psram total=%u free=%u largest=%u | "
             "dma free=%u largest=%u",
             tag,
             (unsigned)esp_get_free_heap_size(),
             (unsigned)esp_get_minimum_free_heap_size(),
             (unsigned)mem_total(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)internal_free,
             (unsigned)s_internal_min,
             (unsigned)mem_largest(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)mem_total(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             (unsigned)mem_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             (unsigned)mem_largest(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             (unsigned)mem_size(MALLOC_CAP_DMA | MALLOC_CAP_8BIT),
             (unsigned)mem_largest(MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
}
