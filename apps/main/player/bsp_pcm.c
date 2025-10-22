#include "qm.h"
#include "qm_work.h"

#include "bsp_pcm.h"
#include "audio_thread.h"

// #include "bsp_audio_opus.h"

// #include "opus_defines.h"
// #include "opus.h"

#define MAX_OPUS_PACKET_SIZE 1920

typedef struct 
{
    void *user_data;

    bsp_pcm_mute_t mute;
    bsp_pcm_send_t send;
    bsp_pcm_event_t event;

    int enable_opus;
    int channels;
    int sample_rate;
    int duration_ms;

    audio_thread_t task;

    bsp_pcm_id_t pcm_id;

    void *opus_handle;

    struct OpusEncoder* audio_enc;

    int exist;
    qm_sem_t wait_sem;
}bsp_pcm_handle_t;

static bsp_pcm_handle_t g_bsp_pcm_handle = {0};


#define OPUS_OFFSET (44)


static void bsp_pcm_process_task(void *arg)
{
    bsp_pcm_handle_t *bsp_pcm_handle = (bsp_pcm_handle_t *)arg;
    
    int offset = OPUS_OFFSET;
   
    bsp_pcm_file_t *opus = &bsp_pcm_file[bsp_pcm_handle->pcm_id];

    bsp_pcm_handle->mute(0);
    
    if(bsp_pcm_handle->event){
        bsp_pcm_handle->event(bsp_pcm_handle->pcm_id, BSP_PCM_EVENT_ID_PLAY_START, bsp_pcm_handle->user_data);
    }
    
    bsp_pcm_handle->exist = 1;
    QM_LOGD("1", "START");
    while (offset < opus->file_len)
    {

        if(!bsp_pcm_handle->exist){
            goto __exit;
        }

        if(bsp_pcm_handle->event){
            bsp_pcm_handle->event(bsp_pcm_handle->pcm_id, BSP_PCM_EVENT_ID_PLAY_DATA, bsp_pcm_handle->user_data);
        }

        bsp_pcm_handle->send(opus->file + offset, 80);

        offset += 80;
    }

    bsp_pcm_handle->mute(1);

    if(bsp_pcm_handle->event){
        bsp_pcm_handle->event(bsp_pcm_handle->pcm_id, BSP_PCM_EVENT_ID_PLAY_STOP, bsp_pcm_handle->user_data);
    }

__exit:
    QM_LOGD("1", "END");

    if(!bsp_pcm_handle->exist){
       qm_sem_signal(&bsp_pcm_handle->wait_sem); 
    }
    bsp_pcm_handle->exist = 0;
    audio_thread_delete_task(NULL);
}

qm_err_t bsp_pcm_init(bsp_pcm_params_t *params)
{
    g_bsp_pcm_handle.user_data = params->user_data;

    g_bsp_pcm_handle.event = params->event;
    g_bsp_pcm_handle.mute = params->mute;
    g_bsp_pcm_handle.send = params->send;
    g_bsp_pcm_handle.channels = params->channels;
    g_bsp_pcm_handle.duration_ms = params->duration_ms;
    g_bsp_pcm_handle.sample_rate = params->sample_rate;
    g_bsp_pcm_handle.enable_opus = params->enable_opus;

    if(params->enable_opus){

    }

    qm_sem_new(&g_bsp_pcm_handle.wait_sem, 0);

    return QM_EOK;
}

qm_err_t bsp_pcm_player(bsp_pcm_id_t pcm_id)
{
    if(pcm_id == BSP_PCM_ID_MAX){
        return -QM_EINVAL;
    }

    if(g_bsp_pcm_handle.exist){
        g_bsp_pcm_handle.exist = 0;
        qm_sem_wait(&g_bsp_pcm_handle.wait_sem, 5000);
    }

    g_bsp_pcm_handle.pcm_id = pcm_id;

    audio_thread_create(&g_bsp_pcm_handle.task, "pcm", bsp_pcm_process_task, 
            (void *)&g_bsp_pcm_handle, CONFIG_BSP_PCM_TASK_SIZE, CONFIG_BSP_PCM_TASK_PRIO, true, 1);

    return QM_EOK;
}