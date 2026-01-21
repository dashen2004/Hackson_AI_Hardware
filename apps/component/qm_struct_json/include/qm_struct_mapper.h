#ifndef QM_STRUCT_MAPPER_H
#define QM_STRUCT_MAPPER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QM_TYPE_INT = 0,
    QM_TYPE_LONG, // treated as double in cJson but useful for casting
    QM_TYPE_FLOAT,
    QM_TYPE_DOUBLE,
    QM_TYPE_STRING, // char *
    QM_TYPE_BOOL,
    QM_TYPE_STRUCT,
    QM_TYPE_RawJSON // cJSON *
} qm_field_type_t;

struct qm_struct_desc_t;

typedef struct {
    const char *name;
    qm_field_type_t type;
    size_t offset;
    size_t size;    // size of the single element
    size_t array_count; // 0 or 1 means scalar, >1 means static array
    const struct qm_struct_desc_t *sub_desc; // for nested structs
} qm_field_desc_t;

typedef struct qm_struct_desc_t {
    const qm_field_desc_t *fields;
    size_t num_fields;
} qm_struct_desc_t;

/**
 * @brief Serialize a C structure to cJSON object
 * 
 * @param obj Pointer to the structure instance
 * @param desc Pointer to the structure descriptor
 * @return cJSON* New JSON object (Ownership transferred to caller)
 */
cJSON *qm_struct_to_json(const void *obj, const qm_struct_desc_t *desc);

/**
 * @brief Deserialize a cJSON object to C structure
 * 
 * @param json Source JSON object
 * @param obj Pointer to the structure instance
 * @param desc Pointer to the structure descriptor
 * @return int 0 on success, <0 on error
 */
int qm_json_to_struct(const cJSON *json, void *obj, const qm_struct_desc_t *desc);

/* Macros for defining the schema */

#define QM_STRUCT_DESC_NAME(TYPE) qm_ ## TYPE ## _desc

#define QM_STRUCT_BEGIN(TYPE) \
    static const qm_field_desc_t qm_ ## TYPE ## _fields[] = {

#define QM_FIELD_INT(TYPE, MEMBER) \
    { #MEMBER, QM_TYPE_INT, offsetof(TYPE, MEMBER), sizeof(((TYPE*)0)->MEMBER), 1, NULL },

#define QM_FIELD_FLOAT(TYPE, MEMBER) \
    { #MEMBER, QM_TYPE_FLOAT, offsetof(TYPE, MEMBER), sizeof(((TYPE*)0)->MEMBER), 1, NULL },

#define QM_FIELD_DOUBLE(TYPE, MEMBER) \
    { #MEMBER, QM_TYPE_DOUBLE, offsetof(TYPE, MEMBER), sizeof(((TYPE*)0)->MEMBER), 1, NULL },

#define QM_FIELD_STRING(TYPE, MEMBER) \
    { #MEMBER, QM_TYPE_STRING, offsetof(TYPE, MEMBER), sizeof(((TYPE*)0)->MEMBER), 1, NULL },

#define QM_FIELD_BOOL(TYPE, MEMBER) \
    { #MEMBER, QM_TYPE_BOOL, offsetof(TYPE, MEMBER), sizeof(((TYPE*)0)->MEMBER), 1, NULL },

#define QM_FIELD_STRUCT(TYPE, MEMBER, SUB_TYPE) \
    { #MEMBER, QM_TYPE_STRUCT, offsetof(TYPE, MEMBER), sizeof(((TYPE*)0)->MEMBER), 1, &QM_STRUCT_DESC_NAME(SUB_TYPE) },

#define QM_FIELD_ARRAY_INT(TYPE, MEMBER, COUNT) \
    { #MEMBER, QM_TYPE_INT, offsetof(TYPE, MEMBER), sizeof(int), COUNT, NULL },

#define QM_FIELD_ARRAY_STRUCT(TYPE, MEMBER, COUNT, SUB_TYPE) \
    { #MEMBER, QM_TYPE_STRUCT, offsetof(TYPE, MEMBER), sizeof(SUB_TYPE), COUNT, &QM_STRUCT_DESC_NAME(SUB_TYPE) },

#define QM_STRUCT_END(TYPE) \
    }; \
    const qm_struct_desc_t QM_STRUCT_DESC_NAME(TYPE) = { \
        .fields = qm_ ## TYPE ## _fields, \
        .num_fields = sizeof(qm_ ## TYPE ## _fields) / sizeof(qm_field_desc_t) \
    };

/* Helper for external declaration */
#define QM_STRUCT_DECLARE(TYPE) \
    extern const qm_struct_desc_t QM_STRUCT_DESC_NAME(TYPE);

#ifdef __cplusplus
}
#endif

#endif // QM_STRUCT_MAPPER_H
