#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Local LAN transport. Does not own the camera or call the inference model. */
esp_err_t camera_web_init(void);
void camera_web_submit_uyvy(const uint8_t *data, size_t size, uint32_t frame_id,
                            uint32_t valid, uint32_t invalid);
