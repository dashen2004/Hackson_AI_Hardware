#ifndef _QM_AUDIO_BOARD_DEFINITION_H_
#define _QM_AUDIO_BOARD_DEFINITION_H_

/**
 * @brief  Audio Codec Chip Function Definition
 */
#define FUNC_AUDIO_CODEC_EN       (1)
#define CODEC_ADC_I2S_PORT        ((i2s_port_t)0)
#define CODEC_ADC_BITS_PER_SAMPLE ((i2s_data_bit_width_t)16)  /* 32bit */
#define CODEC_ADC_SAMPLE_RATE     (16000)
#define RECORD_HARDWARE_AEC       (true)
#define PA_ENABLE_GPIO            (GPIO_NUM_16)
#define HEADPHONE_DETECT          (-1)

#define AUDIO_ADC_INPUT_CH_NUM      (4)
#define AUDIO_SR_INPUT_CH_FORMAT    "MR"
#define AUDIO_VC_INPUT_CH_FORMAT    "MR"

// #define AUDIO_DIY_WAKENET           (1)

#endif
