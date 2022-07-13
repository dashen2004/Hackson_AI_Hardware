#ifndef __SERIAL_COMMON_H__
#define __SERIAL_COMMON_H__

#include "qm_types.h"

#define SERIAL_SOFTWARE_VER  "3.0.0"

//#define SERIAL_ID_MAP_SUPPORT
//#define SERIAL_ALI_ID_MAP_SUPPORT
//#define SERIAL_XM_ID_MAP_SUPPORT
#define SERIAL_REPORT_SUPPORT
#define SERIAL_EVENT_SUPPORT
#define SERIAL_EVENT_CACHE_SUPPORT
//#define SERIAL_MESH_GROUP_ADDR_SUPPORT
#define SERIAL_RTOS_SUPPORT
#define SERIAL_RESEND_SUPPORT
//#define SERIAL_RAW_SUPPORT
//#define SERIAL_LOWPOWER_SUPPORT
//#define SERIAL_HEARTBEAT_NOTIFY_SUPPORT
//#define SERIAL_OTA_SUPPORT
#define SERIAL_QM_PRINTF_SUPPORT

#ifndef SERIAL_QM_PRINTF_SUPPORT
#define SERIAL_LOG(format, ...) \
    do{ \
        qm_serial_printf("[%s:%d]",__FUNCTION__,__LINE__);\
        qm_serial_printf(format,##__VA_ARGS__);\
        qm_serial_printf("\n");\
     }while(0)

#define SERIAL_HEX_LOG(name, p_data, len) \
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
      
#define SERIAL_LOG(format, ...)  qm_serial_printf("serial", format, ##__VA_ARGS__)
#define SERIAL_HEX_LOG(name, p_data, len) \
    do{\
        uint32_t i; \
        uint32_t length = len; \
        char hex_buf[256] = {0};\
        if(length > 128){ \
            length = 128; \
        } \
        hex_to_strs(p_data, length, hex_buf);\
        SERIAL_LOG("%s: %s", name, hex_buf);\
      }while(0)   
    
#endif

#endif
