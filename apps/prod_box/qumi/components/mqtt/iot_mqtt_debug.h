#ifndef __MQTT_DEBUG_H__
#define __MQTT_DEBUG_H__

#include "qm_types.h"
#include "qm_log.h"
#include "qm_config.h"

#ifndef CONFIG_MQTT_DEBUG_PRINT
#define CONFIG_MQTT_DEBUG_PRINT  1
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

#if CONFIG_MQTT_DEBUG_PRINT

#define log_err(M, ...)         QM_LOGE("mqtt", M, ##__VA_ARGS__)
#define log_debug(M, ...)       QM_LOGD("mqtt", M, ##__VA_ARGS__)
#define log_warning(M, ...)     QM_LOGD("mqtt", M, ##__VA_ARGS__)
#define log_info(M, ...)        QM_LOGI("mqtt", M, ##__VA_ARGS__)

#else

#define log_err(M, ...)        
#define log_debug(M, ...)      
#define log_warning(M, ...)     
#define log_info(M, ...)        

#endif


#endif
