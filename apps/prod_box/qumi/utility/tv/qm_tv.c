#include "qm_tv.h"
#include "qm_kernel.h"
#include "qm_errno.h"

qm_tv_ctx_t *qm_tv_ctx_creat(void)
{
    qm_tv_ctx_t *tv_ctx = NULL;
    tv_ctx = (qm_tv_ctx_t*)tv_malloc(sizeof(qm_tv_ctx_t));
    if(tv_ctx == NULL){
        return NULL;
    }
    memset(tv_ctx, 0, sizeof(qm_tv_ctx_t));
    return tv_ctx;
}

qm_tv_block_t *qm_tv_block_creat(void)
{
    qm_tv_block_t *tv_block = NULL;
    tv_block = (qm_tv_block_t*)tv_malloc(sizeof(qm_tv_block_t));
    if(tv_block == NULL){
        return NULL;
    }
    memset(tv_block, 0, sizeof(qm_tv_block_t));
    return tv_block;
}

int qm_tv_ctx_delete(qm_tv_ctx_t *tv_ctx)
{
    qm_tv_block_t *tv_block = NULL;
    qm_tv_block_t *tmp_block = NULL;

    if(tv_ctx == NULL){
        return -QM_EINVAL;
    }
    tv_block = tv_ctx->tv_block;

    while(tv_block){

        if(tv_block->len > CONFIG_QM_TV_BYTES_SIZE){
            tv_free(tv_block->value.pdata);
            tv_block->value.pdata = NULL;
        }

        tmp_block = tv_block;
        tv_block = tv_block->next;
        tv_free(tmp_block);
        tmp_block = NULL;
    }

    tv_free(tv_ctx);

    return QM_EOK;
}

int qm_tv_block_delete(qm_tv_block_t *tv_block)
{
    if(tv_block == NULL){
        return -QM_EINVAL;
    }
    if(tv_block->len > CONFIG_QM_TV_BYTES_SIZE){
        tv_free(tv_block->value.pdata);
    }

    tv_free(tv_block);
    return QM_EOK;
}

qm_tv_block_t *qm_tv_block_find(qm_tv_ctx_t *tv_ctx, uint16_t tag)
{
    qm_tv_block_t *m_tv_block = NULL;
    if(tv_ctx == NULL){
        return NULL;
    }
    m_tv_block = tv_ctx->tv_block;
    while(m_tv_block){
        #if CONFIG_QM_TV_LENGTH_SIZE == 1
        if(m_tv_block->tag == (uint8_t)tag){
            break;
        }
    #elif CONFIG_QM_TV_LENGTH_SIZE == 2
        if(m_tv_block->tag == (uint16_t)tag){
            break;
        }
    #endif
        m_tv_block = m_tv_block->next;
    }
    
    return m_tv_block;
}

int qm_tv_block_serialize(qm_tv_block_t *tv_block)
{
    if(tv_block == NULL){
        return -QM_EINVAL;
    }
    tv_block->serialization = 1;
    return QM_EOK;
}

int qm_tv_block_is_update(qm_tv_block_t *tv_block, int *is_update)
{
    if(tv_block == NULL){
        return -QM_EINVAL;
    }

    *is_update = (int)tv_block->update;

    return QM_EOK;
}

int qm_tv_block_update(qm_tv_block_t *tv_block, int update)
{
    if(tv_block == NULL){
        return -QM_EINVAL;
    }

    tv_block->update = update;

    return QM_EOK;
}

static int qm_tv_block_check(qm_tv_ctx_t *tv_ctx, qm_tv_block_t *tv_block)
{
    qm_tv_block_t *m_tv_block = NULL;
    while(m_tv_block){
        if(tv_block->tag == m_tv_block->tag){
            return 1;
        }
        m_tv_block = m_tv_block->next;
    }
    return 0;
}

int qm_tv_block_add(qm_tv_ctx_t *tv_ctx, qm_tv_block_t *tv_block)
{
    qm_tv_block_t *m_tv_block = NULL;
    qm_tv_block_t *tv_block_pre = NULL;

    if(tv_ctx == NULL || tv_block == NULL){
        return -QM_EINVAL;
    }

    if(tv_ctx->tv_block == NULL){
        tv_ctx->tv_block = tv_block; 
        tv_block->next = NULL;
        tv_ctx->count++;
        tv_block->update = 1;
        return QM_EOK;
    }

    if(qm_tv_block_check(tv_ctx, tv_block)){
        return -QM_EINVAL;
    }

    m_tv_block = tv_ctx->tv_block;
    while(m_tv_block){
        tv_block_pre = m_tv_block;
        m_tv_block = m_tv_block->next;  
    }

    tv_block_pre->next = tv_block;
    tv_block->next = NULL;
    tv_ctx->count++;
    tv_block->update = 1;

    return QM_EOK;
}

int qm_tv_block_del(qm_tv_ctx_t *tv_ctx, uint16_t tag)
{



}

int qm_tv_block_copy(qm_tv_block_t *dest_tv_block, qm_tv_block_t *src_tv_block)
{
    int ret = 0;

    if(dest_tv_block == NULL || src_tv_block == NULL){
        return -QM_EINVAL;
    }

    if(dest_tv_block->tag != src_tv_block->tag){
       dest_tv_block->tag = src_tv_block->tag;
       dest_tv_block->update = 1;
    }

    if(dest_tv_block->type != src_tv_block->type){
       dest_tv_block->type = src_tv_block->type;
       dest_tv_block->update = 1;
    }

    if(dest_tv_block->len == src_tv_block->len){

        if(dest_tv_block->len <= CONFIG_QM_TV_BYTES_SIZE){
            if(0 != memcmp(dest_tv_block->value.bytes, src_tv_block->value.bytes, dest_tv_block->len)){
                ret = qm_tv_pack_bytes(dest_tv_block, src_tv_block->value.bytes, dest_tv_block->len);
                dest_tv_block->update = 1;
            }
        }else{
            if(0 != memcmp(dest_tv_block->value.pdata, src_tv_block->value.pdata, dest_tv_block->len)){
                ret = qm_tv_pack_bytes(dest_tv_block, src_tv_block->value.bytes, dest_tv_block->len);
                dest_tv_block->update = 1;
            }
        }

    }else{
        ret = qm_tv_pack_bytes(dest_tv_block, src_tv_block->value.bytes, dest_tv_block->len);
        dest_tv_block->update = 1;
    }
    return ret;
}

int qm_tv_merge(qm_tv_ctx_t *dest_tv_ctx, qm_tv_ctx_t *src_tv_ctx)
{
    qm_err_t ret = QM_EOK;
    qm_tv_block_t *tv_block = NULL;
    qm_tv_block_t *m_tv_block = NULL;
    if(dest_tv_ctx == NULL || src_tv_ctx == NULL){
        return -QM_EINVAL;
    }

    tv_block = src_tv_ctx->tv_block;
    while(tv_block){

        m_tv_block = qm_tv_block_find(dest_tv_ctx, (uint16_t)tv_block->tag);
        if(m_tv_block){
            ret = qm_tv_block_copy(m_tv_block, tv_block);
            if(ret != QM_EOK){
                return ret;
            }
        }else{
            m_tv_block = qm_tv_block_creat();
            if(m_tv_block == NULL){
                return -QM_ENOMEM;
            }

            ret = qm_tv_block_copy(m_tv_block, tv_block);
            if(ret != QM_EOK){
                return ret;
            }

            ret = qm_tv_block_add(dest_tv_ctx, m_tv_block);
            if(ret != QM_EOK){
                return ret;
            }
        }
        tv_block = tv_block->next;
    }

    return QM_EOK;
}

qm_tv_block_t *qm_tv_block_next(qm_tv_ctx_t *tv_ctx, qm_tv_block_t *tv_block)
{
    if(tv_ctx == NULL){
        return NULL;
    }

    if(tv_block == NULL){
        return tv_ctx->tv_block;
    }else{
        return tv_block->next;
    }
}

int qm_tv_deserialize(qm_tv_ctx_t *tv_ctx, uint8_t *msg, int len)
{
    int index = 0;
#if (CONFIG_QM_TV_TAG_SIZE == 2) || (CONFIG_QM_TV_LENGTH_SIZE == 2)
    uint16_t tmp = 0;
#endif
    qm_err_t ret = QM_EOK;
    qm_tv_block_t *tv_block = NULL;

    if(tv_ctx == NULL || msg == NULL || len == 0){
        return -QM_EINVAL;
    }

    while(index < len){

        tv_block = qm_tv_block_creat();
        if(tv_block == NULL){
            ret = -QM_ENOMEM;
            goto __exit;
        }

    #if CONFIG_QM_TV_TAG_SIZE == 1
        if(len - index < 1){
            goto __exit;
        }
        tv_block->tag = *(msg + index);
        index++;
    #elif CONFIG_QM_TV_TAG_SIZE == 2
        if(len - index < 2){
            goto __exit;
        }
        memcpy(&tmp, msg + index, sizeof(uint16_t));
        tv_block->tag = be16_to_cpu(tmp);
        index += 2;
    #endif

    #if CONFIG_QM_TV_LENGTH_SIZE == 1
        if(len - index < 1){
            goto __exit;
        }
        tv_block->len = *(msg + index);
        index++;
    #elif CONFIG_QM_TV_LENGTH_SIZE == 2
        if(len - index < 2){
            goto __exit;
        }
        memcpy(&tmp, msg + index, sizeof(uint16_t));
        tv_block->len = be16_to_cpu(tmp);
        index += 2;
    #endif

        if(len - index < tv_block->len){
            goto __exit;
        }

        if(tv_block->len <= CONFIG_QM_TV_BYTES_SIZE){
            memcpy(tv_block->value.bytes, msg + index, tv_block->len);
        }else{
            tv_block->value.pdata = (uint8_t*)tv_malloc(tv_block->len);
            if(tv_block->value.pdata == NULL){
                ret = -QM_ENOMEM;
                goto __exit;
            }
            memcpy(tv_block->value.pdata, msg + index, tv_block->len);
        }

        index += tv_block->len;

        ret = qm_tv_block_add(tv_ctx, tv_block);
        if(ret != QM_EOK){
            goto __exit;
        }
    }

    return QM_EOK;

__exit:
    if(tv_block){
        tv_free(tv_block);
        tv_block = NULL;
    }

    qm_tv_ctx_delete(tv_ctx);
    return ret;
}

int qm_tv_serialize(qm_tv_ctx_t *tv_ctx, uint8_t *msg, int *len, qm_tv_serialize_type_t type);
{




}

int qm_tv_pack_bool(qm_tv_block_t *tv_block, bool_t value);
int qm_tv_pack_uint8(qm_tv_block_t *tv_block, uint8_t value);
int qm_tv_pack_int8(qm_tv_block_t *tv_block, int8_t value);
#if CONFIG_QM_TV_INT16_SUPPORT
int qm_tv_pack_uint16(qm_tv_block_t *tv_block, uint16_t value);
int qm_tv_pack_int16(qm_tv_block_t *tv_block, int16_t value);
#endif
#if CONFIG_QM_TV_INT32_SUPPORT
int qm_tv_pack_uint32(qm_tv_block_t *tv_block, uint32_t value);
int qm_tv_pack_int32(qm_tv_block_t *tv_block, int32_t value);
#endif
#if CONFIG_QM_TV_BYTES_SUPPORT
int qm_tv_pack_bytes(qm_tv_block_t *tv_block, uint8_t *bytes, int size);
int qm_tv_pack_string(qm_tv_block_t *tv_block, char *str, int size);
#endif
int qm_tv_unpack_bool(qm_tv_block_t *tv_block, bool_t *value);
int qm_tv_unpack_uint8(qm_tv_block_t *tv_block, uint8_t *value);
int qm_tv_unpack_int8(qm_tv_block_t *tv_block, int8_t *value);
#if CONFIG_QM_TV_INT16_SUPPORT
int qm_tv_unpack_uint16(qm_tv_block_t *tv_block, uint16_t *value);
int qm_tv_unpack_int16(qm_tv_block_t *tv_block, int16_t *value);
#endif
#if CONFIG_QM_TV_INT32_SUPPORT
int qm_tv_unpack_uint32(qm_tv_block_t *tv_block, uint32_t *value);
int qm_tv_unpack_int32(qm_tv_block_t *tv_block, int32_t *value);
#endif

#if CONFIG_QM_TV_BYTES_SUPPORT
int qm_tv_unpack_bytes(qm_tv_block_t *tv_block, uint8_t *bytes, int *size);
int qm_tv_unpack_string(qm_tv_block_t *tv_block, char *str, int *size);
int qm_tv_unpack_bytes_direct(qm_tv_block_t *tv_block, uint8_t **bytes, int *size);
int qm_tv_unpack_string_direct(qm_tv_block_t *tv_block, char **str, int *size);
#endif