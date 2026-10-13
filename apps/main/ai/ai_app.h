#ifndef AI_APP_H
#define AI_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* 在 qm_iot_start 之前调用，好让 getAgentTokenReply 先挂上。 */
int ai_app_bind_cloud(void *iot_handle);

int ai_on_cloud_connected(void);
void ai_on_cloud_disconnected(void);

#ifdef __cplusplus
}
#endif

#endif
