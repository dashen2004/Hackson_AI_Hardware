#ifndef __ID_MAP_H__
#define __ID_MAP_H__

#include "qm_types.h"
#include "qm_utils_list.h"


typedef enum
{
    ID_MAP_MIN=0,
    ID_MAP_ALI=1, //����idӳ������
    ID_MAP_XM,   //С��idӳ������
    ID_MAP_ONENET,
    ID_MAP_MAX,
}id_map_type_t;

typedef enum
{
    ID_MATCH_MIN=0,
    ID_MATCH_ID_BY_PLATFORM, //ͨ��ƽ̨��id��Ϣ����С����id��Ϣ
    ID_MATCH_PLATFORM_BY_ID, //ͨ��С����id��Ϣ��ƽ̨��id��Ϣ
    ID_MATCH_MAX,
}id_match_type_t;


#ifdef SERIAL_ALI_ID_MAP_SUPPORT
typedef struct{
    uint8_t type_id;
    uint8_t attr_id;
    uint16_t ali_attr_id;
}ali_data_type_t;

#define ALI_DATA_TYPE_LEN       (4)
#endif
#ifdef SERIAL_XM_ID_MAP_SUPPORT
typedef struct{
    uint8_t type_id;
    uint8_t attr_id;
    uint8_t siid;
    uint8_t type;
    uint8_t xiid;
}xm_data_type_t;

#define XM_DATA_TYPE_LEN        (5)
#endif

#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
typedef struct{
    uint8_t type_id;
    uint8_t attr_id;
    uint16_t func_id;
    uint8_t type;
}onenet_data_type_t;

#define ONENET_DATA_TYPE_LEN    (5)

#endif

typedef struct{
    id_map_type_t type;
    union{
#ifdef SERIAL_ALI_ID_MAP_SUPPORT
        ali_data_type_t ali_data;
#endif
#ifdef SERIAL_XM_ID_MAP_SUPPORT
        xm_data_type_t xm_data;
#endif
#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
        onenet_data_type_t onenet_data;
#endif

    }data;
}id_map_data_t;


typedef struct{
    qm_list_t *list;

}id_map_context_t;

/**
 * @brief ����idӳ�����ݣ���������ʽ�洢
 * @param[in] context @n
 * @param[in] data @n ����ָ��
 * @param[in] len @n ���ݳ���
 * @param[in] type @n �����������ͣ�Ŀǰ��ҪΪ�����С�׵�idӳ���ϵ
 * @return ������
 * @see None.
 * @note None.
 */
int32_t id_map_parse(id_map_context_t *context, uint8_t *data, uint16_t len, id_map_type_t type);
/**
 * @brief ����idӳ���ϵ
 * @param[in] context @n idӳ���ϵ���
 * @param[in] map_data @n ���ݼ���
 * @param[in] match_type @n ��������
 * @return ӳ���ϵ���
 * @see None.
 * @note None.
 */
int32_t id_map_find(id_map_context_t *context, id_map_data_t *map_data, id_match_type_t match_type);


int32_t id_map_delete(id_map_context_t *context);

#endif

