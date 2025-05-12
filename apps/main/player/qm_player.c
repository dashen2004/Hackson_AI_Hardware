#include "qm.h"
#include "qm_player.h"
#include "qm_player_dev.h"
#include "qm_work.h"

typedef struct {
    int player_status;
    qm_player_dev_t player_dev;
    qm_player_event_t event_fn;
}qm_player_ctx_t;

static void audio_player_dev_event_handler(qm_player_t handle, qm_player_event_type_t audio_event, void *arg)
{
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return ;
    }

    play_ctx->player_status = audio_event;

    if(play_ctx->event_fn == NULL){
        return ;
    }

    switch (audio_event)
    {
        case QM_PLAYER_EVENT_IDLE:
            play_ctx->event_fn(handle, audio_event, arg);
        break;
        
        case QM_PLAYER_EVENT_COMPLETED_PLAYING_NEXT:
            play_ctx->event_fn(handle, audio_event, arg);
        break;
        
        case QM_PLAYER_EVENT_PLAYING:
            play_ctx->event_fn(handle, audio_event, arg);
        break;
        
        case QM_PLAYER_EVENT_PAUSE:
            play_ctx->event_fn(handle, audio_event, arg);
        break;
        
        case QM_PLAYER_EVENT_SHUTDOWN:
            play_ctx->event_fn(handle, audio_event, arg);
        break;
        
        default:
        break;
    }
}

int qm_player_dev_regsiter(qm_player_t handle, qm_player_dev_ops_t *ops, void *priv)
{
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }

    play_ctx->player_dev.ops = ops;
    play_ctx->player_dev.priv = priv;
    play_ctx->player_dev.handle = handle;

    return QM_EOK;
}

qm_player_t qm_player_init(void)
{
    qm_err_t ret = QM_EOK;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)qm_malloc(sizeof(qm_player_ctx_t));
    if(play_ctx == NULL){
        return NULL;
    }
    memset(play_ctx, 0, sizeof(qm_player_ctx_t));

    qm_hal_audio_player_init((qm_player_t)play_ctx);

    play_ctx->player_dev.event_handler = audio_player_dev_event_handler;

    ret = play_ctx->player_dev.ops->init(&play_ctx->player_dev);
    if(ret != QM_EOK){
        qm_free(play_ctx);
        return NULL;
    }

    return (qm_player_t)play_ctx;
}

qm_err_t qm_player_deinit(qm_player_t handle)
{
    qm_err_t ret = QM_EOK;
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->deinit == NULL){
        return -QM_EINVAL;
    }

    ret = player_dev->ops->deinit(player_dev);
    if(ret != QM_EOK){
        return ret;
    }

    qm_free(play_ctx);

    return ret;
}

qm_err_t qm_player_setopt(qm_player_t handle, qm_player_option_t option, void *data)
{
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL || data == NULL){
        return -QM_EINVAL;
    }

    switch (option)
    {

        case QM_PLAYER_OPT_EVENT_HANDLER:
            play_ctx->event_fn = (qm_player_event_t)data;
        break;

        case QM_PLAYER_OPT_USERDATA:
            play_ctx->player_dev.arg = data;
        break;

        default:

        break;
    }

    return QM_EOK;
}

qm_err_t qm_player_play_file(qm_player_t handle, char *file)
{   
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->play_file == NULL){
        return -QM_EINVAL;
    }

    if(file == NULL){
        return -QM_EINVAL;
    }

    return player_dev->ops->play_file(player_dev, file);
}


qm_err_t qm_player_play_bin(qm_player_t handle, uint8_t *audio_buffer, int buffer_len)
{
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->play_bin == NULL){
        return -QM_EINVAL;
    }

    return player_dev->ops->play_bin(player_dev, audio_buffer, buffer_len);
}

qm_err_t qm_player_pause(qm_player_t handle)
{
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->pause == NULL){
        return -QM_EINVAL;
    }

    return player_dev->ops->pause(player_dev);
}

qm_err_t qm_player_resume(qm_player_t handle)
{
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->resume == NULL){
        return -QM_EINVAL;
    }

    return player_dev->ops->resume(player_dev);
}

qm_err_t qm_player_stop(qm_player_t handle)
{
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->stop == NULL){
        return -QM_EINVAL;
    }

    return player_dev->ops->stop(player_dev);
}

bool_t qm_player_is_idle(qm_player_t handle)
{
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    if(play_ctx->player_status == QM_PLAYER_EVENT_PLAYING ||
       play_ctx->player_status == QM_PLAYER_EVENT_COMPLETED_PLAYING_NEXT){
        return QM_FALSE;
    }
    
    return QM_TRUE;
}

qm_err_t qm_player_mute(qm_player_t handle)
{
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->mute == NULL){
        return -QM_EINVAL;
    }

    return player_dev->ops->mute(player_dev);
}

qm_err_t qm_player_unmute(qm_player_t handle)
{
    qm_player_dev_t *player_dev = NULL;
    qm_player_ctx_t *play_ctx = (qm_player_ctx_t *)handle;
    if(play_ctx == NULL){
        return -QM_EINVAL;
    }
    
    player_dev = &play_ctx->player_dev;
    if(player_dev== NULL || player_dev->ops->unmute == NULL){
        return -QM_EINVAL;
    }

    return player_dev->ops->unmute(player_dev);
}