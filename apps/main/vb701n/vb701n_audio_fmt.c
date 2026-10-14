#include "qm.h"
#include "qm_audio.h"

/*
 * 声网线上是 Opus。这里报给 agent 的格式固定是 Opus。
 * 芯片若是 PCM，agora_pcm_bridge 在 read/write 里编解码。
 */
const qm_audio_format_t g_in_fmt = {
    .codec = QM_AUDIO_CODEC_OPUS,
    .data_type = QM_AUDIO_CODEC_OPUS,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 20,
};
const qm_audio_format_t g_out_fmt = {
    .codec = QM_AUDIO_CODEC_OPUS,
    .data_type = QM_AUDIO_CODEC_OPUS,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 20,
};
