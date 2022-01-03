#ifndef __MQTT_DEBUG_H__
#define __MQTT_DEBUG_H__

#include<qmos/qm_types.h>


#define IOT_DEBUG_PRINT  1

// ==== LOGGING ====

#if IOT_DEBUG_PRINT
#define mqtt_log(N,M, ...) \
	   do{printf("[%s][%s:%4d] " M "\r\n", N, __FUNCTION__, __LINE__, ##__VA_ARGS__);}while(0)

#define mqtt_hex_log(name, p_data, len) \
    do{\
        uint32_t i; \
        printf("[%s:%d]", __FUNCTION__,__LINE__);\
        printf("%s ",name);  \
        for(i=0;i<len;i++)\
        {\
          printf("%02X ", *(p_data+i));\
        }\
        printf("\n");\
      }while(0)
      

#else

  #define mqtt_log(N, M, ...)			 
#endif


#define POINTER_SANITY_CHECK(ptr, err) \
    do { \
        if (NULL == (ptr)) { \
            log_err("Invalid argument, %s = %p", #ptr, ptr); \
            return (err); \
        } \
    } while(0)

#define STRING_PTR_SANITY_CHECK(ptr, err) \
    do { \
        if (NULL == (ptr)) { \
            log_err("Invalid argument, %s = %p", #ptr, (ptr)); \
            return (err); \
        } \
        if (0 == strlen((ptr))) { \
            log_err("Invalid argument, %s = '%s'", #ptr, (ptr)); \
            return (err); \
        } \
    } while(0)



#define log_err(M, ...)   mqtt_log("err", M, ##__VA_ARGS__)
#define log_debug(M, ...)   mqtt_log("debug", M, ##__VA_ARGS__)
#define log_warning(M, ...)   mqtt_log("warning", M, ##__VA_ARGS__)
#define log_info(M, ...)   mqtt_log("info", M, ##__VA_ARGS__)


#endif
