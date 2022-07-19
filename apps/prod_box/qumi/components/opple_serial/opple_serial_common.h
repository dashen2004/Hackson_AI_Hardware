#ifndef __OPPLE_SERIAL_COMMON_H__
#define __OPPLE_SERIAL_COMMON_H__

#include "qm_types.h"

#define OPPLE_SERIAL_SOFTWARE_VER  "1.0.0"

#define OPPLE_SERIAL_REPORT_SUPPORT
#define OPPLE_SERIAL_EVENT_SUPPORT
#define OPPLE_SERIAL_EVENT_CACHE_SUPPORT
#define OPPLE_SERIAL_RTOS_SUPPORT
#define OPPLE_SERIAL_RESEND_SUPPORT
//#define OPPLE_SERIAL_RAW_SUPPORT
//#define OPPLE_SERIAL_OTA_SUPPORT
#define OPPLE_SERIAL_QM_PRINTF_SUPPORT

#ifndef OPPLE_SERIAL_QM_PRINTF_SUPPORT
#define OPPLE_SERIAL_LOG(format, ...) \
    do{ \
        qm_serial_printf("[%s:%d]",__FUNCTION__,__LINE__);\
        qm_serial_printf(format,##__VA_ARGS__);\
        qm_serial_printf("\n");\
     }while(0)

#define OPPLE_SERIAL_HEX_LOG(name, p_data, len) \
    do{\
        uint32_t i; \
        qm_serial_printf("[%s:%d]",__FUNCTION__,__LINE__);\
        qm_serial_printf("%s ",name);  \
        for(i=0;i<len;i++)\
        {\
          qm_serial_printf("%02X ", *(p_data+i));\
        }\
        qm_serial_printf("\n");\
      }while(0)
#else 
      
#define OPPLE_SERIAL_LOG(format, ...)           opple_serial_printf("serial", format, ##__VA_ARGS__)
#define OPPLE_SERIAL_HEX_LOG(name, p_data, len) opple_serial_hex_printf("opple serial", name, p_data, len)
    
#endif

#endif
