#include "serial_protocol.h"
#include "qm_utils_timer.h"
#include "qm_utils_list.h"
#include "serial_platform.h"
#include "qm_utils_string.h"
#include "serial_common.h"

#ifdef SERIAL_OTA_SUPPORT
#include "serial_ota.h"
#endif

static uint8_t frame_chksum(uint8_t *buffer, uint16_t length);
static void frame_head_init(uint8_t *buffer, frame_cmd_t cmd, uint16_t len);
static int32_t frame_creat(frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t data_len, uint8_t *buf, uint16_t *buf_len);
static uint8_t packet_id_get(void);
static int32_t serial_heartbeat_send(void);
static int32_t serial_generate_send(frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len);
static int32_t serial_generate_send_with_code(frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t len);
static uint16_t get_packet_len(uint8_t*packet_buffer);
static int32_t serial_keepalive(serial_context_t *pcontext);
static int32_t serial_id_info_push(serial_data_type_t type, platform_id_t platform_id, uint8_t local_id, uint8_t *data);
static int32_t serial_id_uptime(uint8_t local_id);
static int32_t serial_cloud_generate_send(frame_cmd_t cmd, platform_id_t id, uint8_t *data, uint16_t len);
static int id_match(void *a, void *b);
static int32_t serial_local_id_find(uint8_t local_id, platform_id_t *platform_id);
static serial_data_type_t data_type_get(uint8_t local_id);
static int32_t frame_parse(uint8_t *data, uint16_t len);
static int serial_recv_cycle(serial_context_t *pcontext, qm_utils_time_t *timer);
static int32_t serial_deviceinfo_get_send(void);
static int32_t serial_snapshot_get_send(void);
static int32_t serial_process_cycle(serial_context_t *pcontext);
static int32_t id_check_cycle(serial_context_t *pcontext);
static int32_t deviceinfo_handler(serial_context_t *context, uint8_t *data, uint16_t len);
static uint8_t local_id_get(uint8_t *raw);
static int32_t down_queue_send(void);
static int32_t serial_service_register(module_service_type_t service_type, serial_service_fn fn, int32_t is_inner);
static int32_t serial_service_handler(uint8_t *data, uint16_t len);

#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
#define MAX_SERIAL_EVENT_NUM  10
static int event_handler(uint8_t *data, uint16_t len);
#endif
#endif

static serial_context_t *g_serial_context;


#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
static int event_handler(uint8_t *data, uint16_t len)
{
    serial_context_t *context = NULL;
    uint8_t *buf = NULL;
    uint16_t buf_len = 0;
    context = qm_serial_context_get();

    qm_list_node_t *node = NULL;
    qm_list_node_t *tmp_node = NULL;

    buf_len = len + sizeof(qm_list_node_t)+2;
    buf = qm_serial_malloc(buf_len);
    if(buf == NULL){
        return SERIAL_ALLOC_MEM_ERR;
    }
    node = (qm_list_node_t*)buf;
    qm_list_node_init(node, buf + sizeof(qm_list_node_t));
    memcpy(buf+sizeof(qm_list_node_t), &len, 2);
    memcpy(buf+sizeof(qm_list_node_t) + 2, data, len);

    #ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
    #endif
    if(context->event_info.serial_event_queue->len < MAX_SERIAL_EVENT_NUM){
        qm_list_rpush(context->event_info.serial_event_queue, node);
    }else{
        tmp_node = qm_list_lpop(context->event_info.serial_event_queue);
        if(tmp_node){
            qm_serial_free(tmp_node);
        }
        qm_list_rpush(context->event_info.serial_event_queue, node);
    }
    #ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
    #endif

	return SERIAL_NO_ERR;
}

uint8_t is_event_info_update(void)
{
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    if(context == NULL){
        return 0;
    }
    if(context->event_info.serial_event_queue->len){
        return 1;
    }else{
        return 0;
    }
}

int32_t qm_serial_event_info_get(uint8_t *buf, uint16_t *buf_len)
{
    serial_context_t *context = NULL;
    qm_list_node_t *node = NULL;
    uint8_t *tmp_buf = NULL;
    uint16_t tmp_buf_len = 0;
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

    if(buf == NULL){
        return SERIAL_PARAM_ERR;
    }

    if(context->event_info.serial_event_queue->len == 0){
        *buf_len = 0;
        return SERIAL_NO_ERR;
    }

    #ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
    #endif

    node = qm_list_lpop(context->event_info.serial_event_queue);
    if(node){
        tmp_buf = (uint8_t*)node;
        memcpy(&tmp_buf_len, tmp_buf+sizeof(qm_list_node_t), 2);
        if(*buf_len >= tmp_buf_len){
            memcpy(buf, tmp_buf+sizeof(qm_list_node_t)+2, tmp_buf_len);
            *buf_len = tmp_buf_len;
        }
        qm_serial_free(tmp_buf);
    }

    #ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
    #endif

    return SERIAL_NO_ERR;
}
#endif
#endif

serial_context_t *qm_serial_context_get(void)
{
    return g_serial_context;
}

static uint8_t frame_chksum(uint8_t *buffer, uint16_t length)
{
    uint16_t i;
    uint8_t check_sum = 0;

    for (i = 0; i < length; i++){
        check_sum += buffer[i];
    }

    return check_sum;
}

static uint8_t packet_id_get(void)
{
    serial_context_t *serial_context = NULL;
    serial_context = qm_serial_context_get();
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(serial_context->serial_mutex);
#endif
    serial_context->packet_id = (serial_context->packet_id == (MAX_SERIAL_MCU_PACKET_ID+1)) ? MIN_SERIAL_MCU_PACKET_ID : serial_context->packet_id + 1;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(serial_context->serial_mutex);
#endif
    return  serial_context->packet_id;
}

static serial_data_type_t data_type_get(uint8_t local_id)
{
    if(local_id >= MIN_SERIAL_MCU_PACKET_ID && local_id <= MAX_SERIAL_MCU_PACKET_ID){
        return TYPE_MCU;
    }else if(local_id >= MIN_SERIAL_VOICE_PACKET_ID &&  local_id <= MAX_SERIAL_VOICE_PACKET_ID){
        return TYPE_VOICE;
    }else{
        return TYPE_NULL;
    }
}

#ifdef SERIAL_MESH_GROUP_ADDR_SUPPORT
int32_t mesh_group_addr_get(uint16_t *addr)
{
    serial_context_t *context = NULL;
    if(addr == NULL){
        return SERIAL_PARAM_ERR;
    }
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }
    *addr = context->mesh_group_addr;
    return SERIAL_NO_ERR;
}
#endif


#ifdef SERIAL_ID_MAP_SUPPORT
id_map_context_t* id_map_context_get(void)
{
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    if(context == NULL){
        return NULL;
    }
    return &context->map_context;
}
#endif

serial_snapshot_info_t* qm_serial_snapshot_info_get(void)
{
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    if(context == NULL){
        return NULL;
    }
    return &context->serial_snapshot_info;
}

#if defined(SERIAL_PID_INT_SUPPORT) || defined(SERIAL_PID_STRING_SUPPORT)

void *qm_serial_pid_get(void)
{
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    if(context == NULL){
        return NULL;
    }
#ifdef SERIAL_PID_STRING_SUPPORT
    return context->pid;
#endif
#ifdef SERIAL_PID_INT_SUPPORT
    return &context->pid;
#endif

}

#endif

static int32_t deviceinfo_handler(serial_context_t *context, uint8_t *data, uint16_t len)
{
    int32_t err = SERIAL_NO_ERR;
    message_info_t *object = NULL;
    uint8_t *map_data = NULL;
    uint16_t map_data_len = 0;
    message_context_t *message_context = NULL;

    message_context = message_parse(data, len, PARSE_TYPE_BASIC);
    if(message_context == NULL){
       err = SERIAL_PARAM_ERR;
       SERIAL_LOG("device info parse error");
       goto exit;
    }

#ifdef SERIAL_MESH_GROUP_ADDR_SUPPORT
    object = message_object_get(message_context, MESSAGE_MESH_GROUP_ADDR);
    if(object == NULL){
        err = SERIAL_PARAM_ERR;
        goto exit;
    }
    context->mesh_group_addr = object->value.value_u16;
#endif

#ifdef SERIAL_PID_INT_SUPPORT
    object = message_object_get(message_context, MESSAGE_PID_INT);
    if(object == NULL){
        err = SERIAL_PARAM_ERR;
        SERIAL_LOG("missing pid");
        goto exit;
    }
    context->pid = object->value.value_u32;

#endif
#ifdef SERIAL_PID_STRING_SUPPORT
    object = message_object_get(message_context, MESSAGE_PID_STRING);
    if(object == NULL){
        err = SERIAL_PARAM_ERR;
        SERIAL_LOG("missing pid");
        goto exit;
    }
    context->pid = (char*)qm_serial_malloc(object->len + 1);
    if(context->pid == NULL){
        err = SERIAL_ALLOC_MEM_ERR;
        goto exit;
    }
    memset(context->pid, 0, object->len+1);
    memcpy(context->pid, object->data, object->len);

#endif

#ifdef SERIAL_ID_MAP_SUPPORT

#ifdef SERIAL_ALI_ID_MAP_SUPPORT
    object = message_object_get(message_context, MESSAGE_ALI_ID_MAP);
    if(object == NULL){
        err = SERIAL_PARAM_ERR;
        SERIAL_LOG("device info object get error");
        goto exit;
    }
#endif
#ifdef SERIAL_XM_ID_MAP_SUPPORT
       object = message_object_get(message_context, MESSAGE_XM_ID_MAP);
       if(object == NULL){
            err = SERIAL_PARAM_ERR;
            SERIAL_LOG("device info object get error");
            goto exit;
       }
#endif
#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
       object = message_object_get(message_context, MESSAGE_ONENET_ID_MAP);
       if(object == NULL){
            err = SERIAL_PARAM_ERR;
            SERIAL_LOG("device info object get error");
            goto exit;
       }
#endif
#if defined SERIAL_ALI_ID_MAP_SUPPORT || defined SERIAL_XM_ID_MAP_SUPPORT ||defined SERIAL_ONENET_ID_MAP_SUPPORT

       if(object->type_id != DATA_TYPE_ARRAY){
           err = SERIAL_PARAM_ERR;
           goto exit;
       }

       message_data_get(object, &map_data, &map_data_len);
       if(map_data_len == 0 || map_data == NULL){
           err = SERIAL_PARAM_ERR;
           goto exit;
       }

       id_map_delete(&context->map_context);

#endif

#ifdef SERIAL_ALI_ID_MAP_SUPPORT
       err = id_map_parse(&context->map_context, map_data, map_data_len, ID_MAP_ALI);
       if(err != SERIAL_NO_ERR){
            SERIAL_LOG("id map parse error");
            goto exit;
       }
#endif

#ifdef SERIAL_XM_ID_MAP_SUPPORT
       err = id_map_parse(&context->map_context, map_data, map_data_len, ID_MAP_XM);
       if(err != SERIAL_NO_ERR){
            SERIAL_LOG("id map parse error");
            goto exit;
       }
#endif

#ifdef SERIAL_ONENET_ID_MAP_SUPPORT
       err = id_map_parse(&context->map_context, map_data, map_data_len, ID_MAP_ONENET);
       if(err != SERIAL_NO_ERR){
            SERIAL_LOG("id map parse error");
            goto exit;
       }
#endif

#endif

exit:
    if(message_context){
        message_delete(message_context);
    }
    return err;
}

static int32_t serial_service_handler(uint8_t *data, uint16_t len)
{
    int32_t ret = 0;
    serial_context_t *context = NULL;
    message_context_t *message_ctx = NULL;
    message_info_t *info = NULL;
    message_info_t *service_info = NULL;
    serial_service_callbacks_t *service_cbs = NULL;

    context = qm_serial_context_get();

    message_ctx = message_parse(data, len, PARSE_TYPE_BASIC);
    if(message_ctx == NULL){
        return SERIAL_PARAM_ERR;
    }

    info = message_object_next(message_ctx, NULL);

    while(info){
   
        service_info = message_object_get(context->service_ctx, info->attr_id);
        if(service_info == NULL){
            ret = SERIAL_PARAM_ERR;
            goto __exit;
        }
        service_cbs = (serial_service_callbacks_t*)service_info->arg;

        if(service_cbs->inner_fn){
            ret = service_cbs->inner_fn((void*)info);
            if(ret != 0){
                goto __exit;
            }
        }
        if(service_cbs->extern_fn){
            ret = service_cbs->extern_fn((void*)info);
            if(ret != 0){
                goto __exit;
            }
        }

        info = message_object_next(message_ctx, info);
    }

__exit:

    if(message_ctx){
        message_delete(message_ctx);
    }
    return ret;
}

static int32_t frame_parse(uint8_t *data, uint16_t len)
{
    int32_t err = SERIAL_NO_ERR;
    serial_context_t *context;
    frame_header_t *head = (frame_header_t *)data;
    frame_cmd_t cmd = head->cmd;
    context=qm_serial_context_get();
    uint8_t local_id=0;
    platform_id_t platform_id = 0;
    serial_data_type_t type = 0;
    device_ver_t ver = {0};
    uint8_t heartbeat_type = 0;
    uint8_t config_type = 0;
    uint16_t changed_data_len = 0;
    uint8_t is_update = 0;
    serial_time_info_t time_info = {0};

    if(*(data + len - 1) != frame_chksum(data, len - 1)){
        return SERIAL_CHKSUM_ERR;
    }

    switch (cmd){

       case CMD_REPORT:

        if(len < FRAME_REPORT_MIN_LEN){
            return SERIAL_PACKET_LEN_ERR;
        }

        serial_generate_send(cmd, 0, NULL, 0);

        if(context->state != SERIAL_STATE_NORMAL){
            SERIAL_LOG("process error");
            return SERIAL_PROCESS_ERR;
        }

#ifndef SERIAL_LOWPOWER_SUPPORT
        if(qm_serial_module_status_get() != MODULE_CONNECTED){
            SERIAL_LOG("module unconnected");
            return SERIAL_PROCESS_ERR;
        }
#endif

        if(!context->serial_snapshot_info.message_context){
            SERIAL_LOG("no snapshot");
            return SERIAL_PROCESS_ERR;
        }

        err = message_object_add_update(context->serial_snapshot_info.message_context, data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN-1, &is_update);
        if(err != SERIAL_NO_ERR){
            SERIAL_LOG("report data err");
            return SERIAL_DATA_TYPE_ERR;
        }

        if(!is_update){
            SERIAL_LOG("need not report");
#ifndef SERIAL_REPORT_SUPPORT
            if(is_message_update(context->serial_snapshot_info.message_context)){
            	context->serial_snapshot_info.is_changed = 1;
            }else{
            	context->serial_snapshot_info.is_changed = 0;
            }
#endif
            return SERIAL_NO_ERR;
        }else{
            context->serial_snapshot_info.is_changed = 1;
        }

        SERIAL_LOG("need report");

    #ifdef SERIAL_REPORT_SUPPORT
        message_serialize(context->serial_snapshot_info.message_context, NULL, &changed_data_len, SERIALIZE_TYPE_CHANGED);
        if(changed_data_len > 0){
            if(context->changed_data.data){
                if(context->changed_data.len < changed_data_len){
                    qm_serial_free(context->changed_data.data);
                    context->changed_data.data = NULL;
                    context->changed_data.data = (uint8_t*)qm_serial_malloc(changed_data_len);
                    if(context->changed_data.data == NULL){
                        return SERIAL_ALLOC_MEM_ERR;
                    }
                }
            }else{
                context->changed_data.data = (uint8_t*)qm_serial_malloc(changed_data_len);
                if(context->changed_data.data == NULL){
                    return SERIAL_ALLOC_MEM_ERR;
                }
            }
            message_serialize(context->serial_snapshot_info.message_context, context->changed_data.data, &changed_data_len, SERIALIZE_TYPE_CHANGED);
            context->changed_data.len = changed_data_len;

            if(context->callbacks.m_reportcb){
                context->callbacks.m_reportcb(cmd, context->changed_data.data, context->changed_data.len);
            }
        }

    #else

        if(context->callbacks.m_reportcb){
            message_serialize(context->serial_snapshot_info.message_context, NULL, &changed_data_len, SERIALIZE_TYPE_CHANGED_ONLY);
            if(changed_data_len > 0){
                if(context->changed_data.data){
                    if(context->changed_data.len < changed_data_len){
                        qm_serial_free(context->changed_data.data);
                        context->changed_data.data = NULL;
                        context->changed_data.data = (uint8_t*)qm_serial_malloc(changed_data_len);
                        if(context->changed_data.data == NULL){
                            return SERIAL_ALLOC_MEM_ERR;
                        }
                    }
                }else{
                    context->changed_data.data = (uint8_t*)qm_serial_malloc(changed_data_len);
                    if(context->changed_data.data == NULL){
                        return SERIAL_ALLOC_MEM_ERR;
                    }
                }
                message_serialize(context->serial_snapshot_info.message_context, context->changed_data.data, &changed_data_len, SERIALIZE_TYPE_CHANGED_ONLY);
                context->changed_data.len = changed_data_len;
                context->callbacks.m_reportcb(cmd, context->changed_data.data, context->changed_data.len);
            }
        }
    #endif

        break;

        case CMD_GET:
        case CMD_SET:

        if(len < FRAME_MIN_LEN + 1){
            return SERIAL_PACKET_LEN_ERR;
        }

        local_id = *(data+FRAME_HEAD_LEN);
        type = data_type_get(local_id);
        if(type == TYPE_MCU){
            err = serial_local_id_find(local_id, &platform_id);
            if(err == SERIAL_NO_ERR){
                if(context->callbacks.m_responsecb){
                    context->callbacks.m_responsecb(cmd, platform_id, data + FRAME_HEAD_LEN + 1, len - FRAME_HEAD_LEN - 2);
                }
            }
        }
        else if(type > TYPE_MCU && type < TYPE_MAX){
    #ifdef SERIAL_RAW_SUPPORT
            if(context->callbacks.m_ramcb){
                context->callbacks.m_ramcb(type, data, len);
            }
    #endif
        }

        break;

        case CMD_GET_SNAPSHOT:
        if(len < FRAME_SNAPSHOT_MIN_LEN){
            return SERIAL_PACKET_LEN_ERR;
        }

        if(context->state != SERIAL_STATE_GETDEVICESNAPSHOT){
            return SERIAL_PROCESS_ERR;
        }

        if(context->serial_snapshot_info.message_context){
            message_delete(context->serial_snapshot_info.message_context);
            context->serial_snapshot_info.message_context = NULL;
        }
        context->serial_snapshot_info.message_context = message_parse(data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1, PARSE_TYPE_BASIC);
        if(context->serial_snapshot_info.message_context == NULL){
            SERIAL_LOG("snapshot err");
            return SERIAL_PARAM_ERR;
        }

        context->serial_snapshot_info.is_changed = 1;

        context->state = SERIAL_STATE_NORMAL;

        SERIAL_LOG("need report");

#ifdef SERIAL_REPORT_SUPPORT
        message_changed_flag_clear(context->serial_snapshot_info.message_context);

        if(context->callbacks.m_reportcb){
            context->callbacks.m_reportcb(CMD_GET_SNAPSHOT, data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1);
        }
        #else
        if(context->callbacks.m_reportcb){
            context->callbacks.m_reportcb(CMD_GET_SNAPSHOT, data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1);
        }
#endif

       break;

       case CMD_HEARTBEAT:

           if(len < FRAME_MIN_LEN + 1){
               return SERIAL_PACKET_LEN_ERR;
           }

           heartbeat_type = *(data+FRAME_HEAD_LEN);

           if(context->state == SERIAL_STATE_HEARTBEAT ){
               context->state = SERIAL_STATE_GETDEVICEINFO;
               serial_generate_send(CMD_NOTIFY_STATUS, 0, &context->module_status, 1);
               if(context->module_prodtst_status != MODULE_PRODTST_STATUS_NONE){
                    serial_generate_send(CMD_PRODTST_STATUS, 0, &context->module_prodtst_status, 1);
               }
               context->keep_alive_interval = SERIAL_KEPPALIVE_TIMEOUT_DEFAULT_MS;
               qm_utils_time_countdown_ms(&context->next_ping_time, context->keep_alive_interval);
               qm_utils_time_start(&context->process_get_time);

           }else{
                 if(heartbeat_type == HEARTBEAT_RESET){
                      serial_generate_send(CMD_NOTIFY_STATUS, 0, &context->module_status, 1);
                      context->state = SERIAL_STATE_GETDEVICEINFO;
                      qm_utils_time_start(&context->process_get_time);
                 }else if(heartbeat_type == HEARTBEAT_COMMON){

                 }
           }


#ifdef SERIAL_HEARTBEAT_NOTIFY_SUPPORT
    	   if(context->callbacks.m_heartbeatcb){
    			context->callbacks.m_heartbeatcb(heartbeat_type);
    	   }
#endif

       break;

#ifdef SERIAL_EVENT_SUPPORT

       case CMD_EVENT:
           if(len < FRAME_MIN_LEN + 1){
               return SERIAL_PACKET_LEN_ERR;
           }

           serial_generate_send(cmd, 0, NULL, 0);

           if(context->state != SERIAL_STATE_NORMAL){
               SERIAL_LOG("process error");
               return SERIAL_PROCESS_ERR;
           }
#ifdef SERIAL_EVENT_CACHE_SUPPORT
           event_handler(data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1);
#else
           if(context->callbacks.m_reportcb){
               context->callbacks.m_reportcb(CMD_EVENT, data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1);
           }
#endif

       break;  
    #endif
    #ifdef SERIAL_ACTION_SUPPORT
        case CMD_ACTION:
            if(len < FRAME_MIN_LEN + 1){
                return SERIAL_PACKET_LEN_ERR;
            }

            local_id = *(data + FRAME_HEAD_LEN);
            type = data_type_get(local_id);
            err = serial_local_id_find(local_id, &platform_id);
            if(err == SERIAL_NO_ERR){
                if(context->callbacks.m_responsecb){
                    context->callbacks.m_responsecb(cmd, platform_id, data + FRAME_HEAD_LEN + 1, len - FRAME_HEAD_LEN - 2);
                }
            }

        break;
    #endif
        case CMD_NOTIFY_STATUS:


       break;
       
       case CMD_MCU_NOTIFY_STATUS:
        if(len < FRAME_MIN_LEN + 1){
            return SERIAL_PACKET_LEN_ERR;
        }
        
        if(context->callbacks.m_mcu_status_notifycb){
            context->callbacks.m_mcu_status_notifycb(*(data + FRAME_HEAD_LEN));
        }
       
       break;

       case CMD_PRODTST_STATUS:

       break;

       case CMD_NETCONFIG:

        if(len < FRAME_MIN_LEN + 1){
            return SERIAL_PACKET_LEN_ERR;
        }
        config_type = *(data + FRAME_HEAD_LEN);
        if(config_type < NETWORK_CONFIG_START || config_type > NETWORK_CONFIG_END){
            return SERIAL_DATA_TYPE_ERR;
        }
        serial_generate_send(cmd, 0, NULL, 0);
        if(context->callbacks.m_network_configcb){
            context->callbacks.m_network_configcb(config_type);
        }

       break;

       case CMD_PRODTST_ENTERY:

        if(len < FRAME_MIN_LEN + 1){
            return SERIAL_PACKET_LEN_ERR;
        }
        config_type = *(data + FRAME_HEAD_LEN);
        if(config_type <= MODULE_PRODTST_TYPE_START || config_type >= MODULE_PRODTST_TYPE_END){
            return SERIAL_DATA_TYPE_ERR;
        }
        serial_generate_send(cmd, 0, NULL, 0);
        context->prodtst_type = config_type;

        if(context->callbacks.m_prodtst_configcb){
            context->callbacks.m_prodtst_configcb(config_type);
        }

       break;

       case CMD_GET_DEVICEINFO:
           if(len < FRAME_DEVICEINFO_GET_MIN_LEN)
               return SERIAL_PACKET_LEN_ERR;

           if(context->state != SERIAL_STATE_GETDEVICEINFO){
               return SERIAL_PROCESS_ERR;
           }

           ver.main_ver = *(data + FRAME_HEAD_LEN);
           ver.sub_ver = *(data + FRAME_HEAD_LEN + 1);
           ver.fix_ver = *(data + FRAME_HEAD_LEN + 2);

           if(len - FRAME_DEVICEINFO_GET_MIN_LEN ){
               err = deviceinfo_handler(context, data + FRAME_HEAD_LEN + FRAME_VER_LEN, len - FRAME_MIN_LEN - FRAME_VER_LEN);
               if(err != SERIAL_NO_ERR){
                   SERIAL_LOG("device info handler err");
                   return err;
               }
           }

           if(qm_serial_module_status_get() == MODULE_CONNECTED){
               context->state = SERIAL_STATE_GETDEVICESNAPSHOT;
           }else{
#ifndef SERIAL_LOWPOWER_SUPPORT
               context->state = SERIAL_STATE_NORMAL; 
#else
               context->state = SERIAL_STATE_GETDEVICESNAPSHOT;
#endif
           }

           if(context->callbacks.m_device_onlivecb){
               if(len - FRAME_DEVICEINFO_GET_MIN_LEN ){
                   context->callbacks.m_device_onlivecb(&ver, data + FRAME_HEAD_LEN + FRAME_VER_LEN, len - FRAME_MIN_LEN - FRAME_VER_LEN);
               }else{
                   context->callbacks.m_device_onlivecb(&ver, NULL, 0);
               }
           }

       break;

       case CMD_TIMEGET:

            if(len < FRAME_MIN_LEN + 1){
                return SERIAL_PACKET_LEN_ERR;
            }
            time_info.timezone = *(data + FRAME_HEAD_LEN);
            err = qm_serial_get_time(&time_info);
            if(err != 0){
                serial_generate_send_with_code(cmd, 0, SERIAL_COM_ERROR, (uint8_t*)&time_info, sizeof(time_info));
            }else{
                serial_generate_send_with_code(cmd, 0, SERIAL_COM_OK, (uint8_t*)&time_info, sizeof(time_info));
            }

       break;

       case CMD_GETMOD:
       case CMD_SETMOD:
        if(len < FRAME_MIN_LEN + 1){
            return SERIAL_PACKET_LEN_ERR;
        }

        if(context->callbacks.m_extinfocb){
            context->callbacks.m_extinfocb(cmd, data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1);
        }

       break;

       case CMD_SERVICE:


        err = serial_service_handler(data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1);
        if(err != 0){
            serial_generate_send_with_code(cmd, 0, SERIAL_COM_ERROR, NULL, 0);
        }else{
            serial_generate_send_with_code(cmd, 0, SERIAL_COM_OK, NULL, 0);
        }

       break;

       default:
#ifdef SERIAL_OTA_SUPPORT
            if(len < FRAME_MIN_LEN + 1){
                return SERIAL_PACKET_LEN_ERR;
            }
            qm_serial_ota_parse(cmd, data + FRAME_HEAD_LEN, len - FRAME_HEAD_LEN - 1);
#endif
           break;
    }

    return SERIAL_NO_ERR;
}


static void frame_head_init(uint8_t *buffer, frame_cmd_t cmd, uint16_t len)
{
    frame_header_t *head = (frame_header_t *)buffer;

    head->head = cpu_to_be16(FRAME_HEAD);
    head->ver  = PROTOCOL_VER;
    head->cmd  = cmd;
    head->len = cpu_to_be16(len);
}

static int32_t frame_creat(frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t data_len, uint8_t *buf, uint16_t *buf_len)
{
    frame_unit_t *frame_unit = NULL;
    frame_generic_t *frame_generic = NULL;

    switch (cmd){

       case CMD_REPORT:
       case CMD_NETCONFIG:
       case CMD_GET_DEVICEINFO:
       case CMD_GET_SNAPSHOT:
       case CMD_EVENT:
#ifdef SERIAL_OTA_SUPPORT
       case CMD_OTA_QUERYCRC:
#endif
       case CMD_PRODTST_ENTERY:

           if(buf && *buf_len >= sizeof(frame_unit_t)){
               frame_unit = (frame_unit_t*)buf;
               frame_head_init(buf, cmd, 0);
               frame_unit->chksum = frame_chksum(buf, sizeof(frame_unit_t) - 1);
           }
           *buf_len = sizeof(frame_unit_t);

       break;

       case CMD_HEARTBEAT:
       case CMD_NOTIFY_STATUS:
#ifdef SERIAL_OTA_SUPPORT
       case CMD_OTA_START:
#endif
       case CMD_PRODTST_STATUS:

           if(buf && *buf_len >= sizeof(frame_generic_t)){
               frame_generic = (frame_generic_t*)buf;
               frame_head_init(buf, cmd, 1);
               frame_generic->data = *data;
               frame_generic->chksum = frame_chksum(buf, sizeof(frame_generic_t) - 1);
           }
           *buf_len = sizeof(frame_generic_t);

       break;

       case CMD_GET:
       case CMD_SET:
       case CMD_ACTION:
           if(buf && *buf_len >= FRAME_MIN_LEN + data_len + 1){
               frame_head_init(buf, cmd, data_len + 1);
               *(buf + FRAME_HEAD_LEN) = packet_id;
               memcpy(buf + FRAME_HEAD_LEN + 1, data, data_len);
               *(buf + FRAME_HEAD_LEN + data_len + 1) = frame_chksum(buf, FRAME_HEAD_LEN + data_len + 1);
           }
           *buf_len = FRAME_MIN_LEN + data_len + 1;

       break;

       case CMD_GETMOD:
       case CMD_SETMOD:
#ifdef SERIAL_OTA_SUPPORT
       case CMD_OTA_TRANSFER:
       case CMD_OTA_VERIFY:
#endif

        if(buf && *buf_len >= FRAME_MIN_LEN + data_len){
           frame_head_init(buf, cmd, data_len);
           memcpy(buf + FRAME_HEAD_LEN, data, data_len);
           *(buf + FRAME_HEAD_LEN + data_len) = frame_chksum(buf, FRAME_HEAD_LEN + data_len);
        }
        *buf_len = FRAME_MIN_LEN+data_len;
       break;

       case CMD_TIMEGET:
       case CMD_SERVICE:
        if(buf && *buf_len >= FRAME_MIN_LEN + data_len + 1){
           frame_head_init(buf, cmd, data_len + 1);
           memcpy(buf + FRAME_HEAD_LEN, &code, 1);
            if(data && data_len > 0){
                memcpy(buf + FRAME_HEAD_LEN + 1, data, data_len);
            }
           *(buf + FRAME_HEAD_LEN + data_len + 1) = frame_chksum(buf, FRAME_HEAD_LEN + data_len + 1);
        }
        *buf_len = FRAME_MIN_LEN + data_len + 1;
        
       break;

       default:
            return SERIAL_CMD_ERR;
       break;
    }

    return SERIAL_NO_ERR;

}

static int32_t serial_frame_info_creat(frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t len, uint8_t resend, uint8_t **raw)
{
    int32_t ret = 0;
    uint8_t *buf = NULL;
    uint16_t packet_len = 0;
    uint16_t raw_len = 0;
    frame_info_t *info = NULL;
    ret = frame_creat(cmd, packet_id, code, data, len, NULL, &packet_len);
    if(ret != SERIAL_NO_ERR){
        return ret;
    }

    raw_len = sizeof(frame_info_t) + packet_len -1;
    buf = qm_serial_malloc(raw_len);
    if(buf == NULL){
        return SERIAL_ALLOC_MEM_ERR;
    } 

    memset(buf, 0, raw_len);

    info = (frame_info_t*)buf;
    info->len = packet_len;
    info->resend = resend;

    ret = frame_creat(cmd, packet_id, code, data, len, info->data, &packet_len);
    if(ret != SERIAL_NO_ERR){
        return ret;
    } 

    *raw = buf;

    return SERIAL_NO_ERR;
}


static uint32_t queue_len_get(qm_list_t *queue)
{
    uint32_t queue_len = 0;
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    queue_len = queue->len;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif
    return queue_len;
}

static int32_t queue_push(qm_list_t *queue, uint8_t *raw)
{
    qm_list_node_t *node = NULL;
    serial_context_t *context = NULL;
    context = qm_serial_context_get();

    node = (qm_list_node_t*)raw;
    qm_list_node_init(node, raw+sizeof(qm_list_node_t));

   #ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    qm_list_rpush(queue, node);
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_sem_signal(context->send_sem);
#endif
    return SERIAL_NO_ERR; 
}

/*
 *   -------------------------------
 *  |  len  | is_resend | PAYLOAD |
 *   ------------------------------
 *    2Bytes    1byte     nByte
 */

#ifdef SERIAL_RESEND_SUPPORT

static int32_t serial_generate_resend(frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len, uint8_t **buf)
{
    uint32_t ret = 0;
    uint8_t *raw = NULL;
    qm_list_t *queue = NULL;
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    queue = context->down_queue;

    if(queue_len_get(queue) >= SERIAL_DOWN_QUEUE_NUM_MAX){
        return SERIAL_SEND_BUSY_ERR;
    }

    ret = serial_frame_info_creat(cmd, packet_id, 0, data, len, 1, &raw);
    if(ret != SERIAL_NO_ERR){
        return ret;
    }

    *buf = raw;

    return queue_push(queue, raw);
}

#endif

static int32_t serial_generate_send(frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len)
{
    uint32_t ret = 0;
    uint8_t *raw = NULL;
    qm_list_t *queue = NULL;
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    queue = context->send_queue;

    if(queue_len_get(queue) >= SERIAL_SEND_QUEUE_NUM_MAX){
        return SERIAL_SEND_BUSY_ERR;
    }

    ret = serial_frame_info_creat(cmd, packet_id, 0, data, len, 0, &raw);
    if(ret != SERIAL_NO_ERR){
        return ret;
    }

    return queue_push(queue, raw);
}

static int32_t serial_generate_send_with_code(frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t len)
{
    uint32_t ret = 0;
    uint8_t *raw = NULL;
    qm_list_t *queue = NULL;
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    queue = context->send_queue;

    if(queue_len_get(queue) >= SERIAL_SEND_QUEUE_NUM_MAX){
        return SERIAL_SEND_BUSY_ERR;
    }

    ret = serial_frame_info_creat(cmd, packet_id, code, data, len, 0, &raw);
    if(ret != SERIAL_NO_ERR){
        return ret;
    }

    return queue_push(queue, raw);
}

static uint8_t local_id_get(uint8_t *raw)
{
    return *(raw + FRAME_HEAD_LEN);
}

static int32_t serial_id_info_push(serial_data_type_t type, platform_id_t platform_id, uint8_t local_id, uint8_t *data)
{
    serial_context_t *context = NULL;
    serial_id_info_t *id_info = NULL;
    uint8_t *buf = NULL;
    uint16_t buf_len = 0;
    context = qm_serial_context_get();

    qm_list_node_t *node = NULL;

    buf_len = sizeof(serial_id_info_t)+sizeof(qm_list_node_t);
    buf = qm_serial_malloc(buf_len);
    if(buf == NULL){
        return SERIAL_ALLOC_MEM_ERR;
    }
    node = (qm_list_node_t*)buf;

    memset(buf, 0, buf_len);

    qm_list_node_init(node, buf + sizeof(qm_list_node_t));

    id_info = (serial_id_info_t*)(buf + sizeof(qm_list_node_t));
    id_info->platform_id = platform_id;
    id_info->local_id = local_id;
#ifdef SERIAL_RESEND_SUPPORT
    id_info->retry_count = SERIAL_SEND_RETRY_COUNT_MAX;
#else
    id_info->retry_count = 0;
#endif
    id_info->data = data;
    id_info->time = 0; 
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    qm_list_rpush(context->id_queue, node);
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif
    return SERIAL_NO_ERR;
}

static uint8_t is_serial_id_update(void)
{
    serial_context_t *context = NULL;
    qm_list_iterator_t self;
    qm_list_node_t *node = NULL;
    qm_list_t *list = NULL;
    serial_id_info_t *id_info = 0;
    uint8_t *buf = NULL;
    uint8_t is_update = 0;

    context = qm_serial_context_get();
    list = context->id_queue;

    self.direction = LIST_HEAD;
    self.next = list->head;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif

    node = qm_list_iterator_next(&self);
    while (node) {
        buf = (uint8_t*)node;
        id_info = (serial_id_info_t*)(buf+sizeof(qm_list_node_t));
        if(id_info->is_update){
            is_update = 1;
        }
        node = qm_list_iterator_next(&self);
    }
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif

    return is_update;
}

static int32_t serial_id_uptime(uint8_t local_id)
{
    serial_context_t *context = NULL;
    serial_id_info_t id_info = {0};
    qm_list_node_t *node = NULL;
    serial_id_info_t *p_id_info = NULL;

    context = qm_serial_context_get();

    id_info.local_id = local_id;

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_find(context->id_queue, &id_info);
    if(node){
        p_id_info = (serial_id_info_t*)node->val;
        p_id_info->is_update = 1;
        p_id_info->time = qm_serial_get_timems() + SERIAL_ACK_TIMEOUT_DEFAULT_MS;
    }

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif

    return SERIAL_NO_ERR;
}

static int id_match(void *a, void *b)
{
    serial_id_info_t *a_info = (serial_id_info_t*)a;
    serial_id_info_t *b_info = (serial_id_info_t*)b;

    if(a_info->local_id == b_info->local_id){
        return 1;
    }else{
        return 0;
    }
}

static int32_t serial_local_id_find(uint8_t local_id, platform_id_t *platform_id)
{
    qm_list_node_t *node = NULL;
    serial_context_t *context = NULL;
    serial_id_info_t id_info = {0};
    serial_id_info_t *id = NULL;
    context = qm_serial_context_get();

    id_info.local_id = local_id;

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_find(context->id_queue, &id_info);

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif
    if(node){
        id = (serial_id_info_t*)node->val;
        *platform_id = id->platform_id;
#ifdef SERIAL_RTOS_SUPPORT
        qm_serial_mutex_lock(context->serial_mutex);
#endif
        qm_list_remove(context->id_queue, node);
#ifdef SERIAL_RTOS_SUPPORT
        qm_serial_mutex_unlock(context->serial_mutex);
#endif
        qm_serial_free(node);
    }else{
        return SERIAL_MSG_NUM_ERR;
    }
    return SERIAL_NO_ERR;
}

static int32_t serial_cloud_generate_send(frame_cmd_t cmd, platform_id_t id, uint8_t *data, uint16_t len)
{
    int32_t err = SERIAL_NO_ERR;
    uint8_t local_id = 0;
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    uint8_t *buf = NULL;
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

    local_id = packet_id_get();

#ifdef SERIAL_RESEND_SUPPORT
    err = serial_generate_resend(cmd, local_id, data, len, &buf);
    if(err != SERIAL_NO_ERR){
        return err;
    }

    err = serial_id_info_push(TYPE_MCU, id, local_id, buf);
    if(err != SERIAL_NO_ERR){
        return err;
    }
    
#else
    err = serial_generate_send(cmd, local_id, data, len);
    if(err != SERIAL_NO_ERR){
        return err;
    }

    err = serial_id_info_push(TYPE_MCU, id, local_id, NULL);
    if(err != SERIAL_NO_ERR){
        return err;
    }
#endif

    return SERIAL_NO_ERR;
}

#ifdef SERIAL_RESEND_SUPPORT

static int32_t down_queue_send(void)
{
    int32_t err = SERIAL_NO_ERR;
    qm_list_node_t *node = NULL;
    qm_list_t *id_queue = NULL;
    qm_list_t *down_queue = NULL;
    qm_list_t *send_queue = NULL;
    serial_context_t *context = NULL;
    frame_info_t *info = NULL;
    context = qm_serial_context_get();
    uint8_t *buf = NULL;
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

    id_queue = context->id_queue;
    down_queue = context->down_queue;
    send_queue = context->send_queue;

    if(is_serial_id_update()){
        return SERIAL_NO_ERR;
    }

    if(queue_len_get(down_queue) == 0){
        return SERIAL_NO_ERR;
    }

    if(queue_len_get(send_queue) >= SERIAL_SEND_QUEUE_NUM_MAX){
        SERIAL_LOG("send queue is full");
        return SERIAL_SEND_BUSY_ERR;
    }

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_lpop(down_queue);

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif

    queue_push(send_queue, (uint8_t*)node);

    qm_serial_sem_signal(context->send_sem);

    return SERIAL_NO_ERR;
}

#endif

static int32_t id_check_cycle(serial_context_t *pcontext)
{
    qm_list_t *list = pcontext->id_queue;
    qm_list_iterator_t self;
    qm_list_node_t *node = NULL;
    uint8_t *buf = NULL;
    serial_id_info_t *id_info = NULL;
    uint32_t cur_time = 0;
    uint32_t node_index = 0;
    int32_t send = 0;

    if (!qm_utils_time_is_expired(&pcontext->id_check_time)) {
        return SERIAL_NO_ERR;
    }
    qm_utils_time_countdown_ms(&pcontext->id_check_time, pcontext->id_check_interval);

    if(queue_len_get(pcontext->id_queue) == 0){
#ifdef SERIAL_RESEND_SUPPORT
        down_queue_send();
#endif
        return SERIAL_NO_ERR;
    }

    cur_time = qm_serial_get_timems();

    self.direction = LIST_HEAD;
    self.next = list->head;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(pcontext->serial_mutex);
#endif

    node = qm_list_iterator_next(&self);
    while (node) {
        buf = (uint8_t*)node;
        id_info = (serial_id_info_t*)(buf+sizeof(qm_list_node_t));
        if(id_info->is_update && (cur_time - id_info->time) < (UINT32_MAX / 2) ){
            id_info->is_update = 0;
            if(id_info->retry_count == 0){
                qm_list_remove(list, node);
                if(id_info->data){
                    qm_serial_free(id_info->data);
                }
                qm_serial_free(node);
                send = 1;
            }else{
                qm_list_rpush(pcontext->send_queue, (qm_list_node_t*)id_info->data);
#ifdef SERIAL_RTOS_SUPPORT

                qm_serial_sem_signal(pcontext->send_sem);
#endif
            }
            id_info->retry_count--;
        }
        node = qm_list_iterator_next(&self);
    }

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(pcontext->serial_mutex);
#endif

#ifdef SERIAL_RESEND_SUPPORT
    if(send){
        down_queue_send();
    }
#endif

    return SERIAL_NO_ERR;
}

static int32_t serial_heartbeat_send(void)
{
    uint8_t status=0;
    status = qm_serial_module_status_get();
    return serial_generate_send(CMD_HEARTBEAT, 0, &status, 1);
}

static int32_t serial_deviceinfo_get_send(void)
{
    return serial_generate_send(CMD_GET_DEVICEINFO, 0, NULL, 0);
}

static int32_t serial_snapshot_get_send(void)
{
    return serial_generate_send(CMD_GET_SNAPSHOT, 0, NULL, 0);
}

int32_t qm_serial_state_set(serial_state_t state)
{
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    context->state = state;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif
    return SERIAL_NO_ERR;
}

int32_t qm_serial_state_get(serial_state_t *state)
{
    serial_context_t *context = NULL;
	if(state == NULL){
		return SERIAL_PARAM_ERR;
	}
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    *state = context->state;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif
    return SERIAL_NO_ERR;
}

module_status_t qm_serial_module_status_get(void)
{
    module_status_t status;
    serial_context_t *context = NULL;
    context=qm_serial_context_get();
    if(context==NULL){
        return SERIAL_NO_INIT_ERR;
    }
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    status = context->module_status;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif
    return status;
}

int32_t qm_serial_module_status_set(module_status_t status)
{
    int32_t need_notify = 0;
    serial_context_t *context = NULL;
    serial_state_t state = SERIAL_STATE_NONE;
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

    if(context->module_status != status){
#ifdef SERIAL_RTOS_SUPPORT
        qm_serial_mutex_lock(context->serial_mutex);
#endif
        context->module_status = status;
        state = context->state;
#ifdef SERIAL_RTOS_SUPPORT
        qm_serial_mutex_unlock(context->serial_mutex);
#endif
        need_notify = 1;
    }

    if(state != SERIAL_STATE_HEARTBEAT && need_notify){
        serial_generate_send(CMD_NOTIFY_STATUS, 0, &context->module_status, 1);
    }

#ifndef SERIAL_LOWPOWER_SUPPORT
    if(status == MODULE_CONNECTED && need_notify){
        if(context->state == SERIAL_STATE_NORMAL){
            context->state = SERIAL_STATE_GETDEVICESNAPSHOT;
        }
    }
#endif
    return SERIAL_NO_ERR;
}

int32_t qm_serial_module_prodtst_status_set(module_prodtst_status_t status)
{
    int32_t need_notify = 0;
    serial_context_t *context = NULL;
    serial_state_t state = SERIAL_STATE_NONE;
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

    if(context->module_prodtst_status == MODULE_PRODTST_TYPE_NONE){
        return SERIAL_NO_ERR;
    }

    if(context->module_prodtst_status != status){
#ifdef SERIAL_RTOS_SUPPORT
        qm_serial_mutex_lock(context->serial_mutex);
#endif
        context->module_prodtst_status = status;
        state = context->state;
#ifdef SERIAL_RTOS_SUPPORT
        qm_serial_mutex_unlock(context->serial_mutex);
#endif
        need_notify = 1;
    }

    if(state != SERIAL_STATE_HEARTBEAT && need_notify){
        serial_generate_send(CMD_PRODTST_STATUS, 0, &context->module_prodtst_status, 1);
    }

    return SERIAL_NO_ERR;
}

int32_t qm_serial_cloud_set(platform_id_t id, uint8_t *data, uint16_t len)
{
#ifdef SERIAL_OTA_SUPPORT
	serial_state_t state;
	qm_serial_state_get(&state);
	if(state == SERIAL_STATE_OTA){
		return SERIAL_PROCESS_ERR;
	}
#endif
    return serial_cloud_generate_send(CMD_SET, id, data, len);
}

int32_t qm_serial_cloud_action(platform_id_t id, uint8_t *data, uint16_t len)
{
#ifdef SERIAL_OTA_SUPPORT
	serial_state_t state;
	qm_serial_state_get(&state);
	if(state == SERIAL_STATE_OTA){
		return SERIAL_PROCESS_ERR;
	}
#endif
	return serial_cloud_generate_send(CMD_ACTION, id, data, len);
}

int32_t qm_serial_cloud_get(platform_id_t id, uint8_t *data, uint16_t len)
{
#ifdef SERIAL_OTA_SUPPORT
	serial_state_t state;
	qm_serial_state_get(&state);
	if(state == SERIAL_STATE_OTA){
		return SERIAL_PROCESS_ERR;
	}
#endif
    return serial_cloud_generate_send(CMD_GET, id, data, len);
}

int32_t qm_serial_ext_cmd_ack(frame_cmd_t cmd, uint8_t err_code, uint8_t *data, uint16_t len)
{
    uint8_t *buf = NULL;
    int32_t err = SERIAL_NO_ERR;
    uint8_t code = err_code;
    if(data != NULL && len > 0){
        buf = qm_serial_malloc(len + 1);
        if(buf == NULL){
            return SERIAL_ALLOC_MEM_ERR;
        }
        memcpy(buf, &code, 1);
        memcpy(buf + 1, data, len);
        err = serial_generate_send(cmd, 0, buf, len + 1);
        qm_serial_free(buf);
    }else{
        err=serial_generate_send(cmd, 0, &code, 1);
    }

    return err;
}
int32_t qm_serial_init(serial_param_t *serial_param)
{
    serial_context_t *serial_context = NULL;
    if(serial_param == NULL || serial_param->pread_buf == NULL){
        return SERIAL_PARAM_ERR;
    }

    serial_context=(serial_context_t*)qm_serial_malloc(sizeof(serial_context_t));
    if(NULL == serial_context)
    {
       SERIAL_LOG("not enough memory.");
       return SERIAL_ALLOC_MEM_ERR;
    }

    memset(serial_context, 0, sizeof(serial_context_t));

#ifdef SERIAL_RTOS_SUPPORT
    serial_context->serial_mutex = qm_serial_mutex_init();
    serial_context->send_sem = qm_serial_sem_init(0);
#endif

    serial_context->send_queue = qm_list_new();
    serial_context->id_queue = qm_list_new();

#ifdef SERIAL_RESEND_SUPPORT
    serial_context->down_queue = qm_list_new();
#endif

#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
    serial_context->event_info.serial_event_queue = qm_list_new();
#endif
#endif
    serial_context->id_queue->match = id_match;

    qm_utils_time_init(&serial_context->next_ping_time);
    serial_context->keep_alive_interval = SERIAL_KEPPALIVE_TIMEOUT_FAST_MS;
    qm_utils_time_countdown_ms(&serial_context->next_ping_time, serial_context->keep_alive_interval);

    qm_utils_time_init(&serial_context->process_get_time);
    serial_context->process_get_interval = SERIAL_PROCESS_TIMEOUT_MS;
    qm_utils_time_countdown_ms(&serial_context->process_get_time, serial_context->process_get_interval);

    qm_utils_time_init(&serial_context->send_interval_time);
    serial_context->send_interval = SERIAL_SEND_INTERVAL_TIMEOUT_DEFAULT_MS;
    qm_utils_time_countdown_ms(&serial_context->send_interval_time, serial_context->send_interval);

    qm_utils_time_init(&serial_context->id_check_time);
    serial_context->id_check_interval = SERIAL_ID_CHECK_TIMEOUT_MS;
    qm_utils_time_countdown_ms(&serial_context->id_check_time, serial_context->id_check_interval);

    serial_context->callbacks.m_network_configcb = serial_param->callbacks.m_network_configcb;
    serial_context->callbacks.m_reportcb = serial_param->callbacks.m_reportcb;
    serial_context->callbacks.m_responsecb = serial_param->callbacks.m_responsecb;
    serial_context->callbacks.m_device_onlivecb = serial_param->callbacks.m_device_onlivecb;
    serial_context->callbacks.m_extinfocb = serial_param->callbacks.m_extinfocb;
    serial_context->callbacks.m_prodtst_configcb = serial_param->callbacks.m_prodtst_configcb;
    serial_context->callbacks.m_mcu_status_notifycb = serial_param->callbacks.m_mcu_status_notifycb;
#ifdef SERIAL_OTA_SUPPORT
    serial_context->callbacks.ota_event_notifycb = serial_param->callbacks.ota_event_notifycb;
#endif
#ifdef SERIAL_HEARTBEAT_NOTIFY_SUPPORT
    serial_context->callbacks.m_heartbeatcb = serial_param->callbacks.m_heartbeatcb;
#endif

#ifdef SERIAL_RAW_SUPPORT
    serial_context->callbacks.m_ramcb = serial_context->callbacks.m_ramcb;
#endif
    serial_context->pread_buf = serial_param->pread_buf;
    serial_context->read_buf_size = serial_param->read_buf_size;

    serial_context->parse_state = PARSE_STATE_NONE;
    serial_context->need_len = 1;
    serial_context->state = SERIAL_STATE_HEARTBEAT;
    serial_context->module_status = MODULE_UNCONFIGURD_NETWORK;
    serial_context->module_prodtst_status = MODULE_PRODTST_STATUS_NONE;
    serial_context->prodtst_type = MODULE_PRODTST_TYPE_NONE;
    
    serial_context->handle = qm_serial_uart_init();

    serial_context->service_ctx = message_creat(TYPE_MESSAGE_BASIC);

    g_serial_context = serial_context;
    
    return SERIAL_NO_ERR;
}

static uint16_t get_packet_len(uint8_t*packet_buffer)
{
    if(packet_buffer == NULL){
        return 0;
    }

    return  (be16_to_cpu((packet_buffer[5] << 8) + (packet_buffer[4])));
}

static int serial_recv_cycle(serial_context_t *pcontext, qm_utils_time_t *timer)
{
    int rc = SERIAL_NO_ERR;
    int recv_len = 0;
    uint16_t packet_len = 0;
    uint32_t left_t = qm_utils_time_left(timer);
    uint16_t need_len = pcontext->need_len;

    recv_len = qm_serial_recv(pcontext->handle, pcontext->pread_buf+pcontext->read_pos, need_len, left_t);
    pcontext->read_pos += recv_len;

    switch(pcontext->parse_state){

        case PARSE_STATE_NONE:

            if(pcontext->pread_buf[pcontext->read_pos - 1] != FRAME_HEAD_PART){
                goto clear;
            }else{
                pcontext->parse_state = PARSE_STATE_HEAD;
                pcontext->need_len = 1;
            }

        break;

        case PARSE_STATE_HEAD:
            if(pcontext->need_len <= recv_len){
                if(pcontext->pread_buf[pcontext->read_pos - 1] != FRAME_HEAD_PART){
                    goto clear;
                }else{
                    pcontext->parse_state = PARSE_STATE_VER;
                    pcontext->need_len = 1;
                }
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;
        case PARSE_STATE_VER:
            if(pcontext->need_len <= recv_len){
                if(pcontext->pread_buf[pcontext->read_pos - 1] != PROTOCOL_VER){
                    goto clear;
                }else{
                    pcontext->parse_state = PARSE_STATE_LEN;
                    pcontext->need_len = 3;
                }
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;
        case PARSE_STATE_LEN:

            if(pcontext->need_len <= recv_len){
                packet_len = get_packet_len(pcontext->pread_buf);
                if(packet_len > (pcontext->read_buf_size - pcontext->read_pos)){
                    SERIAL_LOG("serial recv buffer over flow");
                    goto clear;
                }else{
                    pcontext->parse_state = PARSE_STATE_DATA;
                    pcontext->need_len = packet_len + 1;
                }
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;

        case PARSE_STATE_DATA:

            if(pcontext->need_len <= recv_len){
                SERIAL_LOG("serial recv len %d", pcontext->read_pos);
                SERIAL_HEX_LOG("serial recv data", pcontext->pread_buf, pcontext->read_pos);
                rc = frame_parse(pcontext->pread_buf, pcontext->read_pos);
                goto clear;
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;

        default:
            break;
    }

    return rc;

clear:
    pcontext->parse_state = PARSE_STATE_NONE;
    pcontext->read_pos = 0;
    pcontext->need_len = 1;
    return rc;
}

int32_t qm_serial_send_loop(void)
{
    frame_info_t *info = NULL;
    uint32_t left = 0;
    uint32_t send_queue_len = 0;
    qm_list_node_t *node = NULL;
    serial_context_t *context = NULL;
    uint8_t local_id = 0;
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_sem_wait(context->send_sem, 0);
#endif

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    send_queue_len = context->send_queue->len;
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif

    if(send_queue_len == 0)
        goto exit;

    if (!qm_utils_time_is_expired(&context->send_interval_time)) {
         goto exit;
    }
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_lpop(context->send_queue);

    send_queue_len = context->send_queue->len;

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif
    if(node){
        info = (frame_info_t*)node;
        qm_serial_send(context->handle, info->data, info->len);
        SERIAL_HEX_LOG("serial send", info->data, info->len);
        if(info->resend){
            local_id = local_id_get(info->data);
            serial_id_uptime(local_id);
        }else{
            qm_serial_free(info);
        }
    }

    qm_utils_time_countdown_ms(&context->send_interval_time, context->send_interval);

exit:

#ifdef SERIAL_RESEND_SUPPORT
    down_queue_send();
#endif

#ifdef SERIAL_RTOS_SUPPORT

    if(send_queue_len > 0){
        qm_serial_sem_signal(context->send_sem);
        left = qm_utils_time_left(&context->send_interval_time);
        if (left > 50){
            qm_serial_msleep(left);
        }
        else{
            qm_serial_msleep(50);
        }
    }

#endif

    return SERIAL_NO_ERR;
}

static int32_t serial_keepalive(serial_context_t *pcontext)
{
    #ifdef SERIAL_LOWPOWER_SUPPORT
    if(pcontext->state == SERIAL_STATE_NORMAL){
        return SERIAL_NO_ERR;
    }
    #endif

    /* if there is no ping_timer timeout, then return success */
    if (!qm_utils_time_is_expired(&pcontext->next_ping_time)) {
        return SERIAL_NO_ERR;
    }

    /* update to next time sending usart keep-alive */
    qm_utils_time_countdown_ms(&pcontext->next_ping_time, pcontext->keep_alive_interval);
    SERIAL_LOG("send uart ping...");

    /*here send usart heart packet*/
    serial_heartbeat_send();

    return SERIAL_NO_ERR;
}

static int32_t serial_process_cycle(serial_context_t *pcontext)
{
    if(pcontext->state != SERIAL_STATE_GETDEVICEINFO &&
        pcontext->state != SERIAL_STATE_GETDEVICESNAPSHOT){
        return SERIAL_NO_ERR;
    }

    if (!qm_utils_time_is_expired(&pcontext->process_get_time)) {
        return SERIAL_NO_ERR;
    }
    qm_utils_time_countdown_ms(&pcontext->process_get_time, pcontext->process_get_interval);
    if(pcontext->state == SERIAL_STATE_GETDEVICEINFO){
        serial_deviceinfo_get_send();
    }else if(pcontext->state == SERIAL_STATE_GETDEVICESNAPSHOT){
        serial_snapshot_get_send();
    }

    return SERIAL_NO_ERR;
}

int32_t qm_serial_recv_loop(int32_t timeout_ms)
{
    int32_t rc = SERIAL_NO_ERR;
    qm_utils_time_t yield_time = {0};
    uint32_t left_t = 0;
    serial_context_t *context = NULL;
    qm_utils_time_init(&yield_time);
    qm_utils_time_countdown_ms(&yield_time, timeout_ms);

    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

#ifdef SERIAL_RTOS_SUPPORT
    do {
        if (SERIAL_NO_ERR != rc) {
            left_t= qm_utils_time_left(&yield_time);
            //serial_printf("error occur ");
            if (left_t > 50){
                qm_serial_msleep(left_t);
            }
            else{
                qm_serial_msleep(50);
            }
        }
#endif
        /* Keep usart alive */
        serial_keepalive(context);

        serial_process_cycle(context);

        id_check_cycle(context);

        /* acquire package in cycle */
        rc = serial_recv_cycle(context, &yield_time);
        if (SERIAL_NO_ERR == rc) {
            /* check list of wait serial ACK to remove node that is ACKED or timeout */

        }
#ifdef SERIAL_OTA_SUPPORT
    qm_serial_ota_process();
#endif

#ifdef SERIAL_RTOS_SUPPORT
    } while (!qm_utils_time_is_expired(&yield_time));
#endif

    return SERIAL_NO_ERR;
}

#ifdef SERIAL_RAW_SUPPORT
int32_t qm_serial_raw_send(uint8_t *data, uint16_t len)
{
    uint16_t data_len = len;
    uint16_t sum_len = 0;
    uint8_t *buf = NULL;
    qm_list_node_t *node = NULL;
    serial_context_t *context = NULL;
    if(data == NULL || len == 0){
        return SERIAL_PARAM_ERR;
    }
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }
    sum_len = data_len + sizeof(qm_list_node_t) + 2;
    buf = qm_serial_malloc(sum_len);
    if(buf == NULL){
        return SERIAL_ALLOC_MEM_ERR;
    }
    node = (qm_list_node_t*)buf;
    qm_list_node_init(node, buf + sizeof(qm_list_node_t));
    memcpy(buf + sizeof(qm_list_node_t), &data_len, 2);
    memcpy(buf + sizeof(qm_list_node_t) + 2, data, len);

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_lock(context->serial_mutex);
#endif
    qm_list_rpush(context->send_queue, node);
#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_mutex_unlock(context->serial_mutex);
#endif

#ifdef SERIAL_RTOS_SUPPORT
    qm_serial_sem_signal(context->send_sem);
#endif

    return SERIAL_NO_ERR;
}
#endif


#ifdef SERIAL_LOWPOWER_SUPPORT
int32_t qm_serial_process_reset(void)
{
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

    context->state = SERIAL_STATE_HEARTBEAT;
    context->keep_alive_interval = SERIAL_KEPPALIVE_TIMEOUT_FAST_MS;
    qm_utils_time_init(&context->next_ping_time);
    return SERIAL_NO_ERR;
}
#endif

serial_state_t serial_state_get(void)
{
    serial_context_t *context = NULL;
    context = qm_serial_context_get();
    if(context == NULL){
    	return SERIAL_STATE_NONE;
    }
    
    return context->state;   
}


static int32_t serial_service_register(module_service_type_t service_type, serial_service_fn fn, int32_t is_inner)
{
    serial_context_t *context = NULL;
    message_info_t *info = NULL;
    serial_service_callbacks_t *service_callbacks = NULL;

    context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    } 

    info = message_object_get(context->service_ctx, (uint8_t)service_type);
    if(info){
        service_callbacks = (serial_service_callbacks_t*)info->arg;
    }else{
        info = message_info_new();
        if(info == NULL){
            return SERIAL_ALLOC_MEM_ERR;
        }
        service_callbacks = (serial_service_callbacks_t*)qm_serial_malloc(sizeof(serial_service_callbacks_t));
        if(service_callbacks == NULL){
            message_info_delete(info);
            return SERIAL_ALLOC_MEM_ERR;
        }
        memset(service_callbacks, 0, sizeof(serial_service_callbacks_t));
        info->arg = (void*)service_callbacks;
        info->attr_id = (uint8_t)service_type;

        message_object_add(context->service_ctx, info);
    }

    if(is_inner){
        service_callbacks->inner_fn = fn;
    }else{
        service_callbacks->extern_fn = fn;
    }
    
    return SERIAL_NO_ERR;
}

int32_t qm_serial_service_register(module_service_type_t service_type, serial_service_fn fn)
{
    return serial_service_register(service_type, fn, 0);
}