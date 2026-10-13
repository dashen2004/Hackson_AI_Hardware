#include "qm.h"
#include "qm_audio.h"

/* 与 vb701n_player.c 用同一个默认值。改的时候两处一起改，或用 -D 覆盖。 */
#ifndef VB701N_APP_PCM
#define VB701N_APP_PCM  1
#endif

#if CONFIG_VOICE_BACKEND_JOYINSIDE

#if VB701N_APP_PCM
/*
 * 协议：上行 512 字节/16 ms，下行 320 字节/10 ms。
 * JoyInside 的 PCM 下行只接受 60..120 ms，按 60 ms 要，驱动再切成 10 ms。
 */
const qm_audio_format_t g_in_fmt = {
    .codec = QM_AUDIO_CODEC_PCM,
    .data_type = QM_AUDIO_CODEC_PCM,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 16,
};
const qm_audio_format_t g_out_fmt = {
    .codec = QM_AUDIO_CODEC_PCM,
    .data_type = QM_AUDIO_CODEC_PCM,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 60,
};
#else
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
#endif

#elif CONFIG_VOICE_BACKEND_AGORA

/*
 * 上下行都是 PCM：16 kHz / 16 bit / 单声道 / 20 ms。
 * 20 ms = 640 字节，等于采集缓冲上限。
 * qm_agent 看到 codec=PCM 会让 SDK 用 Opus 收发，本地帧仍是 PCM。
 */
const qm_audio_format_t g_in_fmt = {
    .codec = QM_AUDIO_CODEC_PCM,
    .data_type = QM_AUDIO_CODEC_PCM,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 20,
};
const qm_audio_format_t g_out_fmt = {
    .codec = QM_AUDIO_CODEC_PCM,
    .data_type = QM_AUDIO_CODEC_PCM,
    .sample_rate = 16000,
    .channel_num = 1,
    .bits_per_sample = 16,
    .frame_ms = 20,
};

#endif
