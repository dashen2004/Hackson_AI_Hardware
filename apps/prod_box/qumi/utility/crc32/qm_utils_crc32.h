#ifndef QM_UTILS_CRC32_H_
#define QM_UTILS_CRC32_H_

#include <qmos/qm_types.h>

uint32_t qm_utils_crc32(uint8_t const * p_data, uint32_t size, uint32_t init_crc);

#endif
