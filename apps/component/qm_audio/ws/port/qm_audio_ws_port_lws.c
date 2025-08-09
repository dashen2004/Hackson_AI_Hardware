
#include "qm.h"
#include "qm_audio_ws_iface.h"
#include "qm_audio_ws.h"
#include "qm_audio_common.h"

#include "qm_ble_gap.h"
#include "qm_utils_list.h"
#include "qm_work.h"
#include "qm_event.h"

#include "cJSON.h"

#include "esp_websocket_client.h"

#include "audio_mem.h"
#include "audio_thread.h"

static const char *LOG_TAG = "ws-client";

typedef struct 
{

    void *user_context;
    qm_audio_ws_monitor_t monitor;

    esp_websocket_client_handle_t client;

}qm_audio_ws_port_handle_t;

static qm_audio_ws_port_handle_t g_ws_port_handle = {0};


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        QM_LOGE(LOG_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    qm_audio_ws_event_data_t ws_data = {0};
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    qm_audio_ws_port_handle_t *audio_dev = (qm_audio_ws_port_handle_t *)data->user_context;

    switch (event_id) {
    case WEBSOCKET_EVENT_BEGIN:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_BEGIN");
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_CONNECTED");
        // qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_CONNECT, NULL, 0);
        break;
        
    case WEBSOCKET_EVENT_DISCONNECTED:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        // qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_DISCONNECT, NULL, 0);

        log_error_if_nonzero("HTTP status code",  data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  data->error_handle.esp_transport_sock_errno);
        }
        break;
    case WEBSOCKET_EVENT_DATA:
        if(data->op_code == 0x01 || data->op_code == 0x02){

            // QM_LOGI(LOG_TAG, "Received opcode=%d size=%d, total:%d", data->op_code, data->data_len, data->payload_len);
            // if(data->op_code == 0x01){
            //     QM_LOGD("LOG_TAG","MSG:%s", data->data_ptr);
            // }
            ws_data.fin = data->fin;
            ws_data.op_code = data->op_code;
            ws_data.data_ptr = data->data_ptr;
            ws_data.data_len = data->data_len;
            ws_data.payload_len = data->payload_len;
            ws_data.payload_offset = data->payload_offset;

            // websocket_audio_recv(audio_dev, data->op_code ,(uint8_t *)data->data_ptr, data->data_len, data->payload_len, data->payload_offset);
        }

        break;
    case WEBSOCKET_EVENT_ERROR:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_ERROR");
        log_error_if_nonzero("HTTP status code",  data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  data->error_handle.esp_transport_sock_errno);
        }
        break;
    case WEBSOCKET_EVENT_FINISH:
        QM_LOGI(LOG_TAG, "WEBSOCKET_EVENT_FINISH");
        // qm_event_post(QM_AUDIO_EVENT_ID, QM_AUDIO_EVENT_TYPE_WS_DISCONNECT, NULL, 0);
        break;
    }


    if(audio_dev && audio_dev->monitor){
        audio_dev->monitor(audio_dev, event_id, &ws_data, audio_dev->user_context);
    }
}

static void* ws_init(qm_audio_ws_iface_cfg_t *cfg)
{
    esp_websocket_client_config_t websocket_cfg = {0};

    websocket_cfg.uri = cfg->uri;
    websocket_cfg.buffer_size = cfg->buffer_size;
    websocket_cfg.task_prio = cfg->task_prio;
    websocket_cfg.task_stack = cfg->task_stack;
    websocket_cfg.uri = cfg->uri;
    websocket_cfg.skip_cert_common_name_check = true;
    websocket_cfg.user_context = &g_ws_port_handle;
    websocket_cfg.disable_auto_reconnect = QM_TRUE;

    g_ws_port_handle.user_context = cfg->user_context;
    g_ws_port_handle.client = esp_websocket_client_init(&websocket_cfg);


    esp_websocket_register_events(g_ws_port_handle.client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)g_ws_port_handle.client);
    
    return &g_ws_port_handle;
}

static qm_err_t ws_set_monitor(void *handle, qm_audio_ws_monitor_t monitor)
{   
    qm_audio_ws_port_handle_t *ws_handle = (qm_audio_ws_port_handle_t *)handle;
    if(ws_handle == NULL){
        return -QM_EINVAL;
    }

    ws_handle->monitor = monitor;

    return QM_EOK;
}

static qm_err_t ws_append_header(void *handle, const char *key, const char *value)
{
    qm_audio_ws_port_handle_t *ws_handle = (qm_audio_ws_port_handle_t *)handle;
    if(ws_handle == NULL){
        return -QM_EINVAL;
    }

    esp_websocket_client_append_header(ws_handle->client, key, value);
    
    return QM_EOK;
}

static qm_err_t ws_start(void *handle)
{

    qm_audio_ws_port_handle_t *ws_handle = (qm_audio_ws_port_handle_t *)handle;
    if(ws_handle == NULL){
        return -QM_EINVAL;
    }

    esp_websocket_client_start(ws_handle->client);
    

    return QM_EOK;
}

static qm_err_t ws_stop(void *handle)
{

    qm_audio_ws_port_handle_t *ws_handle = (qm_audio_ws_port_handle_t *)handle;
    if(ws_handle == NULL){
        return -QM_EINVAL;
    }

    esp_websocket_client_stop(ws_handle->client);

    return QM_EOK;
}

static qm_err_t ws_send_text(void *handle, const char *data, int len, uint32_t timeout)
{
    qm_audio_ws_port_handle_t *ws_handle = (qm_audio_ws_port_handle_t *)handle;
    if(ws_handle == NULL){
        return -QM_EINVAL;
    }
    
    esp_websocket_client_send_text(ws_handle->client, data, len, timeout / portTICK_PERIOD_MS);
    return QM_EOK;
}

static qm_err_t ws_send_bin(void *handle, const char *data, int len, uint32_t timeout)
{
    qm_audio_ws_port_handle_t *ws_handle = (qm_audio_ws_port_handle_t *)handle;
    if(ws_handle == NULL){
        return -QM_EINVAL;
    }

    esp_websocket_client_send_bin(ws_handle->client, data, len, timeout / portTICK_PERIOD_MS);

    return QM_EOK;
}

qm_audio_ws_iface_t g_audio_ws_iface = 
{
    .init = ws_init,
    .append_header = ws_append_header,
    .set_monitor = ws_set_monitor,
    .send_bin = ws_send_bin,
    .send_text = ws_send_text,
    .start = ws_start,
    .stop = ws_stop,
};


