/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _QM_COLOR_H_
#define _QM_COLOR_H_

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RGB value
 */
typedef struct {
    union {
        uint8_t r;        /*!< The red value */
        uint8_t red;      /*!< The red value */
    };
    union {
        uint8_t g;        /*!< The green value */
        uint8_t green;    /*!< The green value */
    };
    union {
        uint8_t b;        /*!< The blue value */
        uint8_t blue;     /*!< The blue value */
    };
} qm_color_rgb_t;

/**
 * @brief HSV value
 */
typedef struct {
    int16_t h;      /*!< The hue value */
    int8_t  s;      /*!< The saturation value */
    int8_t  v;      /*!< The value */
} qm_color_hsv_t;

/**
 * @brief      HSV to RGB
 *
 * @param[in]  hsv    HSV value
 * @param[out] rgb    RGB value
 *
 * @return
 *     - ESP_OK
 */
qm_err_t qm_color_hsv2rgb(qm_color_hsv_t *hsv, qm_color_rgb_t *rgb);

/**
 * @brief      RGB to HSV
 *
 * @param[in]  rgb    RGB value
 * @param[out] hsv    HSV value
 *
 * @return
 *     - ESP_OK
 */
qm_err_t qm_color_rgb2hsv(qm_color_rgb_t *rgb, qm_color_hsv_t *hsv);

/**
 * @brief      Parse color codes as R, G, B values
 *
 * @param[in]  colorcode    Colorcode
 * @param[out] color        Color.r, Color.g, Color.b
 *
 * @return
 *     - ESP_OK
 */
qm_err_t qm_color_code2rgb(uint32_t colorcode, qm_color_rgb_t *color);

qm_err_t qm_color_rgb2code(qm_color_rgb_t color, uint32_t *colorcode);

/**
 * @brief      Cycle through colors using HSV
 *
 * @param[out] color    RGB value after update
 *
 * @return
 *     - ESP_OK
 */
qm_err_t qm_color_update_rainbow(qm_color_rgb_t *color);

qm_err_t qm_color_rgb2value(qm_color_rgb_t *color, qm_color_rgb_t *start_color, qm_color_rgb_t *end_color, 
                            int32_t min_value, int32_t max_value, int32_t *val);

qm_err_t qm_color_value2rgb(int32_t val, int32_t min_value, int32_t max_value,
                            qm_color_rgb_t start_color, qm_color_rgb_t end_color, qm_color_rgb_t *color);

#ifdef __cplusplus
}
#endif
#endif // _QM_COLOR_H_
