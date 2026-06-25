#include "ui_image_preload.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"
#include "draw/lv_image_decoder_private.h"

#define LOG_TAG "ui_preload"

static void *ui_img_buf_malloc(size_t size_bytes, lv_color_format_t color_format)
{
    LV_UNUSED(color_format);
    return heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, size_bytes,
                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static void ui_img_buf_free(void *buf)
{
    heap_caps_free(buf);
}

void ui_image_cache_use_psram(void)
{
    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_image_handlers();
    lv_draw_buf_handlers_init(handlers,
                              ui_img_buf_malloc,
                              ui_img_buf_free,
                              lv_draw_buf_align,
                              NULL,
                              NULL,
                              lv_draw_buf_width_to_stride);
}

static void ui_preload_image(const char *path)
{
    lv_image_decoder_dsc_t dsc;

    if (lv_image_decoder_open(&dsc, path, NULL) != LV_RESULT_OK) {
        ESP_LOGW(LOG_TAG, "preload failed: %s", path);
        return;
    }

    lv_image_decoder_close(&dsc);
}

void ui_preload_level1_images(void)
{
    static const char *const paths[] = {
        "/lvgl/images/icon_main_s.bin",
        "/lvgl/images/icon_ybjc.bin",
        "/lvgl/images/icon_tomato.bin",
        "/lvgl/images/icon_alarm.bin",
        "/lvgl/images/icon_timer.bin",
        "/lvgl/images/icon_timetable.bin",
        "/lvgl/images/icon_checklist.bin",
        "/lvgl/images/icon_settings.bin",
        "/lvgl/images/wifi.bin",
        "/lvgl/images/bat.bin",
    };

    if (!lv_image_cache_is_enabled()) {
        ESP_LOGW(LOG_TAG, "image cache disabled, skip preload");
        return;
    }

    ESP_LOGD(LOG_TAG, "preloading %u images", (unsigned)(sizeof(paths) / sizeof(paths[0])));
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        ui_preload_image(paths[i]);
    }
}
