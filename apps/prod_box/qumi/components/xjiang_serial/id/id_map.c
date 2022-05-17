#include "id_map.h"
#include "serial_protocol.h"
#include "serial_platform.h"

static int32_t id_map_push(qm_list_t *list, uint8_t *data, uint16_t len);
static int32_t id_map_unit_parse(id_map_data_t *map_data, uint8_t *data, uint16_t len);
static int match_platform_by_id(void *a, void *b);
static int match_id_by_platform(void *a, void *b);
static int32_t id_match_set(id_map_context_t *context, id_match_type_t match_type);

static int32_t id_map_push(qm_list_t *list, uint8_t *data, uint16_t len)
{
    uint8_t *buf = NULL;
    uint16_t buf_len = 0;

    qm_list_node_t *node = NULL;

    buf_len = len+sizeof(qm_list_node_t);
    buf = qm_serial_malloc(buf_len);
    if(buf == NULL){
        return SERIAL_ALLOC_MEM_ERR;
    }
    node = (qm_list_node_t*)buf;

    memcpy(buf+sizeof(qm_list_node_t), data, len);

    qm_list_node_init(node, buf+sizeof(qm_list_node_t));

    qm_list_rpush(list, node);

    return SERIAL_NO_ERR;
}

static int32_t id_map_unit_parse(id_map_data_t *map_data, uint8_t *data, uint16_t len)
{
    switch(map_data->type){

#ifdef SERIAL_ALI_ID_MAP_SUPPORT
        case ID_MAP_ALI:
            map_data->data.ali_data.type_id = *(data+0);
            map_data->data.ali_data.attr_id = *(data+1);
            memcpy(&map_data->data.ali_data.ali_attr_id, data+2, 2);
            map_data->data.ali_data.ali_attr_id = be16_to_cpu(map_data->data.ali_data.ali_attr_id);
        break;
#endif
#ifdef SERIAL_XM_ID_MAP_SUPPORT
        case ID_MAP_XM:
            map_data->data.xm_data.type_id = *(data+0);
            map_data->data.xm_data.attr_id = *(data+1);
            map_data->data.xm_data.siid = *(data+2);
            map_data->data.xm_data.type = *(data+3);
            map_data->data.xm_data.xiid = *(data+4);
        break;
#endif
#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
        case ID_MAP_ONENET:
            map_data->data.onenet_data.type_id = *(data+0);
            map_data->data.onenet_data.attr_id = *(data+1);
            map_data->data.onenet_data.func_id = ((*(data+2))<<8) + *(data+3);
            map_data->data.onenet_data.type = *(data+4);
        break;
#endif
        default:
            return SERIAL_PARAM_ERR;
        break;
    }

    return SERIAL_NO_ERR;
}

int32_t id_map_delete(id_map_context_t *context)
{
    if(context == NULL)
        return SERIAL_PARAM_ERR;

    if(context->list){
        qm_list_destroy(context->list);
    }
    return SERIAL_NO_ERR;
}

int32_t id_map_parse(id_map_context_t *context, uint8_t *data, uint16_t len, id_map_type_t type)
{
    int32_t err = SERIAL_NO_ERR;
    uint16_t tmp_len = len;
    uint8_t *tmp_data = data;
    uint16_t uint_len = 0;
    uint16_t offset = 0;
    id_map_data_t id_map_data = {0};

    if(context == NULL || data == NULL || len == 0)
        return SERIAL_PARAM_ERR;
    if(type <= ID_MAP_MIN || type >= ID_MAP_MAX)
        return SERIAL_PARAM_ERR;

    switch(type){
#ifdef SERIAL_ALI_ID_MAP_SUPPORT
        case ID_MAP_ALI:
            if(len % ALI_DATA_TYPE_LEN != 0){
                return SERIAL_PARAM_LEN_ERR;
            }
            uint_len = ALI_DATA_TYPE_LEN;
        break;
#endif
#ifdef SERIAL_XM_ID_MAP_SUPPORT
        case ID_MAP_XM:

            if(len % XM_DATA_TYPE_LEN != 0){
                return SERIAL_PARAM_LEN_ERR;
            }
            uint_len = XM_DATA_TYPE_LEN;

        break;
#endif
#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
        case ID_MAP_ONENET:
            if(len % ONENET_DATA_TYPE_LEN != 0){
                return SERIAL_PARAM_LEN_ERR;
            }
            uint_len = ONENET_DATA_TYPE_LEN;
        break;
#endif
        default:
            return SERIAL_PARAM_ERR;
        break;
    }

    memset(context, 0, sizeof(id_map_context_t));

    context->list = qm_list_new();
    if(context->list == NULL){
        err = SERIAL_ALLOC_MEM_ERR;
        goto exit;
    }

    id_map_data.type=type;

    while(tmp_len){

        id_map_unit_parse(&id_map_data, tmp_data+offset, uint_len);
        err = id_map_push(context->list, (uint8_t*)&id_map_data, sizeof(id_map_data_t));
        if(err != SERIAL_NO_ERR){
            goto exit;
        }
        offset += uint_len;
        tmp_len -= uint_len;
    }

    return SERIAL_NO_ERR;

exit:
    return err;
}

static int match_platform_by_id(void *a, void *b)
{
    id_map_data_t *a_map_data = (id_map_data_t*)a;
    id_map_data_t *b_map_data = (id_map_data_t*)b;
#ifdef SERIAL_ALI_ID_MAP_SUPPORT
    if(a_map_data->type == ID_MAP_ALI){

        if(a_map_data->data.ali_data.attr_id == b_map_data->data.ali_data.attr_id){
            return 1;
        }else{
            return 0;
        }
    }
#endif
#ifdef SERIAL_XM_ID_MAP_SUPPORT
    if(a_map_data->type == ID_MAP_XM){
        if(a_map_data->data.xm_data.attr_id == b_map_data->data.xm_data.attr_id){
             return 1;
         }else{
             return 0;
         }
    }
#endif
#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
    if(a_map_data->type == ID_MAP_ONENET){
        if(a_map_data->data.onenet_data.attr_id == b_map_data->data.onenet_data.attr_id){
            return 1;
        }else{
            return 0;
        }
    }
#endif

    return 0;
}

static int match_id_by_platform(void *a, void *b)
{
    id_map_data_t *a_map_data = (id_map_data_t*)a;
    id_map_data_t *b_map_data = (id_map_data_t*)b;
#ifdef SERIAL_ALI_ID_MAP_SUPPORT
    if(a_map_data->type == ID_MAP_ALI){
        if(a_map_data->data.ali_data.ali_attr_id == b_map_data->data.ali_data.ali_attr_id){
            return 1;
        }else{
            return 0;
        }
    }
#endif
#ifdef SERIAL_XM_ID_MAP_SUPPORT
    if(a_map_data->type == ID_MAP_XM){
        if(a_map_data->data.xm_data.siid == b_map_data->data.xm_data.siid &&
           a_map_data->data.xm_data.type == b_map_data->data.xm_data.type &&
           a_map_data->data.xm_data.xiid == b_map_data->data.xm_data.xiid){
             return 1;
         }else{
             return 0;
         }
    }
#endif
#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
    if(a_map_data->type == ID_MAP_ONENET){
        if(a_map_data->data.onenet_data.func_id == b_map_data->data.onenet_data.func_id ){
             return 1;
         }else{
             return 0;
         }
    }
#endif

    return 0;
}

static int32_t id_match_set(id_map_context_t *context, id_match_type_t match_type)
{

    if(match_type == ID_MATCH_ID_BY_PLATFORM){
        context->list->match = match_id_by_platform;

    }else if(match_type == ID_MATCH_PLATFORM_BY_ID){
        context->list->match = match_platform_by_id;
    }

    return SERIAL_NO_ERR;
}

int32_t id_map_find(id_map_context_t *context, id_map_data_t *map_data, id_match_type_t match_type)
{
    id_map_data_t *tmp_map_data = NULL;
    qm_list_node_t *node = NULL;
    if(context == NULL || map_data == NULL)
        return SERIAL_PARAM_ERR;
    if(match_type <= ID_MATCH_MIN || match_type >= ID_MATCH_MAX)
        return SERIAL_PARAM_ERR;
    if(map_data->type <= ID_MAP_MIN || map_data->type >= ID_MAP_MAX)
        return SERIAL_PARAM_ERR;

    id_match_set(context, match_type);

    node = qm_list_find(context->list, map_data);
    if(node){
        tmp_map_data = (id_map_data_t*)node->val;
        memcpy(map_data, tmp_map_data, sizeof(id_map_data_t));
        return SERIAL_NO_ERR;
    }else{
        return SERIAL_PARAM_ERR;
    }
}


