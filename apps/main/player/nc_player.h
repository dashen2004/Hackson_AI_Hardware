#ifndef __NC_PLAYER_H__
#define __NC_PLAYER_H__

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "qm_audio.h"
#include "nc_audio.h"
#include "bsp_pcm.h"
#include "qm_agora_stream.h"

typedef void (*nc_player_event_t)(bsp_pcm_id_t id, qm_audio_event_t event_id, void *user_data);

typedef struct {
    void *user_data;
    nc_player_event_t event_cb;
    nc_audio_event_t notify;
} nc_player_param_t;

void nc_player_init(nc_player_param_t player_param);
void nc_player_event_regesiter(nc_player_event_t event_cb);

void nc_audio_play_bin(bsp_pcm_id_t pcm_id);
uint8_t nc_audio_play_is_idle(void);
char *nc_audio_get_version(void);

qm_err_t nc_audio_set_volume(int volume);
int nc_audio_get_volume(void);

int32_t nc_audio_set_spk_on_off(bool onoff);
int32_t nc_audio_set_mic_on_off(bool onoff);
int32_t nc_audio_set_wakeup_on_off(bool onoff);
int32_t nc_audio_set_wakeup_state_on_off(bool onoff);
int32_t nc_audio_set_mute(bool mute);
bool nc_audio_get_mute(void);
//void qm_agora_set_agent_state(qm_agent_state_t state, const char *text);

#ifdef __cplusplus
}
#endif

#endif
