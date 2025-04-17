#include "qm.h"
#include "qm_work.h"

#include "bsp_audio.h"
#include "audio_main.h"
#include "audio_cloud.h"

static bsp_audio_cmd_table_t cmd_table[] = 
{
    {BSP_AUDIO_WAKEUP_CMD, CONFIG_AUDIO_ID_LIGHT_WAKEUP1, "ni hao xiao jian", DEAFALUT_REPLY_WAV,"你好小箭" , 0.5},
    {BSP_AUDIO_WAKEUP_CMD, CONFIG_AUDIO_ID_LIGHT_WAKEUP2, "xiao jian xiao jian", DEAFALUT_REPLY_WAV,"小箭小箭" , 0.5},
};

qm_err_t audio_cloud_init(void)
{
    bsp_audio_init(cmd_table, QM_ARRAY_SIZE(cmd_table));

    return QM_EOK;
}
