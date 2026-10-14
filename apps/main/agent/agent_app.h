#ifndef AGENT_APP_H
#define AGENT_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* 创建 agent。唤醒词到来且云已连上时才 start。 */
int agent_app_init(void *iot_handle);

#ifdef __cplusplus
}
#endif

#endif
