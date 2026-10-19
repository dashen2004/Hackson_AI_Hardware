#ifndef LJYD_CLOUD_H
#define LJYD_CLOUD_H

/* 灵机平台产品身份。换产品改这里。 */
#define CONFIG_XJIANG_CLOUD_PID    11735
#define CONFIG_XJIANG_CLOUD_SECERT "B09C6E63D3DE43848CE17B3C55C0429A"

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
