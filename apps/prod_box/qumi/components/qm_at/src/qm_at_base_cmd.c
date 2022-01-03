#include "qm_at.h"
#include "qmos/qm_types.h"


static qm_at_result_t at_exec(void *channel);
static qm_at_result_t ate_setup(void *channel, const char *args);

static qm_at_cmd_t base_at_cmd[] = {
    {"AT", NULL, NULL, NULL, NULL, at_exec},
    {"ATE", "<value>", NULL, NULL, ate_setup, NULL},
};

#define QM_BASE_AT_CMD_NUM (sizeof(base_at_cmd) / sizeof(base_at_cmd[0]))

static qm_at_result_t at_exec(void *channel)
{
    return QM_AT_RESULT_OK;
}

static qm_at_result_t ate_setup(void *channel, const char *args)
{
    int echo_mode = atoi(args);

    printf("echo_mode: %d\r\n", echo_mode);

    if(echo_mode == QM_AT_ECHO_MODE_CLOSE || echo_mode == QM_AT_ECHO_MODE_OPEN)
    {
        qm_at_server_echo_mode_set(channel, echo_mode);
    }
    else
    {
        return QM_AT_RESULT_FAILE;
    }

    return QM_AT_RESULT_OK;
}


int32_t qm_at_base_cmd_init(void)
{
    return qm_at_register_commands(base_at_cmd, QM_BASE_AT_CMD_NUM);
}
