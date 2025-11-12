#include "qm.h"
#include "qm_work.h"

#include "bsp_pcm.h"
#include "bsp_audio_opus.h"

extern const uint8_t opus_dingding_file[1564];
extern const uint8_t opus_conn_file[2764];
extern const uint8_t opus_disconn_file[4604];
extern const uint8_t opus_mode_1_file[4284];        // AI对话模式
extern const uint8_t opus_mode_2_file[4124];        // 安抚模式
extern const uint8_t opus_mode_3_file[3884];        // 录音模式
extern const uint8_t power_on_file[15164];
extern const uint8_t ready_sleep_file[6924];
extern const uint8_t opus_reassure_file[403404];
extern const uint8_t discon_remind_file[5324];
extern const uint8_t low_battery_level_file[5564];
extern const uint8_t loss_rssi_file[5964];
extern const uint8_t sleep_file[4844];
extern const uint8_t xj_test_file[3164];

bsp_pcm_file_t bsp_pcm_file[BSP_PCM_ID_MAX] =
{
    [BSP_PCM_ID_NET_CONN]={2764, (uint8_t *)opus_conn_file},
    [BSP_PCM_ID_NET_DISCONN]={4604, (uint8_t *)opus_disconn_file},
    [BSP_PCM_ID_MODE_1]={4284, (uint8_t *)opus_mode_1_file},
    [BSP_PCM_ID_MODE_2]={4124, (uint8_t *)opus_mode_2_file},
    [BSP_PCM_ID_MODE_3] = {3884, (uint8_t *)opus_mode_3_file},
    [BSP_PCM_ID_DING_DING]={1564, (uint8_t *)opus_dingding_file},
    [BSP_PCM_ID_POWER_ON]={15164, (uint8_t *)power_on_file},
    [BSP_PCM_ID_READY_SLEEP]={6924, (uint8_t *)ready_sleep_file},
    [BSP_PCM_ID_REASSURE]={403404, (uint8_t *)opus_reassure_file},
    [BSP_PCM_ID_SLEEP]={4844, (uint8_t *)sleep_file},
    [BSP_PCM_ID_DISCONN_REMIND]={5324, (uint8_t *)discon_remind_file},
    [BSP_PCM_ID_LOW_BAT_LEVEL]={5564, (uint8_t *)low_battery_level_file},
    [BSP_PCM_ID_LOSS_RSSI]={5964, (uint8_t *)loss_rssi_file},
    [BSP_PCM_ID_XJIANG_TEST]={3164, (uint8_t *)xj_test_file},
};


