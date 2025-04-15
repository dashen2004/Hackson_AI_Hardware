#ifndef _QM_AUDIO_BOARD_DEFINITION_H_
#define _QM_AUDIO_BOARD_DEFINITION_H_

/**
 * @brief  Audio Codec Chip Function Definition
 */
#define FUNC_AUDIO_CODEC_EN       (1)
#define CODEC_ADC_I2S_PORT        ((i2s_port_t)0)
#define CODEC_ADC_BITS_PER_SAMPLE ((i2s_data_bit_width_t)32)  /* 32bit */
#define CODEC_ADC_SAMPLE_RATE     (16000)
#define RECORD_HARDWARE_AEC       (true)
#define BOARD_PA_GAIN             (0)  /* Power amplifier gain defined by board (dB) */
#define PA_ENABLE_GPIO            (GPIO_NUM_46)
#define HEADPHONE_DETECT          (-1)
#define ES7210_MIC_SELECT         (ES7120_SEL_MIC1 | ES7120_SEL_MIC2 | ES7120_SEL_MIC3 | ES7120_SEL_MIC4)

#define AUDIO_ADC_INPUT_CH_NUM      (4)
#define AUDIO_SR_INPUT_CH_FORMAT    "MR"
#define AUDIO_VC_INPUT_CH_FORMAT    "MR"

#define AUDIO_DIY_WAKENET           (1)


static __attribute__((always_inline)) inline qm_err_t short_ich_to_och(int16_t *i_buf, int chunksize, int i_ch, int o_ch)
{

    for (int  i = 0; i < chunksize; i++) {
        i_buf[i * o_ch + 1] =i_buf[i * i_ch + 0];
        i_buf[i * o_ch + 0] =i_buf[i * i_ch + 3];
    }
    
    return QM_EOK;
}

#endif
