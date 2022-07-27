#ifndef _QM_TLV_H_
#define _QM_TLV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm_types.h"

#ifndef CONFIG_QM_TLV_TAG_SIZE
#define CONFIG_QM_TLV_TAG_SIZE        1
#endif

#ifndef CONFIG_QM_TLV_LENGTH_SIZE
#define CONFIG_QM_TLV_LENGTH_SIZE     1
#endif

#define QM_TLV_BYTES_SIZE             4

#ifndef tlv_malloc
#define tlv_malloc   qm_malloc
#endif

#ifndef tlv_free
#define tlv_free     qm_free
#endif

typedef struct qm_tlv_block{

    uint8_t serialization;
    #if CONFIG_QM_TLV_TAG_SIZE == 1
        uint8_t tag;
    #elif CONFIG_QM_TLV_TAG_SIZE == 2
        uint16_t tag;
    #else
    #error tlv tag type is not support.
    #endif

    #if CONFIG_QM_TLV_LENGTH_SIZE == 1
        uint8_t len;
    #elif CONFIG_QM_TLV_LENGTH_SIZE == 2
        uint16_t len;
    #else
    #error tlv length type is not support.
    #endif

    union{
        uint8_t bytes[QM_TLV_BYTES_SIZE];
        uint8_t *pdata;
    }value;

    void *arg;

    struct qm_tlv_block *next;

}qm_tlv_block_t;

typedef struct
{
    int count;
    struct qm_tlv_block *tlv_block;

}qm_tlv_ctx_t;

qm_tlv_ctx_t *qm_tlv_ctx_creat(void);

qm_tlv_block_t *qm_tlv_block_creat(void);

int qm_tlv_ctx_delete(qm_tlv_ctx_t *tlv_ctx);

int qm_tlv_block_delete(qm_tlv_block_t *tlv_block);

int qm_tlv_deserialize(qm_tlv_ctx_t *tlv_ctx, uint8_t *msg, int len);

qm_tlv_block_t *qm_tlv_block_find(qm_tlv_ctx_t *tlv_ctx, uint16_t tag);

int qm_tlv_block_serialize(qm_tlv_block_t *tlv_block);

int qm_tlv_block_add(qm_tlv_ctx_t *tlv_ctx, qm_tlv_block_t *tlv_block);

int qm_tlv_serialize(qm_tlv_ctx_t *tlv_ctx, uint8_t *msg, int *len, int partial);

int qm_tlv_merge(qm_tlv_ctx_t *dest_tlv_ctx, qm_tlv_ctx_t *src_tlv_ctx);

qm_tlv_block_t *qm_tlv_block_next(qm_tlv_ctx_t *tlv_ctx, qm_tlv_block_t *tlv_block);

int qm_tlv_pack_bool(qm_tlv_block_t *tlv_block, bool_t value);
int qm_tlv_pack_uint8(qm_tlv_block_t *tlv_block, uint8_t value);
int qm_tlv_pack_int8(qm_tlv_block_t *tlv_block, int8_t value);
int qm_tlv_pack_uint16(qm_tlv_block_t *tlv_block, uint16_t value);
int qm_tlv_pack_int16(qm_tlv_block_t *tlv_block, int16_t value);
int qm_tlv_pack_uint32(qm_tlv_block_t *tlv_block, uint32_t value);
int qm_tlv_pack_int32(qm_tlv_block_t *tlv_block, int32_t value);
int qm_tlv_pack_bytes(qm_tlv_block_t *tlv_block, uint8_t *bytes, int size);
int qm_tlv_pack_string(qm_tlv_block_t *tlv_block, char *str, int size);

int qm_tlv_unpack_bool(qm_tlv_block_t *tlv_block, bool_t *value);
int qm_tlv_unpack_uint8(qm_tlv_block_t *tlv_block, uint8_t *value);
int qm_tlv_unpack_int8(qm_tlv_block_t *tlv_block, int8_t *value);
int qm_tlv_unpack_uint16(qm_tlv_block_t *tlv_block, uint16_t *value);
int qm_tlv_unpack_int16(qm_tlv_block_t *tlv_block, int16_t *value);
int qm_tlv_unpack_uint32(qm_tlv_block_t *tlv_block, uint32_t *value);
int qm_tlv_unpack_int32(qm_tlv_block_t *tlv_block, int32_t *value);
int qm_tlv_unpack_bytes(qm_tlv_block_t *tlv_block, uint8_t *bytes, int *size);
int qm_tlv_unpack_string(qm_tlv_block_t *tlv_block, char *str, int *size);


#ifdef __cplusplus
}
#endif


#endif

