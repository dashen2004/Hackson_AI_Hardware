#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qm_struct_mapper.h"

static cJSON *serialize_field(const void *base_addr, const qm_field_desc_t *field) {
    const void *data_ptr = (const uint8_t *)base_addr + field->offset;

    // Handle Scalar
    if (field->array_count <= 1) {
        switch (field->type) {
            case QM_TYPE_INT:
                return cJSON_CreateNumber((double)*(const int *)data_ptr);
            case QM_TYPE_LONG:
                return cJSON_CreateNumber((double)*(const long *)data_ptr);
            case QM_TYPE_FLOAT:
                return cJSON_CreateNumber((double)*(const float *)data_ptr);
            case QM_TYPE_DOUBLE:
                return cJSON_CreateNumber(*(const double *)data_ptr);
            case QM_TYPE_BOOL:
                return cJSON_CreateBool(*(const bool *)data_ptr);
            case QM_TYPE_STRING: {
                char *str = *(char **)data_ptr;
                if (str) return cJSON_CreateString(str);
                return cJSON_CreateNull();
            }
            case QM_TYPE_STRUCT:
                if (field->sub_desc) {
                    return qm_struct_to_json(data_ptr, field->sub_desc);
                }
                break;
            case QM_TYPE_RawJSON: {
                 cJSON *raw = *(cJSON **)data_ptr;
                 if (raw) return cJSON_Duplicate(raw, 1);
                 return cJSON_CreateNull();
            }
            default:
                break;
        }
    } else {
        // Handle Array
        cJSON *arr = cJSON_CreateArray();
        size_t elem_size = field->size; // Size of one element
        if (field->type == QM_TYPE_STRUCT) {
            // For struct arrays, field->size passed in macro is typically sizeof(StructType)
            // But verify: QM_FIELD_ARRAY_STRUCT uses sizeof(SUB_TYPE)
        }
        
        for (size_t i = 0; i < field->array_count; i++) {
            const void *elem_ptr = (const uint8_t *)data_ptr + (i * elem_size);
            cJSON *item = NULL;
            
            switch (field->type) {
                case QM_TYPE_INT: item = cJSON_CreateNumber(*(const int *)elem_ptr); break;
                // Add otehr types if needed for arrays
                case QM_TYPE_STRUCT:
                    if (field->sub_desc) {
                        item = qm_struct_to_json(elem_ptr, field->sub_desc);
                    }
                    break;
                default: 
                    // Basic support for int and struct arrays for now
                    break;
            }
            if (item) cJSON_AddItemToArray(arr, item);
        }
        return arr;
    }
    return NULL;
}

cJSON *qm_struct_to_json(const void *obj, const qm_struct_desc_t *desc) {
    if (!obj || !desc) return NULL;
    
    cJSON *root = cJSON_CreateObject();
    
    for (size_t i = 0; i < desc->num_fields; i++) {
        const qm_field_desc_t *field = &desc->fields[i];
        cJSON *item = serialize_field(obj, field);
        if (item) {
            cJSON_AddItemToObject(root, field->name, item);
        }
    }
    
    return root;
}

static void deserialize_field(void *base_addr, const qm_field_desc_t *field, const cJSON *json_item) {
    void *data_ptr = (uint8_t *)base_addr + field->offset;
    
    if (field->array_count <= 1) {
        switch (field->type) {
            case QM_TYPE_INT:
                if (cJSON_IsNumber(json_item)) *(int *)data_ptr = (int)json_item->valueint;
                break;
            case QM_TYPE_LONG:
                if (cJSON_IsNumber(json_item)) *(long *)data_ptr = (long)json_item->valueint; // Use valuedouble if range needed
                break;
            case QM_TYPE_FLOAT:
                if (cJSON_IsNumber(json_item)) *(float *)data_ptr = (float)json_item->valuedouble;
                break;
            case QM_TYPE_DOUBLE:
                if (cJSON_IsNumber(json_item)) *(double *)data_ptr = json_item->valuedouble;
                break;
            case QM_TYPE_BOOL:
                if (cJSON_IsBool(json_item)) *(bool *)data_ptr = cJSON_IsTrue(json_item);
                break;
            case QM_TYPE_STRING:
                if (cJSON_IsString(json_item) && json_item->valuestring) {
                    // Need to manage memory. For now, assume user allocates or we strdup.
                    // WARNING: If this overwrites a pointer, old memory might leak if not freed.
                    // Here we simply strdup. User must free deep structures.
                    // Check if *data_ptr is not NULL to free? Unsafe if uninitialized.
                    // Assume struct is zero-initialized or managed by user.
                    if (*(char **)data_ptr) free(*(char **)data_ptr);
                    *(char **)data_ptr = strdup(json_item->valuestring);
                }
                break;
            case QM_TYPE_STRUCT:
                if (field->sub_desc) {
                    qm_json_to_struct(json_item, data_ptr, field->sub_desc);
                }
                break;
            default:
                break;
        }
    } else {
        // Array
        if (!cJSON_IsArray(json_item)) return;
        int arr_size = cJSON_GetArraySize(json_item);
        if (arr_size > field->array_count) arr_size = field->array_count;
        
        for (int i = 0; i < arr_size; i++) {
            cJSON *item = cJSON_GetArrayItem(json_item, i);
            void *elem_ptr = (uint8_t *)data_ptr + (i * field->size);
            
             switch (field->type) {
                case QM_TYPE_INT: if(cJSON_IsNumber(item)) *(int*)elem_ptr = item->valueint; break;
                case QM_TYPE_STRUCT:
                     if (field->sub_desc) qm_json_to_struct(item, elem_ptr, field->sub_desc);
                     break;
                default: break;
            }
        }
    }
}

int qm_json_to_struct(const cJSON *json, void *obj, const qm_struct_desc_t *desc) {
    if (!json || !obj || !desc) return -1;
    
    for (size_t i = 0; i < desc->num_fields; i++) {
        const qm_field_desc_t *field = &desc->fields[i];
        cJSON *item = cJSON_GetObjectItem(json, field->name);
        if (item) {
            deserialize_field(obj, field, item);
        }
    }
    
    return 0;
}
