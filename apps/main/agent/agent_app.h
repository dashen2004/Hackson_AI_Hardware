#ifndef AGENT_APP_H
#define AGENT_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* 创建 agent。唤醒词到来且云已连上时才 start。 */
int agent_app_init(void *iot_handle);
/* 云端 chat/quit：停会话。成功后清掉已 start 标记，下次唤醒词可以再 start。 */
int agent_app_leave(void);

#ifdef __cplusplus
}
#endif

#endif
