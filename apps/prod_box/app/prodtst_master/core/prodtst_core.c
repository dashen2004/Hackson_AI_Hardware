#include "prodtst_core.h"
#include "qm_errno.h"
#include "prodtst_adapter.h"
#include "qm_log.h"

#define LOG_TAG "prodtst"


static int prodtst_obj_reset(prodtst_obj_t *prodtst_obj);
static int prodtst_obj_set(prodtst_obj_t *prodtst_obj);
static int prodtst_data_reset(prodtst_obj_t *prodtst_obj);
static int prodtst_data_print(prodtst_obj_t *prodtst_obj);

static prodtst_context_t g_prodtst_ctx = {0};

static prodtst_cmd_t wifi_prodtst_generic_cmd[] = {
    {"fw_ver",       "AT+FW_VER?",  CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, 32, prodtst_fw_ver},
    {"fw_name",      "AT+FW_NAME?", CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, 32, prodtst_fw_name},
    {"wifi_sta_mac", "AT+MAC_STA?", CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, 32, wifi_sta_mac},
    {"wifi_ap_mac",  "AT+MAC_AP?",  CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, 32, wifi_ap_mac},
};

#if 0
static prodtst_cmd_t wifi_ble_prodtst_generic_cmd[] = {
    {"AT+FW_VER?", PRODTST_CMD_ACK_DEFAULT_TIMEOUT},
    {"AT+FW_NAME?", PRODTST_CMD_ACK_DEFAULT_TIMEOUT},
    {"AT+MAC_STA?", PRODTST_CMD_ACK_DEFAULT_TIMEOUT},
    {"AT+MAC_AP?", PRODTST_CMD_ACK_DEFAULT_TIMEOUT},
    {"AT+MAC_BLE?", PRODTST_CMD_ACK_DEFAULT_TIMEOUT},

};

#endif

static prodtst_cmd_t harmony_prodtst_specific_cmd[] = {
    {"AT+CERT_HW?", CONFIG_QM_PRODTST_CMD_ACK_DEFAULT_TIMEOUT, 4096, prodtst_hw_config},

};

extern prodtst_adapter_t wifi_prodtst_adapter;

static prodtst_testsuite_t  g_prodtst_testsuite[] = {
    {"ali", "W70", "wifi", 4096, 1, wifi_prodtst_generic_cmd, 4, harmony_prodtst_specific_cmd, 1, &wifi_prodtst_adapter},


};

#define PRODTST_TESTSUITE_NUM (sizeof(g_prodtst_testsuite)/sizeof(g_prodtst_testsuite[0]))

static prodtst_testsuite_t *prodtst_testsuite_alloc(char *platform, char *module_name, char *module_type)
{
    int i = 0;
    if(platform == NULL || module_name == NULL || module_type){
        return NULL;
    }

    for(i = 0; i < PRODTST_TESTSUITE_NUM; i++){
        if( (strlen(platform) == strlen(g_prodtst_testsuite[i].platform)) && (strcmp(platform, g_prodtst_testsuite[i].platform)) && 
            (strlen(module_name) == strlen(g_prodtst_testsuite[i].module_name)) && (strcmp(module_name, g_prodtst_testsuite[i].module_name)) &&
            (strlen(module_type) == strlen(g_prodtst_testsuite[i].module_type)) && (strcmp(module_type, g_prodtst_testsuite[i].module_type))){

                return &g_prodtst_testsuite[i];
        }
    }
    return NULL;
}

int prodtst_device_num_get(void)
{
    return g_prodtst_ctx.testsuite->obj_num;
}


static prodtst_adapter_t *prodtst_adapter_get(void)
{
    return g_prodtst_ctx.testsuite->adapter;
}

static prodtst_testsuite_t *prodtst_testsuite_get(void)
{
    return g_prodtst_ctx.testsuite;
}

int prodtst_start(prodtst_info_t *info)
{
    qm_err_t ret = QM_EOK;
    prodtst_testsuite_t *testsuite = NULL;
    if(info == NULL){
        return -QM_EINVAL;
    }

    testsuite = prodtst_testsuite_alloc(info->platform, info->module_name, info->module_type);
    if(testsuite == NULL){
        QM_LOGE(LOG_TAG, "prodtst param is invalid");
        return -QM_EINVAL;
    }

    if(g_prodtst_ctx.start){
        prodtst_stop();
        g_prodtst_ctx.start = 0;
    }

    g_prodtst_ctx.start = 1;
    g_prodtst_ctx.testsuite = testsuite;

    ret = g_prodtst_ctx.testsuite->adapter->init(NULL);
    if(ret != QM_EOK){
        return ret;
    }

    QM_LOGI(LOG_TAG, "prodtst start success");

    return QM_EOK;
}


int prodtst_stop(void)
{
    qm_err_t ret = QM_EOK;

    ret = g_prodtst_ctx.testsuite->adapter->deinit();
    if(ret != QM_EOK){
        return ret;
    }

    QM_LOGI(LOG_TAG, "prodtst stop success");

    return ret;
}

static int prodtst_data_reset(prodtst_obj_t *prodtst_obj)
{
    int index = 0;
    for(index = 0; index < CONFIG_QM_PRODTST_CMD_NUM; index++){
        if(prodtst_obj->data[index].data){
            qm_free(prodtst_obj->data[index].data);
            prodtst_obj->data[index].data = NULL;
        }
    }

    return QM_EOK;
}

static int prodtst_data_print(prodtst_obj_t *prodtst_obj)
{
    int index = 0;
    for(index = 0; index < CONFIG_QM_PRODTST_CMD_NUM; index++){
        if(prodtst_obj->data[index].data){
            QM_LOGI(LOG_TAG, "cmd name:      %s", prodtst_obj->data[index].name);
            QM_LOGI(LOG_TAG, "response data: %s", prodtst_obj->data[index].data);
        }
    }

    return QM_EOK;
}

static int prodtst_generic_and_specific_handler(prodtst_obj_t *prodtst_obj)
{
    int num = 0;
    int index = 0;
    qm_err_t ret = QM_EOK;
    qm_at_response_t *resp = NULL;
    prodtst_testsuite_t *testsuite = NULL;

    testsuite = prodtst_testsuite_get();

    prodtst_obj->state = PRODTST_STATE_GENERIC;

    for(num = 0; num < testsuite->generic_cmd_num; num++){

        resp = qm_at_client_create_resp(testsuite->generic_cmd[num].recv_size, 0, testsuite->generic_cmd[num].timeout);
        ret = qm_at_client_obj_exec_cmd(prodtst_obj->at_client, resp, testsuite->generic_cmd[num].cmd, testsuite->generic_cmd[num].timeout);
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "wait generic response timeout");
            goto __exit;
        }

        ret = testsuite->generic_cmd[num].parser(resp, &prodtst_obj->data[num].data);
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "parse generic response failed");
            goto __exit;
        }

        prodtst_obj->data[index++].name = testsuite->generic_cmd[num].name;
        qm_at_client_delete_resp(resp);
        resp = NULL;
    }

    prodtst_obj->state = PRODTST_STATE_SPECIFIC;

    for(num = 0; num < testsuite->specific_cmd_num; num++){

        resp = qm_at_client_create_resp(testsuite->specific_cmd[num].recv_size, 0, testsuite->specific_cmd[num].timeout);
        ret = qm_at_client_obj_exec_cmd(prodtst_obj->at_client, resp, testsuite->specific_cmd[num].cmd, testsuite->specific_cmd[num].timeout);
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "wait specific response timeout");
            goto __exit;
        }

        ret = testsuite->generic_cmd[num].parser(resp, &prodtst_obj->data[num].data);
        if(ret != QM_EOK){
            QM_LOGE(LOG_TAG, "parse specific response failed");
            goto __exit;
        }

        prodtst_obj->data[index++].name = testsuite->generic_cmd[num].name;
        qm_at_client_delete_resp(resp);
        resp = NULL;
    }

    return ret;

__exit:

    if(resp){
        qm_at_client_delete_resp(resp);
        resp = NULL;
    }

    prodtst_data_reset(prodtst_obj);

    return ret;
}


static void prodtst_device_task(void *arg)
{
    uint32_t size = 0;
    qm_err_t ret = QM_EOK;
    prodtst_adapter_t *adapter = NULL;
    prodtst_testsuite_t *testsuite = NULL;
    prodtst_notify_t notify = {0};
    qm_at_client_param_t at_param = {0};
    prodtst_obj_t *prodtst_obj = (prodtst_obj_t*)arg;

    prodtst_obj->state = PRODTST_STATE_SETUP;

    adapter = prodtst_adapter_get();
    ret = adapter->establish(prodtst_obj->param, &prodtst_obj->channel_handle);
    if(ret != QM_EOK){
        goto __exit;
    }
    if(prodtst_obj->param){
        qm_free(prodtst_obj->param);
        prodtst_obj->param = NULL;
    }

    testsuite = prodtst_testsuite_get();

    at_param.recv_buf = (char*)qm_malloc(testsuite->recv_size);
    if(at_param.recv_buf == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }
    at_param.recv_buf_len = testsuite->recv_size;
    at_param.handle = prodtst_obj->channel_handle;
    at_param.send = adapter->send;
    at_param.recv = adapter->recv;

    prodtst_obj->at_client = qm_at_client_init(&at_param);
    if(prodtst_obj->at_client == NULL){
        ret = -QM_ERROR;
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "channel established");

    ret = prodtst_generic_and_specific_handler(prodtst_obj);
    if(ret != QM_EOK){
        goto __exit;
    }

    QM_LOGI(LOG_TAG, "generic and specific test success");

    prodtst_data_print(prodtst_obj);
    
    while(1){
        ret = qm_queue_recv(&prodtst_obj->notify, &notify, &size, QM_WAIT_FOREVER);
        if(ret != QM_EOK){
            continue;
        }
         
 


    }

__exit:

    if(at_param.recv_buf){
        qm_free(at_param.recv_buf);
        at_param.recv_buf = NULL;
    }
    
    if(prodtst_obj->param){
        qm_free(prodtst_obj->param);
        prodtst_obj->param = NULL;
    }
    
    if(prodtst_obj->at_client){
        qm_at_client_deinit(prodtst_obj->at_client);
    }
    
    qm_task_exit(&prodtst_obj->task);
}

static int prodtst_obj_set(prodtst_obj_t *prodtst_obj)
{
    int index = 0;
    if(prodtst_obj == NULL){
        return -QM_EINVAL;
    }

    for(index = 0; index < CONFIG_QM_PRODTST_OBJ_NUM; index++){
        if(g_prodtst_ctx.prodtst_obj[index] == NULL){
            g_prodtst_ctx.prodtst_obj[index] = prodtst_obj;
            return QM_EOK;
        }
    }

    return -QM_EFULL;
}

static int prodtst_obj_reset(prodtst_obj_t *prodtst_obj)
{
    int index = 0;
    if(prodtst_obj == NULL){
        return -QM_EINVAL;
    }

    for(index = 0; index < CONFIG_QM_PRODTST_OBJ_NUM; index++){
        if(g_prodtst_ctx.prodtst_obj[index] == prodtst_obj){
            g_prodtst_ctx.prodtst_obj[index] = NULL;
        }
    }

    return -QM_EINVAL;
}


static int prodtst_device_init(void *param)
{
    qm_err_t ret = QM_EOK;
    prodtst_obj_t *prodtst_obj = NULL;

    prodtst_obj = (prodtst_obj_t*)qm_malloc(sizeof(prodtst_obj_t));
    if(prodtst_obj == NULL){
        ret = -QM_ENOMEM;
        goto __exit;
    }

    memset(prodtst_obj, 0, sizeof(prodtst_obj_t));

    ret = qm_queue_new(&prodtst_obj->notify, sizeof(prodtst_obj_t), CONFIG_QM_PRODTST_NOTIFY_QUEUE_NUM);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = qm_task_new(&prodtst_obj->task, "device", prodtst_device_task, (void*)prodtst_obj, 
                                            CONFIG_QM_PRODTST_TASK_SIZE, CONFIG_QM_PRODTST_TASK_PRIO);
    if(ret != QM_EOK){
        goto __exit;
    }

    ret = prodtst_obj_set(prodtst_obj);
    if(ret != QM_EOK){
        goto __exit;
    }

    return ret;

__exit:

    if(prodtst_obj){

        if(prodtst_obj->notify.hdl){
            qm_queue_free(&prodtst_obj->notify);
        }

        if(prodtst_obj->task.hdl){
            qm_task_exit(&prodtst_obj->task);
        }

        qm_free(prodtst_obj);
        prodtst_obj = NULL;
    }

    return ret;
}

static int prodtst_device_deinit(prodtst_obj_t *prodtst_obj)
{
    qm_err_t ret = QM_EOK;
    if(prodtst_obj == NULL){
        return -QM_EINVAL;
    }
    ret = qm_queue_free(&prodtst_obj->notify);
    if(ret != QM_EOK){
        return ret;
    }

    ret = qm_task_exit(&prodtst_obj->task);
    if(ret != QM_EOK){
        return ret;
    }

    prodtst_obj_reset(prodtst_obj);

    qm_free(prodtst_obj);
    prodtst_obj = NULL;

    return ret;
}

int prodtst_found_notify(void *param)
{
    prodtst_notify_t notify = {0};
    if(param == NULL){
        return -QM_EINVAL;
    }

    notify.event = PRODTST_EVENT_DEVICE_FOUND;
    notify.dat.device_info.param = param;

    return qm_queue_send(&g_prodtst_ctx.notify, (void*)&notify, sizeof(prodtst_notify_t));
}

int prodtst_ip_notify(char *ip)
{
    uint8_t *data = NULL;
    if(ip == NULL){
        return -QM_EINVAL;
    }

    data = (uint8_t*)qm_malloc(strlen(ip)+1);
    if(data == NULL){
        return -QM_ENOMEM;
    }
    memset(data, 0, strlen(ip)+1);
    memcpy(data, ip, strlen(ip));

    return prodtst_found_notify(data);
}

int prodtst_data_notify(int event, char* type, char *uuid, uint8_t *data, int len)
{
    prodtst_notify_t notify = {0};
    if(type == NULL){
        return -QM_EINVAL;
    }

    if(strlen(type) > PRODTST_DATA_TYPE_MAX_LEN){
        return -QM_EINVAL;
    }

    if(data == NULL || len <= 0){
        return -QM_EINVAL;
    }

    if(uuid == NULL){
        return -QM_EINVAL;
    }

    notify.event = event;

    notify.dat.data_info.data = (uint8_t*)qm_malloc(len);
    if(notify.dat.data_info.data == NULL){
        return -QM_ENOMEM;
    }

    memcpy(notify.dat.data_info.data, data, len);
    memcpy(notify.dat.data_info.uuid, uuid, 16);

    memcpy(notify.dat.data_info.type, type, strlen(type));
    
    return qm_queue_send(&g_prodtst_ctx.notify, (void*)&notify, sizeof(prodtst_notify_t));
}


int prodtst_config_notify(char *platform, char *module_name, char *module_type)
{
    prodtst_notify_t notify = {0};
    if(platform == NULL || module_name == NULL || module_type == NULL){
        return -QM_EINVAL;
    }
    if(strlen(platform)    > PRODTST_PLATFORM_MAX_LEN    || 
       strlen(module_name) > PRODTST_MOUDLE_NAME_MAX_LEN ||
       strlen(module_type) > PRODTST_MOUDLE_TYPE_MAX_LEN){
        return -QM_EINVAL;
    }

    notify.event = PRODTST_EVENT_CONFIG;
    memcpy(notify.dat.config_info.platform, platform, strlen(platform));
    memcpy(notify.dat.config_info.module_name, module_name, strlen(module_name));
    memcpy(notify.dat.config_info.module_type, module_type, strlen(module_type));
    
    return qm_queue_send(&g_prodtst_ctx.notify, (void*)&notify, sizeof(prodtst_notify_t));
}

int prodtst_finish_notify(char *uuid, int result)
{
    prodtst_notify_t notify = {0};
    if(uuid == NULL){
        return -QM_EINVAL;
    }

    notify.event = PRODTST_EVENT_FINISH;
    notify.dat.finish_info.result = result;
    memcpy(notify.dat.finish_info.uuid, uuid, 16);

    return qm_queue_send(&g_prodtst_ctx.notify, (void*)&notify, sizeof(prodtst_notify_t));

}

static void prodtst_core_task(void *arg)
{
    qm_err_t ret = QM_EOK;
    prodtst_notify_t notify = {0};
    uint32_t size = 0;
    prodtst_context_t *prodtst_ctx = (prodtst_context_t*)arg;

    while(1){
        ret = qm_queue_recv(&prodtst_ctx->notify, &notify, &size, QM_WAIT_FOREVER);
        if(ret != QM_EOK){
            continue;
        }

        switch(notify.event){

            case PRODTST_EVENT_CONFIG:


            //int prodtst_start(prodtst_info_t *info)

            break;

            case PRODTST_EVENT_DEVICE_FOUND:


            break;


 


            default:
            break;
        }


    }

}


int prodtst_init(void)
{
    qm_err_t ret = QM_EOK;
    memset(&g_prodtst_ctx, 0, sizeof(prodtst_context_t));

    ret = qm_task_new(&g_prodtst_ctx.task, "prodtst", prodtst_core_task, (void*)&g_prodtst_ctx, CONFIG_QM_PRODTST_TASK_SIZE, CONFIG_QM_PRODTST_TASK_PRIO);
    if(ret != QM_EOK){
        return ret;
    }

    return QM_EOK;
}







