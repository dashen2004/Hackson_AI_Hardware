#ifndef QM_BLE_OPT_H
#define QM_BLE_OPT_H

#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>


#define  EN_QM_BLE_AUTH
#define  EN_QM_COMBO_NET
#define  EN_QM_BLE_OTA
#define  EN_QM_BLE_MCU_OTA
#define  EN_QM_BLE_OTA_DATA_ASYNC

#define  EN_QM_BLE_POST_ASYNC
#define  QB_VERBOSE_DEBUG

#define QB_VERSION                      (2)

#define QB_AUTH_TIMEOUT               10000  // not allowed to be 0

#define QB_TRANSPORT_TIMEOUT          10000


#ifdef EN_LONG_MTU
#define QB_TRANSPORT_VER              1
#else
#define QB_TRANSPORT_VER              0
#endif

#define QB_ATT_HDR_SIZE               3
#define QB_FRAME_HDR_SIZE             4
#define QB_ENCRY_BLOCK_LENGTH         16     // aes 128, 16 byte a block
#define QB_MAX_FRAME_NUMBER           16     // user payload max frame number
#define QB_GATT_MTU_SIZE_DEFAULT      23     // connect init before mtu exchange, use default mtu size
#define QB_GATT_MTU_SIZE_MAX          247    // when use long mtu, adjust this max mtu value with BLE stack config
#define QB_GATT_MTU_SIZE_LIMIT        103

/* Larger MTU size consumes more memory. If out of memory, you need to reduce the MTU size */
#define QB_FRAME_SIZE_DEFAULT         (QB_GATT_MTU_SIZE_DEFAULT - QB_ATT_HDR_SIZE - QB_FRAME_HDR_SIZE)
#define QB_FRAME_SIZE_MAX             (QB_GATT_MTU_SIZE_MAX - QB_ATT_HDR_SIZE - QB_FRAME_HDR_SIZE)
#define QB_FRAME_SIZE_LIMIT           (QB_GATT_MTU_SIZE_LIMIT - QB_ATT_HDR_SIZE - QB_FRAME_HDR_SIZE)

#ifdef EN_QM_BLE_LONG_MTU
#define QB_MAX_PAYLOAD_SIZE           (QB_FRAME_SIZE_MAX * QB_MAX_FRAME_NUMBER)
#else
#define QB_MAX_PAYLOAD_SIZE           (QB_FRAME_SIZE_DEFAULT * QB_MAX_FRAME_NUMBER)
#endif

#ifdef  EN_QM_BLE_SECURE_ADV
#define QB_ENABLE_SECURE_ADV 1
#else
#define QB_ENABLE_SECURE_ADV 0
#endif

#ifdef  EN_QM_BLE_AUTH
#define QB_ENABLE_AUTH 1
#else
#define QB_ENABLE_AUTH 0
#endif

#ifdef  EN_QM_COMBO_NET
#define QB_ENABLE_COMBO_NET 1
#else 
#define QB_ENABLE_COMBO_NET 0
#endif

#ifdef EN_QM_BLE_POST_ASYNC
#define QB_ENABLE_POST_ASYNC  1
#else
#define QB_ENABLE_POST_ASYNC  0
#endif

#ifdef EN_QM_BLE_OTA
#define QB_ENABLE_OTA  1
#else
#define QB_ENABLE_OTA  0
#endif

#ifdef EN_QM_BLE_MCU_OTA
#define QB_ENABLE_MCU_OTA  1
#else
#define QB_ENABLE_MCU_OTA  0
#endif

#ifdef EN_QM_BLE_OTA_DATA_ASYNC
#define QB_ENABLE_OTA_DATA_ASYNC  1
#else
#define QB_ENABLE_OTA_DATA_ASYNC  0
#endif


#define custom_log(N,M, ...)  printf("[%s][%s:%4d] " M "\r\n", N, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define QM_BLE_FLOW(...)     custom_log("qm ble", __VA_ARGS__)
#define QM_BLE_DEBUG(...)    custom_log("qm ble", __VA_ARGS__)
#define QM_BLE_INFO(...)     custom_log("qm ble", __VA_ARGS__)
#define QM_BLE_WARN(...)     custom_log("qm ble", __VA_ARGS__)
#define QM_BLE_ERR(...)      custom_log("qm ble", __VA_ARGS__)
#define QM_BLE_FATAL(...)    custom_log("qm ble", __VA_ARGS__)
#define QM_BLE_TRACE(...)    custom_log("qm ble", __VA_ARGS__)
#define QM_BLE_EMERG(...)    custom_log("qm ble", __VA_ARGS__)

#define custom_hex_log(name, p_data, len) \
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
      


#if defined(QB_VERBOSE_DEBUG)
#define QM_BLE_VERBOSE(...)  custom_log("qm ble", __VA_ARGS__)
#else
#define QM_BLE_VERBOSE(...)
#endif

#if defined(BLE_4_0)
#define QB_BLUETOOTH_VER 0x00
#define QB_MAX_SUPPORTED_MTU 23
#elif defined(BLE_4_2)
#define QB_BLUETOOTH_VER 0x01
#define QB_MAX_SUPPORTED_MTU 247
#elif defined(BLE_5_0)
#define QB_BLUETOOTH_VER 0x10
#define QB_MAX_SUPPORTED_MTU 247
#else
#define QB_BLUETOOTH_VER 0x00
#define QB_MAX_SUPPORTED_MTU 23
#endif

#endif  // QM_BLE_OPT_H
