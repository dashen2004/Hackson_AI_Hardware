#ifndef __QM_IOT_AGORA_GEN_H__
#define __QM_IOT_AGORA_GEN_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

typedef struct
{
    char *uid;
    uint32_t uid_len;
    char *appId;
    uint32_t appId_len;
    char *channelName;
    uint32_t channelName_len;
    char *token;
    uint32_t token_len;
} qm_iot_agora_gen_handle_t;

int32_t qm_iot_agora_gen_start(void *handle);

int32_t qm_iot_agora_gen_free(void **handle);

int qm_iot_agora_uid_set(char *uid, uint32_t len);
int32_t qm_iot_agora_uid_get(char *uid, uint32_t *len);

int qm_iot_agora_appid_set(char *appid, uint32_t len);
int32_t qm_iot_agora_appid_get(char *appid, uint32_t *len);

int qm_iot_agora_channelName_set(char *channelName, uint32_t len);
int32_t qm_iot_agora_channelName_get(char *channelName, uint32_t *len);

int qm_iot_agora_token_set(char *token, uint32_t len);
int32_t qm_iot_agora_token_get(char *token, uint32_t *len);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_AGORA_GEN_H__ */