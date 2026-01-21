#include "esp_log.h"

#include "filter_resample.h"
#include "raw_stream.h"
#include "spiffs_stream.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "wav_decoder.h"
#include "gx8008.h"
#include "es8311.h"


#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
#include "raw_opus_encoder.h"
#include "raw_opus_decoder.h"
#elif defined (CONFIG_AUDIO_SUPPORT_AAC_DECODER)
#include "aac_encoder.h"
#include "aac_decoder.h"
#elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
#include "g711_encoder.h"
#include "g711_decoder.h"
#endif

#include "qm_kv.h"
#include "qm_audio_sr.h"
#include "qm_audio_vc.h"
#include "qm_audio_stream.h"
#include "qm_audio_board.h"
#include "qm_audio_board_def.h"

#include "esp_afe_config.h"
#include "esp_mn_iface.h"

#include "esp_spiffs.h"
#include "esp_vfs_fat.h"

#define CONFIG_BSP_SPIFFS_MAX_FILES         (5)

static const char *TAG = "gx8008_mic_array";

#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
#define SAMPLE_RATE         16000
#define BIT_RATE            25000
#define COMPLEXITY          0
#define FRAME_TIME_MS       20 
#endif

#if defined (CONFIG_AUDIO_SUPPORT_AAC_DECODER)
#define SAMPLE_RATE         16000
#define BIT_RATE            80000
#endif

#if defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
#define SAMPLE_RATE         8000
#endif

#define CHANNEL             1

qm_audio_stream_handle_t create_record_encoder_stream(void)
{
#if 0
    qm_audio_stream_handle_t encoder_stream = NULL;
#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
    raw_opus_enc_config_t opus_cfg = RAW_OPUS_ENC_CONFIG_DEFAULT();
    opus_cfg.sample_rate        = SAMPLE_RATE;
    opus_cfg.channel            = CHANNEL;
    opus_cfg.bitrate            = BIT_RATE;
    opus_cfg.complexity         = COMPLEXITY;
    opus_cfg.frame_duration     = RAW_OPUS_ENC_FRAME_DURATION_60_MS;
    encoder_stream = raw_opus_encoder_init(&opus_cfg);
#elif defined (CONFIG_AUDIO_SUPPORT_AAC_DECODER)
    aac_encoder_cfg_t aac_cfg = DEFAULT_AAC_ENCODER_CONFIG();
    aac_cfg.sample_rate        = SAMPLE_RATE;
    aac_cfg.channel            = CHANNEL;
    aac_cfg.bitrate            = BIT_RATE;
    encoder_stream = aac_encoder_init(&aac_cfg);
#elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
    g711_encoder_cfg_t g711_cfg = DEFAULT_G711_ENCODER_CONFIG();
    encoder_stream = g711_encoder_init(&g711_cfg);
#endif
    return encoder_stream;
#else
    return NULL;
#endif
}

#if 0
#define FLASH_EGV_KEY                                     "egv_flg"
#define FLASH_ALG_KEY                                     "alg_flg"

int audio_stream_EGV_set(uint8_t type) {
    int flag = type;
    if(type < 0 && type > GAIN_37_5DB) {
        return GAIN_34_5DB;
    }
    qm_kv_set(FLASH_EGV_KEY, &flag, 4, 1);
    return 0;
}

int audio_stream_EGV_get(void) {
    int type = 0;
    int size = 4;
    if(qm_kv_get(FLASH_EGV_KEY, &type, &size) == QM_EOK) {
        if(type < 0 && type > GAIN_37_5DB) {
            return GAIN_34_5DB;
        }
        return type;
    }
    return GAIN_34_5DB;
}

int audio_stream_ALG_set(float type) {
    int flag = (int)(type * 10);
     if(type < 0 && type > 10) {
        return 1;
    }
    qm_kv_set(FLASH_ALG_KEY, &flag, 4, 1);
    return 0;
}

float audio_stream_ALG_get(void) {
    int type = 0;
    int size = 4;
    if(qm_kv_get(FLASH_ALG_KEY, &type, &size) == QM_EOK) {
        if(type < 0 && type > 10) {
            return 5.0;
        }
        return ((float)type) / 10;
    }
    return 5.0;
}

#endif

qm_audio_stream_handle_t create_record_raw_stream(void)
{
    qm_audio_stream_handle_t raw_stream = NULL;
    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_WRITER;
    raw_cfg.out_rb_size = 8 * 1024;
    raw_stream = raw_stream_init(&raw_cfg);
    audio_element_set_output_timeout(raw_stream, portMAX_DELAY);
    return raw_stream;
}

qm_audio_stream_handle_t create_record_i2s_stream(void)
{
    es8311_codec_set_voice_volume(70);
    
    qm_audio_stream_handle_t i2s_stream = NULL;
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(I2S_NUM_1, CODEC_ADC_SAMPLE_RATE, CODEC_ADC_BITS_PER_SAMPLE, AUDIO_STREAM_READER);
    i2s_cfg.stack_in_ext = 1;
    i2s_cfg.out_rb_size = 8 * 1024;
    i2s_cfg.std_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT;
    i2s_cfg.std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    i2s_cfg.std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    i2s_cfg.std_cfg.slot_cfg.ws_width = 32;
    i2s_cfg.std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
    i2s_cfg.std_cfg.slot_cfg.bit_shift = true;
    i2s_cfg.chan_cfg.role = I2S_ROLE_SLAVE;
    i2s_cfg.chan_cfg.dma_desc_num = 6;
    
    i2s_stream = i2s_stream_init(&i2s_cfg);

    return i2s_stream;
}

qm_audio_stream_handle_t create_player_decoder_stream(void)
{
    qm_audio_stream_handle_t decoder_stream = NULL;
#ifdef CONFIG_AUDIO_SUPPORT_OPUS_DECODER
    ESP_LOGI(TAG, "Create opus decoder");
    raw_opus_dec_cfg_t opus_dec_cfg = RAW_OPUS_DEC_CONFIG_DEFAULT();
    opus_dec_cfg.enable_frame_length_prefix = true;
    opus_dec_cfg.sample_rate = 16000;
    opus_dec_cfg.channels = 1;
    decoder_stream = raw_opus_decoder_init(&opus_dec_cfg);
#elif CONFIG_AUDIO_SUPPORT_AAC_DECODER
    ESP_LOGI(TAG, "Create aac decoder");
    aac_decoder_cfg_t  aac_dec_cfg  = DEFAULT_AAC_DECODER_CONFIG();
    aac_dec_cfg.task_core = 1;
    decoder_stream = aac_decoder_init(&aac_dec_cfg);
#elif CONFIG_AUDIO_SUPPORT_G711A_DECODER
    ESP_LOGI(TAG, "Create g711a decoder");
    g711_decoder_cfg_t g711_dec_cfg = DEFAULT_G711_DECODER_CONFIG();
    g711_dec_cfg.out_rb_size = 8 * 1024;
    g711_dec_cfg.task_core = 1;
    g711_dec_cfg.task_prio = 4;
    decoder_stream = g711_decoder_init(&g711_dec_cfg);
#endif
    return decoder_stream;
}

qm_audio_stream_handle_t create_player_wav_decoder_stream()
{
    qm_audio_stream_handle_t decoder_stream = NULL;
    wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();
    wav_cfg.out_rb_size = 2 * 1024;
    decoder_stream = wav_decoder_init(&wav_cfg);
    return decoder_stream;
}

qm_audio_stream_handle_t create_player_raw_stream(void)
{
    qm_audio_stream_handle_t raw_stream = NULL;
    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    raw_cfg.out_rb_size = 8 * 1024;
    raw_stream = raw_stream_init(&raw_cfg);
    return raw_stream;
}

qm_audio_stream_handle_t create_player_i2s_stream(bool_t enable_task)
{
    qm_audio_stream_handle_t i2s_stream = NULL;
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(I2S_NUM_0, CODEC_ADC_SAMPLE_RATE, CODEC_ADC_BITS_PER_SAMPLE, AUDIO_STREAM_WRITER);
    if (enable_task == false) {
        i2s_cfg.task_stack = -1;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs_data",
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
        .format_if_mount_failed = QM_TRUE,
    };
    
    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);
    ESP_ERROR_CHECK(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != QM_EOK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    i2s_cfg.stack_in_ext = 1;
    i2s_cfg.out_rb_size = 8 * 1024;
    i2s_cfg.need_expand = true;
    i2s_cfg.expand_src_bits = 16;
    i2s_cfg.buffer_len = 1416;
    i2s_cfg.chan_cfg.dma_desc_num = 6;
    i2s_stream = i2s_stream_init(&i2s_cfg);
    return i2s_stream;
}

qm_audio_stream_handle_t create_8k_ch1_to_16k_ch2_rsp_stream()
{
    qm_audio_stream_handle_t filter = NULL;
    rsp_filter_cfg_t filter_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    filter_cfg.src_ch = 1;
    filter_cfg.src_rate = 8000;
    filter_cfg.dest_ch = 2;
    filter_cfg.dest_rate = 16000;
    filter_cfg.stack_in_ext = true;
    filter_cfg.task_core = 1;
    filter_cfg.complexity = 5;
    filter = rsp_filter_init(&filter_cfg);
    return filter;
}

qm_audio_stream_handle_t create_ch1_to_ch2_rsp_stream() // create_audio_player_rsp_stream
{
    qm_audio_stream_handle_t filter = NULL;
    rsp_filter_cfg_t filter_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    filter_cfg.src_ch = 1;
    filter_cfg.src_rate = 16000;
    filter_cfg.dest_ch = 2;
    filter_cfg.dest_rate = 16000;
    filter_cfg.stack_in_ext = true;
    filter_cfg.task_core = 1;
    filter_cfg.complexity = 5;
    filter_cfg.task_prio = 23;
    filter = rsp_filter_init(&filter_cfg);
    return filter;
}

int audio_play_stream_mute(bool_t enable)
{
    // nc_audio_set_mute(enable);
    return QM_EOK;
}

qm_audio_sr_cfg_t *get_default_audio_sr_config(void)
{
    static qm_audio_sr_cfg_t recorder_sr_cfg = {0};
#if 1
    qm_audio_sr_cfg_t sr_cfg = DEFAULT_QM_AUDIO_SR_CFG(AUDIO_SR_INPUT_CH_FORMAT, "model", AFE_MODE_LOW_COST);
 
    memcpy(&recorder_sr_cfg, &sr_cfg, sizeof(qm_audio_sr_cfg_t));

#if AUDIO_DIY_WAKENET
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->wakenet_init = false;
#endif
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->aec_init = true;
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->aec_mode = AEC_MODE_SR_LOW_COST;

    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->wakenet_mode = DET_MODE_90;
 
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->vad_init = true;
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->vad_mode = VAD_MODE_2;
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->vad_min_noise_ms = 500;

    recorder_sr_cfg.multinet_init = false;

#if AUDIO_DIY_WAKENET
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->afe_linear_gain = 10.0;
#endif
    // ((afe_config_t* )recorder_sr_cfg.afe_cfg)->afe_ringbuf_size = 100;
 
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->afe_perferred_core = 1;
    ((afe_config_t* )recorder_sr_cfg.afe_cfg)->afe_perferred_priority = 5;

    return &recorder_sr_cfg;
#else
    return NULL;
#endif
}

qm_audio_vc_cfg_t *get_default_audio_vc_config(void)
{
#if 1
    static qm_audio_vc_cfg_t recorder_vc_cfg = {0};

    srmodel_list_t *models = esp_srmodel_init("model");
    char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);
    
    qm_audio_vc_cfg_t vc_cfg = DEFAULT_QM_AUDIO_VC_CFG(AUDIO_VC_INPUT_CH_FORMAT, AFE_MODE_HIGH_PERF);
 
    memcpy(&recorder_vc_cfg, &vc_cfg, sizeof(qm_audio_vc_cfg_t));

    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->aec_init = true;
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->aec_mode = AEC_MODE_VOIP_HIGH_PERF;

    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->vad_init = true;
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->vad_mode = VAD_MODE_2;
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->vad_min_speech_ms = 200;
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->vad_min_noise_ms = 100;

    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->ns_init = true;
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->ns_model_name = ns_model_name;
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->afe_ns_mode = AFE_NS_MODE_NET;

    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->afe_ringbuf_size = 100;

    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->afe_perferred_core = 1;
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->afe_perferred_priority = 5;

    // ((afe_config_t* )recorder_vc_cfg.afe_cfg)->afe_linear_gain = audio_stream_ALG_get();
    ((afe_config_t* )recorder_vc_cfg.afe_cfg)->afe_linear_gain = 5.0;

    return &recorder_vc_cfg;
#else
    return NULL;
#endif
}

qm_err_t short_ich_to_och(int16_t *i_buf, int chunksize, int i_ch, int o_ch)
{
    if(i_ch == o_ch){
        return QM_EOK;
    }

    for (int  i = 0; i < chunksize; i++) {
        i_buf[i * o_ch + 1] =i_buf[i * i_ch + 0];
        i_buf[i * o_ch + 0] =i_buf[i * i_ch + 3];
    }
    
    return QM_EOK;
}
