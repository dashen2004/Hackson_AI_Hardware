#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <stdint.h>

void scanner_init(void);

int scanner_read(uint8_t* buf, uint32_t length, uint32_t timeout);

#endif
