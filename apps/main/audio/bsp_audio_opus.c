#include "qm.h"
#include "opus_defines.h"
#include "opus_encoder.h"

#include "opus.h"

#include "bsp_audio_opus.h"

#define TAG "OpusEncoderWrapper"

typedef struct 
{
    int sample_rate;
    int frame_size;
    int duration_ms;
    qm_mutex_t lock;
    struct OpusEncoder* audio_enc;
    struct OpusDecoder* audio_dec_;
}bsp_audio_opus_handle_t;

void *bsp_audio_opus_encode_create(int sample_rate, int channels, int duration_ms)
{
    int error;
    bsp_audio_opus_handle_t *handle = (bsp_audio_opus_handle_t *)qm_calloc(1, sizeof(bsp_audio_opus_handle_t));
    if(handle == NULL){
        return NULL;
    }
    
    qm_mutex_new(&handle->lock);

    handle->audio_enc = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &error);
    if (handle->audio_enc == NULL) {
        QM_LOGE(TAG, "Failed to create audio encoder, error code: %d", error);
        goto __error;
    }

    handle->audio_dec_ = opus_decoder_create(sample_rate, channels, &error);
    if (handle->audio_dec_ == NULL) {
        QM_LOGE(TAG, "Failed to create audio decoder, error code: %d", error);
        goto __error;
    }

    // Default DTX enabled
    opus_encoder_ctl(handle->audio_enc, OPUS_SET_DTX(1));
    // Complexity 5 almost uses up all CPU of ESP32C3
    opus_encoder_ctl(handle->audio_enc, OPUS_SET_COMPLEXITY(0));

    handle->duration_ms = duration_ms;
    handle->sample_rate = sample_rate;
    handle->frame_size = sample_rate / 1000 * channels * duration_ms;

    return handle;

__error:
    return NULL;
}

int bsp_audio_opus_decode_data(void *handle, void *in_opus, int opus_size, void *out_pcm)
{
    int encode_len = 0;
    bsp_audio_opus_handle_t *encode_handle = (bsp_audio_opus_handle_t *)handle;
    if(encode_handle == NULL || encode_handle->audio_enc == NULL){
        return -QM_EINVAL;
    }

    encode_len = opus_decode(encode_handle->audio_dec_, in_opus, opus_size, out_pcm, encode_handle->frame_size, 0);
    if (encode_len < 0) {
        QM_LOGE(TAG, "Failed to DEcode audio, error code: %ld", encode_len);
        return 0;
    }

    return encode_len * sizeof(int16_t);
}


int bsp_audio_opus_encode_data(void *handle, void *in_pcm, void *out_opus)
{
    int encode_len = 0;
    bsp_audio_opus_handle_t *encode_handle = (bsp_audio_opus_handle_t *)handle;
    if(encode_handle == NULL || encode_handle->audio_enc == NULL){
        return -QM_EINVAL;
    }

    encode_len = opus_encode(encode_handle->audio_enc, in_pcm, encode_handle->frame_size, out_opus, MAX_OPUS_PACKET_SIZE);
 
    if (encode_len < 0) {
        QM_LOGE(TAG, "Failed to encode audio, error code: %ld", encode_len);
        return 0;
    }

    return encode_len;
}

qm_err_t bsp_audio_opus_reset_status(void *handle)
{
    bsp_audio_opus_handle_t *encode_handle = (bsp_audio_opus_handle_t *)handle;
    if(encode_handle == NULL || encode_handle->audio_enc == NULL){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&encode_handle->lock, QM_WAIT_FOREVER);
    opus_encoder_ctl(encode_handle->audio_enc, OPUS_RESET_STATE);
    qm_mutex_unlock(&encode_handle->lock);

    return QM_EOK;
}

qm_err_t bsp_audio_opus_set_complexity(void *handle, int complexity)
{
    return QM_EOK;
}