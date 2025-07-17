#ifndef __QM_AUDIO_WS_IFACE_H__
#define __QM_AUDIO_WS_IFACE_H__

#include "qm.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Websocket Client events id
 */
typedef enum {
    QM_AUDIO_WS_EVENT_ANY = -1,
    QM_AUDIO_WS_EVENT_ERROR = 0,      /*!< This event occurs when there are any errors during execution */
    QM_AUDIO_WS_EVENT_CONNECTED,      /*!< Once the Websocket has been connected to the server, no data exchange has been performed */
    QM_AUDIO_WS_EVENT_DISCONNECTED,   /*!< The connection has been disconnected */
    QM_AUDIO_WS_EVENT_DATA,           /*!< When receiving data from the server, possibly multiple portions of the packet */
    QM_AUDIO_WS_EVENT_CLOSED,         /*!< The connection has been closed cleanly */
    QM_AUDIO_WS_EVENT_BEFORE_CONNECT, /*!< The event occurs before connecting */
    QM_AUDIO_WS_EVENT_BEGIN,          /*!< The event occurs once after thread creation, before event loop */
    QM_AUDIO_WS_EVENT_FINISH,         /*!< The event occurs once after event loop, before thread destruction */
    QM_AUDIO_WS_EVENT_MAX
} qm_audio_ws_event_id_t;

/**
 * @brief Websocket client setup configuration
 */
typedef struct {
    const char                  *uri;                       /*!< Websocket URI, the information on the URI can be overrides the other fields below, if any */
    const char                  *host;                      /*!< Domain or IP as string */
    int                         port;                       /*!< Port to connect, default depend on esp_websocket_transport_t (80 or 443) */
    const char                  *path;                      /*!< HTTP Path, if not set, default is `/` */
    void                        *user_context;              /*!< HTTP user data context */
    int                         task_prio;                  /*!< Websocket task priority */
    const char                 *task_name;                  /*!< Websocket task name */
    int                         task_stack;                 /*!< Websocket task stack */
    int                         buffer_size;                /*!< Websocket buffer size */
} qm_audio_ws_iface_cfg_t;


/**
 * @brief Websocket event data
 */
typedef struct {
    const char *data_ptr;                   /*!< Data pointer */
    int data_len;                           /*!< Data length */
    bool fin;                               /*!< Fin flag */
    uint8_t op_code;                        /*!< Received opcode */
    int payload_len;                        /*!< Total payload length, payloads exceeding buffer will be posted through multiple events */
    int payload_offset;                     /*!< Actual offset for the data associated with this event */
} qm_audio_ws_event_data_t;


/**
 * @brief Monitor of the recorder speech recognition
 */
typedef void (*qm_audio_ws_monitor_t)(void *handle, qm_audio_ws_event_id_t event_id, qm_audio_ws_event_data_t *event_data, void *user_ctx);


typedef struct {

    /**
     * @brief Suspend the afe tasks
     *
     * @param handle    The handle of sr handle
     * @param suspend   true to suspend the afe tasks, false to resume.
     *
     * @returns ESP_OK
     *          ESP_ERR_INVALID_ARG
     */
    void* (*init)(qm_audio_ws_iface_cfg_t *cfg);

    qm_err_t (*set_monitor)(void *handle, qm_audio_ws_monitor_t monitor);

    qm_err_t (*append_header)(void *handle, const char *key, const char *value);

    qm_err_t (*start)(void *handle);

    qm_err_t (*stop)(void *handle);

    qm_err_t (*send_text)(void *handle, const char *data, int len, uint32_t timeout);

    qm_err_t (*send_bin)(void *handle, const char *data, int len, uint32_t timeout);

} qm_audio_ws_iface_t;

extern qm_audio_ws_iface_t g_audio_ws_iface;

#ifdef __cplusplus
}
#endif

#endif /* __RECORDER_SR_IFACE_H__ */
