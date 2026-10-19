#ifndef AGENT_APP_H
#define AGENT_APP_H

#include "qm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 创建 agent。唤醒词到来且云已连上时才 start。 */
int agent_app_init(void *iot_handle);
/* 云端 chat/quit：停会话。成功后清掉已 start 标记，下次唤醒词可以再 start。 */
int agent_app_leave(void);

/* Stop cloud/local output and pause the physical capture path. */
int agent_app_abort_output(void);

/* Ask the cloud agent to speak one application-provided sentence. */
int agent_app_user_corpus(const char *text);

/* Wait until the local TTS queue and speaker have drained. */
int agent_app_wait_tts_complete(uint32_t timeout_ms);

/* Reserve or release the shared audio path for resource playback. */
int agent_app_set_resource_mode(qm_bool active);

#ifdef __cplusplus
}
#endif

#endif
