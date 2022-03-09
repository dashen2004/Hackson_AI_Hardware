#include "prodtst_cloud.h"
#include "qm_errno.h"
#include "cJSON.h"
#include "qm_log.h"
#include "prodtst_core.h"


typedef struct{
    int (*handle)(int id, char *data, int len);
}prodtst_unpack_t;

static int config_unpack_and_handle(int id, char *data, int len);
static int set_unpack_and_handle(int id, char *data, int len);
static int get_unpack_and_handle(int id, char *data, int len);
static int special_unpack_and_handle(int id, char *data, int len);
static int result_unpack_and_handle(int id, char *data, int len);

static prodtst_unpack_t prodtst_unpack[] = {

    {config_unpack_and_handle},
    {set_unpack_and_handle},
    {get_unpack_and_handle},
    {special_unpack_and_handle},
    {result_unpack_and_handle},

};


//{"module_type":"wifi","platform":"harmony","module_name":"W70","dev_type":"serial"}

static int config_unpack_and_handle(int id, char *data, int len)
{
    qm_err_t ret = QM_EOK;
    cJSON *root = NULL;
    cJSON *object = NULL;
    char *platform = NULL;
    char *module_type = NULL;
    char *module_name = NULL;
    char *dev_type = NULL;

    root = cJSON_Parse(data);
    if(root == NULL){
        return -QM_EINVAL;
    }
    
    object = cJSON_GetObjectItem(root, "platform");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    platform = object->string;

    object = cJSON_GetObjectItem(root, "module_name");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    module_name = object->string;

    object = cJSON_GetObjectItem(root, "module_type");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    module_type = object->string;

    object = cJSON_GetObjectItem(root, "dev_type");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    dev_type = object->string;

    prodtst_config_notify(platform, module_name, module_type);

__exit:
    cJSON_Delete(root);
    return ret;
}

static int generic_unpack_and_handle(int event, int id, char *data, int len)
{
    qm_err_t ret = QM_EOK;
    cJSON *root = NULL;
    cJSON *object = NULL;
    char *type = NULL;
    char *mdata = NULL;
    char *uuid = NULL;

    root = cJSON_Parse(data);
    if(root == NULL){
        return -QM_EINVAL;
    }
    
    object = cJSON_GetObjectItem(root, "type");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    type = object->string;

    object = cJSON_GetObjectItem(root, "uuid");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    uuid = object->string;

    object = cJSON_GetObjectItem(root, "data");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    mdata = object->string;

    prodtst_data_notify(event, type, uuid, id, (uint8_t*)mdata, strlen(mdata));

__exit:
    cJSON_Delete(root);
    return ret;
}

static int set_unpack_and_handle(int id, char *data, int len)
{
    return generic_unpack_and_handle(PRODTST_EVENT_SET, id, data, len);
}

static int get_unpack_and_handle(int id, char *data, int len)
{
    return generic_unpack_and_handle(PRODTST_EVENT_GET, id, data, len);
}

static int result_unpack_and_handle(int id, char *data, int len)
{
    qm_err_t ret = QM_EOK;
    cJSON *root = NULL;
    cJSON *object = NULL;
    int result = 0;
    char *uuid = NULL;

    root = cJSON_Parse(data);
    if(root == NULL){
        return -QM_EINVAL;
    }

    object = cJSON_GetObjectItem(root, "uuid");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    uuid = object->string;

    object = cJSON_GetObjectItem(root, "result");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    result = object->valueint;
    
    prodtst_finish_notify(uuid, result);

__exit:
    cJSON_Delete(root);
    return ret;
}

static int special_unpack_and_handle(int id, char *data, int len)
{
    qm_err_t ret = QM_EOK;
    cJSON *root = NULL;
    cJSON *object = NULL;
    char *m_data = NULL;
    char *uuid = NULL;
    char *action = NULL;

    root = cJSON_Parse(data);
    if(root == NULL){
        return -QM_EINVAL;
    }

    object = cJSON_GetObjectItem(root, "uuid");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    uuid = object->string;


    object = cJSON_GetObjectItem(root, "action");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    action = object->string;

    object = cJSON_GetObjectItem(root, "data");
    if(object == NULL){
        ret = -QM_EINVAL;
        goto __exit;
    }
    m_data = object->string;
    
    prodtst_special_notify(uuid, action, m_data, len);

__exit:
    cJSON_Delete(root);
    return ret;
}

int prodtst_generic_unpack_and_handle(int packet_type, int id, char *data, int len)
{
    qm_err_t ret = QM_EOK;

    if(data == NULL || len <= 0){
        return -QM_EINVAL;
    }

    ret = prodtst_unpack[packet_type].handle(id, data, len);

    return ret;
}