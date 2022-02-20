#include "qm_at.h"
#include "qmos/qm_types.h"
#include "qmos/qm_errno.h"
#include "qm_log.h"

#if CONFIG_QM_AT_SERVER

#define LOG_TAG              "at.svr"

#define AT_CMD_CHAR_0                  '0'
#define AT_CMD_CHAR_9                  '9'
#define AT_CMD_QUESTION_MARK           '?'
#define AT_CMD_EQUAL_MARK              '='
#define AT_CMD_L_SQ_BRACKET            '['
#define AT_CMD_R_SQ_BRACKET            ']'
#define AT_CMD_L_ANGLE_BRACKET         '<'
#define AT_CMD_R_ANGLE_BRACKET         '>'
#define AT_CMD_COMMA_MARK              ','
#define AT_CMD_SEMICOLON               ';'
#define AT_CMD_CR                      '\r'
#define AT_CMD_LF                      '\n'

static qm_at_server_t at_server_table[CONFIG_QM_AT_SERVER_MAX_NUM] = {0};

static qm_at_server_t *qm_at_server_get_by_channel(qm_at_channel_t *at_channel);
static int32_t qm_at_server_internel_deinit(qm_at_server_t *server);

extern int32_t qm_at_vprintf(void *channel, const char *format, va_list args);
extern int32_t qm_at_vprintfln(void *channel, const char *format, va_list args);

/**
 * AT server send data to AT device
 *
 * @param format the input format
 */
void qm_at_server_printf(void *channel, const char *format, ...)
{
    va_list args;

    va_start(args, format);

    qm_at_vprintf(channel, format, args);

    va_end(args);
}

/**
 * AT server send data and newline to AT device
 *
 * @param format the input format
 */
void qm_at_server_printfln(void *channel, const char *format, ...)
{
    va_list args;

    va_start(args, format);

    qm_at_vprintfln(channel, format, args);

    va_end(args);
}

/**
 * AT server request arguments parse arguments
 *
 * @param req_args request arguments
 * @param req_expr request expression
 *
 * @return  -1 : parse arguments failed
 *           0 : parse without match
 *          >0 : The number of arguments successfully parsed
 */
int32_t qm_at_req_parse_args(const char *req_args, const char *req_expr, ...)
{
    va_list args;
    int req_args_num = 0;

    QM_ASSERT(req_args);
    QM_ASSERT(req_expr);

    va_start(args, req_expr);

    req_args_num = vsscanf(req_args, req_expr, args);

    va_end(args);

    return req_args_num;
}

/**
 * AT server send command execute result to AT device
 *
 * @param result AT command execute result
 */
void qm_at_server_print_result(void *channel, qm_at_result_t result)
{
    qm_at_server_t *at_server = NULL;
    at_server = qm_at_server_get_by_channel(channel);
    switch (result)
    {
    case QM_AT_RESULT_OK:
        qm_at_server_printfln(channel, "OK");
        if(at_server->cb){
            at_server->cb(QM_AT_RESULT_OK, at_server->arg);
        }
        break;

    case QM_AT_RESULT_FAILE:
        qm_at_server_printfln(channel, "ERROR");
        if(at_server->cb){
            at_server->cb(QM_AT_RESULT_FAILE, at_server->arg);
        }
        break;

    case QM_AT_RESULT_NULL:
        break;

    case QM_AT_RESULT_CMD_ERR:
        qm_at_server_printfln(channel, "ERR CMD MATCH FAILED!");
        qm_at_server_print_result(channel, QM_AT_RESULT_FAILE);
        break;

    case QM_AT_RESULT_CHECK_FAILE:
        qm_at_server_printfln(channel, "ERR CHECK ARGS FORMAT FAILED!");
        qm_at_server_print_result(channel, QM_AT_RESULT_FAILE);
        break;

    case QM_AT_RESULT_PARSE_FAILE:
        qm_at_server_printfln(channel, "ERR PARSE ARGS FAILED!");
        qm_at_server_print_result(channel, QM_AT_RESULT_FAILE);
        break;

    default:
        break;
    }
}

/**
 *  AT server print all commands to AT device
 */
void qm_at_server_print_all_cmd(void *channel)
{
    uint16_t i = 0, j = 0;

    qm_at_channel_t *at_channel = (qm_at_channel_t*)channel;

    qm_at_server_t *at_server = qm_at_server_get_by_channel(at_channel);

    qm_at_server_printfln(channel, "Commands list : ");

    qm_at_cmd_table_t *cmd_table = &at_server->cmd_table;
    for (i = 0; i < CONFIG_QM_AT_CMD_TABLE_MAX_NUM; i++) {

        for (j = 0; j < cmd_table->at_cmd_num[i]; j++) {
            qm_at_cmd_t *cmd_func = (qm_at_cmd_t *) ((cmd_table->at_cmd_table[i] + j));

            qm_at_server_printf(channel, "%s", cmd_func->name);
            if (cmd_func->args_expr){
                qm_at_server_printfln(channel, "%s", cmd_func->args_expr);
            }
            else{
                qm_at_server_printf(channel, "%c%c", AT_CMD_CR, AT_CMD_LF);
            }
        }
    }
}

/**
 * Send data to AT Client by uart device.
 *
 * @param server current AT server object
 * @param buf   send data buffer
 * @param size  send fixed data size
 *
 * @return >0: send data size
 *         <=0: send failed
 */
int32_t qm_at_server_send(void *channel, const char *buf, uint32_t size)
{
    qm_at_channel_t *at_channel = (qm_at_channel_t*)channel;
    
    if (channel == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    return at_channel->send(at_channel->handle, buf, size);
}

/**
 * AT Server receive fixed-length data.
 *
 * @param client current AT Server object
 * @param buf   receive data buffer
 * @param size  receive fixed data size
 * @param timeout  receive data timeout (ms)
 *
 * @note this function can only be used in execution function of AT commands
 *
 * @return >0: receive data size
 *         <=0: receive failed
 */
int32_t qm_at_server_recv(void *channel, char *buf, uint32_t size, int32_t timeout)
{
    int32_t read_idx = 0;
    int32_t recv_len;
    char ch = 0;
    qm_at_channel_t *at_channel = (qm_at_channel_t*)channel;

    if (channel == NULL || buf == NULL || size == 0){
        return -QM_EINVAL;
    }

    while (1)
    {
        if (read_idx < size){
            /* check get data value */
            recv_len = at_channel->recv(at_channel->handle, &ch, 1, timeout);
            if (recv_len > 0){
                buf[read_idx++] = ch;
            }else{
                break;
            }
        }
        else{
            break;
        }
    }

    return read_idx;
}

static qm_err_t qm_at_check_args(const char *args, const char *args_format)
{
    uint32_t left_sq_bracket_num = 0, right_sq_bracket_num = 0;
    uint32_t left_angle_bracket_num = 0, right_angle_bracket_num = 0;
    uint32_t comma_mark_num = 0;
    uint32_t i = 0;

    QM_ASSERT(args);
    QM_ASSERT(args_format);

    for (i = 0; i < strlen(args_format); i++)
    {
        switch (args_format[i])
        {
        case AT_CMD_L_SQ_BRACKET:
            left_sq_bracket_num++;
            break;

        case AT_CMD_R_SQ_BRACKET:
            right_sq_bracket_num++;
            break;

        case AT_CMD_L_ANGLE_BRACKET:
            left_angle_bracket_num++;
            break;

        case AT_CMD_R_ANGLE_BRACKET:
            right_angle_bracket_num++;
            break;

        default:
            break;
        }
    }

    if (left_sq_bracket_num != right_sq_bracket_num || left_angle_bracket_num != right_angle_bracket_num
            || left_sq_bracket_num > left_angle_bracket_num)
    {
        return -QM_ERROR;
    }

    for (i = 0; i < strlen(args); i++)
    {
        if (args[i] == AT_CMD_COMMA_MARK)
        {
            comma_mark_num++;
        }
    }

    if ((comma_mark_num + 1 < left_angle_bracket_num - left_sq_bracket_num)
            || comma_mark_num + 1 > left_angle_bracket_num)
    {
        return -QM_ERROR;
    }

    return QM_EOK;
}

static qm_err_t qm_at_cmd_process(void *channel, qm_at_cmd_t *cmd, const char *cmd_args)
{
    qm_at_result_t result = QM_AT_RESULT_OK;

    QM_ASSERT(cmd);
    QM_ASSERT(cmd_args);

    if (cmd_args[0] == AT_CMD_EQUAL_MARK && cmd_args[1] == AT_CMD_QUESTION_MARK && cmd_args[2] == AT_CMD_CR)
    {
        if (cmd->test == NULL)
        {
            qm_at_server_print_result(channel, QM_AT_RESULT_CMD_ERR);
            return -QM_ERROR;
        }

        result = cmd->test(channel);
        qm_at_server_print_result(channel, result);
    }
    else if (cmd_args[0] == AT_CMD_QUESTION_MARK && cmd_args[1] == AT_CMD_CR)
    {
        if (cmd->query == NULL)
        {
            qm_at_server_print_result(channel, QM_AT_RESULT_CMD_ERR);
            return -QM_ERROR;
        }

        result = cmd->query(channel);
        qm_at_server_print_result(channel, result);
    }
    else if (cmd_args[0] == AT_CMD_EQUAL_MARK
            || (cmd_args[0] >= AT_CMD_CHAR_0 && cmd_args[0] <= AT_CMD_CHAR_9 && cmd_args[1] == AT_CMD_CR))
    {
        if (cmd->setup == NULL)
        {
            qm_at_server_print_result(channel, QM_AT_RESULT_CMD_ERR);
            return -QM_ERROR;
        }
#if 0
        if(qm_at_check_args(cmd_args, cmd->args_expr) < 0)
        {
            qm_at_server_print_result(channel, QM_AT_RESULT_CHECK_FAILE);
            return -QM_ERROR;
        }
#endif
        result = cmd->setup(channel, cmd_args);
        qm_at_server_print_result(channel, result);
    }
    else if (cmd_args[0] == AT_CMD_CR)
    {
        if (cmd->exec == NULL)
        {
            qm_at_server_print_result(channel, QM_AT_RESULT_CMD_ERR);
            return -QM_ERROR;
        }

        result = cmd->exec(channel);
        qm_at_server_print_result(channel, result);
    }
    else
    {
        return -QM_ERROR;
    }

    return QM_EOK;
}

static qm_at_cmd_t *qm_at_find_cmd(qm_at_server_t *at_server, char *cmd_name)
{
    uint16_t i = 0, j = 0;
    qm_at_cmd_t *cmd_func = NULL;

    QM_ASSERT(cmd_name);
    qm_at_cmd_table_t *cmd_table = &at_server->cmd_table;
    for (i = 0; i < CONFIG_QM_AT_CMD_TABLE_MAX_NUM; i++) {

        for (j = 0; j < cmd_table->at_cmd_num[i]; j++) {
            cmd_func = (qm_at_cmd_t *) ((cmd_table->at_cmd_table[i] + j));
            if ((strlen(cmd_name) == strlen(cmd_func->name)) &&
                (strcmp(cmd_name, cmd_func->name) == 0)) {
                return cmd_func;
            }
        }
    }
    return NULL;
}

static qm_err_t qm_at_cmd_get_name(const char *cmd_buffer, char *cmd_name)
{
    uint32_t cmd_name_len = 0, i = 0;

    QM_ASSERT(cmd_name);
    QM_ASSERT(cmd_buffer);

    for (i = 0; i < strlen(cmd_buffer) + 1; i++)
    {
        if (*(cmd_buffer + i) == AT_CMD_QUESTION_MARK || *(cmd_buffer + i) == AT_CMD_EQUAL_MARK
                || *(cmd_buffer + i) == AT_CMD_CR
                || (*(cmd_buffer + i) >= AT_CMD_CHAR_0 && *(cmd_buffer + i) <= AT_CMD_CHAR_9))
        {
            cmd_name_len = i;
            memcpy(cmd_name, cmd_buffer, cmd_name_len);
            *(cmd_name + cmd_name_len) = '\0';

            return QM_EOK;
        }
    }

    return -QM_ERROR;
}


static qm_at_server_t *qm_at_server_get_by_channel(qm_at_channel_t *at_channel)
{
    uint16_t index = 0;
    for(index = 0; index < CONFIG_QM_AT_SERVER_MAX_NUM; index++){
        if(&at_server_table[index].channel == at_channel){
            return &at_server_table[index];
        }
    }
    return -QM_ERROR;
}

int32_t qm_at_server_echo_mode_set(void *channel, uint8_t echo_mode)
{
    qm_at_server_t *at_server = qm_at_server_get_by_channel(channel);

    if(channel == NULL || (echo_mode != QM_AT_ECHO_MODE_CLOSE && echo_mode != QM_AT_ECHO_MODE_OPEN)){
        return -QM_EINVAL;
    }

    if(at_server == NULL){
        return -QM_EINVAL;
    }

    at_server->echo_mode = echo_mode;

    return QM_EOK;
}


static void at_server_parser(void* arg)
{
#define ESC_KEY                 0x1B
#define BACKSPACE_KEY           0x08
#define DELECT_KEY              0x7F

    int32_t channel_index = 0;
    int32_t ret = 0;
    qm_at_server_t *at_server = (qm_at_server_t*)arg;
    qm_at_channel_t *at_channel = &at_server->channel;
    qm_task_t task = {0};
    
    char cur_cmd_name[CONFIG_QM_AT_CMD_NAME_MAX_LEN] = { 0 };
    qm_at_cmd_t *cur_cmd = NULL;
    char *cur_cmd_args = NULL;
    char ch = '\0', last_ch = '\0';

    while (!at_server->exit)
    {
        ret = at_channel->recv(at_channel->handle, &ch, 1, QM_WAIT_FOREVER);
        if(ret < 0){
            if(at_server->cb){
                at_server->cb(QM_AT_RESULT_CHANNEL_DOWN, at_server->arg);
            }
            continue;
        }
        if(ret != 1){
            continue;
        }
        
        if (ESC_KEY == ch){
            break;
        }

        if (at_server->echo_mode)
        {
            if (ch == AT_CMD_CR || (ch == AT_CMD_LF && last_ch != AT_CMD_CR))
            {
                qm_at_server_printf((void*)at_channel, "%c%c", AT_CMD_CR, AT_CMD_LF);
            }
            else if (ch == AT_CMD_LF)
            {
                // skip the end sign check
            }
            else if (ch == BACKSPACE_KEY || ch == DELECT_KEY)
            {
                if (at_channel->cur_recv_len)
                {
                    at_channel->recv_buf[--at_channel->cur_recv_len] = 0;
                    qm_at_server_printf((void*)at_channel, "\b \b");
                }

                continue;
            }
            else
            {
                qm_at_server_printf((void*)at_channel, "%c", ch);
            }
        }

        at_channel->recv_buf[at_channel->cur_recv_len++] = ch;
        last_ch = ch;

        if(!strstr(at_channel->recv_buf, QM_AT_CMD_END_MARK))
        {
            continue;
        }

        if (qm_at_cmd_get_name(at_channel->recv_buf, cur_cmd_name) < 0)
        {
            qm_at_server_print_result((void*)at_channel, QM_AT_RESULT_CMD_ERR);
            goto __retry;
        }

        cur_cmd = qm_at_find_cmd(at_server, cur_cmd_name);
        if (!cur_cmd)
        {
            qm_at_server_print_result((void*)at_channel, QM_AT_RESULT_CMD_ERR);
            goto __retry;
        }

        cur_cmd_args = at_channel->recv_buf + strlen(cur_cmd_name);
        if (qm_at_cmd_process((void*)at_channel, cur_cmd, cur_cmd_args) < 0)
        {
            goto __retry;
        }

__retry:
        memset(at_channel->recv_buf, 0x00, at_channel->recv_buf_len);
        at_channel->cur_recv_len = 0;
    }

    memcpy(&task, &at_server->task, sizeof(task));

    memset(at_server, 0, sizeof(qm_at_server_t));

    qm_task_exit(&task);
}

static qm_err_t qm_check_cmd_tbl(qm_at_cmd_t *cmd_tbl, uint16_t cmd_num)
{
    uint16_t i = 0, j = 0;

    for (i = 0; i < cmd_num; i++) {
        if (strlen(cmd_tbl[i].name) >= CONFIG_QM_AT_CMD_NAME_MAX_LEN) {
            return -QM_EINVAL;
        }

        for (j = 0; j < cmd_num; j++) {
            if (i == j) {
                continue;
            }

            if (((strlen(cmd_tbl[j].name) == strlen(cmd_tbl[i].name)) &&
                (strcmp(cmd_tbl[j].name, cmd_tbl[i].name) == 0)) ||
                ((cmd_tbl[j].test != NULL) && (cmd_tbl[j].test == cmd_tbl[i].test)) ||
                ((cmd_tbl[j].query != NULL) && (cmd_tbl[j].query == cmd_tbl[i].query)) ||
                ((cmd_tbl[j].setup != NULL) && (cmd_tbl[j].setup == cmd_tbl[i].setup)) ||
                ((cmd_tbl[j].exec != NULL) && (cmd_tbl[j].exec == cmd_tbl[i].exec))) {
                return -QM_EINVAL;
            }
        }
    }

    return QM_EOK;
}

static qm_err_t qm_check_cmd_name_and_callback(qm_at_cmd_table_t *cmd_table, uint8_t tbl_index, 
                                                                qm_at_cmd_t *cmd_tbl,
                                                                uint16_t cmd_num)
{
    uint16_t i = 0, j =0;
    qm_at_cmd_t *cmd_func = NULL;

    for (i = 0; i < cmd_table->at_cmd_num[tbl_index]; i++) {
        cmd_func = (qm_at_cmd_t *)((cmd_table->at_cmd_table[tbl_index] + i));

        for (j = 0; j < cmd_num; j++) {
            if (((strlen(cmd_func->name) == strlen(cmd_tbl[j].name)) &&
                (strcmp(cmd_func->name, cmd_tbl[j].name)) == 0) ||
                ((cmd_tbl[j].test != NULL) && (cmd_func->test == cmd_tbl[j].test)) ||
                ((cmd_tbl[j].query != NULL) && (cmd_func->query == cmd_tbl[j].query)) ||
                ((cmd_tbl[j].setup != NULL) && (cmd_func->setup == cmd_tbl[j].setup)) ||
                ((cmd_tbl[j].exec != NULL) && (cmd_func->exec == cmd_tbl[j].exec))) {
                return -QM_EINVAL;
            }
        }
    }

    return QM_EOK;
}

int32_t qm_at_register_commands(qm_at_server_t *at_server, qm_at_cmd_t *cmd_tbl, uint16_t cmd_num)
{
    uint16_t i = 0;
    qm_err_t ret = QM_EOK;
    qm_at_cmd_table_t *cmd_table = NULL;

    if (cmd_tbl == NULL || cmd_num == 0) {
        return -QM_EINVAL;
    }
        
    ret = qm_check_cmd_tbl(cmd_tbl, cmd_num);
    if (ret != QM_EOK) {
        return ret;
    }

    cmd_table = &at_server->cmd_table;
    for (i = 0; i < CONFIG_QM_AT_CMD_TABLE_MAX_NUM; i++) {
        if ((cmd_table->at_cmd_table[i] == NULL) || (cmd_table->at_cmd_num[i] == 0)) {
            cmd_table->at_cmd_table[i] = cmd_tbl;
            cmd_table->at_cmd_num[i] = cmd_num;
            cmd_table->cmd_num += cmd_num;
            ret = QM_EOK;
            break;
        }

        ret = qm_check_cmd_name_and_callback(cmd_table, i, cmd_tbl, cmd_num);
        if (ret != QM_EOK) {
            break;
        }
    }

    return ret;
}

qm_at_server_t *qm_at_server_init(qm_at_server_param_t *param)
{
    int idx = 0;
    qm_at_channel_t *at_channel = NULL;
    qm_at_server_t *at_server = NULL;

    if(param == NULL){
        return -QM_EINVAL;
    }

    for (idx = 0; idx < CONFIG_QM_AT_SERVER_MAX_NUM && at_server_table[idx].channel.recv_buf; idx++);

    at_server = &at_server_table[idx];

    at_channel = &at_server->channel;
    at_server->echo_mode = param->echo_mode;
    at_channel->channel_type = param->channel_type;
    at_channel->handle = param->handle;
    at_channel->recv_buf = param->recv_buf;
    at_channel->recv_buf_len = param->recv_buf_len;
    at_channel->recv = param->recv;
    at_channel->send = param->send;

    memset(at_channel->recv_buf, 0x00, at_channel->recv_buf_len);

    at_server->parser_entry = at_server_parser;
    
    extern int32_t qm_at_base_cmd_init(qm_at_server_t *server);
    qm_at_base_cmd_init(at_server);

    qm_task_new(&at_server->task, "at_server", at_server->parser_entry, (void*)at_server, CONFIG_QM_AT_SERVER_TASK_SIZE, CONFIG_QM_AT_SERVER_TASK_PRIO);

    return (void*)at_server;

}


int32_t qm_at_server_deinit(qm_at_server_t *server)
{
    if(server == NULL){
        return -QM_EINVAL;
    }
    server->exit = 1;
    return QM_EOK;
}

int32_t qm_at_server_register_result_cb(qm_at_server_t *server, at_result_cb cb, void *arg)
{
    if(server == NULL){
        return -QM_EINVAL;
    }
    server->arg = arg;
    server->cb = cb;
    return QM_EOK;
}

#endif