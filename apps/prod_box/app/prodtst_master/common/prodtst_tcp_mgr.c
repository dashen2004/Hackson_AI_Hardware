#include "qm_log.h"
#include "qm.h"
#include "prodtst_core.h"
#include "qm_ringbuf.h"
#include "prodtst_tcp_mgr.h"
#include "prodtst_hal_tcp.h"
#include "prodtst_stream.h"

#define LOG_TAG  "prodtst tcp"

#define PRODTST_TCP_CLIENT_TASK_SIZE   (5*1024)
#define PRODTST_TCP_CLIENT_TASK_PRIO   (18)
#define PRODTST_TCP_STREAM_SIZE        (512)

typedef struct{
    int fd;
    void *stream;
}socket_info_t;

typedef struct {
    qm_task_t task;
    socket_info_t socket_info[CONFIG_QM_PRODTST_OBJ_NUM];
}prodtst_tcp_info_t;

static prodtst_tcp_info_t g_prodtst_tcp_info = {0};

static void prodtst_tcp_info_init(void);
static socket_info_t *prodtst_socket_get_byfd(int fd);

void prodtst_tcp_client_task(void *arg)
{
    int index = 0;
    int recv_len = 0;
    int fd = -1;
    #define PRODTST_TCP_RECV_SIZE 256
    uint8_t buf[PRODTST_TCP_RECV_SIZE] = {0};
    socket_info_t *socket_info = NULL;

    while(1){
        
        for(index = 0; index < CONFIG_QM_PRODTST_OBJ_NUM; index++){
            fd = g_prodtst_tcp_info.socket_info[index].fd;
            if(fd == -1){
                continue;
            }

            recv_len = prodtst_tcp_try_read(fd, buf, PRODTST_TCP_RECV_SIZE);
            if(recv_len < 0){
                prodtst_tcp_mgr_reset(fd);
            }else if(recv_len > 0){
                socket_info = prodtst_socket_get_byfd(fd);
                prodtst_stream_write(socket_info->stream, buf, recv_len);
            }
        }
        qm_msleep(50);
    }
}


static void prodtst_tcp_info_init(void)
{
    memset(&g_prodtst_tcp_info, 0, sizeof(prodtst_tcp_info_t));
    for(int i = 0; i < CONFIG_QM_PRODTST_OBJ_NUM; i++){
        g_prodtst_tcp_info.socket_info[i].fd = -1;
    }
}

static socket_info_t *prodtst_socket_get_byfd(int fd)
{
    for(int i = 0; i < CONFIG_QM_PRODTST_OBJ_NUM; i++){
        if(g_prodtst_tcp_info.socket_info[i].fd == fd){
            return &g_prodtst_tcp_info.socket_info[i];
        }
    }
    return NULL;
}

int prodtst_tcp_mgr_set(int fd)
{
    socket_info_t *socket_info = NULL;
    socket_info = prodtst_socket_get_byfd(fd);
    if(socket_info){
        return QM_EOK;
    }

    for(int i = 0; i < CONFIG_QM_PRODTST_OBJ_NUM; i++){
        if(g_prodtst_tcp_info.socket_info[i].fd == -1){
            g_prodtst_tcp_info.socket_info[i].fd = fd;
            if(g_prodtst_tcp_info.socket_info[i].stream){
                return QM_EOK;
            }

            g_prodtst_tcp_info.socket_info[i].stream = prodtst_stream_creat(PRODTST_TCP_STREAM_SIZE);
            if(g_prodtst_tcp_info.socket_info[i].stream == NULL){
                return -QM_ENOMEM;
            }
        }
    }

    return -QM_EFULL;
} 

int prodtst_tcp_mgr_reset(int fd)
{
    socket_info_t *socket_info = NULL;
    socket_info = prodtst_socket_get_byfd(fd);
    if(socket_info == NULL){
        return -QM_EIO;
    }

    socket_info->fd = -1;

    prodtst_stream_flush(socket_info->stream);
    return QM_EOK;
} 

int prodtst_tcp_mgr_read(int fd, uint8_t *buf, int size, uint32_t timeout)
{
    qm_err_t ret = QM_EOK;
    int recv_len = 0;
    socket_info_t *socket_info = NULL;
    socket_info = prodtst_socket_get_byfd(fd);
    if(socket_info == NULL){
        return -QM_EIO;
    }

    return prodtst_stream_read(socket_info->stream, buf, size, timeout);
}


int prodtst_tcp_mgr_write(int fd, uint8_t *buf, int size)
{
    qm_err_t ret = QM_EOK;
    ret = prodtst_tcp_write(fd, buf, size, 0);
    if(ret > 0){
        return ret;
    }else{
        prodtst_tcp_destroy(fd);
        return -QM_EIO;
    }
}

int prodtst_tcp_mgr_init(void)
{
    qm_err_t ret = QM_EOK;

    prodtst_tcp_info_init();

    ret = qm_task_new(&g_prodtst_tcp_info.task, "prodtst tcp", prodtst_tcp_client_task, NULL, 
                      PRODTST_TCP_CLIENT_TASK_SIZE, PRODTST_TCP_CLIENT_TASK_PRIO);
    if(ret != QM_EOK){
        return ret;
    }
    return ret;
}

int prodtst_tcp_mgr_deinit(void)
{
    qm_err_t ret = QM_EOK;

    ret = qm_task_exit(&g_prodtst_tcp_info.task);
    if(ret != QM_EOK){
        return ret;
    }

    return ret;
}