#include "qm_tlv.h"
#include "qm_kernel.h"
#include "qm_errno.h"

qm_tlv_ctx_t *qm_tlv_ctx_creat(void)
{
    qm_tlv_ctx_t *tlv_ctx = NULL;
    tlv_ctx = (qm_tlv_ctx_t*)tlv_malloc(sizeof(qm_tlv_ctx_t));
    if(tlv_ctx == NULL){
        return NULL;
    }
    memset(tlv_ctx, 0, sizeof(qm_tlv_ctx_t));
    return tlv_ctx;
}

int qm_tlv_ctx_delete(qm_tlv_ctx_t *tlv_ctx)
{
    qm_tlv_block_t *tlv_block = NULL;
    qm_tlv_block_t *tmp_block = NULL;

    if(tlv_ctx == NULL){
        return -QM_EINVAL;
    }
    tlv_block = tlv_ctx->tlv_block;

    while(tlv_block){

        if(tlv_block->len > QM_TLV_BYTES_SIZE){
            tlv_free(tlv_block->value.pdata);
            tlv_block->value.pdata = NULL;
        }

        tmp_block = tlv_block;
        tlv_block = tlv_block->next;
        tlv_free(tmp_block);
        tmp_block = NULL;
    }

    tlv_free(tlv_ctx);

    return QM_EOK;
}

int qm_tlv_block_delete(qm_tlv_block_t *tlv_block)
{
    if(tlv_block == NULL){
        return -QM_EINVAL;
    }
    if(tlv_block->len > QM_TLV_BYTES_SIZE){
        tlv_free(tlv_block->value.pdata);
    }

    tlv_free(tlv_block);
    return QM_EOK;
}

qm_tlv_block_t *qm_tlv_block_creat(void)
{
    qm_tlv_block_t *tlv_block = NULL;
    tlv_block = (qm_tlv_block_t*)tlv_malloc(sizeof(qm_tlv_block_t));
    if(tlv_block == NULL){
        return NULL;
    }
    memset(tlv_block, 0, sizeof(qm_tlv_block_t));
    return tlv_block;
}

static int qm_tlv_block_check(qm_tlv_ctx_t *tlv_ctx, qm_tlv_block_t *tlv_block)
{
    qm_tlv_block_t *m_tlv_block = NULL;
    while(m_tlv_block){
        if(tlv_block->tag == m_tlv_block->tag){
            return 1;
        }
        m_tlv_block = m_tlv_block->next;
    }
    return 0;
}

int qm_tlv_block_add(qm_tlv_ctx_t *tlv_ctx, qm_tlv_block_t *tlv_block)
{
    qm_tlv_block_t *m_tlv_block = NULL;
    qm_tlv_block_t *tlv_block_pre = NULL;
    qm_tlv_block_t *tlv_block_next = NULL;

    if(tlv_ctx == NULL || tlv_block == NULL){
        return -QM_EINVAL;
    }

    if(tlv_ctx->tlv_block == NULL){
        tlv_ctx->tlv_block = tlv_block; 
        tlv_block->next = NULL;
        tlv_ctx->count++;
        return QM_EOK;
    }

    //first check tag
    m_tlv_block = tlv_ctx->tlv_block;
    while(m_tlv_block){
        if(tlv_block->tag == m_tlv_block->tag){   
            if(tlv_ctx->tlv_block == m_tlv_block){
                tlv_block_next = m_tlv_block->next;
                tlv_ctx->tlv_block = tlv_block;
                tlv_block->next = tlv_block_next;
            }else{
                tlv_block_pre->next = tlv_block;
                tlv_block->next = m_tlv_block->next;
            }
            qm_tlv_block_delete(m_tlv_block);   
            return QM_EOK;
        }
        tlv_block_pre = m_tlv_block;
        m_tlv_block = m_tlv_block->next;  
    }

    tlv_block_pre->next = tlv_block;
    tlv_block->next = NULL;
    tlv_ctx->count++;
    return QM_EOK;
}

qm_tlv_block_t *qm_tlv_block_find(qm_tlv_ctx_t *tlv_ctx, uint16_t tag)
{
    qm_tlv_block_t *m_tlv_block = NULL;
    if(tlv_ctx == NULL){
        return NULL;
    }
    m_tlv_block = tlv_ctx->tlv_block;
    while(m_tlv_block){
        #if CONFIG_QM_TLV_LENGTH_SIZE == 1
        if(m_tlv_block->tag == (uint8_t)tag){
            break;
        }
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
        if(m_tlv_block->tag == (uint16_t)tag){
            break;
        }
    #endif
        m_tlv_block = m_tlv_block->next;
    }
    
    return m_tlv_block;
}

int qm_tlv_block_serialize(qm_tlv_block_t *tlv_block)
{
    if(tlv_block == NULL){
        return -QM_EINVAL;
    }
    tlv_block->serialization = 1;
    return QM_EOK;
}

int qm_tlv_serialize(qm_tlv_ctx_t *tlv_ctx, uint8_t *msg, int *len, int partial)
{
    int index = 0;
    #if (CONFIG_QM_TLV_TAG_SIZE == 2) || (CONFIG_QM_TLV_LENGTH_SIZE == 2)
    uint16_t tmp = 0;
    #endif  
    qm_tlv_block_t *tlv_block = NULL;
    if(tlv_ctx == NULL || len == NULL){
        return -QM_EINVAL;
    }

    tlv_block = tlv_ctx->tlv_block;

    while(tlv_block){

        if(partial){
            if(!tlv_block->serialization){
                tlv_block = tlv_block->next;
                continue;
            }else{
                if(msg){
                    tlv_block->serialization = 0;
                }
            }
        }

    #if CONFIG_QM_TLV_TAG_SIZE == 1
        if(msg){
            *(msg+index) = tlv_block->tag;
        }
        index ++;
    #elif CONFIG_QM_TLV_TAG_SIZE == 2
        if(msg){
            tmp = cpu_to_be16(tlv_block->tag);
            memcpy(msg+index, &tmp, 2);
        }
        index += 2;
    #endif

    #if CONFIG_QM_TLV_LENGTH_SIZE == 1
        if(msg){
            *(msg+index) = tlv_block->len;
        }
        index ++;
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
        if(msg){
            tmp = cpu_to_be16(tlv_block->len);
            memcpy(msg+index, &tmp, 2);
        }
        index += 2;
    #endif
        if(msg){
            if(tlv_block->len <= QM_TLV_BYTES_SIZE){
                memcpy(msg + index, tlv_block->value.bytes, tlv_block->len);
            }else{
                memcpy(msg + index, tlv_block->value.pdata, tlv_block->len);
            }
        }

        index += tlv_block->len;
    
        tlv_block = tlv_block->next;
    }

    *len = index;

    return QM_EOK;
}

int qm_tlv_deserialize(qm_tlv_ctx_t *tlv_ctx, uint8_t *msg, int len)
{
    int index = 0;
#if (CONFIG_QM_TLV_TAG_SIZE == 2) || (CONFIG_QM_TLV_LENGTH_SIZE == 2)
    uint16_t tmp = 0;
#endif
    qm_err_t ret = QM_EOK;
    qm_tlv_block_t *tlv_block = NULL;

    if(tlv_ctx == NULL || msg == NULL || len == 0){
        return -QM_EINVAL;
    }

    while(index < len){

        tlv_block = qm_tlv_block_creat();
        if(tlv_block == NULL){
            ret = -QM_ENOMEM;
            goto __exit;
        }

    #if CONFIG_QM_TLV_TAG_SIZE == 1
        if(len - index < 1){
            goto __exit;
        }
        tlv_block->tag = *(msg + index);
        index++;
    #elif CONFIG_QM_TLV_TAG_SIZE == 2
        if(len - index < 2){
            goto __exit;
        }
        memcpy(&tmp, msg + index, sizeof(uint16_t));
        tlv_block->tag = be16_to_cpu(tmp);
        index += 2;
    #endif

    #if CONFIG_QM_TLV_LENGTH_SIZE == 1
        if(len - index < 1){
            goto __exit;
        }
        tlv_block->len = *(msg + index);
        index++;
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
        if(len - index < 2){
            goto __exit;
        }
        memcpy(&tmp, msg + index, sizeof(uint16_t));
        tlv_block->len = be16_to_cpu(tmp);
        index += 2;
    #endif

        if(len - index < tlv_block->len){
            goto __exit;
        }

        if(tlv_block->len <= QM_TLV_BYTES_SIZE){
            memcpy(tlv_block->value.bytes, msg + index, tlv_block->len);
        }else{
            tlv_block->value.pdata = (uint8_t*)tlv_malloc(tlv_block->len);
            if(tlv_block->value.pdata == NULL){
                ret = -QM_ENOMEM;
                goto __exit;
            }
            memcpy(tlv_block->value.pdata, msg + index, tlv_block->len);
        }

        index += tlv_block->len;

        if(qm_tlv_block_check(tlv_ctx, tlv_block)){
            ret = -QM_EINVAL;
            goto __exit;
        }

        qm_tlv_block_add(tlv_ctx, tlv_block);
    }

    return QM_EOK;

__exit:
    if(tlv_block){
        tlv_free(tlv_block);
        tlv_block = NULL;
    }

    qm_tlv_ctx_delete(tlv_ctx);
    return ret;
}

int qm_tlv_merge(qm_tlv_ctx_t *dest_tlv_ctx, qm_tlv_ctx_t *src_tlv_ctx)
{
    qm_err_t ret = QM_EOK;
    qm_tlv_block_t *tlv_block = NULL;
    qm_tlv_block_t *m_tlv_block = NULL;
    if(dest_tlv_ctx == NULL || src_tlv_ctx == NULL){
        return -QM_EINVAL;
    }

    tlv_block = src_tlv_ctx->tlv_block;
    while(tlv_block){
        m_tlv_block = tlv_block->next;
        ret = qm_tlv_block_add(dest_tlv_ctx, tlv_block);
        if(ret != QM_EOK){
            return ret;
        }
        tlv_block = m_tlv_block;
    }

    src_tlv_ctx->tlv_block = NULL;
    src_tlv_ctx->count = 0;

    return QM_EOK;
}

qm_tlv_block_t *qm_tlv_block_next(qm_tlv_ctx_t *tlv_ctx, qm_tlv_block_t *tlv_block)
{
    if(tlv_ctx == NULL){
        return -QM_EINVAL;
    }

    if(tlv_block == NULL){
        return tlv_ctx->tlv_block;
    }else{
        return tlv_block->next;
    }
}

int qm_tlv_unpack_bool(qm_tlv_block_t *tlv_block, bool_t *value)
{
    return qm_tlv_unpack_uint8(tlv_block, (uint8_t*)value);
}

int qm_tlv_unpack_uint8(qm_tlv_block_t *tlv_block, uint8_t *value)
{
    if(tlv_block == NULL || value == NULL){
        return -QM_EINVAL;
    }
    *value = (uint8_t)tlv_block->value.bytes[0];
    return QM_EOK;
}

int qm_tlv_unpack_int8(qm_tlv_block_t *tlv_block, int8_t *value)
{
    return qm_tlv_unpack_uint8(tlv_block, (uint8_t*)value);
}

int qm_tlv_unpack_uint16(qm_tlv_block_t *tlv_block, uint16_t *value)
{
    uint16_t tmp = 0;
    if(tlv_block == NULL || value == NULL){
        return -QM_EINVAL;
    }
    memcpy(&tmp, tlv_block->value.bytes, sizeof(uint16_t));
    *value = be16_to_cpu(tmp);
    return QM_EOK;
}

int qm_tlv_unpack_int16(qm_tlv_block_t *tlv_block, int16_t *value)
{
    return qm_tlv_unpack_uint16(tlv_block, (uint16_t*)value);
}

int qm_tlv_unpack_uint32(qm_tlv_block_t *tlv_block, uint32_t *value)
{
    uint32_t tmp = 0;
    if(tlv_block == NULL || value == NULL){
        return -QM_EINVAL;
    }
    memcpy(&tmp, tlv_block->value.bytes, sizeof(uint32_t));
    *value = be32_to_cpu(tmp);
    return QM_EOK;
}

int qm_tlv_unpack_int32(qm_tlv_block_t *tlv_block, int32_t *value)
{
    return qm_tlv_unpack_uint32(tlv_block, (uint32_t*)value);
}

int qm_tlv_unpack_bytes(qm_tlv_block_t *tlv_block, uint8_t *bytes, int *size)
{
    if(tlv_block == NULL || size == NULL){
        return -QM_EINVAL;
    }
  
    *size = (int)tlv_block->len;

    if(bytes == NULL){
        return QM_EOK;
    }

    if(tlv_block->len <= QM_TLV_BYTES_SIZE){
        memcpy(bytes, tlv_block->value.bytes, tlv_block->len);
    }else{
        memcpy(bytes, tlv_block->value.pdata, tlv_block->len);
    }

    return QM_EOK;
}

int qm_tlv_unpack_string(qm_tlv_block_t *tlv_block, char *str, int *size)
{
    if(tlv_block == NULL || size == NULL){
        return -QM_EINVAL;
    }
  
    *size = (int)tlv_block->len;

    if(str == NULL){
        return QM_EOK;
    }

    if(tlv_block->len <= QM_TLV_BYTES_SIZE){
        memcpy(str, tlv_block->value.bytes, tlv_block->len);
    }else{
        memcpy(str, tlv_block->value.pdata, tlv_block->len);
    }

    *(str+tlv_block->len) = '\0';
    return QM_EOK;
}

int qm_tlv_unpack_bytes_direct(qm_tlv_block_t *tlv_block, uint8_t **bytes, int *size)
{
    if(tlv_block == NULL || bytes == NULL || size == NULL){
        return -QM_EINVAL;
    }

    *size = (int)tlv_block->len;

    if(tlv_block->len <= QM_TLV_BYTES_SIZE){
        *bytes = tlv_block->value.bytes;
    }else{
        *bytes = tlv_block->value.pdata;
    }

    return QM_EOK;
}

int qm_tlv_unpack_string_direct(qm_tlv_block_t *tlv_block, char **str, int *size)
{
    return qm_tlv_unpack_bytes_direct(tlv_block, (uint8_t**)str, size);
}

int qm_tlv_pack_bool(qm_tlv_block_t *tlv_block, bool_t value)
{
    return qm_tlv_pack_uint8(tlv_block, (uint8_t)value);
}

int qm_tlv_pack_uint8(qm_tlv_block_t *tlv_block, uint8_t value)
{
    if(tlv_block == NULL){
        return -QM_EINVAL;
    }
    tlv_block->value.bytes[0] = (uint8_t)value;
    #if CONFIG_QM_TLV_LENGTH_SIZE == 1
    tlv_block->len = (uint8_t)sizeof(uint8_t);
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
    tlv_block->len = (uint16_t)sizeof(uint8_t);
    #endif
    if(tlv_block->value.pdata){
        qm_free(tlv_block->value.pdata);
    }
    return QM_EOK;
}

int qm_tlv_pack_int8(qm_tlv_block_t *tlv_block, int8_t value)
{
    return qm_tlv_pack_uint8(tlv_block, (uint8_t)value);
}

int qm_tlv_pack_uint16(qm_tlv_block_t *tlv_block, uint16_t value)
{
    uint16_t tmp = 0;
    if(tlv_block == NULL){
        return -QM_EINVAL;
    }
    tmp = cpu_to_be16(value);
    memcpy(tlv_block->value.bytes, &tmp, sizeof(uint16_t));
    #if CONFIG_QM_TLV_LENGTH_SIZE == 1
    tlv_block->len = (uint8_t)sizeof(uint16_t);
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
    tlv_block->len = (uint16_t)sizeof(uint16_t);
    #endif
    if(tlv_block->value.pdata){
        qm_free(tlv_block->value.pdata);
    }
    return QM_EOK;
}

int qm_tlv_pack_int16(qm_tlv_block_t *tlv_block, int16_t value)
{
    return qm_tlv_pack_uint16(tlv_block, (uint16_t)value);
}

int qm_tlv_pack_uint32(qm_tlv_block_t *tlv_block, uint32_t value)
{
    uint32_t tmp = 0;
    if(tlv_block == NULL){
        return -QM_EINVAL;
    }
    tmp = cpu_to_be32(value);
    memcpy(tlv_block->value.bytes, &tmp, sizeof(uint32_t));
    #if CONFIG_QM_TLV_LENGTH_SIZE == 1
    tlv_block->len = (uint8_t)sizeof(uint32_t);
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
    tlv_block->len = (uint16_t)sizeof(uint32_t);
    #endif
    if(tlv_block->value.pdata){
        qm_free(tlv_block->value.pdata);
    }
    return QM_EOK;
}

int qm_tlv_pack_int32(qm_tlv_block_t *tlv_block, int32_t value)
{
    return qm_tlv_pack_uint32(tlv_block, (uint32_t)value);
}

int qm_tlv_pack_bytes(qm_tlv_block_t *tlv_block, uint8_t *bytes, int size)
{
    if(tlv_block == NULL){
        return -QM_EINVAL;
    }

#if CONFIG_QM_TLV_LENGTH_SIZE == 1
    if(size > 0xFF){
        return -QM_EINVAL;
    }
#elif CONFIG_QM_TLV_LENGTH_SIZE == 2
    if(size > 0xFFFF){
        return -QM_EINVAL;
    }
#endif

    if(tlv_block->len > QM_TLV_BYTES_SIZE){
        qm_free(tlv_block->value.pdata);
        tlv_block->value.pdata = NULL;
    }

    if(size <= QM_TLV_BYTES_SIZE){
        memcpy(tlv_block->value.bytes, bytes, size);
    }else{
        tlv_block->value.pdata = (uint8_t*)tlv_malloc(size);
        if(tlv_block->value.pdata == NULL){
            return -QM_ENOMEM;   
        }
        memcpy(tlv_block->value.pdata, bytes, size);
    }

    #if CONFIG_QM_TLV_LENGTH_SIZE == 1
    tlv_block->len = (uint8_t)size;
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
    tlv_block->len = (uint16_t)size;
    #endif

    return QM_EOK;
}

int qm_tlv_pack_string(qm_tlv_block_t *tlv_block, char *str, int size)
{
    return qm_tlv_pack_bytes(tlv_block, (uint8_t*)str, size);
}
