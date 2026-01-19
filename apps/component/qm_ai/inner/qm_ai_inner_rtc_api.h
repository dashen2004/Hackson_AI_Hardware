#ifndef __QM_AI_INNER_RTC_API_H__
#define __QM_AI_INNER_RTC_API_H__

#include "qm.h"
#include "qm_ai_api.h"

typedef struct {
    bool audio_enable;
    bool video_enable;

    char* license;

    qm_ai_audio_cb_t audio_cb;
    qm_ai_stream_cb_t stream_cb;
    qm_ai_event_cb_t event_cb;

    void *user_data;
} qm_ai_rtc_cfg_t;

int qm_ai_rtc_init(qm_ai_rtc_cfg_t cfg);
int qm_ai_rtc_deinit(void);

int qm_ai_rtc_connect(void);
int qm_ai_rtc_disconnect(void);

int qm_ai_rtc_join(void);
int qm_ai_rtc_leave(void);

int qm_ai_rtc_send_audio_data(const void *data, size_t len);

int qm_ai_rtc_clear_buffer_data(void);


#endif  /* __QM_AI_INNER_RTC_API_H__ */