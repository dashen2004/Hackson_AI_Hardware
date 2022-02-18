/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-14     chenyong     first version
 */

#include "qm_at.h"
#include "qmos/qm_types.h"
#include "qmos/qm_errno.h"
#include "qm_log.h"

static char send_buf[CONFIG_QM_AT_CMD_MAX_LEN];
static uint32_t last_cmd_len = 0;

#if CONFIG_QM_AT_PRINT_RAW_CMD
/**
 * dump hex format data to console device
 *
 * @param name name for hex object, it will show on log header
 * @param buf hex buffer
 * @param size buffer size
 */
void qm_at_print_raw_cmd(const char *name, const char *buf, uint32_t size)
{
#define __is_print(ch)       ((unsigned int)((ch) - ' ') < 127u - ' ')
#define WIDTH_SIZE           32

    uint32_t i, j;

    for (i = 0; i < size; i += WIDTH_SIZE)
    {
        printf("[D/AT] %s: %04X-%04X: ", name, i, i + WIDTH_SIZE);
        for (j = 0; j < WIDTH_SIZE; j++)
        {
            if (i + j < size)
            {
                printf("%02X ", buf[i + j]);
            }
            else
            {
                printf("   ");
            }
            if ((j + 1) % 8 == 0)
            {
                printf(" ");
            }
        }
        printf("  ");
        for (j = 0; j < WIDTH_SIZE; j++)
        {
            if (i + j < size)
            {
                printf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
            }
        }
        printf("\n");
    }
}

#endif

const char *qm_at_get_last_cmd(uint32_t *cmd_size)
{
    *cmd_size = last_cmd_len;
    return send_buf;
}


int32_t qm_at_vprintf(void *channel, const char *format, va_list args)
{
    int32_t ret = 0;
    char *big_buf = NULL;
    qm_at_channel_t *at_channel = (qm_at_channel_t*)channel;
    char *buf = send_buf+2;
    uint32_t len = sizeof(send_buf)-2;
    last_cmd_len = vsnprintf(buf, len, format, args);
    if(last_cmd_len > len){
        big_buf = (char*)qm_malloc(last_cmd_len+1+2);
        if(big_buf == NULL){
            return -QM_ENOMEM;
        }
        buf = big_buf+2;
        last_cmd_len = vsnprintf(buf, last_cmd_len+1, format, args);
        memcpy(big_buf, "\r\n", 2);
#if CONFIG_QM_AT_PRINT_RAW_CMD
        qm_at_print_raw_cmd("sendline", big_buf, last_cmd_len+2);
#endif
        ret = at_channel->send(at_channel->handle, big_buf, last_cmd_len+2);
        qm_free(big_buf);
        return ret;

    }else{
        memcpy(send_buf, "\r\n", 2);

#if CONFIG_QM_AT_PRINT_RAW_CMD
        qm_at_print_raw_cmd("sendline", send_buf, last_cmd_len+2);
#endif

        return at_channel->send(at_channel->handle, send_buf, last_cmd_len+2);
    }

}

int32_t qm_at_vprintfln(void *channel, const char *format, va_list args)
{
    int32_t ret = 0;
    char *big_buf = NULL;
    qm_at_channel_t *at_channel = (qm_at_channel_t*)channel;

    char *buf = send_buf+2;
    uint32_t len = sizeof(send_buf)-4;

    last_cmd_len = vsnprintf(buf, len, format, args);
    if(last_cmd_len > len){
        big_buf = (char*)qm_malloc(last_cmd_len + 1 + 4);
        if(big_buf == NULL){
            return -QM_ENOMEM;
        }
        buf = big_buf+2;
        last_cmd_len = vsnprintf(buf, last_cmd_len + 1, format, args);
        memcpy(big_buf, "\r\n", 2);
        memcpy(big_buf + last_cmd_len + 2, "\r\n", 2);
#if CONFIG_QM_AT_PRINT_RAW_CMD
        qm_at_print_raw_cmd("sendline", big_buf, last_cmd_len + 4);
#endif
        ret = at_channel->send(at_channel->handle, big_buf, last_cmd_len + 4);
        qm_free(big_buf);
        return ret;

    }else{
        memcpy(send_buf, "\r\n", 2);
        memcpy(buf + last_cmd_len, "\r\n", 2);

#if CONFIG_QM_AT_PRINT_RAW_CMD
        qm_at_print_raw_cmd("sendline", send_buf, last_cmd_len+4);
#endif
        return at_channel->send(at_channel->handle, send_buf, last_cmd_len+4);
    }
}
