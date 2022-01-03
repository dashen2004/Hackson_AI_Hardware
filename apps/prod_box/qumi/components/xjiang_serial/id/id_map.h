#ifndef __ID_MAP_H__
#define __ID_MAP_H__

#include "serial_common.h"
#include "qm_utils_list.h"

#define ALI_ID_MAP

#define ALI_DATA_TYPE_LEN    (4)
#define XM_DATA_TYPE_LEN     (5)

typedef enum
{
    ID_MAP_MIN=0,
    ID_MAP_ALI=1, //阿里id映射类型
    ID_MAP_XM,   //小米id映射类型
    ID_MAP_MAX,
}id_map_type_t;

typedef enum
{
    ID_MATCH_MIN=0,
    ID_MATCH_ID_BY_PLATFORM, //通过平台侧id信息查找小匠侧id信息
    ID_MATCH_PLATFORM_BY_ID, //通过小匠侧id信息查平台侧id信息
    ID_MATCH_MAX,
}id_match_type_t;

typedef struct{
    uint8_t type_id;
    uint8_t attr_id;
    uint16_t ali_attr_id;
}ali_data_type_t;


typedef struct{
    uint8_t type_id;
    uint8_t attr_id;
    uint8_t siid;
    uint8_t type;
    uint8_t xiid;
}xm_data_type_t;


typedef struct{
    id_map_type_t type;
    union{
        ali_data_type_t ali_data;
        xm_data_type_t xm_data;
    }data;
}id_map_data_t;


typedef struct{
    qm_list_t *list;

}id_map_context_t;

/**
 * @brief 解析id映射数据，以链表形式存储
 * @param[in] context @n
 * @param[in] data @n 数据指针
 * @param[in] len @n 数据长度
 * @param[in] type @n 解析数据类型，目前主要为阿里和小米的id映射关系
 * @return 错误码
 * @see None.
 * @note None.
 */
int32_t id_map_parse(id_map_context_t *context, uint8_t *data, uint16_t len, id_map_type_t type);
/**
 * @brief 查找id映射关系
 * @param[in] context @n id映射关系句柄
 * @param[in] map_data @n 数据集合
 * @param[in] match_type @n 查找类型
 * @return 映射关系句柄
 * @see None.
 * @note None.
 */
int32_t id_map_find(id_map_context_t *context, id_map_data_t *map_data, id_match_type_t match_type);


int32_t id_map_delete(id_map_context_t *context);

#endif

