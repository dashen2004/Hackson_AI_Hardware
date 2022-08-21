/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "qm_config.h"
#include "qm_types.h"
#include "qm_tcp.h"
#include "qm_errno.h"

/*** TCP connection ***/
static int read_tcp(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return (pNetwork->fd, buffer, len, timeout_ms);
}


static int write_tcp(util_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return qm_tcp_write(pNetwork->fd, buffer, len, timeout_ms);
}

static int disconnect_tcp(util_network_pt pNetwork)
{
    if (0 == pNetwork->handle) {
        return -QM_EINVAL;
    }

    qm_tcp_destroy(pNetwork->handle);
    pNetwork->handle = NULL;
    return QM_EOK;
}

static int connect_tcp(util_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        return -QM_EINVAL;
    }

    pNetwork->handle = qm_tcp_establish(pNetwork->pHostAddress, pNetwork->port);
    if (pNetwork->handle == NULL) {
        return -QM_EIO;
    }

    return QM_EOK;
}

/*** SSL connection ***/
#if CONFIG_QM_NETWORK_TLS_SUPPORT
static int read_ssl(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        return -QM_EINVAL;
    }

    return qm_ssl_read(pNetwork->handle, buffer, len, timeout_ms);
}

static int write_ssl(util_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        return -QM_EINVAL;
    }

    return hal_ssl_write(pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_ssl(util_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        return -QM_EINVAL;
    }

    hal_ssl_destroy((uintptr_t)pNetwork->handle);
    pNetwork->handle = NULL;

    return ;
}

static int connect_ssl(util_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        return -QM_EINVAL;
    }

    if (NULL != (pNetwork->handle = qm_ssl_establish(
                                     pNetwork->pHostAddress,
                                     pNetwork->port,
                                     pNetwork->ca_crt,
                                     pNetwork->ca_crt_len + 1))) {
        return QM_EOK;
    } else {
        /* TODO SHOLUD not remove this handle space */
        /* The space will be freed by calling disconnect_ssl() */
        /* util_memory_free((void *)pNetwork->handle); */
        return -QM_EIO;
    }
}
#endif  /* #if CONFIG_QM_NETWORK_TLS_SUPPORT */

/****** network interface ******/
int util_net_read(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    int ret = 0;

    if (NULL == pNetwork->ca_crt) {
        ret = read_tcp(pNetwork, buffer, len, timeout_ms);
#ifndef IOT_WITHOUT_TLS
    } else {
        ret = read_ssl(pNetwork, buffer, len, timeout_ms);
#endif
    }

    return ret;
}

int util_net_write(util_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    int     ret = 0;

    if (NULL == pNetwork->ca_crt) {
        ret = write_tcp(pNetwork, buffer, len, timeout_ms);
#if CONFIG_QM_NETWORK_TLS_SUPPORT
    } else {
        ret = write_ssl(pNetwork, buffer, len, timeout_ms);
#endif
    }
    return ret;
}

int util_net_disconnect(util_network_pt pNetwork)
{
    int     ret = 0;

    if (NULL == pNetwork->ca_crt) {
        ret = disconnect_tcp(pNetwork);
#if CONFIG_QM_NETWORK_TLS_SUPPORT
    } else {
       ret =  disconnect_ssl(pNetwork);
#endif
    }
    return  ret;
}

int util_net_connect(util_network_pt pNetwork)
{
    int  ret = 0;

    if (NULL == pNetwork->ca_crt) {
        ret = connect_tcp(pNetwork);
#if CONFIG_QM_NETWORK_TLS_SUPPORT
    } else {
        ret = connect_ssl(pNetwork);
#endif
    }

    return ret;
}

int util_net_init(util_network_pt pNetwork, const char *host, uint16_t port, const char *ca_crt)
{
    if (!pNetwork || !host) {
        return -QM_EINVAL;
    }
    pNetwork->pHostAddress = host;
    pNetwork->port = port;
    pNetwork->ca_crt = ca_crt;

    if (NULL == ca_crt) {
        pNetwork->ca_crt_len = 0;
    } else {
        pNetwork->ca_crt_len = strlen(ca_crt);
    }

    pNetwork->handle = 0;
    pNetwork->read = util_net_read;
    pNetwork->write = util_net_write;
    pNetwork->disconnect = util_net_disconnect;
    pNetwork->connect = util_net_connect;

    return 0;
}
