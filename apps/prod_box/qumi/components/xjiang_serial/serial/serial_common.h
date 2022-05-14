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


#define SERIAL_LOG(format, ...) \
    do{ \
        serial_printf("[%s:%d]",__FUNCTION__,__LINE__);\
        serial_printf(format,##__VA_ARGS__);\
        serial_printf("\n");\
     }while(0)

#define SERIAL_HEX_LOG(name, p_data, len) \
    do{\
        uint32_t i; \
        serial_printf("[%s:%d]",__FUNCTION__,__LINE__);\
        serial_printf("%s ",name);  \
        for(i=0;i<len;i++)\
        {\
          serial_printf("%02X ", *(p_data+i));\
        }\
        serial_printf("\n");\
      }while(0)


#endif
