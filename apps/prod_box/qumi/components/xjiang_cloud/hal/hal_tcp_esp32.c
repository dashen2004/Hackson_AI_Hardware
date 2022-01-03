#include "qm_iot_import.h"
#include <stdio.h>
#include <string.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "iot_mqtt_debug.h"

#define platform_log(M, ...) mqtt_log("tcp", M, ##__VA_ARGS__)

/** @defgroup group_platform_network network
 *  @{
 */

/**
 * @brief Establish a TCP connection.
 *
 * @param [in] host: @n Specify the hostname(IP) of the TCP server
 * @param [in] port: @n Specify the TCP port of TCP server
 *
 * @return The handle of TCP connection.
   @retval   0 : Fail.
   @retval > 0 : Success, the value is handle of this TCP connection.
 */
int32_t hal_tcp_establish(const char *host, uint16_t port)
{
    int err ;
    int fd = -1;
    struct sockaddr_in addr;
    struct hostent *hostent_content = NULL;

    platform_log("host name: %s", host);
    hostent_content = gethostbyname( host );
    if(hostent_content==NULL)
    {
        platform_log("gethostbyname err");
       return 0;
    }

    fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        platform_log("socket creat err");
        return 0;
     }

     addr.sin_family = AF_INET;
     addr.sin_addr.s_addr = *(uint32_t *) (*hostent_content->h_addr_list);
     addr.sin_port = htons(port);

     platform_log("ip %s port %d fd %d\n", inet_ntoa(addr.sin_addr), port, fd);

     err = connect( fd, (struct sockaddr *) &addr, sizeof(addr) );
     if (err < 0) {
         platform_log("ERROR: Failed to connect socket %d.", fd);
         close(fd);
         return 0;
       }

    return fd;
}


static uint32_t _platform_time_left(uint32_t t_end, uint32_t t_now)
{
    uint32_t res;
    if(t_now - t_end < (UINT32_MAX/2)){
      return 0;
    }
    else{
      res=t_end-t_now;
      return res;
    }
    
}


/**
 * @brief Destroy the specific TCP connection.
 *
 * @param [in] fd: @n Specify the TCP connection by handle.
 *
 * @return The result of destroy TCP connection.
 * @retval < 0 : Fail.
 * @retval   0 : Success.
 */
int32_t hal_tcp_destroy(_IN_ int fd)
{
    int rc;

    //Shutdown both send and receive operations.
    rc = shutdown((int) fd, 2);

    if (0 != rc) {
        platform_log("shutdown error");
        return -1;
    }

    rc = close((int) fd);

    if (0 != rc) {
        platform_log("closesocket error");
        return -1;
    }

    return 0;

}


int32_t hal_tcp_write(int fd, const char *buf, uint32_t len, uint32_t timeout_ms)
{
    return send(fd, buf, len , 0);    
}


int32_t hal_tcp_read(int fd, char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret, err_code;
    uint32_t len_recv;
    uint32_t t_end, t_left;
    fd_set sets;
    struct timeval timeout;

    t_end = hal_uptime_ms() + timeout_ms;
    len_recv = 0;
    err_code = 0;

    do {
        t_left = _platform_time_left(t_end, hal_uptime_ms());
        FD_ZERO(&sets);
        FD_SET(fd, &sets);

        timeout.tv_sec = t_left / 1000;
        timeout.tv_usec = (t_left % 1000) * 1000;

        ret = select(fd + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            if (FD_ISSET(fd, &sets))
             {
                ret = recv(fd, buf + len_recv, len - len_recv, 0);
                if (ret > 0) 
                {
                    len_recv += ret;
                } else if (0 == ret) {
                    platform_log("connection is closed");
                    err_code = -1;
                    break;
                } 
                else 
                {
                    if (EINTR == errno) {
                        platform_log("EINTR be caught");
                        continue;
                    }
                    platform_log("recv fail");
                    err_code = -2;
                    break;
                 }
               }
            } else if (0 == ret) {
                break;
            } else {
                    platform_log("select-recv fail");
                    err_code = -2;
                    break;
        }
    } while ((len_recv < len) && (_platform_time_left(t_end, hal_uptime_ms()) > 0));

    /* priority to return data bytes if any data be received from TCP connection. */
    /* It will get error code on next calling */
    return (0 != len_recv) ? len_recv : err_code;
}


