#ifndef _QM_AUDIO_BOARD_DEFINITION_H_
#define _QM_AUDIO_BOARD_DEFINITION_H_

/**
 * @brief Audio Codec Chip Function Definition
 */
#define FUNC_AUDIO_CODEC_EN         (1)
#define ES8311_MCLK_SOURCE          (1)   /* 0 From MCLK of esp32   1 From BCLK */
#define HEADPHONE_DETECT            (-1)
#define PA_ENABLE_GPIO              GPIO_NUM_46
#define CODEC_ADC_I2S_PORT          ((i2s_port_t)0)
#define CODEC_ADC_BITS_PER_SAMPLE   ((i2s_data_bit_width_t)16) /* 16bit */
#define CODEC_ADC_SAMPLE_RATE       (16000)
#define RECORD_HARDWARE_AEC         (true)
#define BOARD_PA_GAIN               (1) /* Power amplifier gain defined by board (dB) */

/**
 * @brief ADC input data format
 */
#define AUDIO_ADC_INPUT_CH_FORMAT "MR"

#endif
