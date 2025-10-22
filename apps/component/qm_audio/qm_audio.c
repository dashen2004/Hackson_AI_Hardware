#include "qm_audio.h"
#include "qm.h"
#include "qm_ringbuf.h"
#include "qm_utils_list.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    qm_task_t task;
    qm_mutex_t lock;
    qm_list_t *list;
    qm_audio_state_t state;
    qm_queue_t queue;
    qm_audio_param_t audio_param;
    TickType_t xLastWakeTime;
}qm_audio_ctx_t;

typedef struct {
    int len;
    qm_audio_type_t type;
    uint8_t data[1];
}qm_audio_data_t;

typedef struct {
    qm_audio_event_t event_type;
    qm_audio_type_t audio_type;
    union 
    {
        qm_play_data_t play_data;
    }data;
}qm_audio_event_info_t;

#ifndef CONFIG_QM_AUDIO_TASK_SIZE
#define CONFIG_QM_AUDIO_TASK_SIZE    (20 * 1024)
#endif

#ifndef CONFIG_QM_AUDIO_TASK_PRIO   
#define CONFIG_QM_AUDIO_TASK_PRIO    22
#endif

#ifndef CONFIG_QM_AUDIO_EVENT_QUEUE_LEN   
#define CONFIG_QM_AUDIO_EVENT_QUEUE_LEN   5
#endif

static qm_audio_ctx_t g_qm_audio_ctx = {0};

static void audio_task(void *arg);
static int data_match(void *a, void *b);
static int qm_audio_state_set(qm_audio_state_t state);
static int qm_audio_playing(void);
static int qm_audio_local_stop(void);
static int qm_audio_data_reset(qm_list_t *list);
static int qm_audio_get_high_prior(qm_list_t *list, qm_audio_type_t *audio_type);
static int qm_audio_data_find(qm_list_t *list, qm_audio_type_t type, qm_list_node_t **node);

int qm_audio_init(qm_audio_param_t *audio_param)
{
    qm_err_t ret = QM_EOK;
    if(audio_param == NULL){
        return -QM_EINVAL;
    }
    memcpy(&g_qm_audio_ctx.audio_param, audio_param, sizeof(qm_audio_param_t));

    if(g_qm_audio_ctx.audio_param.audio_init){
        g_qm_audio_ctx.audio_param.audio_init(g_qm_audio_ctx.audio_param.arg);
    }

    g_qm_audio_ctx.list = qm_list_new();
    if(g_qm_audio_ctx.list == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    g_qm_audio_ctx.list->match = data_match;
    
    ret = qm_mutex_new(&g_qm_audio_ctx.lock);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_queue_new(&g_qm_audio_ctx.queue, CONFIG_QM_AUDIO_EVENT_QUEUE_LEN, sizeof(qm_audio_event_info_t));
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_task_new_to_core(&g_qm_audio_ctx.task, "audio", audio_task, &g_qm_audio_ctx, CONFIG_QM_AUDIO_TASK_SIZE, CONFIG_QM_AUDIO_TASK_PRIO, 0);
    if(ret != QM_EOK){
        goto __exit;
    }

    return QM_EOK;

__exit:
    if(g_qm_audio_ctx.list){
        qm_list_destroy(g_qm_audio_ctx.list);
        g_qm_audio_ctx.list = NULL;
    }

    if(qm_mutex_is_valid(&g_qm_audio_ctx.lock)){
        qm_mutex_free(&g_qm_audio_ctx.lock);
    }

    if(!qm_queue_is_valid(&g_qm_audio_ctx.queue)){
        qm_queue_free(&g_qm_audio_ctx.queue);
    }
    return ret;
}

static void audio_task(void *arg)
{
    #define CONFIG_QM_AUDIO_NORMAL_WAIT_TIME  3000
    #define CONFIG_QM_AUDIO_PLAY_WAIT_TIME    0

    uint8_t *data = NULL;
    int len = 0;
    unsigned int size = 0;
    qm_task_t task = {0};
    qm_list_node_t *node = NULL;
    qm_play_data_t play_data = {0};
    qm_audio_data_t *audio_data = NULL;
    qm_audio_event_info_t event_info = {0};
    qm_audio_type_t audio_type = QM_AUDIO_TYPE_NONE;
    qm_audio_ctx_t *audio_ctx = (qm_audio_ctx_t*)arg;
    uint32_t timeout = CONFIG_QM_AUDIO_NORMAL_WAIT_TIME;
    int duration_ms = 0;
    memcpy(&task, &audio_ctx->task, sizeof(qm_task_t));

    while(1)
    {
        qm_queue_recv(&audio_ctx->queue, (void*)&event_info, &size, QM_WAIT_FOREVER);

        switch(event_info.event_type){

            case QM_AUDIO_EVENT_NONE:

                timeout = CONFIG_QM_AUDIO_NORMAL_WAIT_TIME;
                
            break;

            case QM_AUDIO_EVENT_PLAY_START:

                timeout = CONFIG_QM_AUDIO_PLAY_WAIT_TIME;
                audio_ctx->xLastWakeTime = xTaskGetTickCount();
                
                memcpy(&play_data, &event_info.data.play_data, sizeof(qm_play_data_t));

                if(audio_ctx->audio_param.audio_eventcb){
                    audio_ctx->audio_param.audio_eventcb(QM_AUDIO_EVENT_PLAY_START, NULL);
                }

                qm_audio_playing();

            break;
            case QM_AUDIO_EVENT_PLAYING:
                if(play_data.play_get){
                    play_data.play_get(&data, &len);
                    if(data && len){
                        if(audio_ctx->audio_param.audio_play){
                            audio_ctx->audio_param.audio_play(QM_AUDIO_TYPE_LOCAL, data, len,  &duration_ms);
                        }
                        event_info.event_type = QM_AUDIO_EVENT_PLAYING;
                        qm_queue_send(&audio_ctx->queue, &event_info, sizeof(qm_audio_event_info_t));

                        if(duration_ms != 0) {
                            vTaskDelayUntil(&audio_ctx->xLastWakeTime, pdMS_TO_TICKS(duration_ms));
                        }
                    }else{
                        memset(&play_data, 0, sizeof(qm_play_data_t));
                        qm_audio_local_stop();
                    }
                }else{
                    qm_audio_data_find(audio_ctx->list, QM_AUDIO_TYPE_AGENT, &node);
                    if(node){
                        audio_data = (qm_audio_data_t*)qm_list_node_val_get(node);
                        
                        if(audio_ctx->audio_param.audio_play){
                            audio_ctx->audio_param.audio_play(QM_AUDIO_TYPE_AGENT, &audio_data->data[0], audio_data->len, &duration_ms);
                        }

                        qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
                        qm_list_remove(audio_ctx->list, node);
                        len = qm_list_len_get(audio_ctx->list);
                        qm_mutex_unlock(&g_qm_audio_ctx.lock);
                        qm_list_node_destroy(node);
                        if(duration_ms != 0) {
                            vTaskDelayUntil(&audio_ctx->xLastWakeTime, pdMS_TO_TICKS(duration_ms));
                        }
                        if(len){
                            event_info.event_type = QM_AUDIO_EVENT_PLAYING;
                            qm_queue_send(&audio_ctx->queue, &event_info, sizeof(qm_audio_event_info_t));
                        } else {
                            // if(audio_ctx->audio_param.audio_eventcb){
                            //     audio_ctx->audio_param.audio_eventcb(QM_AUDIO_EVENT_PLAY_STOP, NULL);
                            // }
                        }
                    }
                }
            break;
            case QM_AUDIO_EVENT_PLAY_STOP:
            case QM_AUDIO_EVENT_PLAY_LOCAL_STOP:
                timeout = CONFIG_QM_AUDIO_NORMAL_WAIT_TIME;
                audio_type = QM_AUDIO_TYPE_NONE;
                
                if(audio_ctx->audio_param.audio_eventcb){
                    audio_ctx->audio_param.audio_eventcb(event_info.event_type, NULL);
                } 
                memset(&event_info, 0, sizeof(qm_audio_event_info_t));

            break;

            case QM_AUDIO_EVENT_PLAY_MUTE:

                timeout = CONFIG_QM_AUDIO_NORMAL_WAIT_TIME;
                qm_audio_data_reset(audio_ctx->list);

                if(audio_ctx->audio_param.audio_mute){
                    audio_ctx->audio_param.audio_mute(audio_ctx->audio_param.arg);
                }
                memset(&event_info, 0, sizeof(qm_audio_event_info_t));
                audio_type = QM_AUDIO_TYPE_NONE;

                if(audio_ctx->audio_param.audio_eventcb){
                    audio_ctx->audio_param.audio_eventcb(QM_AUDIO_EVENT_PLAY_MUTE, NULL);
                } 

            break;

            case QM_AUDIO_EVENT_PLAY_PAUSE:
                
                timeout = CONFIG_QM_AUDIO_NORMAL_WAIT_TIME;
                memset(&event_info, 0, sizeof(qm_audio_event_info_t));
                audio_type = QM_AUDIO_TYPE_NONE;

                if(audio_ctx->audio_param.audio_eventcb){
                    audio_ctx->audio_param.audio_eventcb(QM_AUDIO_EVENT_PLAY_PAUSE, NULL);
                } 

            break;

            case QM_AUDIO_EVENT_PLAY_EXIT:
                if(audio_ctx->audio_param.audio_eventcb){
                    audio_ctx->audio_param.audio_eventcb(QM_AUDIO_EVENT_PLAY_EXIT, NULL);
                } 
                goto __exit;
            break;

            default:
            break;

        }
    }

__exit:

    if(g_qm_audio_ctx.audio_param.audio_deinit){
        g_qm_audio_ctx.audio_param.audio_deinit(audio_ctx->audio_param.arg);
    }

    if(g_qm_audio_ctx.list){
        qm_list_destroy(audio_ctx->list);
        audio_ctx->list = NULL;
    }

    if(qm_mutex_is_valid(&audio_ctx->lock)){
        qm_mutex_free(&audio_ctx->lock);
    }

    if(!qm_queue_is_valid(&audio_ctx->queue)){
        qm_queue_free(&audio_ctx->queue);
    }
    
    qm_task_exit(&task);
}

int qm_audio_data_push(qm_audio_type_t type, uint8_t *data, int len)
{
    int list_len = 0;
    qm_audio_data_t *audio_data = NULL;
    qm_list_node_t *node = NULL;
    qm_audio_event_info_t event_info = {0};

    if(data == NULL || len == 0){
        return -QM_EINVAL;
    }

    if(type <= QM_AUDIO_TYPE_NONE || type >= QM_AUDIO_TYPE_MAX){
        return -QM_EINVAL;
    }
    
    if(g_qm_audio_ctx.state != QM_AUDIO_STATE_SPEAK){
        return QM_EOK;
    }
    node = qm_list_node_extra_new(sizeof(qm_audio_data_t) - 1 + len);
    if(node == NULL){
        return -QM_ENOMEM;
    }

    audio_data = (qm_audio_data_t*)qm_list_node_val_get(node);
    audio_data->type = QM_AUDIO_TYPE_AGENT;
    audio_data->len = len;
    memcpy(&audio_data->data[0], data, len);

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    list_len = qm_list_len_get(g_qm_audio_ctx.list);
    qm_list_rpush(g_qm_audio_ctx.list, node);
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    if(!list_len){
        g_qm_audio_ctx.xLastWakeTime = xTaskGetTickCount();
        event_info.event_type = QM_AUDIO_EVENT_PLAYING;
        qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    }

    return QM_EOK;
}

static int data_match(void *a, void *b)
{
    qm_audio_data_t *a_audio_data = (qm_audio_data_t*)a;
    qm_audio_data_t *b_audio_data = (qm_audio_data_t*)b;
    if(a_audio_data->type == b_audio_data->type){
        return 1;
    }else{
        return 0;
    }
}

static int qm_audio_get_high_prior(qm_list_t *list, qm_audio_type_t *audio_type)
{
    int i = 0;
    qm_list_node_t *node = NULL;

    for(i = 1; i < QM_AUDIO_TYPE_MAX; i++){
        qm_audio_data_find(g_qm_audio_ctx.list, (qm_audio_type_t)i, &node);
        if(node){
            *audio_type = (qm_audio_type_t)i;
            return QM_EOK;
        }
    }
    return -QM_EEMPTY;
}

static int qm_audio_data_find(qm_list_t *list, qm_audio_type_t type, qm_list_node_t **node)
{
    qm_audio_data_t audio_data = {0};

    audio_data.type = type;
    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    *node = qm_list_find(g_qm_audio_ctx.list, &audio_data);
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    return QM_EOK;
}

static int qm_audio_data_reset(qm_list_t *list)
{
    qm_list_node_t *node = NULL;

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    while (1) {
        node = qm_list_lpop(g_qm_audio_ctx.list);
        if(node == NULL) {
            break;
        }
        qm_list_node_destroy(node);
        node = NULL;
    }
    qm_mutex_unlock(&g_qm_audio_ctx.lock);
    return QM_EOK;
}

static int qm_audio_state_set(qm_audio_state_t state)
{
    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = state;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);
    return QM_EOK;
}

qm_audio_state_t qm_audio_state_get(void)
{
    qm_audio_state_t state = QM_AUDIO_STATE_NONE;
    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    state = g_qm_audio_ctx.state;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);
    return state;
}

int qm_audio_play_with_data(qm_audio_type_t type, qm_play_data_t *play_data)
{
    qm_audio_event_info_t event_info = {0};
    if(play_data == NULL){
        return -QM_EINVAL;
    }

    if(type <= QM_AUDIO_TYPE_NONE || type >= QM_AUDIO_TYPE_MAX){
        return -QM_EINVAL;
    }

    memcpy(&event_info.data.play_data, play_data, sizeof(qm_play_data_t));

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = QM_AUDIO_STATE_LOCAL_SPEAK;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    event_info.event_type = QM_AUDIO_EVENT_PLAY_START;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));

    return QM_EOK;
}

int qm_audio_play(qm_audio_type_t type)
{
    qm_audio_event_info_t event_info = {0};
    
    if(type <= QM_AUDIO_TYPE_NONE || type >= QM_AUDIO_TYPE_MAX){
        return -QM_EINVAL;
    }

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = QM_AUDIO_STATE_SPEAK;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    event_info.event_type = QM_AUDIO_EVENT_PLAY_START;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    return QM_EOK;
}

static int qm_audio_playing(void)
{
    qm_audio_event_info_t event_info = {0};

    event_info.event_type = QM_AUDIO_EVENT_PLAYING;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    return QM_EOK;
}

int qm_audio_mute(void)
{
    qm_audio_event_info_t event_info = {0};

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = QM_AUDIO_STATE_SLIENT;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    event_info.event_type = QM_AUDIO_EVENT_PLAY_MUTE;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    return QM_EOK;
}

int qm_audio_pause(void)
{
    qm_audio_event_info_t event_info = {0};

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = QM_AUDIO_STATE_SLIENT;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    event_info.event_type = QM_AUDIO_EVENT_PLAY_PAUSE;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    return QM_EOK;
}

int qm_audio_stop(void)
{
    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = QM_AUDIO_STATE_SLIENT;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    return QM_EOK;
}

int qm_audio_direct_stop(void)
{
    qm_audio_event_info_t event_info = {0};

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = QM_AUDIO_STATE_SLIENT;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    event_info.event_type = QM_AUDIO_EVENT_PLAY_STOP;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    return QM_EOK;
}

static int qm_audio_local_stop(void)
{
    qm_audio_event_info_t event_info = {0};

    qm_mutex_lock(&g_qm_audio_ctx.lock, QM_WAIT_FOREVER);
    g_qm_audio_ctx.state = QM_AUDIO_STATE_SLIENT;
    qm_mutex_unlock(&g_qm_audio_ctx.lock);

    event_info.event_type = QM_AUDIO_EVENT_PLAY_LOCAL_STOP;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    return QM_EOK;
}

int qm_audio_deinit(void)
{
    qm_audio_event_info_t event_info = {0};

    event_info.event_type = QM_AUDIO_EVENT_PLAY_EXIT;
    qm_queue_send(&g_qm_audio_ctx.queue, &event_info, sizeof(qm_audio_event_info_t));
    return QM_EOK;
}
