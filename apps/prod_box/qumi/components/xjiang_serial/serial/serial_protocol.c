#include "serial_protocol.h"
#include "qm_utils_timer.h"
#include "qm_utils_list.h"
#include "serial_platform.h"

static serial_context_t *serial_context_get(void);
static uint8_t frame_chksum(uint8_t *buffer, uint16_t length);
static void frame_head_init(uint8_t *buffer, frame_cmd_t cmd, uint16_t len);
static int32_t frame_creat(frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t data_len, uint8_t *buf, uint16_t *buf_len);
static uint8_t packet_id_get(void);
static int32_t serial_heartbeat_send(void);
static int32_t serial_generate_send(frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len, uint8_t is_resend, uint8_t **send_buf);
static uint16_t get_packet_len(uint8_t*packet_buffer);
static int32_t serial_keepalive(serial_context_t *pcontext);
static int32_t serial_id_info_push(serial_data_type_t type, platform_id_t platform_id, uint8_t local_id, uint8_t *data);
static int32_t serial_id_uptime(uint8_t local_id);
static int32_t serial_cloud_generate_send(frame_cmd_t cmd, platform_id_t id, uint8_t *data, uint16_t len);
static int id_match(void *a, void *b);
int32_t serial_id_find(uint8_t local_id, platform_id_t *platform_id);
static serial_data_type_t data_type_get(uint8_t local_id);
static int32_t frame_parse(uint8_t *data, uint16_t len);
static int serial_recv_cycle(serial_context_t *pcontext, qm_utils_time_t *timer);
static int32_t serial_deviceinfo_get_send(void);
static int32_t serial_snapshot_get_send(void);
static int32_t serial_process_cycle(serial_context_t *pcontext);
static int32_t id_check_cycle(serial_context_t *pcontext);
static int32_t deviceinfo_handler(serial_context_t *context, uint8_t *data, uint16_t len);

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
	context = serial_context_get();

	qm_list_node_t *node=NULL;
	qm_list_node_t *tmp_node=NULL;

	buf_len = len + sizeof(qm_list_node_t)+2;
	buf = serial_malloc(buf_len);
	if(buf == NULL){
		return SERIAL_ALLOC_MEM_ERR;
	}
	node = (qm_list_node_t*)buf;
	qm_list_node_init(node, buf+sizeof(qm_list_node_t));
	memcpy(buf+sizeof(qm_list_node_t), &len, 2);
	memcpy(buf+sizeof(qm_list_node_t)+2, data, len);

	#ifdef SERIAL_RTOS_SUPPORT
	serial_mutex_lock(context->serial_mutex);
	#endif
	if(context->event_info.serial_event_queue->len < MAX_SERIAL_EVENT_NUM){
		qm_list_rpush(context->event_info.serial_event_queue, node);
	}else{
		tmp_node = qm_list_lpop(context->event_info.serial_event_queue);
		if(tmp_node){
			serial_free(tmp_node);
		}
		qm_list_rpush(context->event_info.serial_event_queue, node);
	}
	#ifdef SERIAL_RTOS_SUPPORT
	serial_mutex_unlock(context->serial_mutex);
	#endif

	return SERIAL_NO_ERR;
}

uint8_t is_event_info_update(void)
{
	serial_context_t *context = NULL;
	context = serial_context_get();
    if(context == NULL){
    	return 0;
    }
    if(context->event_info.serial_event_queue->len){
    	return 1;
    }else{
    	return 0;
    }
}

int32_t serial_event_info_get(uint8_t *buf, uint16_t *buf_len)
{
	serial_context_t *context = NULL;
	qm_list_node_t *node = NULL;
	uint8_t *tmp_buf = NULL;
	uint16_t tmp_buf_len = 0;
	context = serial_context_get();
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
	serial_mutex_lock(context->serial_mutex);
	#endif

	node = qm_list_lpop(context->event_info.serial_event_queue);
	if(node){
		tmp_buf = (uint8_t*)node;
		memcpy(&tmp_buf_len, tmp_buf+sizeof(qm_list_node_t), 2);
		if(*buf_len >= tmp_buf_len){
			memcpy(buf, tmp_buf+sizeof(qm_list_node_t)+2, tmp_buf_len);
			*buf_len = tmp_buf_len;
		}
		serial_free(tmp_buf);
	}

	#ifdef SERIAL_RTOS_SUPPORT
	serial_mutex_unlock(context->serial_mutex);
	#endif

	return SERIAL_NO_ERR;
}
#endif
#endif

static serial_context_t *serial_context_get(void)
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
    serial_context_t *serial_context=NULL;
    serial_context=serial_context_get();
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(serial_context->serial_mutex);
#endif
    serial_context->packet_id =(serial_context->packet_id == (MAX_SERIAL_MCU_PACKET_ID+1)) ? MIN_SERIAL_MCU_PACKET_ID : serial_context->packet_id + 1;
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(serial_context->serial_mutex);
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
    serial_context_t *context=NULL;
    if(addr == NULL){
        return SERIAL_PARAM_ERR;
    }
    context = serial_context_get();
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
    serial_context_t *context=NULL;
    context = serial_context_get();
    if(context == NULL){
        return NULL;
    }
    return &context->map_context;
}
#endif

serial_snapshot_info_t* serial_snapshot_info_get(void)
{
    serial_context_t *context=NULL;
    context = serial_context_get();
    if(context == NULL){
        return NULL;
    }
    return &context->serial_snapshot_info;
}

#if defined(SERIAL_PID_INT_SUPPORT) || defined(SERIAL_PID_STRING_SUPPORT)

void *serial_pid_get(void)
{
    serial_context_t *context=NULL;
    context = serial_context_get();
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
    int32_t err=SERIAL_NO_ERR;
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
    context->pid = (char*)serial_malloc(object->len+1);
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
       if(err != SERIAL_NO_ERR)
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

static int32_t frame_parse(uint8_t *data, uint16_t len)
{
    int32_t err=SERIAL_NO_ERR;
    serial_context_t *context;
    frame_header_t *head = (frame_header_t *)data;
    frame_cmd_t cmd = head->cmd;
    context=serial_context_get();
    uint8_t local_id=0;
    platform_id_t platform_id=0;
    serial_data_type_t type=0;
    device_ver_t ver={0};
    uint8_t heartbeat_type=0;
    network_config_type_t network_config_type = 0;
    uint16_t changed_data_len = 0;
    uint8_t is_update = 0;
    uint8_t *buf = NULL;

    if(*(data+len-1)!=frame_chksum(data, len-1)){
        return SERIAL_CHKSUM_ERR;
    }

    switch (cmd){

       case CMD_REPORT:

        if(len < FRAME_REPORT_MIN_LEN){
            return SERIAL_PACKET_LEN_ERR;
        }

        serial_generate_send(cmd, 0, NULL, 0, 0, &buf);

        if(context->serial_state != SERIAL_STATE_NORMAL){
            SERIAL_LOG("process error");
            return SERIAL_PROCESS_ERR;
        }

#ifndef SERIAL_LOWPOWER_SUPPORT
        if(serial_module_status_get() != MODULE_CONNECTED){
            SERIAL_LOG("module unconnected");
            return SERIAL_PROCESS_ERR;
        }
#endif

        if(!context->serial_snapshot_info.message_context){
            SERIAL_LOG("no snapshot");
            return SERIAL_PROCESS_ERR;
        }

        err = message_object_add_update(context->serial_snapshot_info.message_context, data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1, &is_update);
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
                    serial_free(context->changed_data.data);
                    context->changed_data.data = NULL;
                    context->changed_data.data = (uint8_t*)serial_malloc(changed_data_len);
                    if(context->changed_data.data == NULL){
                        return SERIAL_ALLOC_MEM_ERR;
                    }
                }
            }else{
                context->changed_data.data = (uint8_t*)serial_malloc(changed_data_len);
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
					   serial_free(context->changed_data.data);
					   context->changed_data.data = NULL;
					   context->changed_data.data = (uint8_t*)serial_malloc(changed_data_len);
					   if(context->changed_data.data == NULL){
						   return SERIAL_ALLOC_MEM_ERR;
					   }
				   }
			   }else{
				   context->changed_data.data = (uint8_t*)serial_malloc(changed_data_len);
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

        if(len < FRAME_MIN_LEN+1){
            return SERIAL_PACKET_LEN_ERR;
        }

        local_id=*(data+FRAME_HEAD_LEN);
        type=data_type_get(local_id);
        if(type==TYPE_MCU){
            err=serial_id_find(local_id, &platform_id);
            if(err==SERIAL_NO_ERR){
                if(context->callbacks.m_responsecb){
                   context->callbacks.m_responsecb(cmd, platform_id, data+FRAME_HEAD_LEN+1, len-FRAME_HEAD_LEN-2);
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

        if(context->serial_state != SERIAL_STATE_GETDEVICESNAPSHOT){
        	return SERIAL_PROCESS_ERR;
        }

		if(context->serial_snapshot_info.message_context){
			message_delete(context->serial_snapshot_info.message_context);
			context->serial_snapshot_info.message_context = NULL;
		}
		context->serial_snapshot_info.message_context = message_parse(data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1, PARSE_TYPE_BASIC);
		if(context->serial_snapshot_info.message_context == NULL){
			SERIAL_LOG("snapshot err");
			return SERIAL_PARAM_ERR;
		}

		context->serial_snapshot_info.is_changed = 1;

		context->serial_state = SERIAL_STATE_NORMAL;

		SERIAL_LOG("need report");

#ifdef SERIAL_REPORT_SUPPORT
		message_changed_flag_clear(context->serial_snapshot_info.message_context);

		if(context->callbacks.m_reportcb){
			context->callbacks.m_reportcb(CMD_GET_SNAPSHOT, data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1);
		}
#else
	   if(context->callbacks.m_reportcb){
			context->callbacks.m_reportcb(CMD_GET_SNAPSHOT, data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1);
	   }
#endif

       break;

       case CMD_HEARTBEAT:

           if(len < FRAME_MIN_LEN+1){
               return SERIAL_PACKET_LEN_ERR;
           }

           heartbeat_type = *(data+FRAME_HEAD_LEN);

           if(context->serial_state==SERIAL_STATE_HEARTBEAT ){
               context->serial_state=SERIAL_STATE_GETDEVICEINFO;
               serial_generate_send(CMD_NOTIFY_STATUS, 0, &context->module_status, 1, 0, &buf);
               context->keep_alive_interval=SERIAL_KEPPALIVE_TIMEOUT_DEFAULT_MS;
               qm_utils_time_countdown_ms(&context->next_ping_time, context->keep_alive_interval);
               qm_utils_time_start(&context->process_get_time);

           }else{
                 if(heartbeat_type == HEARTBEAT_RESET){
                      serial_generate_send(CMD_NOTIFY_STATUS, 0, &context->module_status, 1, 0, &buf);
                      context->serial_state=SERIAL_STATE_GETDEVICEINFO;
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
           if(len < FRAME_MIN_LEN+1){
               return SERIAL_PACKET_LEN_ERR;
           }

           serial_generate_send(cmd, 0, NULL, 0, 0, &buf);

           if(context->serial_state != SERIAL_STATE_NORMAL){
               SERIAL_LOG("process error");
               return SERIAL_PROCESS_ERR;
           }
#ifdef SERIAL_EVENT_CACHE_SUPPORT
           event_handler(data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1);
#else
           if(context->callbacks.m_reportcb){
               context->callbacks.m_reportcb(CMD_EVENT, data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1);
           }
#endif

       break;
#endif
#ifdef SERIAL_ACTION_SUPPORT
       case CMD_ACTION:
           if(len < FRAME_MIN_LEN+1){
               return SERIAL_PACKET_LEN_ERR;
           }

           local_id=*(data+FRAME_HEAD_LEN);
           type=data_type_get(local_id);
		   err=serial_id_find(local_id, &platform_id);
		   if(err==SERIAL_NO_ERR){
			   if(context->callbacks.m_responsecb){
				  context->callbacks.m_responsecb(cmd, platform_id, data+FRAME_HEAD_LEN+1, len-FRAME_HEAD_LEN-2);
			   }
		   }

       break;
#endif
       case CMD_NOTIFY_STATUS:


       break;

       case CMD_NETCONFIG:

        if(len < FRAME_MIN_LEN+1){
            return SERIAL_PACKET_LEN_ERR;
        }
        network_config_type = *(data+FRAME_HEAD_LEN);
        if(network_config_type < NETWORK_CONFIG_START || network_config_type > NETWORK_CONFIG_END){
            return SERIAL_DATA_TYPE_ERR;
        }
        serial_generate_send(cmd, 0, NULL, 0, 0, &buf);
        if(context->callbacks.m_network_configcb){
            context->callbacks.m_network_configcb(network_config_type);
        }

       break;

       case CMD_GET_DEVICEINFO:
           if(len < FRAME_DEVICEINFO_GET_MIN_LEN)
               return SERIAL_PACKET_LEN_ERR;

           if(context->serial_state != SERIAL_STATE_GETDEVICEINFO){
               return SERIAL_PROCESS_ERR;
           }

           ver.main_ver=*(data+FRAME_HEAD_LEN);
           ver.sub_ver=*(data+FRAME_HEAD_LEN+1);
           ver.fix_ver=*(data+FRAME_HEAD_LEN+2);

           if(len - FRAME_DEVICEINFO_GET_MIN_LEN ){
               err = deviceinfo_handler(context, data+FRAME_HEAD_LEN+FRAME_VER_LEN, len-FRAME_MIN_LEN-FRAME_VER_LEN);
               if(err != SERIAL_NO_ERR){
                   SERIAL_LOG("device info handler err");
                   return err;
               }
           }

           if(serial_module_status_get() == MODULE_CONNECTED){
               context->serial_state = SERIAL_STATE_GETDEVICESNAPSHOT;
           }else{
#ifndef SERIAL_LOWPOWER_SUPPORT
               context->serial_state = SERIAL_STATE_NORMAL; 
#else
               context->serial_state = SERIAL_STATE_GETDEVICESNAPSHOT;
#endif
           }

           if(context->callbacks.m_device_onlivecb){
               if(len - FRAME_DEVICEINFO_GET_MIN_LEN ){
                   context->callbacks.m_device_onlivecb(&ver, data+FRAME_HEAD_LEN+FRAME_VER_LEN, len-FRAME_MIN_LEN-FRAME_VER_LEN);
               }else{
                   context->callbacks.m_device_onlivecb(&ver, NULL, 0);
               }
           }

       break;

       case CMD_GETMOD:
       case CMD_SETMOD:
        if(len < FRAME_MIN_LEN+1){
            return SERIAL_PACKET_LEN_ERR;
        }

        if(context->callbacks.m_extinfocb){
            context->callbacks.m_extinfocb(cmd, data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1);
        }

       break;

       default:
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

static int32_t frame_creat(frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t data_len, uint8_t *buf, uint16_t *buf_len)
{
    frame_unit_t *frame_unit=NULL;
    frame_generic_t *frame_generic=NULL;

    switch (cmd){

       case CMD_REPORT:
       case CMD_NETCONFIG:
       case CMD_GET_DEVICEINFO:
       case CMD_GET_SNAPSHOT:
       case CMD_EVENT:

           if(buf && *buf_len>=sizeof(frame_unit_t)){
               frame_unit=(frame_unit_t*)buf;
               frame_head_init(buf, cmd, 0);
               frame_unit->chksum=frame_chksum(buf, sizeof(frame_unit_t)-1);
           }
           *buf_len=sizeof(frame_unit_t);

       break;

       case CMD_HEARTBEAT:
       case CMD_NOTIFY_STATUS:

           if(buf && *buf_len>=sizeof(frame_generic_t)){
               frame_generic=(frame_generic_t*)buf;
               frame_head_init(buf, cmd, 1);
               frame_generic->data=*data;
               frame_generic->chksum=frame_chksum(buf, sizeof(frame_generic_t)-1);
           }
           *buf_len=sizeof(frame_generic_t);

       break;

       case CMD_GET:
       case CMD_SET:
       case CMD_ACTION:
           if(buf && *buf_len>=FRAME_MIN_LEN+data_len+1){
               frame_head_init(buf, cmd, data_len+1);
               *(buf+FRAME_HEAD_LEN)=packet_id;
               memcpy(buf+FRAME_HEAD_LEN+1, data, data_len);
               *(buf+FRAME_HEAD_LEN+data_len+1)=frame_chksum(buf, FRAME_HEAD_LEN+data_len+1);
           }
           *buf_len=FRAME_MIN_LEN+data_len+1;

       break;

       case CMD_GETMOD:
       case CMD_SETMOD:

        if(buf && *buf_len>=FRAME_MIN_LEN+data_len){
           frame_head_init(buf, cmd, data_len);
           memcpy(buf+FRAME_HEAD_LEN, data, data_len);
           *(buf+FRAME_HEAD_LEN+data_len)=frame_chksum(buf, FRAME_HEAD_LEN+data_len);
        }
        *buf_len=FRAME_MIN_LEN+data_len;
       break;

       default:
           break;
    }

    return SERIAL_NO_ERR;

}

/*
 *   -------------------------------
 *  |  len  | is_resend | PAYLOAD |
 *   ------------------------------
 *    2Bytes    1byte     nByte
 */
static int32_t serial_generate_send(frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len, uint8_t is_resend, uint8_t**send_buf)
{
    serial_context_t *context=NULL;
    uint8_t *buf=NULL;
    uint16_t buf_len=0;
    uint16_t sum_len=0;
    uint32_t queue_len = 0;
    context=serial_context_get();

    qm_list_node_t *node=NULL;

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    queue_len = context->serial_send_queue->len;
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif

    if(queue_len >= SERIAL_SEND_QUEUE_NUM_MAX){
        return SERIAL_SEND_BUSY_ERR;
    }

    frame_creat(cmd, packet_id, data, len, NULL, &buf_len);

    sum_len = buf_len + sizeof(qm_list_node_t) + 2 + 1;
    buf = serial_malloc(sum_len);
    if(buf == NULL){
        return SERIAL_ALLOC_MEM_ERR;
    }
    node = (qm_list_node_t*)buf;
    qm_list_node_init(node, buf+sizeof(qm_list_node_t));
    memcpy(buf+sizeof(qm_list_node_t), &buf_len, 2);
    memcpy(buf+sizeof(qm_list_node_t)+2, &is_resend, 1);
    frame_creat(cmd, packet_id, data, len, buf+sizeof(qm_list_node_t)+3, &buf_len);

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif

    qm_list_rpush(context->serial_send_queue, node);

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif

#ifdef SERIAL_RTOS_SUPPORT
    serial_sem_signal(context->send_sem);
#endif

    *send_buf = buf;

    return SERIAL_NO_ERR;
}

static int32_t serial_id_info_push(serial_data_type_t type, platform_id_t platform_id, uint8_t local_id, uint8_t *data)
{
    serial_context_t *context=NULL;
    serial_id_info_t *id_info=NULL;
    uint8_t *buf=NULL;
    uint16_t buf_len=0;
    context=serial_context_get();

    qm_list_node_t *node=NULL;

    buf_len=sizeof(serial_id_info_t)+sizeof(qm_list_node_t);
    buf=serial_malloc(buf_len);
    if(buf==NULL){
        return SERIAL_ALLOC_MEM_ERR;
    }
    node=(qm_list_node_t*)buf;

    qm_list_node_init(node, buf+sizeof(qm_list_node_t));

    id_info=(serial_id_info_t*)(buf+sizeof(qm_list_node_t));
    id_info->platform_id=platform_id;
    id_info->local_id=local_id;
    id_info->retry_count = SERIAL_SEND_RETRY_COUNT_MAX;
    id_info->data = data;
    id_info->time = 0; 
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    qm_list_rpush(context->serial_id_queue, node);
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif
    return SERIAL_NO_ERR;
}

static int32_t serial_id_uptime(uint8_t local_id)
{
    serial_context_t *context = NULL;
    serial_id_info_t id_info;
    qm_list_node_t *node=NULL;
    serial_id_info_t *p_id_info = NULL;

    context = serial_context_get();

    id_info.local_id = local_id;

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_find(context->serial_id_queue, &id_info);

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif
    if(node){
        p_id_info = (serial_id_info_t*)node->val;
        p_id_info->time = serial_get_timems() + SERIAL_ACK_TIMEOUT_DEFAULT_MS;
    }

    return SERIAL_NO_ERR;

}


static int32_t id_check_cycle(serial_context_t *pcontext)
{
    qm_list_t *list=pcontext->serial_id_queue;
    qm_list_iterator_t self;
    qm_list_node_t *node=NULL;
    uint8_t *buf=NULL;
    serial_id_info_t *id_info=0;
    uint32_t cur_time=0;
    uint32_t node_index = 0;

    if (!qm_utils_time_is_expired(&pcontext->id_check_time)) {
        return SERIAL_NO_ERR;
    }
    qm_utils_time_countdown_ms(&pcontext->id_check_time, pcontext->id_check_interval);

    cur_time=serial_get_timems();

    self.direction=LIST_HEAD;
    self.next=list->head;
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(pcontext->serial_mutex);
#endif

    node = qm_list_iterator_next(&self);
    while (node) {
        buf=(uint8_t*)node;
        id_info=(serial_id_info_t*)(buf+sizeof(qm_list_node_t));
        if(id_info->time > 0 && (cur_time - id_info->time) < (UINT32_MAX / 2) ){
            id_info->time = 0;
            id_info->retry_count--;
            if(id_info->retry_count == 0){
                qm_list_remove(list, node);
                serial_free(id_info->data);
                serial_free(node);
            }else{
                qm_list_rpush(pcontext->serial_send_queue, (qm_list_node_t*)id_info->data);
#ifdef SERIAL_RTOS_SUPPORT
                serial_sem_signal(pcontext->send_sem);
#endif
            }
        }
        node = qm_list_iterator_next(&self);
    }

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(pcontext->serial_mutex);
#endif

    return SERIAL_NO_ERR;
}


static int id_match(void *a, void *b)
{
    serial_id_info_t *a_info=(serial_id_info_t*)a;
    serial_id_info_t *b_info=(serial_id_info_t*)b;

    if(a_info->local_id==b_info->local_id){
        return 1;
    }else{
        return 0;
    }
}

int32_t serial_id_find(uint8_t local_id, platform_id_t *platform_id)
{
    qm_list_node_t *node=NULL;
    serial_context_t *context=NULL;
    serial_id_info_t id_info={0};
    serial_id_info_t *id=NULL;
    context=serial_context_get();

    id_info.local_id=local_id;

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    node=qm_list_find(context->serial_id_queue, &id_info);

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif
    if(node){
        id=(serial_id_info_t*)node->val;
        *platform_id=id->platform_id;
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
        qm_list_remove(context->serial_id_queue, node);

#ifdef SERIAL_RTOS_SUPPORT
        serial_mutex_unlock(context->serial_mutex);
#endif
        serial_free(node);

    }else{
        return SERIAL_MSG_NUM_ERR;
    }
    return SERIAL_NO_ERR;
}

static int32_t serial_heartbeat_send(void)
{
    uint8_t status=0;
    uint8_t *buf = NULL;
    status = serial_module_status_get();
    serial_generate_send(CMD_HEARTBEAT, 0, &status, 1, 0, &buf);
    return SERIAL_NO_ERR;
}


static int32_t serial_deviceinfo_get_send(void)
{
    uint8_t *buf = NULL;
    serial_generate_send(CMD_GET_DEVICEINFO, 0, NULL, 0, 0, &buf);
    return SERIAL_NO_ERR;
}

static int32_t serial_snapshot_get_send(void)
{
    uint8_t *buf = NULL;
    serial_generate_send(CMD_GET_SNAPSHOT, 0, NULL, 0, 0, &buf);
    return SERIAL_NO_ERR;
}

module_status_t serial_module_status_get(void)
{
    module_status_t status;
    serial_context_t *context = NULL;
    context=serial_context_get();
    if(context==NULL){
        return SERIAL_NO_INIT_ERR;
    }
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    status = context->module_status;
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif
    return status;
}


int32_t serial_module_status_set(module_status_t status)
{
    int32_t need_notify=0;
    serial_context_t *context=NULL;
    serial_state_t state=SERIAL_STATE_NONE;
    uint8_t *buf = NULL;
    context=serial_context_get();
    if(context==NULL){
        return SERIAL_NO_INIT_ERR;
    }

    if(context->module_status != status){
#ifdef SERIAL_RTOS_SUPPORT
        serial_mutex_lock(context->serial_mutex);
#endif
        context->module_status = status;
        state = context->serial_state;
#ifdef SERIAL_RTOS_SUPPORT
        serial_mutex_unlock(context->serial_mutex);
#endif
        need_notify=1;
    }

    if(state != SERIAL_STATE_HEARTBEAT && need_notify){
        serial_generate_send(CMD_NOTIFY_STATUS, 0, &context->module_status, 1, 0, &buf);
    }

#ifndef SERIAL_LOWPOWER_SUPPORT
    if(status == MODULE_CONNECTED && need_notify){
        if(context->serial_state == SERIAL_STATE_NORMAL){
            context->serial_state = SERIAL_STATE_GETDEVICESNAPSHOT;
        }
    }
#endif

    return SERIAL_NO_ERR;
}

static int32_t serial_cloud_generate_send(frame_cmd_t cmd, platform_id_t id, uint8_t *data, uint16_t len)
{
    int32_t err=SERIAL_NO_ERR;
    uint8_t local_id=0;
    serial_context_t *context=NULL;
    context=serial_context_get();
    uint8_t *buf = NULL;
    if(context==NULL){
        return SERIAL_NO_INIT_ERR;
    }

    local_id=packet_id_get();

    err=serial_generate_send(cmd, local_id, data, len, 1, &buf);
    if(err!=SERIAL_NO_ERR){
        return err;
    }

    err=serial_id_info_push(TYPE_MCU, id, local_id, buf);
    if(err!=SERIAL_NO_ERR){
        return err;
    }

    return SERIAL_NO_ERR;
}

int32_t serial_cloud_set(platform_id_t id, uint8_t *data, uint16_t len)
{
    return serial_cloud_generate_send(CMD_SET, id, data, len);
}

int32_t serial_generic_cmd_set(frame_cmd_t cmd)
{
    uint8_t *buf = NULL;
    return serial_generate_send(cmd, 0, NULL, 0, 0, &buf);
}

int32_t serial_cloud_action(platform_id_t id, uint8_t *data, uint16_t len)
{
	return serial_cloud_generate_send(CMD_ACTION, id, data, len);
}

int32_t serial_cloud_get(platform_id_t id, uint8_t *data, uint16_t len)
{
    return serial_cloud_generate_send(CMD_GET, id, data, len);
}

int32_t serial_ext_cmd_ack(frame_cmd_t cmd, uint8_t err_code, uint8_t *data, uint16_t len)
{
    uint8_t *buf=NULL;
    int32_t err=SERIAL_NO_ERR;
    uint8_t code=err_code;
    if(data!=NULL && len >0){
        buf=serial_malloc(len+1);
        if(buf==NULL){
            return SERIAL_ALLOC_MEM_ERR;
        }
        memcpy(buf, &code, 1);
        memcpy(buf+1, data, len);
        err=serial_cloud_generate_send(cmd, 0, buf, len+1);
        serial_free(buf);
    }else{
        err=serial_cloud_generate_send(cmd, 0, &code, 1);
    }

    return err;
}
int32_t qm_serial_init(serial_param_t *serial_param)
{
    serial_context_t *serial_context=NULL;
    if(serial_param == NULL || serial_param->pread_buf == NULL){
        return SERIAL_PARAM_ERR;
    }

    serial_context=(serial_context_t*)serial_malloc(sizeof(serial_context_t));
    if(NULL==serial_context)
    {
       SERIAL_LOG("not enough memory.");
       return SERIAL_ALLOC_MEM_ERR;
    }

    memset(serial_context, 0, sizeof(serial_context_t));

#ifdef SERIAL_RTOS_SUPPORT
    serial_context->serial_mutex=serial_mutex_init();
    serial_context->send_sem=serial_sem_init(2);
#endif

    serial_context->serial_send_queue=qm_list_new();
    serial_context->serial_id_queue=qm_list_new();
#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
    serial_context->event_info.serial_event_queue = qm_list_new();
#endif
#endif
    serial_context->serial_id_queue->match=id_match;

    qm_utils_time_init(&serial_context->next_ping_time);
    serial_context->keep_alive_interval=SERIAL_KEPPALIVE_TIMEOUT_FAST_MS;
    qm_utils_time_countdown_ms(&serial_context->next_ping_time, serial_context->keep_alive_interval);

    qm_utils_time_init(&serial_context->process_get_time);
    serial_context->process_get_interval=SERIAL_PROCESS_TIMEOUT_MS;
    qm_utils_time_countdown_ms(&serial_context->process_get_time, serial_context->process_get_interval);

    qm_utils_time_init(&serial_context->send_interval_time);
    serial_context->send_interval=SERIAL_SEND_INTERVAL_TIMEOUT_DEFAULT_MS;
    qm_utils_time_countdown_ms(&serial_context->send_interval_time, serial_context->send_interval);

    qm_utils_time_init(&serial_context->id_check_time);
    serial_context->id_check_interval=SERIAL_ID_CHECK_TIMEOUT_MS;
    qm_utils_time_countdown_ms(&serial_context->id_check_time, serial_context->id_check_interval);

    serial_context->callbacks.m_network_configcb=serial_param->callbacks.m_network_configcb;
    serial_context->callbacks.m_reportcb=serial_param->callbacks.m_reportcb;
    serial_context->callbacks.m_responsecb=serial_param->callbacks.m_responsecb;
    serial_context->callbacks.m_device_onlivecb=serial_param->callbacks.m_device_onlivecb;
    serial_context->callbacks.m_extinfocb=serial_param->callbacks.m_extinfocb;
#ifdef SERIAL_HEARTBEAT_NOTIFY_SUPPORT
    serial_context->callbacks.m_heartbeatcb = serial_param->callbacks.m_heartbeatcb;
#endif

#ifdef SERIAL_RAW_SUPPORT
    serial_context->callbacks.m_ramcb=serial_context->callbacks.m_ramcb;
#endif
    serial_context->pread_buf=serial_param->pread_buf;
    serial_context->read_buf_size=serial_param->read_buf_size;

    serial_context->parse_state=PARSE_STATE_NONE;
    serial_context->need_len=1;
    serial_context->serial_state=SERIAL_STATE_HEARTBEAT;
    serial_context->module_status = MODULE_UNCONFIGURD_NETWORK;

    g_serial_context=serial_context;

    return SERIAL_NO_ERR;
}

static uint16_t get_packet_len(uint8_t*packet_buffer)
{
    if(packet_buffer==NULL)
        return 0;

    return  (be16_to_cpu((packet_buffer[5]<<8)+(packet_buffer[4])));
}

static int serial_recv_cycle(serial_context_t *pcontext, qm_utils_time_t *timer)
{
    int rc = SERIAL_NO_ERR;
    int recv_len = 0;
    uint16_t packet_len;
    uint32_t left_t = qm_utils_time_left(timer);
    uint16_t need_len = pcontext->need_len;

    recv_len = serial_recv(pcontext->pread_buf+pcontext->read_pos, need_len, left_t);
    pcontext->read_pos += recv_len;

    switch(pcontext->parse_state){

        case PARSE_STATE_NONE:

            if(pcontext->pread_buf[pcontext->read_pos-1] != FRAME_HEAD_PART){
                goto clear;
            }else{
                pcontext->parse_state = PARSE_STATE_HEAD;
                pcontext->need_len = 1;
            }

        break;

        case PARSE_STATE_HEAD:
        	if(pcontext->need_len <= recv_len){
				if(pcontext->pread_buf[pcontext->read_pos-1] != FRAME_HEAD_PART){
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
				if(pcontext->pread_buf[pcontext->read_pos-1] != PROTOCOL_VER){
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
                    pcontext->need_len = packet_len+1;
                }
        	}else{
        		pcontext->need_len = pcontext->need_len - recv_len;
        	}

        break;

        case PARSE_STATE_DATA:

        	if(pcontext->need_len <= recv_len){
				//SERIAL_LOG("serial recv len %d", pcontext->read_pos);
				//SERIAL_HEX_LOG("serial recv data", pcontext->pread_buf, pcontext->read_pos);
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

int32_t serial_send_loop(void)
{
    uint8_t *buf=NULL;
    uint32_t left=0;
    uint16_t buf_len=0;
    int send_queue_len=0;
    qm_list_node_t *node=NULL;
    serial_context_t *context=NULL;
    uint8_t is_resend = 0;
    uint8_t local_id = 0;
    context=serial_context_get();
    if(context==NULL){
        return SERIAL_NO_INIT_ERR;
    }
#ifdef SERIAL_RTOS_SUPPORT
    serial_sem_wait(context->send_sem, 0);
#endif

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    send_queue_len=context->serial_send_queue->len;
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif

    if(send_queue_len==0)
        goto exit;

    if (!qm_utils_time_is_expired(&context->send_interval_time)) {
         goto exit;
    }
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    node=qm_list_lpop(context->serial_send_queue);

    send_queue_len=context->serial_send_queue->len;

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif
    if(node){
        buf=(uint8_t*)node;
        memcpy(&buf_len, buf+sizeof(qm_list_node_t), 2);
        memcpy(&is_resend, buf+sizeof(qm_list_node_t)+2, 1);
        serial_send(buf+sizeof(qm_list_node_t)+3, buf_len);
        if(is_resend){
            local_id = *(buf + sizeof(qm_list_node_t) + 3 + FRAME_HEAD_LEN);
            serial_id_uptime(local_id);
        }

        //SERIAL_HEX_LOG("serial send", buf+sizeof(qm_list_node_t)+2, buf_len);
        if(!is_resend){
            serial_free(buf);
        }
    }

    qm_utils_time_countdown_ms(&context->send_interval_time, context->send_interval);

exit:

#ifdef SERIAL_RTOS_SUPPORT

    if(send_queue_len > 0){
        serial_sem_signal(context->send_sem);
        left= qm_utils_time_left(&context->send_interval_time);
        if (left > 50){
            serial_msleep(left);
        }
        else{
            serial_msleep(50);
        }
    }

#endif

    return SERIAL_NO_ERR;
}

static int32_t serial_keepalive(serial_context_t *pcontext)
{

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
    if(pcontext->serial_state != SERIAL_STATE_GETDEVICEINFO &&
        pcontext->serial_state != SERIAL_STATE_GETDEVICESNAPSHOT){
        return SERIAL_NO_ERR;
    }

    if (!qm_utils_time_is_expired(&pcontext->process_get_time)) {
        return SERIAL_NO_ERR;
    }
    qm_utils_time_countdown_ms(&pcontext->process_get_time, pcontext->process_get_interval);
    if(pcontext->serial_state == SERIAL_STATE_GETDEVICEINFO){
        serial_deviceinfo_get_send();
    }else if(pcontext->serial_state == SERIAL_STATE_GETDEVICESNAPSHOT){
        serial_snapshot_get_send();
    }

    return SERIAL_NO_ERR;
}

int32_t serial_recv_loop(int32_t timeout_ms)
{
    int32_t rc=SERIAL_NO_ERR;
    qm_utils_time_t  yield_time;
    uint32_t left_t=0;
    serial_context_t *context=NULL;
    qm_utils_time_init(&yield_time);
    qm_utils_time_countdown_ms(&yield_time, timeout_ms);

    context=serial_context_get();
    if(context==NULL){
        return SERIAL_NO_INIT_ERR;
    }

#ifdef SERIAL_RTOS_SUPPORT
    do {
        if (SERIAL_NO_ERR != rc) {
            left_t= qm_utils_time_left(&yield_time);
            //serial_printf("error occur ");
            if (left_t > 50){
                serial_msleep(left_t);
            }
            else{
                serial_msleep(50);
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

#ifdef SERIAL_RTOS_SUPPORT
    } while (!qm_utils_time_is_expired(&yield_time));
#endif

    return SERIAL_NO_ERR;
}

#ifdef SERIAL_RAW_SUPPORT
int32_t serial_raw_send(uint8_t *data, uint16_t len)
{
    uint16_t data_len=len;
    uint16_t sum_len=0;
    uint8_t *buf=NULL;
    qm_list_node_t *node=NULL;
    serial_context_t *context=NULL;
    if(data==NULL || len==0){
        return SERIAL_PARAM_ERR;
    }
    context=serial_context_get();
    if(context==NULL){
        return SERIAL_NO_INIT_ERR;
    }
    sum_len=data_len+sizeof(qm_list_node_t)+2;
    buf=serial_malloc(sum_len);
    if(buf==NULL){
        return SERIAL_ALLOC_MEM_ERR;
    }
    node=(qm_list_node_t*)buf;
    qm_list_node_init(node, buf+sizeof(qm_list_node_t));
    memcpy(buf+sizeof(qm_list_node_t), &data_len, 2);
    memcpy(buf+sizeof(qm_list_node_t)+2, data, len);

#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_lock(context->serial_mutex);
#endif
    qm_list_rpush(context->serial_send_queue, node);
#ifdef SERIAL_RTOS_SUPPORT
    serial_mutex_unlock(context->serial_mutex);
#endif

#ifdef SERIAL_RTOS_SUPPORT
    serial_sem_signal(context->send_sem);
#endif

    return SERIAL_NO_ERR;
}
#endif


#ifdef SERIAL_LOWPOWER_SUPPORT
int32_t serial_process_reset(void)
{
	serial_context_t *context = NULL;
    context = serial_context_get();
    if(context == NULL){
    	return SERIAL_NO_INIT_ERR;
    }

    context->serial_state = SERIAL_STATE_HEARTBEAT;
    context->keep_alive_interval = SERIAL_KEPPALIVE_TIMEOUT_FAST_MS;
    qm_utils_time_init(&context->next_ping_time);
    return SERIAL_NO_ERR;
}
#endif
