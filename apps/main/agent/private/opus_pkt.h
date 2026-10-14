#ifndef AGENT_OPUS_PKT_H
#define AGENT_OPUS_PKT_H

#include <stdint.h>

/*
 * 芯片是 PCM 时，由 pcm_drv 调用。
 * 返回值：>0 字节数，0 还凑不齐一帧，<0 失败。
 */
int opus_pcm_to_packet(const uint8_t *pcm, uint32_t pcm_len,
                       uint8_t *opus, uint32_t opus_cap);

int opus_packet_to_pcm(const uint8_t *opus, uint32_t opus_len,
                       uint8_t *pcm, uint32_t pcm_cap);

#endif
