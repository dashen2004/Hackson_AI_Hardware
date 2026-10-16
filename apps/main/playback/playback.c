#include "playback.h"

#include "qm_errno.h"
#include "qm_log.h"
#include "qm_string.h"

#if defined(CONFIG_QM_RES_MGR_SUPPORT) && CONFIG_QM_RES_MGR_SUPPORT
#include "qm_res_mgr.h"
#endif

#define LOG_TAG "playback"

#define PLAYBACK_AUDIO_WAIT_MS    3000U
#define PLAYBACK_MQTT_TIMEOUT_MS  15000U
#define PLAYBACK_PAGE_SIZE        10U

static qm_player_t s_player;
static uint32_t s_operation_id;

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
}

int playback_init(qm_audio_handle_t audio,
                  const qm_player_decoder_if_t *decoder_if,
                  const qm_player_source_if_t *source_if)
{
    qm_player_config_t config;

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

static qm_player_resource_type_t media_type(const char *audio_type)
{
    if (audio_type == NULL) {
        return QM_PLAYER_RESOURCE_TYPE_UNKNOWN;
    }
    if (qm_strcasecmp(audio_type, "mp3") == 0) {
        return QM_PLAYER_RESOURCE_TYPE_MP3;
    }
    if (qm_strcasecmp(audio_type, "wav") == 0) {
        return QM_PLAYER_RESOURCE_TYPE_WAV;
    }
    if (qm_strcasecmp(audio_type, "aac") == 0) {
        return QM_PLAYER_RESOURCE_TYPE_AAC;
    }
    if (qm_strcasecmp(audio_type, "flac") == 0) {
        return QM_PLAYER_RESOURCE_TYPE_FLAC;
    }
    if (qm_strcasecmp(audio_type, "pcm") == 0) {
        return QM_PLAYER_RESOURCE_TYPE_PCM;
    }
    return QM_PLAYER_RESOURCE_TYPE_UNKNOWN;
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
