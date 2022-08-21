#ifndef _QM_UDP_H_
#define _QM_UDP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm_types.h"

#define QM_NETWORK_ADDR_LEN        (16)        /* IP网络地址的长度 */

typedef struct
{
    char addr[QM_NETWORK_ADDR_LEN];  /* 目标UDP主机地址, 点分十进制IP地址 */
    uint16_t port; /* 目标UDP端口, 范围是0-65535 */
}qm_netaddr_t;

/**
 * @brief Create a udp server with the specified port.
 * @param [in] host: @n Specify the hostname(IP) of the UDP server
 * @param[in] port @n The specified udp sever listen port.
 * @return Server handle.
 @verbatim
 =  NULL: fail.
 != NULL: success.
 @endverbatim
 * @see None.
 * @note It is recommended to add handle value by 1, if 0(NULL) is a valid handle value in your platform.
 */
void *qm_udp_server_create(char *host, uint16_t port);

/**
 * @brief Establish a UDP connection.
 *
 * @param [in] host: @n Specify the hostname(IP) of the UDP server
 * @param [in] port: @n Specify the UDP port of UDP server
 *
 * @retval  < 0 : Fail.
 * @retval >= 0 : Success, the value is handle of this UDP connection.
 * @see None.
 */
void *qm_udp_client_create(char *host, uint16_t port);

/**
 * @brief Closes an existing udp connection.
 *
 * @param[in] handle @n the specified connection.
 * @retval  < 0 : Fail.
 * @retval >= 0 : Success
 * @see None.
 * @note None.
 */
int32_t qm_udp_close(void *handle);

/**
 * @brief Sends data to a specific destination.
 *
 * @param[in] handle @n A descriptor identifying a connection.
 * @param[in] buffer @n A pointer to a buffer containing the data to be transmitted.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @param[in] netaddr @n A pointer to a netaddr structure that contains the address of the target.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 * @return
 @verbatim
 > 0: the total number of bytes sent, which can be less than the number indicated by length.
 = -1: error occur.
 @endverbatim
 * @see None.
 * @note blocking API.
 */
int32_t qm_udp_sendto(void *handle, const char *buffer,
                uint32_t length, qm_netaddr_t *netaddr, uint32_t timeout_ms);

/**
 * @brief Receives data from a udp connection.
 *
 * @param[in] handle @n A descriptor identifying a connection.
 * @param[out] buffer @n A pointer to a buffer to receive incoming data.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @param[out] netaddr @n A pointer to a netaddr structure that contains the address of the source.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 * @return
 @verbatim
 >  0: The total number of bytes received, which can be less than the number indicated by length.
 <  0: Error occur.
 @endverbatim
 *
 * @see None.
 * @note blocking API.
 */
int32_t qm_udp_recvfrom( void *handle, char *buffer,
                  uint32_t length, qm_netaddr_t *netaddr, uint32_t timeout_ms);

/**
 * @brief Receives data from a udp connection.
 *
 * @param[in] handle @n A descriptor identifying a connection.
 * @param[out] buffer @n A pointer to a buffer to receive incoming data.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @return
 @verbatim
 >  0: The total number of bytes received, which can be less than the number indicated by length.
 <  0: Error occur.
 @endverbatim
 *
 * @see None.
 * @note blocking API.
 */
int32_t qm_udp_read(void *handle, char *buffer, uint32_t length);

/**
 * @brief Receives data from a udp connection.
 *
 * @param[in] handle @n A descriptor identifying a connection.
 * @param[out] buffer @n A pointer to a buffer to receive incoming data.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 * @return
 @verbatim
 >  0: The total number of bytes received, which can be less than the number indicated by length.
 <  0: Error occur.
 @endverbatim
 *
 * @see None.
 * @note blocking API.
 */
int32_t qm_udp_readtimeout(void *handle, char *buffer, uint32_t length, uint32_t timeout_ms);


/**
 * @brief Sends data to a specific destination.
 *
 * @param[in] handle @n A descriptor identifying a connection.
 * @param[in] buffer @n A pointer to a buffer containing the data to be transmitted.
 * @param[in] length @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 * @return
 @verbatim
 > 0: the total number of bytes sent, which can be less than the number indicated by length.
 = -1: error occur.
 @endverbatim
 * @see None.
 * @note blocking API.
 */
int32_t qm_udp_write(void *handle, char *buffer, uint32_t length, uint32_t timeout_ms);


#ifdef __cplusplus
}
#endif
#endif 