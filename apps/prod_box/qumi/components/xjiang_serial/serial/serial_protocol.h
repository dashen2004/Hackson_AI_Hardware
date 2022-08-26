#ifndef _SERIAL_PROTOCOL_H_
#define _SERIAL_PROTOCOL_H_

#include "serial_common.h"
#include "qm_utils_timer.h"
#include "qm_utils_list.h"
#include "message.h"

#ifdef SERIAL_ID_MAP_SUPPORT
#include "id_map.h"
#endif

#ifdef SERIAL_OTA_SUPPORT
#include "serial_ota.h"
#endif


/* ============================================================================================== */
/*                                   MACRO & TYPE DEFINITION REGION                               */
/* ============================================================================================== */


#define SERIAL_NO_ERR              0
#define SERIAL_PARAM_ERR           1
#define SERIAL_ALLOC_MEM_ERR       2
#define SERIAL_CHKSUM_ERR          3
#define SERIAL_MSG_NUM_ERR         4
#define SERIAL_CMD_ERR             5
#define SERIAL_PARAM_LEN_ERR       6
#define SERIAL_PACKET_LEN_ERR      7
#define SERIAL_TAG_ERR             8
#define SERIAL_NO_INIT_ERR         9
#define SERIAL_PACKET_ID_ERR       10
#define SERIAL_DATA_TYPE_ERR       11
#define SERIAL_TYPE_ID_ERR         12
#define SERIAL_PROCESS_ERR         13
#define SERIAL_SEND_BUSY_ERR       14


enum{
	SERIAL_COM_OK,
	SERIAL_COM_ERROR,
	SERIAL_COM_OPERATION_NOT_PERMITTED,
	SERIAL_COM_IO_ERROR,
	SERIAL_COM_NO_DEVICE,
	SERIAL_COM_MEM_ERROR,
	SERIAL_COM_DEVICE_BUSY,
	SERIAL_COM_PARAM_ERROR,
	SERIAL_COM_FUNC_NOT_SUPPORT,
	SERIAL_COM_TIMEOUT,
	SERIAL_COM_COMMUNICATION_ERROR,
	SERIAL_COM_CHKSUM_ERROR,
};
/*
 *   ----------------------------------------------------------------
 *  |  HEAD  |  VER  |  CMD   |   LENGTH |    PAYLOAD    |  CHKSUM  |
 *   ----------------------------------------------------------------
 *    2Bytes   1Byte    1Byte     2Bytes      n Byte        1Byte
 */
#define FRAME_HEAD                  0xA5A5
#define FRAME_HEAD_PART             0xA5
#define FRAME_HEAD_LEN              6  /*HEAD + VER + CMD + LENGTH*/
#define FRAME_MIN_LEN               7  /*HEAD + VER + CMD + LENGTH + CHKSUM*/
#define PROTOCOL_VER                0x03
#define FRAME_DEVICEINFO_GET_MIN_LEN  (FRAME_MIN_LEN + 3)
#define FRAME_VER_LEN               (3)
#define FRAME_SNAPSHOT_MIN_LEN  (FRAME_MIN_LEN + 3)
#define FRAME_REPORT_MIN_LEN    (FRAME_MIN_LEN + 3)
#define FRAME_SETMODE_MIN_LEN   (FRAME_MIN_LEN + 3)

//service list
typedef enum
{
    MODULE_SERVICE_LOWPOWER = 0xD1,

}module_service_type_t;

//define module status
typedef enum
{
    MODULE_CONNECTED                        = 0x00,
    MODULE_CONFIGURD_UNCONNECT_ROUTER       = 0x01,
    MODULE_CONNECT_ROUTER_UNCONNECT_CLOUD   = 0x02,
    MODULE_CONFIGURING_NETWORK              = 0x03,
    MODULE_UNCONFIGURD_NETWORK              = 0x04,
    MODULE_ENTER_LOWPOWER              		= 0x05,
}module_status_t;

typedef enum
{
	MCU_ENTER_LOWPOWER              		= 0x01,
}mcu_status_t;

typedef enum
{
    MODULE_PRODTST_SUCCESS                  = 0x00,
    MODULE_PRODTST_START                    = 0x01,
    MODULE_PRODTST_FAILED                   = 0x02,
    MODULE_PRODTST_NFC                      = 0x03,
    MODULE_PRODTST_STATUS_NONE,
}module_prodtst_status_t;

typedef enum{
    MODULE_PRODTST_TYPE_START               = 0,
    MODULE_PRODTST_TYPE_NONE                = 0,
    MODULE_PRODTST_TYPE_COMPLETE_MACHINE    = 1,
    MODULE_PRODTST_TYPE_EXIT                = 2,
    MODULE_PRODTST_TYPE_MODULE              = 3, 
    MODULE_PRODTST_TYPE_END                   
}module_prodtst_type_t;

typedef enum
{
    CMD_SET                 = 0x01,
    CMD_SERVICE             = 0x02,
    CMD_GET                 = 0x03,
    CMD_NOTIFY_STATUS       = 0x22,
    CMD_REPORT              = 0x10,
    CMD_HEARTBEAT           = 0x20,
    CMD_GET_DEVICEINFO      = 0x21,
    CMD_GET_SNAPSHOT        = 0x24,
    CMD_MCU_NOTIFY_STATUS   = 0x25,
    CMD_TIMEGET             = 0x26,
    CMD_NETCONFIG           = 0x23,
    CMD_GETMOD              = 0x50,
    CMD_SETMOD              = 0x51,
    CMD_EVENT               = 0x11,
    CMD_ACTION              = 0x12,
    #ifdef SERIAL_OTA_SUPPORT
    CMD_OTA_START		    = 0x30,
    CMD_OTA_TRANSFER	    = 0x31,
    CMD_OTA_QUERYCRC        = 0x32,
    CMD_OTA_VERIFY          = 0x33,
    CMD_OTA_SWITCH          = 0x34,
    #endif
    CMD_PRODTST_ENTERY      = 0x61,
    CMD_PRODTST_STATUS      = 0x60,
} frame_cmd_t;

#pragma pack(1)

typedef struct
{
    uint16_t head;
    uint8_t  ver;
    uint8_t  cmd;
    uint16_t len;
} frame_header_t;

typedef struct
{
    frame_header_t fixed;
    uint8_t chksum;
}frame_unit_t;

typedef struct
{
    frame_header_t fixed;
    uint8_t data;
    uint8_t chksum;
}frame_generic_t;

typedef struct 
{
    int8_t timezone;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t wday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
}serial_time_info_t;

typedef struct 
{
    uint8_t code;
    serial_time_info_t info;
}serial_time_packet_t;

#pragma pack()

typedef uint32_t platform_id_t;

#define MIN_SERIAL_MCU_PACKET_ID   1
#define MAX_SERIAL_MCU_PACKET_ID   100
#define MIN_SERIAL_VOICE_PACKET_ID  248
#define MAX_SERIAL_VOICE_PACKET_ID  255

typedef enum{
    TYPE_NULL=0,
    TYPE_MCU=1,  //id 1~100
    TYPE_VOICE,  //id 248~255
    TYPE_MAX,
}serial_data_type_t;

typedef enum{
    NETWORK_CONFIG_START   = 0x01,
    NETWORK_CONFIG_DEFAULT = NETWORK_CONFIG_START,
    NETWORK_CONFIG_ONEKEY  = 0x02,
    NETWORK_CONFIG_AP      = 0x03,
    NETWORK_CONFIG_ZERO    = 0x04,
    NETWORK_CONFIG_BLUE    = 0x05,
    NETWORK_CONFIG_END     = NETWORK_CONFIG_BLUE ,
}network_config_type_t;

typedef enum{
    HEARTBEAT_RESET=0x00,
    HEARTBEAT_COMMON=0x01,
}heartbeat_type_t;

typedef struct{
    uint8_t main_ver;
    uint8_t sub_ver;
    uint8_t fix_ver;
}device_ver_t;

typedef enum{

    PARSE_STATE_NONE=0,
    PARSE_STATE_HEAD,
    PARSE_STATE_VER,
    PARSE_STATE_LEN,
    PARSE_STATE_DATA,
}parse_state_t;

typedef enum{
   SERIAL_STATE_NONE=0,
   SERIAL_STATE_HEARTBEAT,
   SERIAL_STATE_GETDEVICEINFO,
   SERIAL_STATE_GETDEVICESNAPSHOT,
   SERIAL_STATE_NORMAL,
}serial_state_t;

/* Minimum timeout interval of usart request in millisecond */
#define SERIAL_REQUEST_TIMEOUT_MIN_MS               (500)
/* Maximum timeout interval of usart request in millisecond */
#define SERIAL_REQUEST_TIMEOUT_MAX_MS               (1500)
#define SERIAL_REQUEST_TIMEOUT_DEFAULT_MS           (1000)
/* Default timeout interval of usart send in millisecond */
#define SERIAL_SEND_INTERVAL_TIMEOUT_DEFAULT_MS     (200)

#define SERIAL_KEPPALIVE_TIMEOUT_DEFAULT_MS         (10000)
#define SERIAL_KEPPALIVE_TIMEOUT_FAST_MS            (1000)

#define SERIAL_PROCESS_TIMEOUT_MS                  (1000)
#define SERIAL_RECV_TIMEOUT_DEFAULT_MS              (500)

#define SERIAL_ACK_TIMEOUT_DEFAULT_MS               (500)

#define SERIAL_ID_CHECK_TIMEOUT_MS                  (500)

#define SERIAL_SEND_QUEUE_NUM_MAX                    (20)
#define SERIAL_DOWN_QUEUE_NUM_MAX                     (10)
#define SERIAL_SEND_RETRY_COUNT_MAX                   (1)

typedef void (*serial_response_fn)(frame_cmd_t cmd, platform_id_t id, uint8_t *data, uint16_t len);
typedef void (*serial_report_fn)(frame_cmd_t cmd, uint8_t *data, uint16_t len);
typedef void (*serial_network_config_fn)(network_config_type_t type);
typedef void (*serial_device_online_fn)(device_ver_t *ver, uint8_t *data, uint16_t len);
typedef void (*serial_prodtst_config_fn)(module_prodtst_type_t type);
typedef void (*serial_mcu_status_notify_fn)(mcu_status_t status);
typedef int32_t (*serial_service_fn)(void *arg);
typedef int32_t (*serial_setmode_fn)(void *arg);
typedef int32_t (*serial_getmode_fn)(void *arg);

#ifdef SERIAL_RAW_SUPPORT
typedef void (*serial_raw_fn)(serial_data_type_t type, uint8_t *data, uint16_t len);
#endif

#ifdef SERIAL_HEARTBEAT_NOTIFY_SUPPORT
typedef void (*serial_heartbeat_fn)(uint8_t heartbeat_type);
#endif

typedef struct
{
    serial_report_fn                m_reportcb;
    serial_response_fn              m_responsecb;
    serial_network_config_fn        m_network_configcb;
    serial_device_online_fn         m_device_onlivecb;
    serial_prodtst_config_fn        m_prodtst_configcb;
    serial_mcu_status_notify_fn     m_mcu_status_notifycb;
#ifdef SERIAL_RAW_SUPPORT
    serial_raw_fn                   m_ramcb;
#endif
#ifdef SERIAL_HEARTBEAT_NOTIFY_SUPPORT
    serial_heartbeat_fn             m_heartbeatcb;
#endif
#ifdef SERIAL_OTA_SUPPORT
    serial_ota_event_notify_fn  	ota_event_notifycb;
#endif
}serial_callbacks_t;

typedef struct 
{
    serial_service_fn extern_fn;
    serial_service_fn inner_fn;
}serial_service_callbacks_t;

typedef struct
{
    serial_setmode_fn  m_setmode;
    serial_getmode_fn  m_getmode;
}serial_mode_callbacks_t;

typedef struct
{
    qm_list_node_t node;
    uint16_t len;
    uint8_t resend;
    uint8_t data[1];
}frame_info_t;

typedef struct{
    uint8_t *data;
    uint16_t len;
}serial_data_info_t;

typedef struct
{
    platform_id_t platform_id;
    uint32_t time;
    uint8_t is_update;
    uint8_t local_id;
    uint8_t retry_count;
    uint8_t *data;
}serial_id_info_t;

typedef struct{
	uint8_t is_changed;
	message_context_t *message_context;
}serial_snapshot_info_t;

#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
typedef struct{
    qm_list_t *serial_event_queue;
}serial_event_info_t;
#endif
#endif

typedef struct
{
    uint8_t is_exist;
    uint8_t is_status_changed;
    uint8_t module_prodtst_status;
    uint8_t module_status;
    uint8_t prodtst_type;
    uint8_t packet_id;
    uint8_t *pwrite_buf;
    uint32_t write_buf_size; /* Specify size of write-buffer in byte */

    uint8_t *pread_buf;
    uint32_t read_pos;
    uint32_t read_buf_size;  /* Specify size of read-buffer in byte */
    void *serial_mutex;
    void *send_sem;
    serial_data_info_t changed_data;
    serial_snapshot_info_t serial_snapshot_info;
#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
    serial_event_info_t event_info;
#endif
#endif
#ifdef SERIAL_ID_MAP_SUPPORT
    id_map_context_t map_context;
#endif
#ifdef SERIAL_MESH_GROUP_ADDR_SUPPORT
    uint16_t mesh_group_addr;
#endif
#ifdef SERIAL_PID_INT_SUPPORT
    uint32_t pid;
#endif
#ifdef SERIAL_PID_STRING_SUPPORT
    char *pid;
#endif

    parse_state_t parse_state;
    uint16_t need_len;
    serial_state_t state;
    qm_utils_time_t next_ping_time;
    uint16_t keep_alive_interval; //unit ms

    qm_utils_time_t process_get_time;
    uint16_t process_get_interval; //unit ms

    qm_utils_time_t send_interval_time;
    uint32_t send_interval; //unit ms

    qm_utils_time_t id_check_time;
    uint32_t id_check_interval; //unit ms

    qm_list_t *send_queue;
    qm_list_t *id_queue;
#ifdef SERIAL_RESEND_SUPPORT
    qm_list_t *down_queue;
#endif

    void *handle;

#ifdef SERIAL_OTA_SUPPORT
    serial_ota_context_t *ota_context;
#endif

    message_context_t *tlv_ctx;

    serial_callbacks_t callbacks;

}serial_context_t;


typedef struct
{
    uint8_t *pread_buf;
    uint32_t read_buf_size;  /* Specify size of read-buffer in byte */
    serial_callbacks_t callbacks;

}serial_param_t;

int32_t qm_serial_init( serial_param_t *serial_param);
int32_t qm_serial_recv_loop(int32_t timeout_ms);
int32_t qm_serial_send_loop(void);
int32_t qm_serial_cloud_set(platform_id_t id, uint8_t *data, uint16_t len);
int32_t qm_serial_cloud_get(platform_id_t id, uint8_t *data, uint16_t len);
int32_t qm_serial_cloud_action(platform_id_t id, uint8_t *data, uint16_t len);
module_status_t qm_serial_module_status_get(void);
int32_t qm_serial_module_status_set(module_status_t status);
serial_context_t *qm_serial_context_get(void);
int32_t qm_serial_state_set(serial_state_t state);
int32_t qm_serial_state_get(serial_state_t *state);
int32_t qm_serial_module_prodtst_status_set(module_prodtst_status_t status);
int32_t qm_serial_service_register(module_service_type_t service_type, serial_service_fn fn);
int32_t qm_serial_mode_register(uint8_t attr_id, serial_setmode_fn setmode_fn, serial_getmode_fn getmode_fn);

#ifdef SERIAL_RAW_SUPPORT
int32_t qm_serial_raw_send(uint8_t *data, uint16_t len);
#endif

#ifdef SERIAL_MESH_GROUP_ADDR_SUPPORT
int32_t mesh_group_addr_get(uint16_t *addr);
#endif

#ifdef SERIAL_ID_MAP_SUPPORT
id_map_context_t* id_map_context_get(void);
#endif

#if defined(SERIAL_PID_INT_SUPPORT) || defined(SERIAL_PID_STRING_SUPPORT)
void *qm_serial_pid_get(void);
#endif

serial_snapshot_info_t* qm_serial_snapshot_info_get(void);

#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
int32_t qm_serial_event_info_get(uint8_t *buf, uint16_t *buf_len);
uint8_t is_event_info_update(void);
#endif
#endif

#ifdef SERIAL_LOWPOWER_SUPPORT
int32_t qm_serial_process_reset(void);
#endif


#endif
