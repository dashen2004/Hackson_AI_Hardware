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

#include <qmos/qm_types.h>
#include "qm_iot_import.h"
#include "util_net.h"
#include "iot_mqtt_debug.h"
#include "iot_guider.h"


/*** TCP connection ***/
static int read_tcp(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return hal_tcp_read(pNetwork->fd, buffer, len, timeout_ms);
}


static int write_tcp(util_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return hal_tcp_write(pNetwork->fd, buffer, len, timeout_ms);
}

static int disconnect_tcp(util_network_pt pNetwork)
{
    if (0 == pNetwork->handle) {
        return -1;
    }

    hal_tcp_destroy(pNetwork->fd);
    pNetwork->fd = 0 ;
    pNetwork->handle = 0;
    return 0;
}

static int connect_tcp(util_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return 1;
    }

    pNetwork->fd = hal_tcp_establish(pNetwork->pHostAddress, pNetwork->port);
    if (pNetwork->fd <0) {
        return -1;
    }

    pNetwork->handle=(uintptr_t)(&pNetwork->fd);

    return 0;
}

/*** SSL connection ***/
#ifndef IOT_WITHOUT_TLS
static int read_ssl(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return -1;
    }

    return hal_ssl_read((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int write_ssl(util_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return -1;
    }

    return hal_ssl_write((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_ssl(util_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return -1;
    }

    hal_ssl_destroy((uintptr_t)pNetwork->handle);
    pNetwork->handle = 0;

    return 0;
}

static int connect_ssl(util_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return 1;
    }

    if (0 != (pNetwork->handle = (intptr_t)hal_ssl_establish(
                                     pNetwork->pHostAddress,
                                     pNetwork->port,
                                     pNetwork->ca_crt,
                                     pNetwork->ca_crt_len + 1))) {
        return 0;
    } else {
        /* TODO SHOLUD not remove this handle space */
        /* The space will be freed by calling disconnect_ssl() */
        /* util_memory_free((void *)pNetwork->handle); */
        return -1;
    }
}
#endif  /* #ifndef IOT_WITHOUT_TLS */

/****** network interface ******/
int util_net_read(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    int     ret = 0;

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
#ifndef IOT_WITHOUT_TLS
    } else {
        ret = write_ssl(pNetwork, buffer, len, timeout_ms);
#endif
    }

    return ret;
}

int iot_net_disconnect(util_network_pt pNetwork)
{
    int     ret = 0;

    if (NULL == pNetwork->ca_crt) {
        ret = disconnect_tcp(pNetwork);
#ifndef IOT_WITHOUT_TLS
    } else {
       ret =  disconnect_ssl(pNetwork);
#endif
    }

    return  ret;
}

int iot_net_connect(util_network_pt pNetwork)
{
    int     ret = 0;

    if (NULL == pNetwork->ca_crt) {
        ret = connect_tcp(pNetwork);
#ifndef IOT_WITHOUT_TLS
    } else {
        ret = connect_ssl(pNetwork);
#endif
    }

    return ret;
}

int iot_net_init(util_network_pt pNetwork, const char *host, uint16_t port, const char *ca_crt)
{
    if (!pNetwork || !host) {
        log_err("parameter error! pNetwork=%p, host = %p", pNetwork, host);
        return -1;
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
    pNetwork->disconnect = iot_net_disconnect;
    pNetwork->connect = iot_net_connect;

    return 0;
}
