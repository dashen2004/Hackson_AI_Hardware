#ifndef LJYD_CLOUD_H
#define LJYD_CLOUD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LJYD_CLOUD_EVENT_CONNECTED = 0,
    LJYD_CLOUD_EVENT_DISCONNECTED,
} ljyd_cloud_event_t;

typedef void (*ljyd_cloud_event_cb_t)(ljyd_cloud_event_t event, void *iot_handle, void *userdata);

int ljyd_cloud_init(void);
void *ljyd_cloud_get_handle(void);
void ljyd_cloud_set_event_cb(ljyd_cloud_event_cb_t cb, void *userdata);
void ljyd_cloud_on_network_up(void);
void ljyd_cloud_on_network_down(void);
/* 返回 QM_EOK 表示云已接手解绑，等 RESET 事件再清配网。 */
int ljyd_cloud_request_reset(void);

#ifdef __cplusplus
}
#endif

#endif
