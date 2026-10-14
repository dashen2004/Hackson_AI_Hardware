#ifndef AGENT_PCM_DRV_H
#define AGENT_PCM_DRV_H

#include "qm_audio.h"

/*
 * 芯片是 PCM、声网是 Opus 时套在 vb701n 驱动外面。
 * read 把麦的 PCM 编成 Opus，write 把 Opus 解成 PCM 再交给芯片。
 */
extern const qm_audio_driver_if_t pcm_driver;

#endif
