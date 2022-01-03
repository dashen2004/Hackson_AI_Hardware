

#ifndef _IOT_COMMON_NET_H_
#define _IOT_COMMON_NET_H_

#include "qm_iot_import.h"


/**
 * @brief The structure of network connection(TCP or SSL).
 *   The user has to allocate memory for this structure.
 */

struct util_network;
typedef struct util_network util_network_t, *util_network_pt;

struct util_network {
    const char *pHostAddress;
    uint16_t port;
    uint16_t ca_crt_len;

    /**< NULL, TCP connection; NOT NULL, SSL connection */
    const char *ca_crt;

    /**< connection handle: 0, NOT connection; NOT 0, handle of the connection */
    uintptr_t handle;

    int fd;

    /**< Read data from server function pointer. */
    int (*read)(util_network_pt, char *, uint32_t, uint32_t);

    /**< Send data to server function pointer. */
    int (*write)(util_network_pt, const char *, uint32_t, uint32_t);

    /**< Disconnect the network */
    int (*disconnect)(util_network_pt);

    /**< Establish the network */
    int (*connect)(util_network_pt);
};


int util_net_read(util_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms);
int util_net_write(util_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms);
int iot_net_disconnect(util_network_pt pNetwork);
int iot_net_connect(util_network_pt pNetwork);
int iot_net_init(util_network_pt pNetwork, const char *host, uint16_t port, const char *ca_crt);

#endif /* IOT_COMMON_NET_H */
