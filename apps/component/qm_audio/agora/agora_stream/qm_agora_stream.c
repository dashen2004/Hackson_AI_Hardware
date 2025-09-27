#include "qm.h"
#include "qm_work.h"
#include "qm_event.h"

#include "cJSON.h"
#include "qm_spec_api.h"
#include "qm_utils_list.h"

#include "qm_kv.h"
#include "qm_utils_base64.h"

#include "qm_agora.h"
#include "qm_agora_stream.h"
#include "qm_utils_string.h"

#define LOG_TAG "AGORA_STREAM"

#define OBJECT_TYPE             "object"
#define TEXT_TYPE               "text"
#define WORDS_TYPE              "words"
#define WORD_TYPE               "word"
#define START_MS_TYPE           "start_ms"
#define DURATION_MS_TYPE        "duration_ms"
#define TURN_STATUS_TYPE        "turn_status"
#define FINAL_TYPE              "final"
#define STATE_TYPE              "state"
#define TURN_ID_TYPE            "turn_id"

#define STREAM_MSG_CACHE_MAX_LEN    10
#define STREAM_MSG_ID_MAX_LEN       32

typedef struct {
    qm_task_t                     stream_task;
    qm_sem_t                      stream_sem;
    qm_list_t                     stream_list;
    qm_mutex_t                    stream_lock;

    qm_work_t                     tts_work;
    qm_list_t                     tts_list;
    qm_mutex_t                    tts_lock;
    bool                          tts_timer_is_running;

    qm_agora_stream_event_cb_t    event_cb;
    void*                         user_data;
} qm_agora_stream_handle_t;

typedef struct {
    uint32_t last_word_len;
    uint32_t word_len;
    uint64_t last_utc_time;
    uint64_t utc_time;
    uint64_t duration_ms;
} qm_agora_stream_word_t;

typedef struct{
    int index;
    int total;
    char msg_id[STREAM_MSG_ID_MAX_LEN];
    uint32_t payload_length;
    uint8_t *payload;
}qm_audio_stream_t;

typedef struct {
    int total;
    int count;
    int is_used;
    uint32_t index;
    char msg_id[STREAM_MSG_ID_MAX_LEN];
    qm_audio_stream_t **packet_data;
} stream_message_cache_t;

typedef struct {
    const char *text;
    qm_err_t (*handler)(cJSON *root);
} qm_agora_stream_cmd_table_t;

static qm_err_t qm_stream_assistant_trans_handler(cJSON *root);
static qm_err_t qm_stream_user_trans_handler(cJSON *root);
static qm_err_t qm_stream_message_state_handler(cJSON *root);
static qm_err_t qm_stream_message_interrupt_handler(cJSON *root);
static void tts_timer_callback(void *arg);

static const char *agent_state_list[] = {
    "silent",
    "speaking",
    "thinking",
};

static qm_agora_stream_handle_t g_stream_handle = {0};
static stream_message_cache_t current_msg_cache[STREAM_MSG_CACHE_MAX_LEN] = {0};
static uint32_t msg_cache_count = 0;
static qm_agora_stream_word_t word_info = {0};

static qm_agora_stream_cmd_table_t cmd_table[] = {
    {"assistant.transcription",     qm_stream_assistant_trans_handler},
    {"user.transcription",          qm_stream_user_trans_handler},
    {"message.state",               qm_stream_message_state_handler},
    {"message.interrupt",           qm_stream_message_interrupt_handler},
};

static void message_cache_clear(uint32_t index)
{
    if (current_msg_cache[index].packet_data) {
        for (int i = 0; i < current_msg_cache[index].total; i++) {
            if (current_msg_cache[index].packet_data[i]) {
                if(current_msg_cache[index].packet_data[i]->payload) {
                    qm_free(current_msg_cache[index].packet_data[i]->payload);
                    current_msg_cache[index].packet_data[i]->payload = NULL;
                }
                qm_free(current_msg_cache[index].packet_data[i]);
                current_msg_cache[index].packet_data[i] = NULL;
            }
        }
        qm_free(current_msg_cache[index].packet_data);
        current_msg_cache[index].packet_data = NULL;
    }

    current_msg_cache[index].total = 0;
    current_msg_cache[index].count = 0;
    current_msg_cache[index].is_used = 0;
}

static uint32_t message_cache_get_new_index(void)
{
    uint32_t min_index = 0;
    for(int i = 0; i < STREAM_MSG_CACHE_MAX_LEN; i++) {
        if(current_msg_cache[i].is_used == 0) {
            message_cache_clear(i);
            current_msg_cache[i].is_used = 1;
            current_msg_cache[i].index = msg_cache_count++;
            return i;
        }
        if(current_msg_cache[min_index].index > current_msg_cache[i].index) {
            min_index = i;
        }
    }
    if(min_index < STREAM_MSG_CACHE_MAX_LEN) {
        message_cache_clear(min_index);
        current_msg_cache[min_index].is_used = 1;
        current_msg_cache[min_index].index = msg_cache_count++;
        return min_index;
    }
    return STREAM_MSG_CACHE_MAX_LEN;
}

static uint32_t message_cache_get_index_by_msg_id(char *msg_id)
{
    if(msg_id == NULL) {
        QM_LOGW(LOG_TAG, "msg_id is NULL");
        return STREAM_MSG_CACHE_MAX_LEN;
    }
    for(int i = 0; i < STREAM_MSG_CACHE_MAX_LEN; i++) {
        if(current_msg_cache[i].is_used == 0) {
            continue;
        }
        if(strncmp(current_msg_cache[i].msg_id, msg_id, STREAM_MSG_ID_MAX_LEN) == 0) {
            return i;
        }
    }
    return STREAM_MSG_CACHE_MAX_LEN;
}

static void message_cache_printf(uint32_t index)
{
    if (current_msg_cache[index].packet_data == NULL) {
        return;
    }
    for (int i = 0; i < current_msg_cache[index].total; i++) {
        if (current_msg_cache[index].packet_data[i] == NULL) {
            continue;
        }
        if(current_msg_cache[index].packet_data[i]->payload == NULL) {
            continue;
        }
        QM_LOGD(LOG_TAG, "index=%d, total=%d, payload=%.*s", 
            current_msg_cache[index].packet_data[i]->index, current_msg_cache[index].packet_data[i]->total,
            current_msg_cache[index].packet_data[i]->payload_length, current_msg_cache[index].packet_data[i]->payload);
    }
}

static void tts_timer_start(int ms)
{
    g_stream_handle.tts_timer_is_running = QM_TRUE;
    qm_cancel_delayed_action(&g_stream_handle.tts_work);
    qm_post_delayed_action(&g_stream_handle.tts_work, tts_timer_callback, NULL, ms);
}

static void tts_timer_callback(void *arg)
{
    qm_list_node_t *node = NULL;
    qm_agora_stream_event_t *event = NULL;
    qm_mutex_lock(&g_stream_handle.tts_lock, QM_WAIT_FOREVER);
    node = qm_list_lpop(&g_stream_handle.tts_list);

    if(node == NULL){
        g_stream_handle.tts_timer_is_running = QM_FALSE;
        qm_mutex_unlock(&g_stream_handle.tts_lock);
        return;
    }

    event = (qm_agora_stream_event_t *)qm_list_node_val_get(node);

    if(event == NULL) {
        qm_mutex_unlock(&g_stream_handle.tts_lock);
        QM_LOGW(LOG_TAG, "tts msg_node is null");
        goto __FREE;
    }

    if (event->data.tts.text == NULL) {
        qm_mutex_unlock(&g_stream_handle.tts_lock);
        QM_LOGW(LOG_TAG, "tts msg_node payload is null");
        goto __FREE;
    }

    tts_timer_start(event->data.tts.duration_ms);
    qm_mutex_unlock(&g_stream_handle.tts_lock);
    
    if(g_stream_handle.event_cb) {
        g_stream_handle.event_cb(event, g_stream_handle.user_data);
    }

__FREE:
    if(event->data.tts.text) {
        qm_free(event->data.tts.text);
        event->data.tts.text = NULL;
    }
    if(node) {
        qm_list_node_destroy(node);
        node = NULL;
    }
}

static void qm_stream_assistant_trans_insert(qm_agora_stream_event_t *event) 
{
    qm_agora_stream_event_t *msg_node = NULL;
    qm_list_node_t *node = qm_list_node_extra_new(sizeof(qm_agora_stream_event_t));
    if(node == NULL) {
        QM_LOGW(LOG_TAG, "Failed to create list node");
        return ;
    }
    msg_node = (qm_agora_stream_event_t *)qm_list_node_val_get(node);
    msg_node->type = event->type;
    msg_node->data.tts.text_len = event->data.tts.text_len;
    msg_node->data.tts.duration_ms = event->data.tts.duration_ms;
    msg_node->data.tts.text = (char *)qm_malloc(msg_node->data.tts.text_len + 1);
    if(msg_node->data.tts.text == NULL || msg_node->data.tts.text_len == 0) {
        QM_LOGW(LOG_TAG, "Failed to allocate memory for payload: %d", msg_node->data.tts.text_len);
        qm_list_node_destroy(node);
        return;
    }
    memset(msg_node->data.tts.text, 0, msg_node->data.tts.text_len + 1);
    memcpy(msg_node->data.tts.text, event->data.tts.text, msg_node->data.tts.text_len);
    qm_mutex_lock(&g_stream_handle.tts_lock, QM_WAIT_FOREVER);
    qm_list_rpush(&g_stream_handle.tts_list, node);
    if(g_stream_handle.tts_timer_is_running == QM_FALSE) {
        tts_timer_start(0);
    }
    qm_mutex_unlock(&g_stream_handle.tts_lock);
}

static qm_err_t qm_stream_assistant_trans_handler(cJSON *root)
{
    static int old_turn_id = -1;
    qm_err_t ret = QM_EOK;
    char *new_text = NULL;
    size_t new_text_size = 0;
    cJSON *text = NULL;
    cJSON *turn_status = NULL;
    cJSON *turn_id = NULL;
    qm_agora_stream_event_t event = {0};

    if(root == NULL) {
        return QM_EINVAL;
    }

    turn_id = cJSON_GetObjectItem(root, TURN_ID_TYPE);
    if (cJSON_IsNumber(turn_id) == false) {
        QM_LOGW(LOG_TAG, "assistant.transcription turn_id is NULL");
        return QM_EINVAL;
    }
    if(old_turn_id != turn_id->valueint) {
        word_info.last_utc_time = 0;
        word_info.last_word_len = 0;
        old_turn_id = turn_id->valueint;
    }

    text = cJSON_GetObjectItem(root, TEXT_TYPE);
    if (cJSON_IsString(text) == false) {
        ret = QM_EINVAL;
        QM_LOGW(LOG_TAG, "assistant.transcription text is NULL");
        goto __FREE;
    }
    
    word_info.word_len = strlen(cJSON_GetStringValue(text));
    if(word_info.word_len == 0 || word_info.word_len < word_info.last_word_len) {
        ret = QM_EINVAL;
        word_info.last_utc_time = 0;
        word_info.last_word_len = 0;
        QM_LOGW(LOG_TAG, "assistant.transcription word_len is zero, %d, %d", word_info.word_len, word_info.last_word_len);
        goto __FREE;
    }

    new_text_size = word_info.word_len - word_info.last_word_len;
    new_text = (char *)qm_malloc(new_text_size + 1);
    if(new_text == NULL) {
        ret = QM_EINVAL;
        word_info.last_utc_time = 0;
        word_info.last_word_len = 0;
        QM_LOGW(LOG_TAG, "assistant.transcription word_info.word is NULL");
        goto __FREE;
    }

    memset(new_text, 0, new_text_size + 1);
    memcpy(new_text, cJSON_GetStringValue(text) + word_info.last_word_len, new_text_size);
    
#if CONFIG_AGORA_XJIANG_TTS
    cJSON *start_ms = NULL;
    start_ms = cJSON_GetObjectItem(root, START_MS_TYPE);
    if (cJSON_IsNumber(start_ms) == false) {
        QM_LOGW(LOG_TAG, "assistant.transcription start_ms is NULL");
        ret = QM_EINVAL;
        word_info.last_utc_time = 0;
        word_info.last_word_len = 0;
        goto __FREE;
    }
    word_info.utc_time = (uint64_t)cJSON_GetNumberValue(start_ms);
    if(word_info.last_utc_time == 0) {
        word_info.duration_ms = 0;
    } else {
        word_info.duration_ms = word_info.utc_time - word_info.last_utc_time;
    }
    word_info.last_utc_time = word_info.utc_time;
#else
    cJSON *word = NULL;
    cJSON *words = NULL;
    words = cJSON_GetObjectItem(root, WORDS_TYPE);
    if (cJSON_IsArray(words) == true) {
        cJSON_ArrayForEach(word, words) {
            if(cJSON_IsObject(word) == false) {
                QM_LOGW(LOG_TAG, "assistant.transcription word is NULL");
                continue;
            }
            cJSON *start_ms = NULL;
            start_ms = cJSON_GetObjectItem(word, START_MS_TYPE);
            if (cJSON_IsNumber(start_ms) == false) {
                QM_LOGW(LOG_TAG, "assistant.transcription start_ms is NULL");
                continue;
            }

            word_info.utc_time = (uint64_t)cJSON_GetNumberValue(start_ms);
            if(word_info.last_utc_time >= word_info.utc_time) {
                continue;
            } else if(word_info.last_utc_time == 0) {
                word_info.last_utc_time = word_info.utc_time;
            }
        }
    } else {
        ret = QM_EINVAL;
        QM_LOGW(LOG_TAG, "assistant.transcription words is NULL");
    }
#endif
    
    if(new_text_size != 0) {
        event.type = QM_AGORA_STREAM_TYPE_TTS;
#if CONFIG_AGORA_XJIANG_TTS
        event.data.tts.duration_ms = word_info.duration_ms;
#else
        event.data.tts.duration_ms = word_info.utc_time - word_info.last_utc_time;
#endif
        event.data.tts.text = new_text;
        event.data.tts.text_len = new_text_size;

        qm_stream_assistant_trans_insert(&event);
    } else {
        QM_LOGW(LOG_TAG, "No new words found");
    }

    turn_status = cJSON_GetObjectItem(root, TURN_STATUS_TYPE); 
    if (cJSON_IsNumber(turn_status) && turn_status->valueint == 1) {
        QM_LOGI("1", "Assistant says: %s", cJSON_GetStringValue(text));
        word_info.last_utc_time = 0;
        word_info.last_word_len = 0;
    } else {
        word_info.last_utc_time = word_info.utc_time;
        word_info.last_word_len = word_info.word_len;
    }
__FREE:
    if(new_text) {
        qm_free(new_text);
        new_text = NULL;
    }
    return ret;
}

static qm_err_t qm_stream_user_trans_handler(cJSON *root)
{
    cJSON *text = NULL;
    cJSON *final = NULL;
    qm_agora_stream_event_t event = {0};
    
    if(root == NULL) {
        return QM_EINVAL;
    }

    text = cJSON_GetObjectItem(root, TEXT_TYPE);
    if(cJSON_IsString(text) == false) {
        QM_LOGW(LOG_TAG, "user.transcription text is NULL");
        return QM_EINVAL;
    }
    final = cJSON_GetObjectItem(root, FINAL_TYPE);
    if(cJSON_IsBool(final) == false) {
        QM_LOGW(LOG_TAG, "user.transcription final is NULL");
        return QM_EINVAL;
    }
#if CONFIG_STREAM_USER_TRANS_NEED_FINAL
    if(cJSON_IsFalse(final)) {
        return QM_EOK;
    }
#endif
    event.type = QM_AGORA_STREAM_TYPE_STT;
    event.data.stt.text = cJSON_GetStringValue(text);
    if(g_stream_handle.event_cb) {
        g_stream_handle.event_cb(&event, g_stream_handle.user_data);
    }
    return QM_EOK;
}

static qm_err_t qm_stream_message_state_handler(cJSON *root)
{
    cJSON *state = NULL;
    qm_agora_stream_event_t event = {0};

    if(root == NULL) {
        return QM_EINVAL;
    }

    state = cJSON_GetObjectItem(root, STATE_TYPE);
    if (cJSON_IsString(state) == false) {
        QM_LOGW(LOG_TAG, "message.state state is NULL");
        return QM_EINVAL;
    }
    event.data.agent_state.text = cJSON_GetStringValue(state);
    if(event.data.agent_state.text == NULL) {
        QM_LOGW(LOG_TAG, "message.state state_str is NULL");
        return QM_EINVAL;
    }
    event.type = QM_AGORA_STREAM_TYPE_AGENT_STATE;
    for(int i = 0; i < QM_ARRAY_SIZE(agent_state_list); i++) {
        if(strcmp(event.data.agent_state.text, agent_state_list[i]) == 0) {
            event.data.agent_state.state = i + 1;
            break;
        }
    }
    if(g_stream_handle.event_cb) {
        g_stream_handle.event_cb(&event, g_stream_handle.user_data);
    }
    return QM_EOK;
}

static qm_err_t qm_stream_message_interrupt_handler(cJSON *root)
{
    qm_agora_stream_event_t event = {0};
    cJSON *turn_id = NULL;

    if(root == NULL) {
        return QM_EINVAL;
    }
    word_info.last_utc_time = 0;
    word_info.last_word_len = 0;

    turn_id = cJSON_GetObjectItem(root, TURN_ID_TYPE);
    if (cJSON_IsNumber(turn_id) == false) {
        QM_LOGW(LOG_TAG, "message.interrupt turn_id is NULL");
        return QM_EINVAL;
    }
    event.type = QM_AGORA_STREAM_TYPE_INTERRUPT;
    event.data.interrupt.turn_id = turn_id->valueint;
    if(g_stream_handle.event_cb) {
        g_stream_handle.event_cb(&event, g_stream_handle.user_data);
    }
    return QM_EOK;
}

static void stream_msg_recv_handler(uint8_t *decoded_msg, uint32_t decoded_len)
{
    cJSON *root = NULL; 
    cJSON *object = NULL;
    const char *obj_type = NULL;
    
    root = cJSON_ParseWithLength((char*)decoded_msg, decoded_len);
    if (root == NULL) {
        QM_LOGW(LOG_TAG, "root is NULL");
        goto __FREE;
    }
#if 0
    char *data = cJSON_Print(root);
    int len = strlen(data);
    for(int i = 0; i < len;) {
        if(len - i > 500) {
            QM_LOGD(LOG_TAG, "%.*s", 500, data + i);
            i += 500;
        } else {
            QM_LOGD(LOG_TAG, "%.*s", len - i, data + i);
            break;
        }
    }
#endif
    object = cJSON_GetObjectItem(root, OBJECT_TYPE);
    if (cJSON_IsString(object) == false) {
        QM_LOGW(LOG_TAG, "object is NULL");
        goto __FREE;
    }
    
    obj_type = cJSON_GetStringValue(object);
    for (int i = 0; i < QM_ARRAY_SIZE(cmd_table); i++) {
        if (strcmp(obj_type, cmd_table[i].text) == 0) {
            if(cmd_table[i].handler) {
                if(cmd_table[i].handler(root) != QM_EOK) {
                    QM_LOGE("LOG_TAG", "cmd_table[%d].handler failed: %.*s", i, decoded_len, decoded_msg);
                }
            }
            goto __FREE;
        }
    }
    QM_LOGD("LOG_TAG", "other: %.*s", decoded_len, decoded_msg);

__FREE:
    cJSON_Delete(root);
}

static void stream_msg_base64_decode(uint8_t *data, size_t len) 
{
    uint8_t *decoded_msg = NULL;
    uint32_t decoded_len = 0;
    decoded_msg = (uint8_t *)qm_malloc(len + 1);
    if (decoded_msg == NULL) {
        QM_LOGW(LOG_TAG, "decoded_msg is NULL");
        return;
    }
    memset(decoded_msg, 0, len + 1);
    qm_utils_base64decode((const uint8_t*)data, len, len, decoded_msg, &decoded_len);
    if(decoded_len == 0) {
        QM_LOGW(LOG_TAG, "decoded_len is NULL");
        QM_HEX_LOGW(LOG_TAG, "stream msg:", data, len);
        return;
    }
    stream_msg_recv_handler(decoded_msg, decoded_len);
    if (decoded_msg) {
        qm_free(decoded_msg);
        decoded_msg = NULL;
    }
}

static void stream_msg_recv_task(void *args)
{
    int list_len = 0;
    qm_list_node_t *node = NULL;
    qm_audio_stream_t *msg_node = NULL;
    qm_agora_stream_handle_t *audio_ctx = (qm_agora_stream_handle_t *)args;
    stream_message_cache_t *cache;
    while (1) {
        qm_sem_wait(&audio_ctx->stream_sem, QM_WAIT_FOREVER);

        qm_mutex_lock(&audio_ctx->stream_lock, QM_WAIT_FOREVER);
        node = qm_list_lpop(&audio_ctx->stream_list);
        qm_mutex_unlock(&audio_ctx->stream_lock);

        if(node == NULL){
            continue;
        }

        msg_node = (qm_audio_stream_t *)qm_list_node_val_get(node);

        if(msg_node == NULL) {
            QM_LOGW(LOG_TAG, "msg_node is null");
            goto __FREE;
        }

        if (msg_node->payload == NULL) {
            QM_LOGW(LOG_TAG, "msg_node payload is null");
            goto __FREE;
        }
        
        if(msg_node->total == 1){
            stream_msg_base64_decode(msg_node->payload, msg_node->payload_length);
            goto __FREE;
        }
        
        // QM_LOGD(LOG_TAG, "%s|%d|%d|%.*s", msg_node->msg_id, msg_node->index, msg_node->total, msg_node->payload_length, msg_node->payload);
        uint32_t cache_index = message_cache_get_index_by_msg_id(msg_node->msg_id);
        if(cache_index == STREAM_MSG_CACHE_MAX_LEN) {
            cache_index = message_cache_get_new_index();
            if(cache_index == STREAM_MSG_CACHE_MAX_LEN) {
                QM_LOGW(LOG_TAG, "current_msg_cache is NULL");
                goto __FREE;
            }
        }
        cache = &current_msg_cache[cache_index];
        if(cache->count == 0) {
            cache->packet_data = (qm_audio_stream_t **)qm_malloc(sizeof(qm_audio_stream_t *) * msg_node->total);
            if(cache->packet_data == NULL) {
                message_cache_clear(cache_index);
                QM_LOGW(LOG_TAG, "cache->packet_data malloc failed1");
                QM_LOGW(LOG_TAG, "index=%d, total=%d, payload=%.*s",
                                msg_node->index, msg_node->total, 
                                msg_node->payload_length, msg_node->payload);
                goto __FREE;
            }
            cache->total = msg_node->total;
            memcpy(cache->msg_id, msg_node->msg_id, STREAM_MSG_ID_MAX_LEN);
            memset(cache->packet_data, 0, sizeof(qm_audio_stream_t *) * cache->total);
        }
        if(cache->packet_data == NULL) {
            message_cache_clear(cache_index);
            QM_LOGW(LOG_TAG, "cache->packet_data malloc failed2");
            QM_LOGW(LOG_TAG, "index=%d, total=%d, payload=%.*s",
                                msg_node->index, msg_node->total, 
                                msg_node->payload_length, msg_node->payload);
            goto __FREE;
        }

        if(msg_node->index > cache->total) {
            QM_LOGW(LOG_TAG, "msg_node->index:%d bigger cache->total:%d", msg_node->index, cache->total);
            goto __FREE;
        }

        if(cache->packet_data[msg_node->index - 1] != NULL) {
            QM_LOGW(LOG_TAG, "msg_node->index:%d repeat", msg_node->index);
            goto __FREE;
        }

        qm_audio_stream_t *msg_node_temp = qm_malloc(sizeof(qm_audio_stream_t));
        if(msg_node_temp == NULL) {
            QM_LOGW(LOG_TAG, "msg_node_temp malloc failed");
            goto __FREE;
        }

        memset(msg_node_temp, 0, sizeof(qm_audio_stream_t));
        msg_node_temp->index = msg_node->index;
        msg_node_temp->total = msg_node->total;
        msg_node_temp->payload_length = msg_node->payload_length;

        msg_node_temp->payload = (uint8_t *)qm_malloc(msg_node->payload_length + 1);
        if(msg_node_temp->payload == NULL) {
            QM_LOGW(LOG_TAG, "msg_node_temp payload malloc failed");
            qm_free(msg_node_temp);
            goto __FREE;
        }
        memset(msg_node_temp->payload, 0, msg_node_temp->payload_length + 1);
        memcpy(msg_node_temp->payload, msg_node->payload, msg_node->payload_length);

        cache->count++;
        cache->packet_data[msg_node_temp->index - 1] = msg_node_temp;

        // QM_LOGD(LOG_TAG, "[%d]msg_id[%d]:%s [%d][%d/%d]", 
        //                 cache_index, cache->index, 
        //                 cache->msg_id, cache->count, 
        //                 msg_node->index, cache->total);
        if(cache->count != cache->total) {
            goto __FREE;
        }
        size_t total_length = 0;
        for(int i = 0; i < cache->total; i++) {
            if(cache->packet_data[i] != NULL) {
                total_length += cache->packet_data[i]->payload_length;
            } else {
                total_length = 0;
                break;
            }
        }
        if(total_length > 0) {
            uint8_t *assembled = (uint8_t *)qm_malloc(total_length + 1);
            if(assembled) {
                size_t offset = 0;
                memset(assembled, 0, total_length + 1);
                for(int i = 0; i < cache->total; i++) {
                    if(cache->packet_data[i] && (offset + cache->packet_data[i]->payload_length) <= total_length) {
                        memcpy(assembled + offset, 
                                cache->packet_data[i]->payload, 
                                cache->packet_data[i]->payload_length);
                        offset += cache->packet_data[i]->payload_length;
                    }
                }
                stream_msg_base64_decode(assembled, total_length);
                qm_free(assembled);
                assembled = NULL;
            }
        } else {
            QM_LOGW(LOG_TAG, "Failed to assemble message");
            message_cache_printf(cache_index);
        }
        message_cache_clear(cache_index);
__FREE:
        cache = NULL;
        if(msg_node->payload) {
            qm_free(msg_node->payload);
            msg_node->payload = NULL;
        }
        if(node) {
            qm_list_node_destroy(node);
            node = NULL;
        }

        qm_mutex_lock(&audio_ctx->stream_lock, QM_WAIT_FOREVER);
        list_len = qm_list_len_get(&audio_ctx->stream_list);
        qm_mutex_unlock(&audio_ctx->stream_lock);

        if(list_len) {
            qm_sem_signal(&audio_ctx->stream_sem);
        }
    }
}


static size_t get_packet_len(const char *data, size_t length)
{
    uint8_t count = 0;
    for(size_t i = 0; i < length; i++){
        if(data[i] == '|'){
            count++;
            if(count == 3){
                return length - i - 1;
            }
        }
    }
    return 0;
}

void __on_stream_message(connection_id_t conn_id, uint32_t uid, int stream_id, 
                                const char* data, size_t length, uint64_t sent_ts)
{
    if(data == NULL || length == 0) {
        return;
    }
    qm_audio_stream_t *msg_node = NULL;
    qm_list_node_t *node = qm_list_node_extra_new(sizeof(qm_audio_stream_t));
    if(node == NULL){
        QM_LOGW(LOG_TAG, "Failed to create list node");
        return ;
    }
    msg_node = (qm_audio_stream_t *)qm_list_node_val_get(node);

    msg_node->payload_length = get_packet_len(data, length);
    msg_node->payload = (uint8_t *)qm_malloc(msg_node->payload_length + 1);
    if(msg_node->payload == NULL || msg_node->payload_length == 0) {
        QM_LOGW(LOG_TAG, "Failed to allocate memory for payload: %d:%d", length, msg_node->payload_length);
        qm_list_node_destroy(node);
        return;
    }
    memset(msg_node->payload, 0, msg_node->payload_length + 1);
    char format[128];
    snprintf(format, sizeof(format), "%%31[^|]|%%d|%%d|%%%ds", (int)msg_node->payload_length);
    int ret = sscanf(data, format, msg_node->msg_id, &msg_node->index, &msg_node->total, msg_node->payload);

    if (ret != 4) {
        QM_LOGW(LOG_TAG, "Parse failed, only got %d fields", ret);
        qm_list_node_destroy(node);
        qm_free(msg_node->payload);
        return;
    }

    qm_mutex_lock(&g_stream_handle.stream_lock, QM_WAIT_FOREVER);
    qm_list_rpush(&g_stream_handle.stream_list, node);
    qm_mutex_unlock(&g_stream_handle.stream_lock);

    qm_sem_signal(&g_stream_handle.stream_sem);
}

void qm_agora_stream_register(qm_agora_stream_event_cb_t func, void *user_data)
{
    if (func == NULL) {
        return;
    }
    g_stream_handle.event_cb = func;
    g_stream_handle.user_data = user_data;
}

int32_t qm_agora_stream_init(void)
{
    memset(&g_stream_handle, 0, sizeof(qm_agora_stream_handle_t));
    qm_mutex_new(&g_stream_handle.tts_lock);
    qm_sem_new(&g_stream_handle.stream_sem, 0);
    qm_mutex_new(&g_stream_handle.stream_lock);
    qm_task_new(&g_stream_handle.stream_task, "stream_msg_recv_task", stream_msg_recv_task, &g_stream_handle, 10*1024, 5);
    return QM_EOK;
}

