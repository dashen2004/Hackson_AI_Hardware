#include "qm.h"
#include "qm_work.h"
#include "qm_player.h"

extern const uint8_t dingding_file[23898];

bsp_pcm_file_t bsp_pcm_file[BSP_PCM_ID_MAX] =
{
    [BSP_PCM_ID_DINGDING]={23898, (uint8_t *)dingding_file},
};


