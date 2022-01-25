#ifndef __PRODTST_TCP_MGR_H__
#define __PRODTST_TCP_MGR_H__


int prodtst_tcp_mgr_init(void);
int prodtst_tcp_mgr_deinit(void);

int prodtst_tcp_mgr_read(int fd, uint8_t *buf, int size, uint32_t timeout);
int prodtst_tcp_mgr_write(int fd, uint8_t *buf, int size);

int prodtst_tcp_mgr_set(int fd);
int prodtst_tcp_mgr_reset(int fd);

#endif

