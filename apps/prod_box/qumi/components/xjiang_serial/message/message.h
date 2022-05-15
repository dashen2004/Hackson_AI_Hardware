#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "qm_types.h"

#define MESSAGE_INT_SUPPORT
#define MESSAGE_INT64_SUPPORT
#define MESSAGE_BYTES_SUPPORT
#define MESSAGE_GROUP_SUPPORT
#define MESSAGES_SYNC_SUPPORT
//#define MESSAGES_ASYNC_SUPPORT

#define MESSAGE_ID_INT_ATTR_ID      0xF1
#define MESSAGE_ID_STRING_ATTR_ID   0xFC
#define MESSAGE_XM_ID_MAP       0xF3
#define MESSAGE_ONENET_ID_MAP   0xF9
#define MESSAGE_PID_STRING      0xFA
#define MESSAGE_PID_INT         0xF7
#define MESSAGE_MESH_GROUP_ADDR 0xF4
#define RAW_DATA_ATTR_ID        0xFF

#define MAX_MESSAGE_BASIC_LEN  6 // typeid + attr_id + value(sizeof(int))
#define MIN_MESSAGE_LEN        3 // typeid + attr_id + value
#define DEFAULT_DATA_UNIT_LEN  8

#define message_malloc serial_malloc
#define message_free   serial_free

typedef enum{
    DATA_TYPE_MIN = 0,
    DATA_TYPE_BOOL = DATA_TYPE_MIN,
    DATA_TYPE_INT8 = 1,
    DATA_TYPE_UINT8 = 2,
    DATA_TYPE_INT16 = 3,
    DATA_TYPE_UINT16 = 4,
    DATA_TYPE_INT32 = 5,
    DATA_TYPE_UINT32 = 6,
    DATA_TYPE_INT64 = 7,
    DATA_TYPE_UINT64 = 8,
    DATA_TYPE_FLOAT32 = 9,
    DATA_TYPE_FLOAT64 = 10,
    DATA_TYPE_STRING = 11,
    DATA_TYPE_DATE = 12,
    DATA_TYPE_STRUCT = 13,
    DATA_TYPE_ARRAY = 14,
    DATA_TYPE_FLOAT_ONE_UINT16 = 15,
    DATA_TYPE_FLOAT_TWO_UINT16 = 16,
    DATA_TYPE_FLOAT_ONE_UINT32 = 17,
    DATA_TYPE_FLOAT_TWO_UINT32 = 18,
    DATA_TYPE_FLOAT_ONE_INT16 = 19,
    DATA_TYPE_FLOAT_TWO_INT16 = 20,
    DATA_TYPE_FLOAT_ONE_INT32 = 21,
    DATA_TYPE_FLOAT_TWO_INT32 = 22,
    DATA_TYPE_GROUP           = 23,
    DATA_TYPE_RAW,
    DATA_TYPE_MAX = DATA_TYPE_GROUP,
}data_type_t;

enum{
    TYPE_MESSAGE_ID=1,
    TYPE_MESSAGE_DATA=2,
    TYPE_MESSAGE_ALL_DATA=3,
};

typedef struct
{
    uint16_t len;
    uint8_t *data;
}data_info_t;

typedef struct
{
    uint8_t packet_sum;
    uint8_t packet_id;
    uint8_t unit_sum;
    uint8_t unit_num;
    uint8_t unit_len;
    data_info_t data_info;
}group_info_t;


typedef struct message
{
    uint8_t attr_id;
    uint8_t type_id;
    uint8_t is_changed;
    uint16_t len;
#ifdef MESSAGE_BYTES_SUPPORT
    uint8_t *data;
#endif
    union{
        bool_t  value_bool;
        uint8_t value_u8;
        int8_t value_i8;
        uint16_t value_u16;
        int16_t value_i16;
#ifdef MESSAGE_INT_SUPPORT
        uint32_t value_u32;
        int32_t value_i32;
#endif
#ifdef MESSAGE_INT64_SUPPORT
        uint64_t value_u64;
        int64_t value_i64;
#endif
#ifdef MESSAGE_FLOAT_SUPPORT
        float32_t value_f32;
        float64_t value_f64;
#endif
#ifdef MESSAGE_BYTES_SUPPORT
        uint8_t value_byte[DEFAULT_DATA_UNIT_LEN];
#endif
#ifdef MESSAGE_GROUP_SUPPORT
        group_info_t group_info;
#endif

    }value;

    struct message *next;

}message_info_t;

typedef enum{
    TYPE_MESSAGE_GET=0x03,
    TYPE_MESSAGE_SET=0x01,
    TYPE_MESSAGE_REPORT=0x10,
    TYPE_MESSAGE_BASIC = 0x00,
}message_type_t;

typedef struct
{
    message_type_t type;
    uint16_t count;
    struct message *object;
}message_context_t;

typedef struct
{
    void *(*malloc_fn)(int sz);
    void (*free_fn)(void *ptr);
}message_hooks_t;

typedef enum{
    PARSE_TYPE_COMMON=1,
    PARSE_TYPE_RAW,
    PARSE_TYPE_BASIC,
}message_parse_type_t;


#define IS_NEED_REPORT(type_id)    ( type_id&0x80 )
#define NEED_REPORT_CLEAR(type_id)    ( type_id = type_id&0x7F )
#define REAL_TYPEID_GET(type_id)     ( type_id&0x7F )


#define DATA_CHANGED_WAIT_REPORT(flag)       	do{\
													flag = 0; \
													flag |= 0x01;\
												}while(0)

#define DATA_CHANGED_WAIT_REPORT_CLEAR(flag)    (flag &= 0xFE)
#define IS_DATA_CHANGED_WAIT_REPORT(flag)    	(flag & 0x01)

#define DATA_CHANGED_WAIT_REPORT_ACK(flag)   	 do{\
													flag = 0; \
													flag |=  0x10;\
												}while(0)

#define IS_DATA_CHANGED_WAIT_REPORT_ACK(flag)   (flag & 0x10)
#define DATA_CHANGED_REPORT_SUCCESS(flag)    	(flag &= 0xEF)

#define DATA_CHANGED_REPORT_FAIL(flag)    	    do{\
													flag &= 0xEF; \
													flag |= 0x01;\
												}while(0)

typedef enum{
   SERIALIZE_TYPE_ALL = 1,
   SERIALIZE_TYPE_CHANGED = 2,
   SERIALIZE_TYPE_CHANGED_REPORT = 3,
   SERIALIZE_TYPE_CHANGED_ONLY = 4,
}message_serialize_type_t;

typedef enum{
	REPORT_RESULT_SUCCESS = 0x00,
	REPORT_RESULT_FAIL = 0x01,
}message_report_result_t;

typedef enum
{
	BYTE_ORDER_NONE = 0x00,
    BYTE_ORDER_LITTLE_ENDIAN = 0x01,
    BYTE_ORDER_BIG_ENDIAN = 0x02,
}byte_order_t;

/**
 * @brief ���Ľ���
 *
 * @param[in] data @n ��������ָ��
 * @param[in] len @n �������ݳ���
 * @param[in] type @n ��������
 * @return ���Ĳ������.
 * @see None.
 * @note None.
 */
message_context_t *message_parse(uint8_t *data, uint16_t len, message_parse_type_t type);
/**
 * @brief ������Դ���
 *
 * @param[in] context @n ���Ĳ������
 * @return none.
 * @see None.
 * @note None.
 */
void message_delete(message_context_t *context);

/**
 * @brief ������Դ���
 *
 * @param[in] context @n ���Ĳ������
 * @param[in] attr_id @n ����id
 * @return ���ݲ������.
 * @see None.
 * @note None.
 */
message_info_t *message_object_get(message_context_t *context, uint8_t attr_id);
/**
 * @brief ���Ĵ���
 *
 * @param[in] type @n ������������,�������Ϊ0������Ϊ�������ݵ�Ԫ��û�������
 * @return ���Ĳ������.
 * @see None.
 * @note None.
 */
message_context_t *message_creat(message_type_t type);
/**
 * @brief ����ʵ���������
 * @return ���ݾ��
 * @see None.
 * @note None.
 */
message_info_t *message_info_new(void);
/**
 * @brief ���˻�����������֮����������ӣ������ַ����������
 * @param[in] type_id @n ��������
 * @param[in] attr_id @n ����id
 * @param[in] data @n ���ݵ�Ԫָ��
 * @param[in] len @n ���ݳ���
 * @return none
 * @see None.
 * @note None.
 */
void message_data_add(message_context_t *context, data_type_t type_id, uint8_t attr_id, uint8_t *data, uint16_t len);
/**
 * @brief ��������ʵ������Ҫ������ͨ�������ͣ��������͵���������
 *
 * @param[in] context @n ���Ĳ������
 * @param[in] object @n ����ʵ�����
 * @return ���Ĳ������.
 * @see None.
 * @note None.
 */
void message_object_add(message_context_t *context, message_info_t *object);
/**
 * @brief �������л�
 *
 * @param[in] context @n ���Ĳ������
 * @param[in] buf @n ���ݿռ�ָ�룬����ΪNULL,�ɻ�ȡ���л�������ݳ���.
 * @param[in] len @n ���ݳ��ȣ����ݳ��ȱ���������л��������ʵ�ʳ���
 * @return none
 * @see None.
 * @note buf
 */
void message_serialize(message_context_t *context, uint8_t *buf, uint16_t *len, message_serialize_type_t type);

/**
 * @brief ��ȡ����͸�����ݵ�Ԫ
 *
 * @param[in] context @n ���Ĳ������
 * @param[in] data @n ����ָ��
 * @param[in] len @n ���ݳ���
 * @return none
 * @see None.
 * @note None.
 */
void message_raw_data_get(message_context_t *context, uint8_t **data, uint16_t *len);
/**
 * @brief ����͸����������
 *
 * @param[in] context @n ���Ĳ������
 * @param[in] data @n ����ָ��
 * @param[in] len @n ���ݳ���
 * @return none
 * @see None.
 * @note None.
 */
void message_raw_data_add(message_context_t *context, uint8_t *data, uint16_t len);

int32_t message_object_copy(message_info_t *dest_object, message_info_t *src_object);

int32_t message_value_set(message_info_t *object, uint8_t *data, uint8_t len, byte_order_t byte_order);

int32_t message_object_update(message_context_t *context, uint8_t *data, uint16_t len, uint8_t *is_update);

int32_t message_object_add_update(message_context_t *context, uint8_t *data, uint16_t len, uint8_t *is_update);

void message_data_get(message_info_t *object, uint8_t **data, uint16_t *len);

uint16_t data_unit_len_get(data_type_t type_id);

int32_t message_value_serialize(message_info_t *object, uint8_t *data, uint8_t len, byte_order_t byte_order);

void message_info_delete(message_info_t *object);

int32_t message_change_value_set(message_info_t *object, int8_t data);

int32_t message_sync_report_confirm(message_context_t *context, message_report_result_t result);

int32_t message_changed_flag_clear(message_context_t *context);

void message_serialize_respect_len(message_context_t *context, uint8_t *buf, uint16_t *len, message_serialize_type_t type, uint16_t expected_len);

uint8_t is_message_update(message_context_t *context);

int32_t message_serialize_by_attrid(message_context_t *context, uint8_t* attr_id, uint16_t attr_id_count, uint8_t *buf, uint16_t *len);


#endif
