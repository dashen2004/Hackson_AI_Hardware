#include "opple_serial_protocol.h"
#include "opple_serial_platform.h"
#include "opple_serial_common.h"
#include "qm_utils_string.h"
#include "qm_utils_timer.h"
#include "qm_utils_list.h"
#include "qm_tlv.h"

static uint8_t frame_chksum(uint8_t *buffer, uint16_t length);
static void frame_head_init(uint8_t *buffer, opple_frame_cmd_t cmd, uint8_t seq, uint8_t len);
static int32_t frame_creat(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t data_len, uint8_t *buf, uint16_t *buf_len);
static int32_t serial_generate_send(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len);
static int32_t serial_generate_send_with_code(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t len);
static int32_t serial_keepalive(opple_serial_context_t *pcontext);
static int32_t serial_id_info_push(opple_platform_id_t platform_id, uint8_t local_id, uint8_t *data);
static int32_t serial_id_update(uint8_t local_id);
static int32_t serial_cloud_generate_send(opple_frame_cmd_t cmd, opple_platform_id_t id, uint8_t *data, uint16_t len);
static int id_match(void *a, void *b);
static int32_t serial_local_id_find(uint8_t local_id, opple_platform_id_t *platform_id);
static int32_t frame_parse(uint8_t *data, uint16_t len);
static int serial_recv_cycle(opple_serial_context_t *pcontext, qm_utils_time_t *timer);
static int32_t serial_snapshot_get_send(void);
static int32_t serial_process_cycle(opple_serial_context_t *pcontext);
static int32_t id_check_cycle(opple_serial_context_t *pcontext);
static int32_t down_queue_send(void);


static opple_serial_context_t *g_serial_context = NULL;

opple_serial_context_t *opple_serial_context_get(void)
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
    opple_serial_context_t *serial_context = NULL;
    serial_context = opple_serial_context_get();
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(serial_context->serial_mutex);
#endif  
    serial_context->packet_id = (serial_context->packet_id == OPPLE_SEQ_MAX) ? 0 : serial_context->packet_id + 1;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(serial_context->serial_mutex);
#endif
    return serial_context->packet_id;
}

opple_serial_snapshot_info_t* opple_serial_snapshot_info_get(void)
{
    opple_serial_context_t *context=NULL;
    context = opple_serial_context_get();
    if(context == NULL){
        return NULL;
    }
    return &context->serial_snapshot_info;
}

static int32_t frame_parse(uint8_t *data, uint16_t len)
{
    int32_t err = OPPLE_SERIAL_NO_ERR;
    opple_serial_context_t *context = NULL;
    opple_frame_header_t *head = (opple_frame_header_t*)data;
    opple_frame_cmd_t cmd = head->cmd;
    context = opple_serial_context_get();
    uint8_t local_id = 0;
    opple_platform_id_t platform_id=0;
    uint8_t config_type = 0;
    opple_serial_time_info_t time_info = {0};
    qm_tlv_ctx_t *tlv_ctx = NULL;

    if(*(data+len-3) != frame_chksum(data+2, len-5)){
        return OPPLE_SERIAL_CHKSUM_ERR;
    }

    switch (cmd){

       case OPPLE_CMD_REPORT:

        if(len < OPPLE_FRAME_REPORT_MIN_LEN){
            return OPPLE_SERIAL_PACKET_LEN_ERR;
        }

        serial_generate_send(cmd, 0, NULL, 0);

        if(context->state != OPPLE_SERIAL_STATE_NORMAL){
            OPPLE_SERIAL_LOG("process error");
            return OPPLE_SERIAL_PROCESS_ERR;
        }

#ifndef OPPLE_SERIAL_LOWPOWER_SUPPORT
        if(opple_serial_module_status_get() != OPPLE_MODULE_CONNECTED){
            OPPLE_SERIAL_LOG("module unconnected");
            return OPPLE_SERIAL_PROCESS_ERR;
        }
#endif

        if(!context->serial_snapshot_info.tlv_ctx){
            OPPLE_SERIAL_LOG("no snapshot");
            return OPPLE_SERIAL_PROCESS_ERR;
        }

#ifdef OPPLE_SERIAL_REPORT_SUPPORT

        tlv_ctx = qm_tlv_ctx_creat();
        if(tlv_ctx == NULL){
            OPPLE_SERIAL_LOG("tlv ctx creat fail");
            return OPPLE_SERIAL_ALLOC_MEM_ERR;
        }
        err = qm_tlv_deserialize(tlv_ctx, data + OPPLE_FRAME_HEAD_LEN, len - OPPLE_FRAME_MIN_LEN);
        if(err != 0){
            OPPLE_SERIAL_LOG("tlv ctx creat fail");
            qm_tlv_ctx_delete(tlv_ctx);
            return OPPLE_SERIAL_PARAM_ERR;
        }

        qm_tlv_merge(context->serial_snapshot_info.tlv_ctx, tlv_ctx);
        qm_tlv_ctx_delete(tlv_ctx);

        if(context->callbacks.m_reportcb){
            context->callbacks.m_reportcb(cmd, data + OPPLE_FRAME_HEAD_LEN, len - OPPLE_FRAME_MIN_LEN);
        }
#endif
       break;

       case OPPLE_CMD_GET:
       case OPPLE_CMD_SET:

        if(len < OPPLE_FRAME_GET_MIN_LEN){
            return OPPLE_SERIAL_PACKET_LEN_ERR;
        }

        local_id = head->seq;
        err = serial_local_id_find(local_id, &platform_id);
        if(err == OPPLE_SERIAL_NO_ERR){
            if(context->callbacks.m_responsecb){
                context->callbacks.m_responsecb(cmd, platform_id, data + OPPLE_FRAME_HEAD_LEN, len - OPPLE_FRAME_MIN_LEN);
            }
        }
    
       break;

       case OPPLE_CMD_GET_SNAPSHOT:
        if(len < OPPLE_FRAME_SNAPSHOT_MIN_LEN){
            return OPPLE_SERIAL_PACKET_LEN_ERR;
        }

        if(context->state != OPPLE_SERIAL_STATE_GETDEVICESNAPSHOT){
            OPPLE_SERIAL_LOG("process error");
        	return OPPLE_SERIAL_PROCESS_ERR;
        }

		if(context->serial_snapshot_info.tlv_ctx){
            qm_tlv_ctx_delete(tlv_ctx);
            context->serial_snapshot_info.tlv_ctx = NULL;
		}

        tlv_ctx = qm_tlv_ctx_creat();
        if(tlv_ctx == NULL){
            OPPLE_SERIAL_LOG("tlv ctx creat fail");
            return OPPLE_SERIAL_ALLOC_MEM_ERR;
        }
        err = qm_tlv_deserialize(tlv_ctx, data + OPPLE_FRAME_HEAD_LEN, len - OPPLE_FRAME_MIN_LEN);
        if(err != 0){
            OPPLE_SERIAL_LOG("tlv ctx creat fail");
            qm_tlv_ctx_delete(tlv_ctx);
            return OPPLE_SERIAL_PARAM_ERR;
        }
        context->serial_snapshot_info.tlv_ctx = tlv_ctx;

		context->state = OPPLE_SERIAL_STATE_NORMAL;

#ifdef OPPLE_SERIAL_REPORT_SUPPORT
        if(context->callbacks.m_reportcb){
            context->callbacks.m_reportcb(OPPLE_CMD_GET_SNAPSHOT, data + OPPLE_FRAME_HEAD_LEN, len - OPPLE_FRAME_MIN_LEN);
        }
#endif

       break;

#ifdef SERIAL_EVENT_SUPPORT

       case CMD_EVENT:
           if(len < FRAME_MIN_LEN+1){
               return SERIAL_PACKET_LEN_ERR;
           }

           serial_generate_send(cmd, 0, NULL, 0);

           if(context->state != SERIAL_STATE_NORMAL){
               OPPLE_SERIAL_LOG("process error");
               return SERIAL_PROCESS_ERR;
           }
           if(context->callbacks.m_reportcb){
               context->callbacks.m_reportcb(CMD_EVENT, data+FRAME_HEAD_LEN, len-FRAME_HEAD_LEN-1);
           }
#endif

       break;

#ifdef SERIAL_ACTION_SUPPORT
       case CMD_ACTION:
           if(len < FRAME_MIN_LEN+1){
               return SERIAL_PACKET_LEN_ERR;
           }

           local_id=*(data+FRAME_HEAD_LEN);
           type=data_type_get(local_id);
		   err=serial_local_id_find(local_id, &platform_id);
		   if(err==SERIAL_NO_ERR){
			   if(context->callbacks.m_responsecb){
				  context->callbacks.m_responsecb(cmd, platform_id, data+FRAME_HEAD_LEN+1, len-FRAME_HEAD_LEN-2);
			   }
		   }

       break;
#endif

       case OPPLE_CMD_NETCONFIG:

        if(len < OPPLE_FRAME_MIN_LEN + 1){
            return OPPLE_SERIAL_PACKET_LEN_ERR;
        }
        config_type = *(data + OPPLE_FRAME_HEAD_LEN);
        if(config_type < OPPLE_NETWORK_CONFIG_START || config_type > OPPLE_NETWORK_CONFIG_END){
            return OPPLE_SERIAL_DATA_TYPE_ERR;
        }

        serial_generate_send(cmd, 0, NULL, 0);
        if(context->callbacks.m_network_configcb){
            context->callbacks.m_network_configcb(config_type);
        }

       break;

       case OPPLE_CMD_TIMEGET:

            if(len < OPPLE_FRAME_MIN_LEN + 1){
                return OPPLE_SERIAL_PACKET_LEN_ERR;
            }
            time_info.timezone = *(data + OPPLE_FRAME_HEAD_LEN);
            err = opple_serial_get_time(&time_info);
            if(err != 0){
                serial_generate_send_with_code(cmd, 0, OPPLE_SERIAL_COM_ERROR, (uint8_t*)&time_info, sizeof(time_info));
            }else{
                serial_generate_send_with_code(cmd, 0, OPPLE_SERIAL_COM_OK, (uint8_t*)&time_info, sizeof(time_info));
            }

       break;

       case OPPLE_CMD_GETMOD:
       case OPPLE_CMD_SETMOD:
        if(len < OPPLE_FRAME_MIN_LEN + 1){
            return OPPLE_SERIAL_PACKET_LEN_ERR;
        }

        if(context->callbacks.m_extinfocb){
            context->callbacks.m_extinfocb(cmd, data + OPPLE_FRAME_HEAD_LEN, len - OPPLE_FRAME_MIN_LEN);
        }

       break;

       default:
           break;
    }

    return OPPLE_SERIAL_NO_ERR;
}


static void frame_head_init(uint8_t *buffer, opple_frame_cmd_t cmd, uint8_t seq, uint8_t len)
{
    opple_frame_header_t *head = (opple_frame_header_t *)buffer;

    head->head = cpu_to_be16(OPPLE_FRAME_HEAD);
    head->ver  = OPPLE_PROTOCOL_VER;
    head->seq = seq;
    head->cmd  = cmd;
    head->len = len;
}

static int32_t frame_creat(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t data_len, uint8_t *buf, uint16_t *buf_len)
{
    opple_frame_unit_t *frame_unit = NULL;
    opple_frame_generic_t *frame_generic = NULL;
    opple_frame_tail_t *frame_tail = NULL;
    uint8_t len = 0;

    switch (cmd){

       case OPPLE_CMD_REPORT:
       case OPPLE_CMD_NETCONFIG:
       case OPPLE_CMD_GET_SNAPSHOT:
       case OPPLE_CMD_EVENT:

        len = sizeof(opple_frame_unit_t);
        if(buf && *buf_len >= len){
            frame_unit = (opple_frame_unit_t*)buf;
            frame_head_init(buf, cmd, packet_id, len);
            frame_unit->chksum = frame_chksum(buf+1, len-5);
            frame_unit->tail = cpu_to_be16(OPPLE_FRAME_TAIL);
        }
        *buf_len = len;

       break;

       case OPPLE_CMD_NOTIFY_STATUS:

        len = sizeof(opple_frame_generic_t);
        if(buf && *buf_len >= len){
            frame_generic=(opple_frame_generic_t*)buf;
            frame_head_init(buf, cmd, packet_id, len);
            frame_generic->data = *data;
            frame_generic->chksum = frame_chksum(buf+1, len-5);
            frame_generic->tail = cpu_to_be16(OPPLE_FRAME_TAIL);
        }
        *buf_len = len;

       break;

       case OPPLE_CMD_GET:
       case OPPLE_CMD_SET:
       case OPPLE_CMD_ACTION:
        len = OPPLE_FRAME_MIN_LEN + data_len;
        if(buf && *buf_len >= +1){
            frame_head_init(buf, cmd, packet_id, len);
            memcpy(buf + OPPLE_FRAME_HEAD_LEN, data, data_len);
            frame_tail = (opple_frame_tail_t*)(buf + len - 3);
            frame_tail->chksum = frame_chksum(buf+1, len-5);
            frame_tail->tail = cpu_to_be16(OPPLE_FRAME_TAIL);
        }
        *buf_len = len;

       break;

       case OPPLE_CMD_GETMOD:
       case OPPLE_CMD_SETMOD:
       case OPPLE_CMD_TIMEGET:

        len = OPPLE_FRAME_MIN_LEN + data_len + 1;
        if(buf && *buf_len >= len){
            frame_head_init(buf, cmd, packet_id, len);
            memcpy(buf + OPPLE_FRAME_HEAD_LEN, &code, 1);
            memcpy(buf + OPPLE_FRAME_HEAD_LEN + 1, data, data_len);
            frame_tail = (opple_frame_tail_t*)(buf + len - 3);
            frame_tail->chksum = frame_chksum(buf+1, len-5);
            frame_tail->tail = cpu_to_be16(OPPLE_FRAME_TAIL);
        }
        *buf_len = len;
        
       break;

       default:
            return OPPLE_SERIAL_CMD_ERR;
       break;
    }

    return OPPLE_SERIAL_NO_ERR;

}

static int32_t serial_frame_info_creat(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t len, uint8_t resend, uint8_t **raw)
{
    int32_t ret = 0;
    uint8_t *buf = NULL;
    uint16_t packet_len = 0;
    uint16_t raw_len = 0;
    opple_frame_info_t *info = NULL;
    ret = frame_creat(cmd, packet_id, code, data, len, NULL, &packet_len);
    if(ret != OPPLE_SERIAL_NO_ERR){
        return ret;
    }

    raw_len = sizeof(opple_frame_info_t) + packet_len -1;
    buf = (uint8_t*)opple_serial_malloc(raw_len);
    if(buf == NULL){
        return OPPLE_SERIAL_ALLOC_MEM_ERR;
    } 

    memset(buf, 0, raw_len);

    info = (opple_frame_info_t*)buf;
    info->len = packet_len;
    info->resend = resend;

    ret = frame_creat(cmd, packet_id, code, data, len, info->data, &packet_len);
    if(ret != OPPLE_SERIAL_NO_ERR){
        return ret;
    } 
    *raw = buf;
    return OPPLE_SERIAL_NO_ERR;
}


static uint32_t queue_len_get(qm_list_t *queue)
{
    uint32_t queue_len = 0;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    queue_len = queue->len;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif
    return queue_len;
}

static int32_t queue_push(qm_list_t *queue, uint8_t *raw)
{
    qm_list_node_t *node = NULL;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();

    node = (qm_list_node_t*)raw;
    qm_list_node_init(node, raw+sizeof(qm_list_node_t));

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    qm_list_rpush(queue, node);
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_sem_signal(context->send_sem);
#endif
    return OPPLE_SERIAL_NO_ERR; 
}

/*
 *   -------------------------------
 *  |  len  | is_resend | PAYLOAD |
 *   ------------------------------
 *    2Bytes    1byte     nByte
 */

#ifdef OPPLE_SERIAL_RESEND_SUPPORT

static int32_t serial_generate_resend(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len, uint8_t **buf)
{
    uint32_t ret = 0;
    uint8_t *raw = NULL;
    qm_list_t *queue = NULL;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
    queue = context->down_queue;

    if(queue_len_get(queue) >= OPPLE_SERIAL_DOWN_QUEUE_NUM_MAX){
        return OPPLE_SERIAL_SEND_BUSY_ERR;
    }

    ret = serial_frame_info_creat(cmd, packet_id, 0, data, len, 1, &raw);
    if(ret != OPPLE_SERIAL_NO_ERR){
        return ret;
    }
    *buf = raw;
    return queue_push(queue, raw);
}

#endif

static int32_t serial_generate_send(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t *data, uint16_t len)
{
    uint32_t ret = 0;
    uint8_t *raw = NULL;
    qm_list_t *queue = NULL;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
    queue = context->send_queue;

    if(queue_len_get(queue) >= OPPLE_SERIAL_SEND_QUEUE_NUM_MAX){
        return OPPLE_SERIAL_SEND_BUSY_ERR;
    }

    ret = serial_frame_info_creat(cmd, packet_id, 0, data, len, 0, &raw);
    if(ret != OPPLE_SERIAL_NO_ERR){
        return ret;
    }

    return queue_push(queue, raw);
}

static int32_t serial_generate_send_with_code(opple_frame_cmd_t cmd, uint8_t packet_id, uint8_t code, uint8_t *data, uint16_t len)
{
    uint32_t ret = 0;
    uint8_t *raw = NULL;
    qm_list_t *queue = NULL;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
    queue = context->send_queue;

    if(queue_len_get(queue) >= OPPLE_SERIAL_SEND_QUEUE_NUM_MAX){
        return OPPLE_SERIAL_SEND_BUSY_ERR;
    }

    ret = serial_frame_info_creat(cmd, packet_id, code, data, len, 0, &raw);
    if(ret != OPPLE_SERIAL_NO_ERR){
        return ret;
    }

    return queue_push(queue, raw);
}

static int32_t serial_id_info_push(opple_platform_id_t platform_id, uint8_t local_id, uint8_t *data)
{
    opple_serial_context_t *context = NULL;
    opple_serial_id_info_t *id_info = NULL;
    uint8_t *buf = NULL;
    uint16_t buf_len = 0;
    context = opple_serial_context_get();

    qm_list_node_t *node = NULL;

    buf_len = sizeof(opple_serial_id_info_t) + sizeof(qm_list_node_t);
    buf = (uint8_t*)opple_serial_malloc(buf_len);
    if(buf == NULL){
        return OPPLE_SERIAL_ALLOC_MEM_ERR;
    }
    node = (qm_list_node_t*)buf;

    memset(buf, 0, buf_len);

    qm_list_node_init(node, buf + sizeof(qm_list_node_t));

    id_info = (opple_serial_id_info_t*)(buf + sizeof(qm_list_node_t));
    id_info->platform_id = platform_id;
    id_info->local_id = local_id;
#ifdef OPPLE_SERIAL_RESEND_SUPPORT
    id_info->retry_count = OPPLE_SERIAL_SEND_RETRY_COUNT_MAX;
#else
    id_info->retry_count = 0;
#endif
    id_info->data = data;
    id_info->time = 0; 
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    qm_list_rpush(context->id_queue, node);
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif
    return OPPLE_SERIAL_NO_ERR;
}

static uint8_t is_serial_id_update(void)
{
    opple_serial_context_t *context = NULL;
    qm_list_iterator_t self;
    qm_list_node_t *node = NULL;
    qm_list_t *list = NULL;
    opple_serial_id_info_t *id_info = NULL;
    uint8_t *buf = NULL;
    uint8_t is_update = 0;

    context = opple_serial_context_get();
    list = context->id_queue;

    self.direction = LIST_HEAD;
    self.next = list->head;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif

    node = qm_list_iterator_next(&self);
    while (node) {
        buf = (uint8_t*)node;
        id_info = (opple_serial_id_info_t*)(buf+sizeof(qm_list_node_t));
        if(id_info->is_update){
            is_update = 1;
        }
        node = qm_list_iterator_next(&self);
    }
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif

    return is_update;
}

static int32_t serial_id_update(uint8_t local_id)
{
    opple_serial_context_t *context = NULL;
    opple_serial_id_info_t id_info = {0};
    qm_list_node_t *node = NULL;
    opple_serial_id_info_t *p_id_info = NULL;

    context = opple_serial_context_get();

    id_info.local_id = local_id;

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_find(context->id_queue, &id_info);
    if(node){
        p_id_info = (opple_serial_id_info_t*)node->val;
        p_id_info->is_update = 1;
        p_id_info->time = opple_serial_get_timems() + OPPLE_SERIAL_ACK_TIMEOUT_DEFAULT_MS;
    }

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif

    return OPPLE_SERIAL_NO_ERR;
}

static int id_match(void *a, void *b)
{
    opple_serial_id_info_t *a_info = (opple_serial_id_info_t*)a;
    opple_serial_id_info_t *b_info = (opple_serial_id_info_t*)b;

    if(a_info->local_id == b_info->local_id){
        return 1;
    }else{
        return 0;
    }
}

static int32_t serial_local_id_find(uint8_t local_id, opple_platform_id_t *platform_id)
{
    qm_list_node_t *node = NULL;
    opple_serial_context_t *context = NULL;
    opple_serial_id_info_t id_info = {0};
    opple_serial_id_info_t *id = NULL;
    context = opple_serial_context_get();

    id_info.local_id = local_id;

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_find(context->id_queue, &id_info);

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif
    if(node){
        id = (opple_serial_id_info_t*)node->val;
        *platform_id = id->platform_id;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
        opple_serial_mutex_lock(context->serial_mutex);
#endif
        qm_list_remove(context->id_queue, node);
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
        opple_serial_mutex_unlock(context->serial_mutex);
#endif
        opple_serial_free(node);
    }else{
        return OPPLE_SERIAL_MSG_NUM_ERR;
    }
    return OPPLE_SERIAL_NO_ERR;
}

static int32_t serial_cloud_generate_send(opple_frame_cmd_t cmd, opple_platform_id_t id, uint8_t *data, uint16_t len)
{
    int32_t err = OPPLE_SERIAL_NO_ERR;
    uint8_t local_id = 0;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
    uint8_t *buf = NULL;
    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }

    local_id = packet_id_get();

#ifdef OPPLE_SERIAL_RESEND_SUPPORT

    err = serial_id_info_push(id, local_id, NULL);
    if(err != OPPLE_SERIAL_NO_ERR){
        return err;
    }

    err = serial_generate_resend(cmd, local_id, data, len, &buf);
    if(err != OPPLE_SERIAL_NO_ERR){
        return err;
    }
#else
    err = serial_id_info_push(id, local_id, NULL);
    if(err != OPPLE_SERIAL_NO_ERR){
        return err;
    }

    err = serial_generate_send(cmd, local_id, data, len);
    if(err != OPPLE_SERIAL_NO_ERR){
        return err;
    }

#endif

    return OPPLE_SERIAL_NO_ERR;
}

#ifdef OPPLE_SERIAL_RESEND_SUPPORT

static int32_t down_queue_send(void)
{
    qm_list_node_t *node = NULL;
    qm_list_t *id_queue = NULL;
    qm_list_t *down_queue = NULL;
    qm_list_t *send_queue = NULL;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();

    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }

    id_queue = context->id_queue;
    down_queue = context->down_queue;
    send_queue = context->send_queue;

    if(is_serial_id_update()){
        return OPPLE_SERIAL_NO_ERR;
    }

    if(queue_len_get(down_queue) == 0){
        return OPPLE_SERIAL_NO_ERR;
    }

    if(queue_len_get(send_queue) >= OPPLE_SERIAL_SEND_QUEUE_NUM_MAX){
        OPPLE_SERIAL_LOG("send queue is full");
        return OPPLE_SERIAL_SEND_BUSY_ERR;
    }

#ifdef SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_lpop(down_queue);

#ifdef SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif

    queue_push(send_queue, (uint8_t*)node);
    return OPPLE_SERIAL_NO_ERR;
}

#endif

static int32_t id_check_cycle(opple_serial_context_t *pcontext)
{
    qm_list_t *list = pcontext->id_queue;
    qm_list_iterator_t self;
    qm_list_node_t *node = NULL;
    uint8_t *buf = NULL;
    opple_serial_id_info_t *id_info = NULL;
    uint32_t cur_time = 0;
    int32_t send = 0;

    if (!qm_utils_time_is_expired(&pcontext->id_check_time)) {
        return OPPLE_SERIAL_NO_ERR;
    }
    qm_utils_time_countdown_ms(&pcontext->id_check_time, pcontext->id_check_interval);

    if(queue_len_get(pcontext->id_queue) == 0){
#ifdef OPPLE_SERIAL_RESEND_SUPPORT
        down_queue_send();
#endif
        return OPPLE_SERIAL_NO_ERR;
    }

    cur_time = opple_serial_get_timems();

    self.direction = LIST_HEAD;
    self.next = list->head;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(pcontext->serial_mutex);
#endif

    node = qm_list_iterator_next(&self);
    while (node) {
        buf = (uint8_t*)node;
        id_info = (opple_serial_id_info_t*)(buf+sizeof(qm_list_node_t));
        if(id_info->is_update && (cur_time - id_info->time) < (UINT32_MAX / 2) ){
            id_info->is_update = 0;
            if(id_info->retry_count == 0){
                qm_list_remove(list, node);
                if(id_info->data){
                    opple_serial_free(id_info->data);
                }
                opple_serial_free(node);
                send = 1;
            }else{
                qm_list_rpush(pcontext->send_queue, (qm_list_node_t*)id_info->data);
#ifdef OPPLE_SERIAL_RTOS_SUPPORT

                opple_serial_sem_signal(pcontext->send_sem);
#endif
            }
            id_info->retry_count--;
        }
        node = qm_list_iterator_next(&self);
    }

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(pcontext->serial_mutex);
#endif

#ifdef OPPLE_SERIAL_RESEND_SUPPORT
    if(send){
        down_queue_send();
    }
#endif

    return OPPLE_SERIAL_NO_ERR;
}

static int32_t serial_snapshot_get_send(void)
{
    return serial_generate_send(OPPLE_CMD_GET_SNAPSHOT, 0, NULL, 0);
}

int32_t opple_serial_state_set(opple_serial_state_t state)
{
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    context->state = state;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif
    return OPPLE_SERIAL_NO_ERR;
}

int32_t opple_serial_state_get(opple_serial_state_t *state)
{
    opple_serial_context_t *context = NULL;
	if(state == NULL){
		return OPPLE_SERIAL_PARAM_ERR;
	}
    context = opple_serial_context_get();
    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    *state = context->state;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif
    return OPPLE_SERIAL_NO_ERR;
}

opple_module_status_t opple_serial_module_status_get(void)
{
    opple_module_status_t status;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    status = context->module_status;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif
    return status;
}

int32_t opple_serial_module_status_set(opple_module_status_t status)
{
    int32_t need_notify = 0;
    opple_serial_context_t *context = NULL;
    context = opple_serial_context_get();
    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }

    if(context->module_status != status){
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
        opple_serial_mutex_lock(context->serial_mutex);
#endif
        context->module_status = status;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
        opple_serial_mutex_unlock(context->serial_mutex);
#endif
        need_notify = 1;
    }

    if(need_notify){
        serial_generate_send(OPPLE_CMD_NOTIFY_STATUS, 0, &context->module_status, 1);
    }

#ifndef OPPLE_SERIAL_LOWPOWER_SUPPORT
    if(status == OPPLE_MODULE_CONNECTED && need_notify){
        if(context->state == OPPLE_SERIAL_STATE_NORMAL){
            context->state = OPPLE_SERIAL_STATE_GETDEVICESNAPSHOT;
        }
    }
#endif
    return OPPLE_SERIAL_NO_ERR;
}

int32_t qm_serial_cloud_set(opple_platform_id_t id, uint8_t *data, uint16_t len)
{
    return serial_cloud_generate_send(OPPLE_CMD_SET, id, data, len);
}

int32_t qm_serial_cloud_action(opple_platform_id_t id, uint8_t *data, uint16_t len)
{
	return serial_cloud_generate_send(OPPLE_CMD_ACTION, id, data, len);
}

int32_t qm_serial_cloud_get(opple_platform_id_t id, uint8_t *data, uint16_t len)
{
    return serial_cloud_generate_send(OPPLE_CMD_GET, id, data, len);
}

int32_t qm_serial_ext_cmd_ack(opple_frame_cmd_t cmd, uint8_t err_code, uint8_t *data, uint16_t len)
{
    uint8_t *buf = NULL;
    int32_t err = OPPLE_SERIAL_NO_ERR;
    uint8_t code = err_code;
    if(data != NULL && len >0){
        buf = opple_serial_malloc(len+1);
        if(buf == NULL){
            return OPPLE_SERIAL_ALLOC_MEM_ERR;
        }
        memcpy(buf, &code, 1);
        memcpy(buf+1, data, len);
        err = serial_generate_send(cmd, 0, buf, len+1);
        opple_serial_free(buf);
    }else{
        err = serial_generate_send(cmd, 0, &code, 1);
    }
    return err;
}

int32_t opple_serial_init(opple_serial_param_t *serial_param)
{
    opple_serial_context_t *serial_context = NULL;
    if(serial_param == NULL || serial_param->pread_buf == NULL){
        return OPPLE_SERIAL_PARAM_ERR;
    }

    serial_context = (opple_serial_context_t*)opple_serial_malloc(sizeof(opple_serial_context_t));
    if(NULL == serial_context){
        return OPPLE_SERIAL_ALLOC_MEM_ERR;
    }

    memset(serial_context, 0, sizeof(opple_serial_context_t));

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    serial_context->serial_mutex = opple_serial_mutex_init();
    serial_context->send_sem = opple_serial_sem_init(0);
#endif

    serial_context->send_queue = qm_list_new();
    serial_context->id_queue = qm_list_new();

#ifdef OPPLE_SERIAL_RESEND_SUPPORT
    serial_context->down_queue = qm_list_new();
#endif

#ifdef SERIAL_EVENT_SUPPORT
#ifdef SERIAL_EVENT_CACHE_SUPPORT
    serial_context->event_info.serial_event_queue = qm_list_new();
#endif
#endif
    serial_context->id_queue->match = id_match;

    qm_utils_time_init(&serial_context->process_get_time);
    serial_context->process_get_interval = OPPLE_SERIAL_PROCESS_TIMEOUT_MS;
    qm_utils_time_countdown_ms(&serial_context->process_get_time, serial_context->process_get_interval);

    qm_utils_time_init(&serial_context->send_interval_time);
    serial_context->send_interval = OPPLE_SERIAL_SEND_INTERVAL_TIMEOUT_DEFAULT_MS;
    qm_utils_time_countdown_ms(&serial_context->send_interval_time, serial_context->send_interval);

    qm_utils_time_init(&serial_context->id_check_time);
    serial_context->id_check_interval = OPPLE_SERIAL_ID_CHECK_TIMEOUT_MS;
    qm_utils_time_countdown_ms(&serial_context->id_check_time, serial_context->id_check_interval);

    serial_context->callbacks.m_network_configcb = serial_param->callbacks.m_network_configcb;
    serial_context->callbacks.m_reportcb = serial_param->callbacks.m_reportcb;
    serial_context->callbacks.m_responsecb = serial_param->callbacks.m_responsecb;
    serial_context->callbacks.m_extinfocb = serial_param->callbacks.m_extinfocb;

    serial_context->pread_buf = serial_param->pread_buf;
    serial_context->read_buf_size = serial_param->read_buf_size;

    serial_context->parse_state = OPPLE_PARSE_STATE_HEAD;
    serial_context->need_len = 1;
    serial_context->state = OPPLE_SERIAL_STATE_GETDEVICESNAPSHOT;
    serial_context->module_status = OPPLE_MODULE_UNCONFIGURD_NETWORK;
    
    serial_context->handle = opple_serial_uart_init();

    g_serial_context = serial_context;
    
    return OPPLE_SERIAL_NO_ERR;
}

static int serial_recv_cycle(opple_serial_context_t *pcontext, qm_utils_time_t *timer)
{
    int rc = OPPLE_SERIAL_NO_ERR;
    int recv_len = 0;
    uint16_t packet_len;
    uint32_t left_t = qm_utils_time_left(timer);
    uint16_t need_len = pcontext->need_len;

    recv_len = opple_serial_recv(pcontext->handle, pcontext->pread_buf+pcontext->read_pos, need_len, left_t);
    if(recv_len == 0){
        goto clear;
    }

    pcontext->read_pos += recv_len;
    
    switch(pcontext->parse_state){

        case OPPLE_PARSE_STATE_HEAD:

            if(pcontext->pread_buf[pcontext->read_pos-1] != OPPLE_FRAME_HEAD_PART){
                goto clear;
            }else{
                pcontext->parse_state = OPPLE_PARSE_STATE_HEAD_1;
                pcontext->need_len = 1;
            }

        break;

        case OPPLE_PARSE_STATE_HEAD_1:
            if(pcontext->need_len <= recv_len){
                if(pcontext->pread_buf[pcontext->read_pos-1] != OPPLE_FRAME_HEAD_1_PART){
                    goto clear;
                }else{
                    pcontext->parse_state = OPPLE_PARSE_STATE_LEN;
                    pcontext->need_len = 1;
                }
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;
        case OPPLE_PARSE_STATE_VER:
            if(pcontext->need_len <= recv_len){
                if( OPPLE_VER_GET(pcontext->pread_buf[pcontext->read_pos-1]) != OPPLE_PROTOCOL_VER){
                    goto clear;
                }else{
                    pcontext->parse_state = OPPLE_PARSE_STATE_DATA;
                    pcontext->need_len = pcontext->packet_len - 4 -2;
                }
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;
        case OPPLE_PARSE_STATE_LEN:

            if(pcontext->need_len <= recv_len){
                packet_len = pcontext->pread_buf[pcontext->read_pos-1];
                if(packet_len > (pcontext->read_buf_size - pcontext->read_pos)){
                    OPPLE_SERIAL_LOG("serial recv buffer over flow");
                    goto clear;
                }else if(packet_len < OPPLE_FRAME_MIN_LEN){
                    OPPLE_SERIAL_LOG("serial recv buffer is too short");
                    goto clear;
                }else{
                    pcontext->parse_state = OPPLE_PARSE_STATE_VER;
                    pcontext->packet_len = packet_len;
                    pcontext->need_len = 1;
                }
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;

        case OPPLE_PARSE_STATE_DATA:

            if(pcontext->need_len <= recv_len){
                pcontext->parse_state = OPPLE_PARSE_STATE_TAIL;
                pcontext->need_len = 1;
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;

        case OPPLE_PARSE_STATE_TAIL:
            if(pcontext->need_len <= recv_len){
                if(pcontext->pread_buf[pcontext->read_pos-1] != OPPLE_FRAME_TAIL_PART){
                    goto clear;
                }else{
                    pcontext->parse_state = OPPLE_PARSE_STATE_TAIL_1;
                    pcontext->need_len = 1;
                }

            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }

        break;

        case OPPLE_PARSE_STATE_TAIL_1:

            if(pcontext->need_len <= recv_len){
                if(pcontext->pread_buf[pcontext->read_pos-1] != OPPLE_FRAME_TAIL_1_PART){
                    goto clear;
                }else{
                    OPPLE_SERIAL_LOG("opple serial recv len %d", pcontext->read_pos);
                    OPPLE_SERIAL_HEX_LOG("opple serial recv data", (char*)pcontext->pread_buf, (int)pcontext->read_pos);
                    rc = frame_parse(pcontext->pread_buf, pcontext->read_pos);
                    goto clear;
                }
            }else{
                pcontext->need_len = pcontext->need_len - recv_len;
            }
        break;

        default:
            break;
    }

    return rc;

clear:
    pcontext->parse_state = OPPLE_PARSE_STATE_HEAD;
    pcontext->read_pos = 0;
    pcontext->need_len = 1;
    return rc;
}

int32_t opple_serial_send_loop(void)
{
    opple_frame_info_t *info = NULL;
    uint32_t left = 0;
    uint32_t send_queue_len = 0;
    qm_list_node_t *node = NULL;
    opple_serial_context_t *context = NULL;
    opple_frame_header_t *frame_header = NULL;

    context = opple_serial_context_get();
    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_sem_wait(context->send_sem, 0);
#endif

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    send_queue_len = context->send_queue->len;
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif

    if(send_queue_len == 0)
        goto exit;

    if (!qm_utils_time_is_expired(&context->send_interval_time)) {
         goto exit;
    }
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_lock(context->serial_mutex);
#endif
    node = qm_list_lpop(context->send_queue);

    send_queue_len = context->send_queue->len;

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    opple_serial_mutex_unlock(context->serial_mutex);
#endif
    if(node){
        info = (opple_frame_info_t*)node;
        opple_serial_send(context->handle, info->data, info->len);
        OPPLE_SERIAL_HEX_LOG("serial send", (char*)info->data, (int)info->len);
        if(info->resend){
            frame_header = (opple_frame_header_t*)info->data;
            serial_id_update(frame_header->seq);
        }else{
            opple_serial_free(info);
        }
    }

    qm_utils_time_countdown_ms(&context->send_interval_time, context->send_interval);

exit:

#ifdef OPPLE_SERIAL_RESEND_SUPPORT
    down_queue_send();
#endif

#ifdef OPPLE_SERIAL_RTOS_SUPPORT

    if(send_queue_len > 0){
        opple_serial_sem_signal(context->send_sem);
        left = qm_utils_time_left(&context->send_interval_time);
        if (left > 50){
            opple_serial_msleep(left);
        }
        else{
            opple_serial_msleep(50);
        }
    }

#endif

    return OPPLE_SERIAL_NO_ERR;
}

static int32_t serial_process_cycle(opple_serial_context_t *pcontext)
{
    if(pcontext->state != OPPLE_SERIAL_STATE_GETDEVICESNAPSHOT){
        return OPPLE_SERIAL_NO_ERR;
    }

    if (!qm_utils_time_is_expired(&pcontext->process_get_time)) {
        return OPPLE_SERIAL_NO_ERR;
    }
    qm_utils_time_countdown_ms(&pcontext->process_get_time, pcontext->process_get_interval);
    
    if(pcontext->state == OPPLE_SERIAL_STATE_GETDEVICESNAPSHOT){
        serial_snapshot_get_send();
    }

    return OPPLE_SERIAL_NO_ERR;
}

int32_t opple_serial_recv_loop(int32_t timeout_ms)
{
    int32_t rc = OPPLE_SERIAL_NO_ERR;
    qm_utils_time_t yield_time = {0};
    uint32_t left_t = 0;
    opple_serial_context_t *context = NULL;
    qm_utils_time_init(&yield_time);
    qm_utils_time_countdown_ms(&yield_time, timeout_ms);

    context = opple_serial_context_get();
    if(context == NULL){
        return OPPLE_SERIAL_NO_INIT_ERR;
    }

#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    do {
        if (OPPLE_SERIAL_NO_ERR != rc) {
            left_t= qm_utils_time_left(&yield_time);
            //serial_printf("error occur ");
            if (left_t > 50){
                opple_serial_msleep(left_t);
            }
            else{
                opple_serial_msleep(50);
            }
        }
#endif
        serial_process_cycle(context);

        id_check_cycle(context);

        /* acquire package in cycle */
        rc = serial_recv_cycle(context, &yield_time);
        if (OPPLE_SERIAL_NO_ERR == rc) {
            /* check list of wait serial ACK to remove node that is ACKED or timeout */

        }
#ifdef OPPLE_SERIAL_RTOS_SUPPORT
    } while (!qm_utils_time_is_expired(&yield_time));
#endif

    return OPPLE_SERIAL_NO_ERR;
}