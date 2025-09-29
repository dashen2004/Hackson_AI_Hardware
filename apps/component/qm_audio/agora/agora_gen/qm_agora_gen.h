#ifndef __QM_IOT_AGORA_GEN_H__
#define __QM_IOT_AGORA_GEN_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

typedef enum {
    QM_AGORA_GEN_TYPE_NONE = 0,
    QM_AGORA_GEN_TYPE_UID,
    QM_AGORA_GEN_TYPE_APPID,
    QM_AGORA_GEN_TYPE_CHANNAL_NAME,
    QM_AGORA_GEN_TYPE_TOKEN,
    QM_AGORA_GEN_TYPE_MAX,
} qm_agora_gen_data_type_t;

typedef struct
{
    bool exist_data;
    qm_timer_t timer;
    int uid;
    char *appId;
    uint32_t appId_len;
    char *channelName;
    uint32_t channelName_len;
    char *token;
    uint32_t token_len;
} qm_agora_gen_handle_t;

void *qm_agora_gen_init(void);

int32_t qm_agora_gen_start(void *handle);

void *qm_agora_gen_get_data(qm_agora_gen_data_type_t type);

int32_t qm_agora_gen_set_data(qm_agora_gen_data_type_t type, void *data, int len);

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_AGORA_GEN_H__ */