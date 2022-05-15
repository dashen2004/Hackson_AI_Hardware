#include "message.h"
#include "serial_protocol.h"
#include "serial_platform.h"

static int32_t type_id_check(data_type_t type_id);
static uint8_t is_expected_len(message_info_t *object, uint16_t offset, uint16_t expected_len);

static int32_t type_id_check(data_type_t type_id)
{
    if(type_id < DATA_TYPE_MIN || type_id >DATA_TYPE_MAX){
        return SERIAL_TYPE_ID_ERR;
    }else{
        return SERIAL_NO_ERR;
    }
}

uint16_t data_unit_len_get(data_type_t type_id)
{

    switch(type_id){
        case DATA_TYPE_BOOL:
        case DATA_TYPE_UINT8:
        case DATA_TYPE_INT8:
            return sizeof(uint8_t);
        break;
        case DATA_TYPE_UINT16:
        case DATA_TYPE_INT16:
        case DATA_TYPE_FLOAT_ONE_UINT16:
        case DATA_TYPE_FLOAT_TWO_UINT16:
        case DATA_TYPE_FLOAT_ONE_INT16:
        case DATA_TYPE_FLOAT_TWO_INT16:
            return sizeof(uint16_t);
        break;
#ifdef MESSAGE_INT_SUPPORT
        case DATA_TYPE_UINT32:
        case DATA_TYPE_INT32:
        case DATA_TYPE_FLOAT_ONE_UINT32:
        case DATA_TYPE_FLOAT_TWO_UINT32:
        case DATA_TYPE_FLOAT_ONE_INT32:
        case DATA_TYPE_FLOAT_TWO_INT32:
            return sizeof(uint32_t);
        break;
#endif
#ifdef MESSAGE_INT64_SUPPORT
        case DATA_TYPE_UINT64:
        case DATA_TYPE_INT64:
            return sizeof(uint64_t);
        break;
#endif
        default:
            return 0;
        break;
    }
}

int32_t message_change_value_set(message_info_t *object, int8_t data)
{

    switch(object->type_id){
        case DATA_TYPE_UINT8:
            object->value.value_u8 += data;
        break;
        case DATA_TYPE_INT8:
            object->value.value_i8 += data;
        break;
        case DATA_TYPE_UINT16:
        case DATA_TYPE_FLOAT_ONE_UINT16:
        case DATA_TYPE_FLOAT_TWO_UINT16:
            object->value.value_u16 += data;
        break;
        case DATA_TYPE_INT16:
        case DATA_TYPE_FLOAT_ONE_INT16:
        case DATA_TYPE_FLOAT_TWO_INT16:
            object->value.value_i16 += data;
        break;
#ifdef MESSAGE_INT_SUPPORT
        case DATA_TYPE_UINT32:
        case DATA_TYPE_FLOAT_ONE_UINT32:
        case DATA_TYPE_FLOAT_TWO_UINT32:
            object->value.value_u32 += data;
        break;
        case DATA_TYPE_INT32:
        case DATA_TYPE_FLOAT_ONE_INT32:
        case DATA_TYPE_FLOAT_TWO_INT32:
            object->value.value_i32 += data;
        break;
#endif
#ifdef MESSAGE_INT64_SUPPORT
        case DATA_TYPE_UINT64:
        	object->value.value_u64 += data;
        break;

        case DATA_TYPE_INT64:
        	object->value.value_i64 += data;
        break;
#endif
        default:
            break;
    }
    return SERIAL_NO_ERR;

}

int32_t message_value_set(message_info_t *object, uint8_t *data, uint8_t len, byte_order_t byte_order)
{
    uint16_t data_u16 = 0;
    uint32_t data_u32 = 0;
    uint64_t data_u64 = 0;

    switch(REAL_TYPEID_GET(object->type_id)){
        case DATA_TYPE_BOOL:
            object->value.value_bool = *data;
            object->len = len;
        break;
        case DATA_TYPE_UINT8:
            object->value.value_u8=*data;
            object->len = len;
        break;
        case DATA_TYPE_INT8:
            object->value.value_i8=*data;
            object->len = len;
        break;
        case DATA_TYPE_UINT16:
        case DATA_TYPE_FLOAT_ONE_UINT16:
        case DATA_TYPE_FLOAT_TWO_UINT16:
            memcpy(&data_u16, data, sizeof(uint16_t));
            if(byte_order == BYTE_ORDER_BIG_ENDIAN){
                object->value.value_u16 = be16_to_cpu(data_u16);
            }else{
                object->value.value_u16 = data_u16;
            }
            object->len = len;
        break;
        case DATA_TYPE_INT16:
        case DATA_TYPE_FLOAT_ONE_INT16:
        case DATA_TYPE_FLOAT_TWO_INT16:
            memcpy(&data_u16, data, sizeof(uint16_t));
            if(byte_order == BYTE_ORDER_BIG_ENDIAN){
                object->value.value_i16 = be16_to_cpu(data_u16);
            }else{
                object->value.value_i16 = data_u16;
            }
            object->len = len;
        break;
#ifdef MESSAGE_INT_SUPPORT
        case DATA_TYPE_UINT32:
        case DATA_TYPE_FLOAT_ONE_UINT32:
        case DATA_TYPE_FLOAT_TWO_UINT32:
            memcpy(&data_u32, data, sizeof(uint32_t));
            if(byte_order == BYTE_ORDER_BIG_ENDIAN){
                object->value.value_u32 = be32_to_cpu(data_u32);
            }else{
                object->value.value_u32 = data_u32;
            }
            object->len = len;
        break;
        case DATA_TYPE_INT32:
        case DATA_TYPE_FLOAT_ONE_INT32:
        case DATA_TYPE_FLOAT_TWO_INT32:
            memcpy(&data_u32, data, sizeof(uint32_t));
            if(byte_order == BYTE_ORDER_BIG_ENDIAN){
                object->value.value_i32 = be32_to_cpu(data_u32);
            }else{
                object->value.value_i32 = data_u32;
            }
            object->len = len;
        break;
#endif
#ifdef MESSAGE_INT64_SUPPORT
        case DATA_TYPE_UINT64:
            memcpy(&data_u64, data, sizeof(uint64_t));
            if(byte_order == BYTE_ORDER_BIG_ENDIAN){
                object->value.value_u64 = be64_to_cpu(data_u64);
            }else{
                object->value.value_u64 = data_u64;
            }
            object->len = len;
        break;
        case DATA_TYPE_INT64:
            memcpy(&data_u64, data, sizeof(uint64_t));
            if(byte_order == BYTE_ORDER_BIG_ENDIAN){
                object->value.value_i64 = be64_to_cpu(data_u64);
            }else{
                object->value.value_i64 = data_u64;
            }
            object->len = len;
        break;
#endif
        case DATA_TYPE_STRING:
        case DATA_TYPE_ARRAY:
            if(len <= DEFAULT_DATA_UNIT_LEN){
            	if(object->len > DEFAULT_DATA_UNIT_LEN && object->data){
           			message_free(object->data);
                	object->data = NULL;
            	}
                memcpy(object->value.value_byte, data, len);
                object->data = object->value.value_byte;
            }else{
            	if(object->data){
            		if(object->len != len){
            			message_free(object->data);
            			object->data = NULL;
            		}
            	}
            	if(object->data == NULL){
					object->data = message_malloc(len);
					if(object->data == NULL){
						return SERIAL_ALLOC_MEM_ERR;
					}
            	}
				memcpy(object->data, data, len);
            }
            object->len = len;
        break;
        
#ifdef MESSAGE_GROUP_SUPPORT
        case DATA_TYPE_GROUP:

        break;
#endif

        default:
            break;
    }

    return SERIAL_NO_ERR;
}

message_context_t *message_parse(uint8_t *data, uint16_t len, message_parse_type_t parse_type)
{
    int32_t err=SERIAL_NO_ERR;
    uint8_t *tmp_data=data;
    uint16_t tmp=0;
    uint16_t offset=0;
    uint16_t data_len=0;
    message_info_t *object=NULL;
    message_info_t *pre_object=NULL;
    message_context_t *message_context=NULL;
    data_type_t type_id=0;
    uint8_t attr_id=0;

    if(data==NULL || len==0){
        return NULL;
    }
    if(len < MIN_MESSAGE_LEN){
        return NULL;
    }

    message_context=(message_context_t*)message_malloc(sizeof(message_context_t));
    if(message_context==NULL){
        goto exit;
    }
    memset(message_context, 0, sizeof(message_context_t));

    if(parse_type==PARSE_TYPE_BASIC){
        goto basic;
    }

    message_context->type=*data;
    ++offset;

    if(*(data+2) != MESSAGE_ID_INT_ATTR_ID && *(data+2) != MESSAGE_ID_STRING_ATTR_ID){
        goto raw;
    }

basic:
    while(offset < len){
        type_id=*(tmp_data+offset);
        ++offset;
        err=type_id_check(REAL_TYPEID_GET(type_id));
        if(err!=SERIAL_NO_ERR){
            goto exit;
        }
        attr_id=*(tmp_data+offset);
        ++offset;

        object=message_malloc(sizeof(message_info_t));
        if(object==NULL){
            goto exit;
        }
        memset(object, 0, sizeof(message_info_t));

        if(message_context->object==NULL){
            message_context->object=object;
            pre_object=object;
        }else{
            pre_object->next=object;
            pre_object=object;
        }

        object->type_id=type_id;
        object->attr_id=attr_id;
        object->is_changed = 0;

        tmp=data_unit_len_get(REAL_TYPEID_GET(type_id));
        if(tmp!=0){
            if(tmp > len-offset){
                goto exit;
            }
            message_value_set(object, tmp_data+offset, tmp, BYTE_ORDER_BIG_ENDIAN);
            offset+=tmp;
            object->len = tmp;
            DATA_CHANGED_WAIT_REPORT(object->is_changed);
        }else{
            if(len-offset < MIN_MESSAGE_LEN){
                goto exit;
            }
            memcpy(&data_len, tmp_data+offset, sizeof(uint16_t));
            data_len = be16_to_cpu(data_len);
            offset+=2;
            if(data_len > len-offset){
                goto exit;
            }
            if(data_len <= DEFAULT_DATA_UNIT_LEN){
                memcpy(object->value.value_byte, tmp_data+offset, data_len);
                object->len = data_len;
                object->data = object->value.value_byte;
            }else{
                object->data=message_malloc(data_len);
                if(object->data==NULL){
                    goto exit;
                }
                memcpy(object->data, tmp_data+offset, data_len);
                object->len=data_len;
            }
            offset+=data_len;
            DATA_CHANGED_WAIT_REPORT(object->is_changed);
        }
        message_context->count++;
        if(parse_type==PARSE_TYPE_RAW){
            goto raw;
        }
    }

    return message_context;

raw:

    object=message_malloc(sizeof(message_info_t));
    if(object==NULL){
        goto exit;
    }
    memset(object, 0, sizeof(message_info_t));

    object->type_id=DATA_TYPE_RAW;
    object->attr_id=RAW_DATA_ATTR_ID;

    if(message_context->object==NULL){
        message_context->object=object;
        pre_object=object;
    }else{
        pre_object->next=object;
        pre_object=object;
    }

    DATA_CHANGED_WAIT_REPORT(object->is_changed);

    message_context->count++;

    if(len-offset <= DEFAULT_DATA_UNIT_LEN){
        memcpy(object->value.value_byte, tmp_data+offset, len-offset);
        object->len=len-offset;
        object->data = object->value.value_byte;
    }else{

        object->data=message_malloc(len-offset);
        if(object->data==NULL){
            goto exit;
        }
        memcpy(object->data, tmp_data+offset, len-offset);
        object->len=len-offset;
    }

    return message_context;

exit:
    message_delete(message_context);
    return NULL;
}

message_info_t *message_object_get(message_context_t *context, uint8_t attr_id)
{
    message_info_t *head_object=NULL;
    if(context==NULL)
         return NULL;

    head_object=context->object;

    while(head_object){
        if(head_object->attr_id==attr_id){
            return head_object;
        }
        head_object=head_object->next;
    }
    return NULL;
}

void message_data_get(message_info_t *object, uint8_t **data, uint16_t *len)
{
    if(object==NULL || len==NULL)
       return;

    *data=object->data;
    *len=object->len;
}

void message_delete(message_context_t *context)
{
    message_info_t *head_object=NULL;
    message_info_t *tmp_object=NULL;
    if(context==NULL)
        return;
    head_object=context->object;

    while(head_object){
        if(head_object->len > DEFAULT_DATA_UNIT_LEN){
            message_free(head_object->data);
        }
        tmp_object=head_object;
        head_object=head_object->next;
        message_free(tmp_object);
    }
    message_free(context);
}

message_info_t *message_info_new(void)
{
    message_info_t *object=NULL;
    object=message_malloc(sizeof(message_info_t));
    if(object==NULL){
        return NULL;
    }
    memset(object, 0, sizeof(message_info_t));
    return object;
}

void message_info_delete(message_info_t *object)
{
    if(object){
        message_free(object);
    }
}

message_context_t *message_creat(message_type_t type)
{
    message_context_t *context=NULL;
    context=message_malloc(sizeof(message_context_t));
    if(context==NULL){
        return NULL;
    }
    memset(context, 0, sizeof(message_context_t));

    context->type=type;
    return context;
}

void message_object_add(message_context_t *context, message_info_t *object)
{
    message_info_t *head_object=NULL;
    message_info_t *pre_object=NULL;
    if(context==NULL || object==NULL)
        return;

    head_object=context->object;
    if(head_object){
        while(head_object){
            pre_object=head_object;
            head_object=head_object->next;

        }
        pre_object->next=object;
    }else{
        context->object=object;
    }
    context->count++;
}

void message_data_add(message_context_t *context, data_type_t type_id, uint8_t attr_id, uint8_t *data, uint16_t len)
{
    message_info_t *object=NULL;
    if(context==NULL || data==NULL || len==0){
        return;
    }
    object=message_info_new();
    if(!object)
        return;

    object->attr_id=attr_id;
    object->type_id=type_id;

    if(len <=DEFAULT_DATA_UNIT_LEN){
        memcpy(object->value.value_byte, data, len);
        object->len=len;
        object->data = object->value.value_byte;
    }else{
        object->data=message_malloc(len);
        if(object->data==NULL){
            goto exit;
        }
        memcpy(object->data, data, len);
        object->len=len;
    }

    message_object_add(context, object);
    return;

exit:
    message_free(object);
}

int32_t message_value_serialize(message_info_t *object, uint8_t *data, uint8_t len, byte_order_t byte_order)
{
    uint16_t tmp_u16 = 0;
    int16_t tmp_i16 = 0;
    uint32_t tmp_u32 = 0;
    int32_t tmp_i32 = 0;
    uint64_t tmp_u64 = 0;
    int64_t tmp_i64 = 0;
    switch(REAL_TYPEID_GET(object->type_id)){
        case DATA_TYPE_BOOL:
            *data=object->value.value_bool;
        break;
        case DATA_TYPE_UINT8:
            *data=object->value.value_u8;
        break;
        case DATA_TYPE_INT8:
            *data = object->value.value_i8;
        break;
        case DATA_TYPE_UINT16:
        case DATA_TYPE_FLOAT_ONE_UINT16:
        case DATA_TYPE_FLOAT_TWO_UINT16:
            if(byte_order == BYTE_ORDER_LITTLE_ENDIAN){
                tmp_u16 = cpu_to_be16(object->value.value_u16);
            }else{
                tmp_u16 = object->value.value_u16;
            }
            memcpy(data, &tmp_u16, sizeof(uint16_t));
        break;
        case DATA_TYPE_INT16:
        case DATA_TYPE_FLOAT_ONE_INT16:
        case DATA_TYPE_FLOAT_TWO_INT16:
            if(byte_order == BYTE_ORDER_LITTLE_ENDIAN){
                tmp_i16 = cpu_to_be16(object->value.value_i16);
            }else{
                tmp_i16 = object->value.value_i16;
            }
            memcpy(data, &tmp_i16, sizeof(uint16_t));
        break;
#ifdef MESSAGE_INT_SUPPORT
        case DATA_TYPE_UINT32:
        case DATA_TYPE_FLOAT_ONE_UINT32:
        case DATA_TYPE_FLOAT_TWO_UINT32:
            if(byte_order == BYTE_ORDER_LITTLE_ENDIAN){
                tmp_u32 = cpu_to_be32(object->value.value_u32);
            }else{
                tmp_u32 = object->value.value_u32;
            }
            memcpy(data, &tmp_u32, sizeof(uint32_t));
        break;
        case DATA_TYPE_INT32:
        case DATA_TYPE_FLOAT_ONE_INT32:
        case DATA_TYPE_FLOAT_TWO_INT32:
            if(byte_order == BYTE_ORDER_LITTLE_ENDIAN){
                tmp_i32 = cpu_to_be32(object->value.value_i32);
            }else{
                tmp_i32 = object->value.value_i32;
            }
            memcpy(data, &tmp_i32, sizeof(int32_t));
        break;
#endif
#ifdef MESSAGE_INT64_SUPPORT
        case DATA_TYPE_UINT64:
            if(byte_order == BYTE_ORDER_LITTLE_ENDIAN){
                tmp_u64 = cpu_to_be64(object->value.value_u64);
            }else{
            	tmp_u64 = object->value.value_u64;
            }
            memcpy(data, &tmp_u64, sizeof(uint64_t));
        break;
        case DATA_TYPE_INT64:
            if(byte_order == BYTE_ORDER_LITTLE_ENDIAN){
                tmp_i64 = cpu_to_be64(object->value.value_i64);
            }else{
            	tmp_i64 = object->value.value_i64;
            }
            memcpy(data, &tmp_i64, sizeof(int64_t));
        break;
#endif
        default:
            break;
    }
    return SERIAL_NO_ERR;
}

void message_serialize(message_context_t *context, uint8_t *buf, uint16_t *len, message_serialize_type_t type)
{
	message_serialize_respect_len(context, buf, len, type, 0);
}

static uint8_t is_expected_len(message_info_t *object, uint16_t offset, uint16_t expected_len)
{
	uint16_t value_len = 0;
    if(object == NULL || expected_len ==0){
    	return 1;
    }

	value_len = data_unit_len_get(REAL_TYPEID_GET(object->type_id));
	if(value_len){
		if(offset+value_len+2 <= expected_len){
			return 1;
		}else{
			return 0;
		}
	}else{
		value_len = object->len;
		if(offset+value_len+4 <= expected_len){
			return 1;
		}else{
			return 0;
		}
	}
}

uint8_t is_message_update(message_context_t *context)
{
    message_info_t *head_object = NULL;
    if(context == NULL)
        return 0;

    head_object = context->object;

    while(head_object){

		if(IS_DATA_CHANGED_WAIT_REPORT(head_object->is_changed)){
			return 1;
		}
        head_object=head_object->next;
    }
    return 0;
}

void message_serialize_respect_len(message_context_t *context, uint8_t *buf, uint16_t *len, message_serialize_type_t type, uint16_t expected_len)
{
    uint16_t message_len = 0;
    uint16_t offset = 0;
    uint16_t value_len = 0;
    uint8_t data_unit_len = 0;
    uint16_t sum_len = 0;
    message_info_t *head_object = NULL;
    if(context == NULL || len == NULL)
        return;

    head_object = context->object;

    while(head_object){

        if(type != SERIALIZE_TYPE_ALL){
            if(!IS_DATA_CHANGED_WAIT_REPORT(head_object->is_changed)){
                head_object=head_object->next;
                continue;
            }
        }
        data_unit_len=data_unit_len_get(REAL_TYPEID_GET(head_object->type_id));
        if(data_unit_len){
            message_len+=2+data_unit_len;
        }else{
            if(REAL_TYPEID_GET(head_object->type_id)!=DATA_TYPE_RAW){
            	message_len+=2+sizeof(uint16_t)+head_object->len;
            }else{
            	message_len+=head_object->len;
            }
        }
        head_object=head_object->next;
    }

    head_object=context->object;

    if(context->type==0){
        sum_len=message_len;
    }else{
        sum_len=message_len+1;
    }

    if(!buf){
        *len=sum_len;
    }else{
    	if(expected_len == 0){
		   if(*len < sum_len){
			   return;
		   }
    	}

		if(context->type){
			*buf=context->type;
			++offset;
		}

		while(head_object){

			if(IS_DATA_CHANGED_WAIT_REPORT(head_object->is_changed)){
				if(type == SERIALIZE_TYPE_CHANGED){
					if(!is_expected_len(head_object, offset, expected_len)){
						break;
					}
					DATA_CHANGED_WAIT_REPORT_CLEAR(head_object->is_changed);
				}else if(type == SERIALIZE_TYPE_CHANGED_REPORT){
					if(!IS_DATA_CHANGED_WAIT_REPORT_ACK(head_object->is_changed)){
						if(!is_expected_len(head_object, offset, expected_len)){
							break;
						}
						DATA_CHANGED_WAIT_REPORT_ACK(head_object->is_changed);
					}else{
						head_object = head_object->next;
						continue;
					}
				}else if(type == SERIALIZE_TYPE_CHANGED_ONLY){
					if(!IS_DATA_CHANGED_WAIT_REPORT_ACK(head_object->is_changed)){
						if(!is_expected_len(head_object, offset, expected_len)){
							break;
						}
					}else{
						head_object = head_object->next;
						continue;
					}
				}else{
					DATA_CHANGED_WAIT_REPORT_CLEAR(head_object->is_changed);
				}
			}else{
				if(type != SERIALIZE_TYPE_ALL){
					head_object = head_object->next;
					continue;
				}

				if(!is_expected_len(head_object, offset, expected_len)){
					break;
				}
			}

			if(REAL_TYPEID_GET(head_object->type_id) != DATA_TYPE_RAW){
				*(buf+offset) = head_object->type_id;
				++offset;
				*(buf+offset) = head_object->attr_id;
				++offset;
			}

			value_len=data_unit_len_get(REAL_TYPEID_GET(head_object->type_id));
			if(!value_len){
				if(REAL_TYPEID_GET(head_object->type_id) != DATA_TYPE_RAW){
					value_len = cpu_to_be16(head_object->len);
					memcpy(buf+offset, &value_len, sizeof(uint16_t));
					offset += sizeof(uint16_t);
				}

                memcpy(buf+offset, head_object->data, head_object->len);
                offset+=head_object->len;

			}else{
				message_value_serialize(head_object, buf+offset, value_len, BYTE_ORDER_LITTLE_ENDIAN);
				offset+=value_len;
			}

			head_object=head_object->next;
		}

		*len=offset;
    }
}

void message_raw_data_get(message_context_t *context, uint8_t **data, uint16_t *len)
{
    message_info_t *object=NULL;
    if(context==NULL)
       return;
    object = message_object_get(context, RAW_DATA_ATTR_ID);
    if(object){

        *data=object->data;
        *len=object->len;
    }
}

void message_raw_data_add(message_context_t *context, uint8_t *data, uint16_t len)
{
    message_data_add(context, DATA_TYPE_RAW, RAW_DATA_ATTR_ID, data, len);
}


int32_t message_object_copy(message_info_t *dest_object, message_info_t *src_object)
{
	uint16_t value_len = 0;
	int32_t err = SERIAL_NO_ERR;

	value_len = data_unit_len_get(REAL_TYPEID_GET(src_object->type_id));
	if(value_len != 0){
		memcpy(dest_object, src_object, sizeof(message_info_t));
		dest_object->next = NULL;
	}else{

		if(src_object->len <= DEFAULT_DATA_UNIT_LEN){
			memcpy(dest_object, src_object, sizeof(message_info_t));
			dest_object->data = dest_object->value.value_byte;
			dest_object->next = NULL;
		}else{
			dest_object->data = message_malloc(src_object->len);
			if(dest_object->data == NULL){
				err = SERIAL_ALLOC_MEM_ERR;
				goto exit;
			}

			dest_object->type_id = src_object->type_id;
			dest_object->attr_id = src_object -> attr_id;
			dest_object->is_changed = 0;

			memcpy(dest_object->data, src_object->data, src_object->len);
			dest_object->len = src_object->len;
		}
	}

exit:
	return err;
}


int32_t message_object_add_update(message_context_t *context, uint8_t *data, uint16_t len, uint8_t *is_update)
{
    int32_t err = SERIAL_NO_ERR;
    message_info_t *head_object = NULL;
    message_info_t *object = NULL;
    message_info_t *add_object = NULL;
    message_context_t *update_context = NULL;
    uint16_t value_len = 0;

    if(context == NULL || data == NULL || len == 0 || is_update == NULL){
        return SERIAL_PARAM_ERR;
    }
    update_context = message_parse(data, len, PARSE_TYPE_BASIC);
    if(update_context == NULL){
        return SERIAL_PARAM_ERR;
    }

    head_object = update_context->object;
    while(head_object){
        object = message_object_get(context, head_object->attr_id);
        if(!object){
        	add_object = message_info_new();
        	if(!add_object){
        		err = SERIAL_ALLOC_MEM_ERR;
        		goto exit;
        	}
        	err = message_object_copy(add_object, head_object);
        	if(err != SERIAL_NO_ERR){
        		goto exit;
        	}
        	DATA_CHANGED_WAIT_REPORT(add_object->is_changed);
        	message_object_add(context, add_object);
        	*is_update = 1;
        	head_object = head_object->next;
            continue;
        }

        if(REAL_TYPEID_GET(object->type_id) != REAL_TYPEID_GET(head_object->type_id)){
            err = SERIAL_TYPE_ID_ERR;
            goto exit;
        }

        value_len = data_unit_len_get(REAL_TYPEID_GET(object->type_id));
        if(value_len != 0){
            if(IS_NEED_REPORT(head_object->type_id) || memcmp(&object->value, &head_object->value, sizeof(object->value)) != 0){
                NEED_REPORT_CLEAR(head_object->type_id);
                memcpy(&object->value, &head_object->value, sizeof(object->value));
                DATA_CHANGED_WAIT_REPORT(object->is_changed);
                *is_update = 1;
            }
        }else{

            if(IS_NEED_REPORT(head_object->type_id) || object->len != head_object->len || memcmp(object->data, head_object->data, head_object->len) != 0){
                NEED_REPORT_CLEAR(head_object->type_id);
                message_value_set(object, head_object->data, head_object->len, BYTE_ORDER_NONE);
                DATA_CHANGED_WAIT_REPORT(object->is_changed);
                *is_update = 1;
            }
        }
        head_object = head_object->next;
    }

exit:
    message_delete(update_context);

    return err;
}


int32_t message_object_update(message_context_t *context, uint8_t *data, uint16_t len, uint8_t *is_update)
{
    int32_t err = SERIAL_NO_ERR;
    message_info_t *head_object = NULL;
    message_info_t *object = NULL;
    message_context_t *update_context = NULL;
    uint16_t value_len = 0;

    if(context == NULL || data == NULL || len == 0 || is_update == NULL){
        return SERIAL_PARAM_ERR;
    }
    update_context=message_parse(data, len, PARSE_TYPE_BASIC);
    if(update_context == NULL){
        return SERIAL_PARAM_ERR;
    }

    head_object = update_context->object;
    while(head_object){
        object = message_object_get(context, head_object->attr_id);
        if(!object){
            err = SERIAL_TYPE_ID_ERR;
            goto exit;
        }
        if(REAL_TYPEID_GET(object->type_id) != REAL_TYPEID_GET(head_object->type_id)){
            err = SERIAL_TYPE_ID_ERR;
            goto exit;
        }

        value_len = data_unit_len_get(REAL_TYPEID_GET(object->type_id));
        if(value_len != 0){
            if(IS_NEED_REPORT(head_object->type_id) || memcmp(&object->value, &head_object->value, sizeof(object->value)) != 0){
                NEED_REPORT_CLEAR(head_object->type_id);
                memcpy(&object->value, &head_object->value, sizeof(object->value));
                DATA_CHANGED_WAIT_REPORT(object->is_changed);
                *is_update = 1;
            }
        }else{

            if(IS_NEED_REPORT(head_object->type_id) || object->len != head_object->len || memcmp(object->data, head_object->data, head_object->len) != 0){
                NEED_REPORT_CLEAR(head_object->type_id);
                message_value_set(object, head_object->data, head_object->len, BYTE_ORDER_NONE);
                DATA_CHANGED_WAIT_REPORT(object->is_changed);
                *is_update = 1;
            }
        }
        head_object = head_object->next;
    }

exit:
    message_delete(update_context);

    return err;
}


int32_t message_sync_report_confirm(message_context_t *context, message_report_result_t result)
{
	message_info_t *head_object = NULL;

    if(context == NULL){
        return SERIAL_PARAM_ERR;
    }

    head_object=context->object;

    while(head_object){

		if(IS_DATA_CHANGED_WAIT_REPORT_ACK(head_object->is_changed)){
			if(result == REPORT_RESULT_SUCCESS){
				DATA_CHANGED_REPORT_SUCCESS(head_object->is_changed);
			}else{
				DATA_CHANGED_REPORT_FAIL(head_object->is_changed);
			}
		}
		head_object = head_object->next;
    }

    return SERIAL_NO_ERR;
}

int32_t message_changed_flag_clear(message_context_t *context)
{
	message_info_t *head_object = NULL;

    if(context == NULL){
        return SERIAL_PARAM_ERR;
    }

    head_object=context->object;

    while(head_object){

    	DATA_CHANGED_WAIT_REPORT_CLEAR(head_object->is_changed);
		head_object = head_object->next;
    }

    return SERIAL_NO_ERR;
}

int32_t message_serialize_by_attrid(message_context_t *context, uint8_t* attr_id, uint16_t attr_id_count, uint8_t *buf, uint16_t *len)
{
	uint16_t message_len = 0;
	uint16_t offset = 0;
	uint16_t i = 0;
	uint16_t value_len = 0;
	message_info_t *object = NULL;

    if(context == NULL || attr_id == NULL || attr_id_count == 0){
        return SERIAL_PARAM_ERR;
    }

    for(i = 0;i < attr_id_count; i++){
    	object = message_object_get(context, *(attr_id+i));
    	if(object == NULL){
    		continue;
    	}
    	value_len = data_unit_len_get(REAL_TYPEID_GET(object->type_id));
		if(value_len){
			message_len += 2 + value_len;
		}else{
			if(REAL_TYPEID_GET(object->type_id) != DATA_TYPE_RAW){
				message_len += 2 + sizeof(uint16_t) + object->len;
			}else{
				message_len += object->len;
			}
		}
    }

    if(buf == NULL){
    	*len = message_len;
    	return SERIAL_NO_ERR;
    }

    if(*len < message_len){
    	return SERIAL_PARAM_ERR;
    }

    if(message_len == 0){
    	*len = 0;
    	return SERIAL_PARAM_ERR;
    }

    for(i = 0;i < attr_id_count; i++){

    	object = message_object_get(context, *(attr_id+i));
    	if(object == NULL){
    		continue;
    	}

		if(REAL_TYPEID_GET(object->type_id) != DATA_TYPE_RAW){
			*(buf+offset) = object->type_id;
			++offset;
			*(buf+offset) = object->attr_id;
			++offset;
		}

		value_len = data_unit_len_get(REAL_TYPEID_GET(object->type_id));
		if(value_len){
			message_value_serialize(object, buf+offset, value_len, BYTE_ORDER_LITTLE_ENDIAN);
			offset += value_len;
		}else{

			if(REAL_TYPEID_GET(object->type_id) != DATA_TYPE_RAW){
				value_len = cpu_to_be16(object->len);
				memcpy(buf+offset, &value_len, sizeof(uint16_t));
				offset += sizeof(uint16_t);
			}

            memcpy(buf+offset, object->data, object->len);
            offset += object->len;

		}
    }

	*len = offset;

	return SERIAL_NO_ERR;
}


