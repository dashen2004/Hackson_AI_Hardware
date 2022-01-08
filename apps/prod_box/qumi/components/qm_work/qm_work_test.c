#include "qm_work.h"
#include "qm_log.h"

#define QM_TEST_EVENT 0x1000 

char *test_arg = "hello";

#define LOG_TAG "qm_work"

void test_event_callback(qm_input_event_t *event, void *arg)
{
    QM_LOGD("LOG_TAG", "event type: %d, code: %d, value: %d, arg: %s", event->type, event->code, event->value, arg);
    
}

void test_action_callback(void *arg)
{
    QM_LOGD("LOG_TAG", "arg: %s", arg);
    
}

void qm_work_test(void)
{
    int ret = 0;
    void *handle = NULL;
    ret = qm_work_on_init();
    QM_LOGD("LOG_TAG", "ret: %d", ret);

    ret = qm_event_register(QM_TEST_EVENT, test_event_callback, test_arg);
    QM_LOGD("LOG_TAG", "ret: %d", ret);
    ret = qm_event_post(QM_TEST_EVENT, 88, 1314);
    QM_LOGD("LOG_TAG", "ret: %d", ret);

    qm_post_delayed_action(&handle, test_action_callback, test_arg, 0);
    qm_post_delayed_action(&handle, test_action_callback, test_arg, 1000);
}


