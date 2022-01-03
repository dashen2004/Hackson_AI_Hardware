#ifndef _QM_H
#define _QM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <config/qm_config.h>
//#include <qmos/qm_cli.h>
#include <qmos/qm_kernel.h>
//#include <qmos/qm_log.h>
#include <qmos/qm_errno.h>
#include <qmos/qm_types.h>


/**
 * Transmit data on a UART interface
 *
 * @param  data     pointer to the start of data
 * @param  size     number of bytes to transmit
 * @param  timeout  time to wait
 *
 * @return  0 success, EIO if an error occurred with any step
 */
int32_t qm_dbg_write(void *data, uint32_t size, uint32_t timeout);


/**
 * Recv data on a UART interface
 *
 * @param  data     pointer to the start of recv buf
 * @param  expect_size     number of bytes expect to recv
 * @param  timeout  time to wait
 *
 * @return  0 success, EIO if an error occurred with any step
 */
int32_t qm_dbg_read(void *data, uint32_t expect_size, uint32_t timeout);



#ifdef __cplusplus
}
#endif

#endif /* QM_QM_H */

