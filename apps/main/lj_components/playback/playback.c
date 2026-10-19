#include "playback.h"

#include "agent_app.h"

#include "qm.h"
#include "qm_errno.h"
#include "qm_log.h"
#include "qm_string.h"

#include <string.h>

#if defined(CONFIG_QM_RES_MGR_SUPPORT) && CONFIG_QM_RES_MGR_SUPPORT
#include "qm_res_mgr.h"
#endif

#define LOG_TAG "playback"

#define PLAYBACK_AUDIO_WAIT_MS    3000U
#define PLAYBACK_MQTT_TIMEOUT_MS  15000U
#define PLAYBACK_PAGE_SIZE        10U
#define PLAYBACK_COMMAND_QUEUE_LEN 4U
#define PLAYBACK_WORKER_STACK      (12 * 1024)
#define PLAYBACK_WORKER_PRIORITY   18
#define PLAYBACK_GUIDE_TIMEOUT_MS  30000U

static qm_player_t s_player;
static uint32_t s_operation_id;
static qm_queue_t s_command_queue;
static qm_task_t s_command_task;
static qm_sem_t s_command_done;
static uint8_t s_command_ready;

static void playback_command_task(void *arg);
int playback_stop(void);

typedef struct {
    char *guide_words;
    char *url;
    char *audio_type;
    char *title;
    uint32_t file_size;
    uint32_t duration_s;
} playback_cloud_command_t;

static qm_player_resource_type_t media_type(const char *audio_type)
{
    if (audio_type == NULL) {
        return QM_PLAYER_RESOURCE_TYPE_UNKNOWN;
    }
    if (qm_strcasecmp(audio_type, "mp3") == 0) return QM_PLAYER_RESOURCE_TYPE_MP3;
    if (qm_strcasecmp(audio_type, "wav") == 0) return QM_PLAYER_RESOURCE_TYPE_WAV;
    if (qm_strcasecmp(audio_type, "aac") == 0) return QM_PLAYER_RESOURCE_TYPE_AAC;
    if (qm_strcasecmp(audio_type, "flac") == 0) return QM_PLAYER_RESOURCE_TYPE_FLAC;
    if (qm_strcasecmp(audio_type, "pcm") == 0) return QM_PLAYER_RESOURCE_TYPE_PCM;
    return QM_PLAYER_RESOURCE_TYPE_UNKNOWN;
}

static char *copy_text(const char *text)
{
    size_t len;
    char *copy;

    if (text == NULL || text[0] == '\0') {
        return NULL;
    }
    len = qm_strlen(text);
    copy = (char *)qm_malloc(len + 1U);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, len + 1U);
    return copy;
}

static void free_cloud_command(playback_cloud_command_t *command)
{
    if (command == NULL) return;
    qm_free(command->guide_words);
    qm_free(command->url);
    qm_free(command->audio_type);
    qm_free(command->title);
    qm_free(command);
}

static void playback_restore_audio(void)
{
    (void)agent_app_set_resource_mode(QM_FALSE);
}

static void playback_event(qm_player_t player,
                           const qm_player_event_t *event,
                           void *user_data)
{
    QM_UNUSED_ARG(player);
    QM_UNUSED_ARG(user_data);

    if (event == NULL) {
        return;
    }

    QM_LOGI(LOG_TAG,
            "event=%d operation=%u error=%d position=%u",
            (int)event->type,
            (unsigned int)event->operation_id,
            event->error,
            (unsigned int)event->position_ms);
    if (event->operation_id == s_operation_id &&
        (event->type == QM_PLAYER_EVENT_FINISHED ||
         event->type == QM_PLAYER_EVENT_STOPPED ||
         event->type == QM_PLAYER_EVENT_PREEMPTED ||
         event->type == QM_PLAYER_EVENT_ERROR)) {
        s_operation_id = 0U;
        playback_restore_audio();
    }
}

int playback_init(qm_audio_handle_t audio,
                  const qm_player_decoder_if_t *decoder_if,
                  const qm_player_source_if_t *source_if)
{
    qm_player_config_t config;
    int ret;
    uint8_t queue_ready = 0U;
    uint8_t sem_created = 0U;
    uint8_t sem_ready = 0U;

    if (audio == NULL || decoder_if == NULL) {
        return -QM_EINVAL;
    }
    if (s_player != NULL) {
        return -QM_EBUSY;
    }

    qm_memset(&config, 0, sizeof(config));
    config.audio = audio;
    config.decoder_if = decoder_if;
    config.source_if = source_if;
    config.event_cb = playback_event;
    s_player = qm_player_create(&config);
    if (s_player == NULL) {
        return -QM_EINIT;
    }

    s_operation_id = 0U;
    ret = qm_queue_new(&s_command_queue,
                       PLAYBACK_COMMAND_QUEUE_LEN,
                       sizeof(playback_cloud_command_t *));
    if (ret == QM_EOK) {
        queue_ready = 1U;
        ret = qm_sem_new(&s_command_done, 0);
        if (ret == QM_EOK) {
            sem_created = 1U;
        }
    }
    if (ret == QM_EOK && qm_sem_is_valid(&s_command_done)) {
        sem_ready = 1U;
    }
    if (!queue_ready || !sem_ready) {
        if (sem_created) {
            (void)qm_sem_free(&s_command_done);
        }
        if (queue_ready) {
            (void)qm_queue_free(&s_command_queue);
        }
        (void)qm_player_destroy(s_player);
        s_player = NULL;
        return -QM_ENOMEM;
    }
    if (qm_task_new(&s_command_task, "playback", playback_command_task, NULL,
                    PLAYBACK_WORKER_STACK, PLAYBACK_WORKER_PRIORITY) != QM_EOK) {
        (void)qm_sem_free(&s_command_done);
        (void)qm_queue_free(&s_command_queue);
        (void)qm_player_destroy(s_player);
        s_player = NULL;
        return -QM_ENOMEM;
    }
    s_command_ready = 1U;
    return QM_EOK;
}

static int submit_resource(const qm_player_resource_t *resource)
{
    qm_audio_playback_request_t request;
    uint32_t operation_id = 0U;
    int ret;

    if (s_player == NULL) {
        return -QM_EINIT;
    }
    if (resource == NULL || resource->location == NULL ||
        resource->location[0] == '\0') {
        return -QM_EINVAL;
    }
    if (resource->type != QM_PLAYER_RESOURCE_TYPE_PCM) {
        QM_LOGW(LOG_TAG, "unsupported resource type=%d", (int)resource->type);
        return -QM_ENOSYS;
    }
    if (resource->location_type == QM_PLAYER_LOCATION_URI &&
        qm_strncmp(resource->location, "http://", 7U) != 0 &&
        qm_strncmp(resource->location, "https://", 8U) != 0) {
        return -QM_EINVAL;
    }

    qm_memset(&request, 0, sizeof(request));
    request.priority = QM_AUDIO_PLAYBACK_PRIORITY_LOW;
    request.policy = QM_AUDIO_PLAYBACK_POLICY_QUEUE;
    request.capture_policy = QM_AUDIO_CAPTURE_POLICY_PAUSE;
    request.delivery = QM_AUDIO_PLAYBACK_DELIVERY_RELIABLE;
    request.acquire_timeout_ms = PLAYBACK_AUDIO_WAIT_MS;

    ret = qm_player_play(s_player, resource, &request, &operation_id);
    if (ret == QM_EOK) {
        s_operation_id = operation_id;
        QM_LOGI(LOG_TAG, "play submitted operation=%u type=%d",
                (unsigned int)s_operation_id, (int)resource->type);
    } else {
        QM_LOGW(LOG_TAG, "play submit failed ret=%d", ret);
    }
    return ret;
}

static void process_cloud_command(playback_cloud_command_t *command)
{
    int ret;

    if (command == NULL) {
        return;
    }
    if (s_operation_id != 0U) {
        (void)playback_stop();
    }
    (void)agent_app_abort_output();

    if (command->guide_words != NULL) {
        ret = agent_app_user_corpus(command->guide_words);
        if (ret == QM_EOK) {
            ret = agent_app_wait_tts_complete(PLAYBACK_GUIDE_TIMEOUT_MS);
        }
        if (ret != QM_EOK) {
            QM_LOGW(LOG_TAG, "guide words failed ret=%d", ret);
            (void)agent_app_set_resource_mode(QM_FALSE);
            return;
        }
        QM_LOGI(LOG_TAG, "guide words playback complete");
    }

    if (command->url == NULL || command->url[0] == '\0') {
        (void)agent_app_set_resource_mode(QM_FALSE);
        return;
    }
    (void)agent_app_set_resource_mode(QM_TRUE);
    ret = playback_play_url(command->url,
                            media_type(command->audio_type),
                            command->file_size,
                            command->duration_s);
    if (ret != QM_EOK) {
        QM_LOGW(LOG_TAG, "resource play failed ret=%d", ret);
        (void)agent_app_set_resource_mode(QM_FALSE);
    }
}

static void playback_command_task(void *arg)
{
    (void)arg;
    for (;;) {
        playback_cloud_command_t *command = NULL;
        unsigned int size = sizeof(command);

        if (qm_queue_recv(&s_command_queue, &command, &size,
                          QM_WAIT_FOREVER) != QM_EOK) {
            continue;
        }
        if (command == NULL) {
            break;
        }
        process_cloud_command(command);
        free_cloud_command(command);
    }
    (void)qm_sem_signal(&s_command_done);
    qm_task_exit(NULL);
}

int playback_submit_cloud_play(const playback_cloud_play_request_t *request)
{
    playback_cloud_command_t *command;

    if (!s_command_ready || request == NULL || request->url == NULL ||
        request->url[0] == '\0') {
        return -QM_EINVAL;
    }
    if (media_type(request->audio_type != NULL ? request->audio_type : "pcm") !=
        QM_PLAYER_RESOURCE_TYPE_PCM) {
        return -QM_ENOSYS;
    }
    command = (playback_cloud_command_t *)qm_malloc(sizeof(*command));
    if (command == NULL) {
        return -QM_ENOMEM;
    }
    qm_memset(command, 0, sizeof(*command));
    command->guide_words = copy_text(request->guide_words);
    command->url = copy_text(request->url);
    command->audio_type = copy_text(request->audio_type != NULL ?
                                    request->audio_type : "pcm");
    command->title = copy_text(request->title);
    command->file_size = request->file_size;
    command->duration_s = request->duration_s;
    if (command->url == NULL || command->audio_type == NULL ||
        (request->guide_words != NULL && command->guide_words == NULL) ||
        (request->title != NULL && command->title == NULL)) {
        free_cloud_command(command);
        return -QM_ENOMEM;
    }
    QM_LOGI(LOG_TAG, "cloud play queued type=%s size=%u duration=%u guide=%u",
            command->audio_type,
            (unsigned)command->file_size,
            (unsigned)command->duration_s,
            command->guide_words != NULL ? 1U : 0U);
    if (qm_queue_send(&s_command_queue, &command, sizeof(command)) != QM_EOK) {
        free_cloud_command(command);
        return -QM_EBUSY;
    }
    return QM_EOK;
}

int playback_play_url(const char *url,
                      qm_player_resource_type_t type,
                      uint32_t file_size,
                      uint32_t duration_s)
{
    qm_player_resource_t resource;

    qm_memset(&resource, 0, sizeof(resource));
    resource.location_type = QM_PLAYER_LOCATION_URI;
    resource.location = url;
    resource.type = type;
    resource.file_size = file_size;
    resource.duration_s = duration_s;
    return submit_resource(&resource);
}

int playback_play_file(const char *path,
                       qm_player_resource_type_t type,
                       uint32_t file_size,
                       uint32_t duration_s)
{
    qm_player_resource_t resource;

    qm_memset(&resource, 0, sizeof(resource));
    resource.location_type = QM_PLAYER_LOCATION_FILE;
    resource.location = path;
    resource.type = type;
    resource.file_size = file_size;
    resource.duration_s = duration_s;
    return submit_resource(&resource);
}

#if defined(CONFIG_QM_RES_MGR_SUPPORT) && CONFIG_QM_RES_MGR_SUPPORT
static int metadata_valid(const playback_metadata_t *metadata)
{
    if (metadata == NULL || metadata->did == NULL || metadata->did[0] == '\0') {
        return 0;
    }
#if defined(CONFIG_QM_RES_MGR_HTTP_SUPPORT) && CONFIG_QM_RES_MGR_HTTP_SUPPORT
    return metadata->token != NULL && metadata->token[0] != '\0';
#elif defined(CONFIG_QM_RES_MGR_MQTT_SUPPORT) && CONFIG_QM_RES_MGR_MQTT_SUPPORT
    return metadata->iot_handle != NULL;
#else
    return 0;
#endif
}

static int print_page(const qm_res_mgr_list_page_t *page, void *user)
{
    uint32_t i;
    int *received = (int *)user;

    if (page == NULL || received == NULL) {
        return -QM_EINVAL;
    }
    if (page->biz_code != 0) {
        return -QM_EIO;
    }

    *received = 1;
    for (i = 0U; i < page->count; i++) {
        const qm_res_mgr_resource_t *item = &page->items[i];
        QM_LOGI(LOG_TAG, "resource id=%u title=%s type=%s audio_type=%s",
                (unsigned int)item->id,
                item->title != NULL ? item->title : "",
                item->type != NULL ? item->type : "",
                item->audio_type != NULL ? item->audio_type : "");
    }

    /* Keep the synchronous example bounded to one page. */
    return -QM_EINTR;
}

int playback_set_resource_env(qm_res_mgr_env_t env)
{
    return qm_res_mgr_set_env(env);
}

int playback_print_first_page(const playback_metadata_t *metadata,
                              const char *bundle)
{
    qm_res_mgr_list_req_t request;
    qm_res_mgr_list_t list;
    int received = 0;
    int ret;

    if (!metadata_valid(metadata) || bundle == NULL || bundle[0] == '\0') {
        return -QM_EINVAL;
    }

    qm_memset(&request, 0, sizeof(request));
    qm_memset(&list, 0, sizeof(list));
    request.did = metadata->did;
    request.resource_bundle_name = bundle;
    request.page_num = 1U;
    request.page_size = PLAYBACK_PAGE_SIZE;
    request.on_page = print_page;
    request.page_user = &received;
#if defined(CONFIG_QM_RES_MGR_HTTP_SUPPORT) && CONFIG_QM_RES_MGR_HTTP_SUPPORT
    request.token = metadata->token;
#elif defined(CONFIG_QM_RES_MGR_MQTT_SUPPORT) && CONFIG_QM_RES_MGR_MQTT_SUPPORT
    request.iot_handle = metadata->iot_handle;
    request.timeout_ms = PLAYBACK_MQTT_TIMEOUT_MS;
#endif

    ret = qm_res_mgr_get_list(&request, &list);
    if (ret == -QM_EINTR && received && list.biz_code == 0) {
        ret = QM_EOK;
    }
    if (ret == QM_EOK && list.biz_code != 0) {
        ret = -QM_EIO;
    }
    qm_res_mgr_list_free(&list);
    return ret;
}

int playback_get_categories(const playback_metadata_t *metadata,
                            qm_res_mgr_category_list_t *out)
{
    qm_res_mgr_category_req_t request;

    if (!metadata_valid(metadata) || out == NULL) {
        return -QM_EINVAL;
    }

    qm_memset(&request, 0, sizeof(request));
    qm_memset(out, 0, sizeof(*out));
    request.did = metadata->did;
#if defined(CONFIG_QM_RES_MGR_HTTP_SUPPORT) && CONFIG_QM_RES_MGR_HTTP_SUPPORT
    request.product_id = metadata->product_id;
    request.token = metadata->token;
#elif defined(CONFIG_QM_RES_MGR_MQTT_SUPPORT) && CONFIG_QM_RES_MGR_MQTT_SUPPORT
    request.iot_handle = metadata->iot_handle;
    request.timeout_ms = PLAYBACK_MQTT_TIMEOUT_MS;
#endif
    return qm_res_mgr_get_category(&request, out);
}

int playback_get_child_categories(const playback_metadata_t *metadata,
                                  uint32_t category_id,
                                  const char *bundle,
                                  qm_res_mgr_child_category_list_t *out)
{
    qm_res_mgr_children_category_req_t request;

    if (!metadata_valid(metadata) || out == NULL) {
        return -QM_EINVAL;
    }

    qm_memset(&request, 0, sizeof(request));
    qm_memset(out, 0, sizeof(*out));
    request.did = metadata->did;
    request.category_id = category_id;
    request.resource_bundle_name = bundle;
#if defined(CONFIG_QM_RES_MGR_HTTP_SUPPORT) && CONFIG_QM_RES_MGR_HTTP_SUPPORT
    request.product_id = metadata->product_id;
    request.token = metadata->token;
#elif defined(CONFIG_QM_RES_MGR_MQTT_SUPPORT) && CONFIG_QM_RES_MGR_MQTT_SUPPORT
    request.iot_handle = metadata->iot_handle;
    request.timeout_ms = PLAYBACK_MQTT_TIMEOUT_MS;
#endif
    return qm_res_mgr_get_children_category(&request, out);
}

int playback_play_id(const playback_metadata_t *metadata, uint32_t resource_id)
{
    qm_res_mgr_detail_req_t request;
    qm_res_mgr_resource_t detail;
    qm_player_resource_t resource;
    int ret;

    if (s_player == NULL) {
        return -QM_EINIT;
    }
    if (!metadata_valid(metadata) || resource_id == 0U) {
        return -QM_EINVAL;
    }

    qm_memset(&request, 0, sizeof(request));
    qm_memset(&detail, 0, sizeof(detail));
    request.did = metadata->did;
    request.resource_id = resource_id;
#if defined(CONFIG_QM_RES_MGR_HTTP_SUPPORT) && CONFIG_QM_RES_MGR_HTTP_SUPPORT
    request.token = metadata->token;
#elif defined(CONFIG_QM_RES_MGR_MQTT_SUPPORT) && CONFIG_QM_RES_MGR_MQTT_SUPPORT
    request.iot_handle = metadata->iot_handle;
    request.timeout_ms = PLAYBACK_MQTT_TIMEOUT_MS;
#endif

    ret = qm_res_mgr_get_detail(&request, &detail);
    if (ret == QM_EOK) {
        qm_memset(&resource, 0, sizeof(resource));
        resource.location_type = QM_PLAYER_LOCATION_URI;
        resource.location = detail.url;
        resource.type = media_type(detail.audio_type);
        resource.file_size = detail.file_size;
        resource.duration_s = detail.total_duration;
        resource.title = detail.title;
        resource.description = detail.description;
        ret = submit_resource(&resource);
    }

    /* qm_player clones resource strings before returning from play(). */
    qm_res_mgr_resource_free(&detail);
    return ret;
}

int playback_download(const qm_res_mgr_resource_t *resource,
                      qm_res_mgr_download_data_cb_t on_data,
                      void *user,
                      qm_res_mgr_download_result_t *result)
{
    qm_res_mgr_download_req_t request;

    if (resource == NULL || resource->url == NULL || resource->url[0] == '\0' ||
        on_data == NULL) {
        return -QM_EINVAL;
    }

    qm_memset(&request, 0, sizeof(request));
    request.resource = *resource;
    request.on_data = on_data;
    request.user = user;
    return qm_res_mgr_download(&request, result);
}
#endif

int playback_pause(void)
{
    if (s_player == NULL || s_operation_id == 0U) {
        return -QM_ENOENT;
    }
    return qm_player_pause(s_player, s_operation_id);
}

int playback_resume(void)
{
    if (s_player == NULL || s_operation_id == 0U) {
        return -QM_ENOENT;
    }
    return qm_player_resume(s_player, s_operation_id);
}

int playback_stop(void)
{
    if (s_player == NULL || s_operation_id == 0U) {
        return -QM_ENOENT;
    }
    return qm_player_stop(s_player, s_operation_id);
}

int playback_get_status(qm_player_status_t *status)
{
    if (s_player == NULL) {
        return -QM_EINIT;
    }
    if (status == NULL) {
        return -QM_EINVAL;
    }
    return qm_player_get_status(s_player, status);
}

int playback_deinit(void)
{
    int ret;

    if (s_command_ready) {
        playback_cloud_command_t *stop = NULL;

        ret = qm_queue_send(&s_command_queue, &stop, sizeof(stop));
        if (ret != QM_EOK ||
            qm_sem_wait(&s_command_done, PLAYBACK_GUIDE_TIMEOUT_MS) != QM_EOK) {
            QM_LOGW(LOG_TAG, "command worker stop timeout ret=%d", ret);
            return -QM_ETIMEOUT;
        }
        (void)qm_queue_free(&s_command_queue);
        (void)qm_sem_free(&s_command_done);
        s_command_ready = 0U;
    }
    if (s_player == NULL) {
        return QM_EOK;
    }

    ret = qm_player_destroy(s_player);
    if (ret == QM_EOK) {
        s_player = NULL;
        s_operation_id = 0U;
    }
    return ret;
}
