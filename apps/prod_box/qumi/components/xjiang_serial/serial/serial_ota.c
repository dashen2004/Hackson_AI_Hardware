#include "serial_ota.h"
#include "serial_protocol.h"

#ifdef SERIAL_OTA_SUPPORT

static int32_t serial_ota_data_send(void);
static int32_t serial_ota_verify_send(void);

int32_t qm_serial_ota_start(uint8_t verify_type, uint8_t is_restart)
{
	if(verify_type != SERIAL_OTA_VERIFY_MD5 && verify_type != SERIAL_OTA_VERIFY_SHA256){
		return SERIAL_PARAM_LEN_ERR;
	}

	serial_context_t *context = qm_serial_context_get();
	if(context == NULL){
		return  SERIAL_NO_INIT_ERR;
	}
	if(context->ota_context == NULL){
		context->ota_context = (serial_ota_context_t*)qm_serial_malloc(sizeof(serial_ota_context_t));
		if(context->ota_context == NULL){
			return SERIAL_ALLOC_MEM_ERR;
		}
	}
	memset(context->ota_context, 0, sizeof(serial_ota_context_t));

	context->ota_context->verify_type = verify_type;
	context->ota_context->is_restart = is_restart;

	qm_utils_time_init(&context->ota_context->ota_time);

	if(context->ota_context->buffer == NULL){
		context->ota_context->buffer = (uint8_t*)qm_serial_malloc(SERIAL_OTA_BUFFER_LEN);
		if(context->ota_context->buffer == NULL){
			qm_serial_free(context->ota_context);
			context->ota_context = NULL;
			return SERIAL_ALLOC_MEM_ERR;
		}
	}

	qm_serial_state_set(SERIAL_STATE_OTA);

	context->ota_context->status = SERIAL_OTA_STATUS_START;

	context->send_interval = 0;
	qm_utils_time_countdown_ms(&context->send_interval_time, context->send_interval);

	return SERIAL_NO_ERR;
}


int32_t qm_serial_ota_end(void)
{
	serial_context_t *context = qm_serial_context_get();
	if(context == NULL){
		return SERIAL_NO_INIT_ERR;
	}
	if(context->ota_context == NULL){
		return SERIAL_NO_INIT_ERR;
	}

	if(context->ota_context->buffer){
		qm_serial_free(context->ota_context->buffer);
		context->ota_context->buffer = NULL;
	}

	qm_serial_free(context->ota_context);
	context->ota_context  = NULL;

	qm_serial_state_set(SERIAL_STATE_HEARTBEAT);

	context->send_interval = SERIAL_SEND_INTERVAL_TIMEOUT_DEFAULT_MS;
	context->keep_alive_interval = SERIAL_KEPPALIVE_TIMEOUT_FAST_MS;
	qm_utils_time_countdown_ms(&context->next_ping_time, context->keep_alive_interval);
	qm_utils_time_countdown_ms(&context->send_interval_time, context->send_interval);

	return SERIAL_NO_ERR;
}

int32_t qm_serial_ota_data_set(uint16_t partition_id, uint32_t crc32, uint8_t *data, uint16_t len)
{

	serial_context_t *context = qm_serial_context_get();
    if(context == NULL){
        return SERIAL_NO_INIT_ERR;
    }

    if(context->ota_context->partition_id != partition_id){
    	return SERIAL_PARAM_LEN_ERR;
    }

    context->ota_context->his_crc32 = crc32;
    if(data){
    	if(len <= SERIAL_OTA_BUFFER_LEN){
        	memcpy(context->ota_context->buffer, data, len);
            context->ota_context->buf_len = len;
    	}else{
    		return SERIAL_PARAM_LEN_ERR;
    	}
    }else{
    	context->ota_context->buf_len = 0;
    }

	return SERIAL_NO_ERR;
}

int32_t qm_serial_ota_verify_set(uint8_t verify_type, uint8_t *verify_data, uint8_t len)
{
	if(verify_data == NULL){
		return SERIAL_PARAM_LEN_ERR;
	}
	if(verify_type != SERIAL_OTA_VERIFY_MD5 && verify_type != SERIAL_OTA_VERIFY_SHA256){
		return SERIAL_PARAM_LEN_ERR;
	}

	serial_context_t *context = qm_serial_context_get();
	if(context == NULL){
		return SERIAL_NO_INIT_ERR;
	}

	if(verify_type == SERIAL_OTA_VERIFY_MD5){
		if(len != 16){
			return SERIAL_PARAM_LEN_ERR;
		}
		memcpy(context->ota_context->verify_data, verify_data, len);
	}else if(verify_type == SERIAL_OTA_VERIFY_SHA256){
		if(len != 32){
			return SERIAL_PARAM_LEN_ERR;
		}
		memcpy(context->ota_context->verify_data, verify_data, len);
	}else{
		return SERIAL_PARAM_LEN_ERR;
	}

	return SERIAL_NO_ERR;
}

static int32_t serial_ota_data_send(uint8_t is_resend)
{
	serial_transfer_info_t transfer_info = {0};
	serial_context_t *context = qm_serial_context_get();

	transfer_info.partition_id = cpu_to_be16(context->ota_context->partition_id);
	transfer_info.packet_id = context->ota_context->packet_id+1;

	if(context->ota_context->buf_len - context->ota_context->offset < SERIAL_OTA_BUFFER_UNIT_LEN){
		memcpy(transfer_info.data, context->ota_context->buffer + context->ota_context->offset, context->ota_context->buf_len - context->ota_context->offset);
		qm_serial_generate_send(CMD_OTA_TRANSFER, 0, (uint8_t*)&transfer_info, sizeof(transfer_info)-(SERIAL_OTA_BUFFER_UNIT_LEN-(context->ota_context->buf_len - context->ota_context->offset)));
		if(!is_resend){
			context->ota_context->offset += context->ota_context->buf_len - context->ota_context->offset;
		}

	}else{
		memcpy(transfer_info.data, context->ota_context->buffer + context->ota_context->offset, SERIAL_OTA_BUFFER_UNIT_LEN);
		if(!is_resend){
			context->ota_context->offset += SERIAL_OTA_BUFFER_UNIT_LEN;
		}
		qm_serial_generate_send(CMD_OTA_TRANSFER, 0, (uint8_t*)&transfer_info, sizeof(transfer_info));
	}

	SERIAL_LOG("ota offset: %d", context->ota_context->offset);

	return SERIAL_NO_ERR;
}

static int32_t serial_ota_verify_send(void)
{
	serial_context_t *context = qm_serial_context_get();

	if(context->ota_context->verify_type == SERIAL_OTA_VERIFY_MD5){
		qm_serial_generate_send(CMD_OTA_VERIFY, 0, context->ota_context->verify_data, 16);
	}else if(context->ota_context->verify_type == SERIAL_OTA_VERIFY_SHA256){
		qm_serial_generate_send(CMD_OTA_VERIFY, 0, context->ota_context->verify_data, 32);
	}
	return SERIAL_NO_ERR;
}

int32_t qm_serial_ota_parse(uint8_t cmd, uint8_t *data, uint16_t len)
{
	uint8_t err_code = SERIAL_COM_OK;
	serial_partition_info_t *partition_info = NULL;
	serial_transfer_ack_info_t *transfer_ack_info = NULL;
	serial_context_t *context = qm_serial_context_get();
	switch(cmd){

	case CMD_OTA_START:
		if(len != sizeof(uint8_t)){
			return SERIAL_PACKET_LEN_ERR;
		}
		err_code = *data;
		if(err_code != SERIAL_COM_OK){
			if(context->callbacks.ota_event_notifycb){
				context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
			}
			qm_serial_state_set(SERIAL_STATE_HEARTBEAT);
		}else{
			context->ota_context->status = SERIAL_OTA_STATUS_QUERYCRC;
		}

	   break;
	case CMD_OTA_TRANSFER:
		if(len != sizeof(serial_transfer_ack_info_t)){
			return SERIAL_PACKET_LEN_ERR;
		}
		transfer_ack_info = (serial_transfer_ack_info_t*)data;
		if(transfer_ack_info->err_code != SERIAL_COM_OK){
			if(context->callbacks.ota_event_notifycb){
				context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
			}
			return SERIAL_NO_ERR;
		}
		transfer_ack_info->partition_id = be16_to_cpu(transfer_ack_info->partition_id);

		SERIAL_LOG("ota transfer ack partition_id: %d, packet id: %d", transfer_ack_info->partition_id, transfer_ack_info->packet_id);

		if(transfer_ack_info->partition_id != context->ota_context->partition_id && transfer_ack_info->packet_id != context->ota_context->packet_id){
			SERIAL_LOG("ota partition id or packet id not equal");
			context->ota_context->partition_id = transfer_ack_info->partition_id;
			context->ota_context->packet_id = 0;
			context->ota_context->offset = 0;
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &context->ota_context->partition_id);
			context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;
		}else{
			context->ota_context->packet_id++;
			context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;
		}

	   break;
	case CMD_OTA_QUERYCRC:
		if(len != sizeof(serial_partition_info_t)){
			return SERIAL_PACKET_LEN_ERR;
		}
		partition_info = (serial_partition_info_t*)data;
		partition_info->crc32 = be32_to_cpu(partition_info->crc32);
		partition_info->partition_id = be16_to_cpu(partition_info->partition_id);

		SERIAL_LOG("ota query ack partition_id: %d, crc32: 0x%04X, ",partition_info->partition_id, partition_info->crc32);

		if(partition_info->partition_id == 0 && partition_info->crc32 == 0){
			SERIAL_LOG("first query crc32");
			context->ota_context->partition_id = 1;
			context->ota_context->packet_id = 0;
			context->ota_context->offset = 0;
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &context->ota_context->partition_id);
			context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;

		}else if(partition_info->partition_id != 0 && partition_info->crc32 != 0){
			context->ota_context->partition_id = partition_info->partition_id;
			if(context->ota_context->his_crc32 == 0){
				SERIAL_LOG("first query crc32");
				context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &partition_info->partition_id);
				if(context->ota_context->his_crc32 == partition_info->crc32){
					context->ota_context->crc32_invalid_count = 0;
					context->ota_context->partition_id++;
					context->ota_context->packet_id = 0;
					context->ota_context->offset = 0;
					context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &context->ota_context->partition_id);
					if(context->ota_context->buf_len == 0){
						context->ota_context->status = SERIAL_OTA_STATUS_VERIFY;
					}else{
						context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;
					}
				}else{
					SERIAL_LOG("partition id: %d, crc32 is invalid", context->ota_context->partition_id);
					if(context->ota_context->is_restart){
						context->ota_context->partition_id = 1;
						context->ota_context->packet_id = 0;
						context->ota_context->offset = 0;
						context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &context->ota_context->partition_id);
						context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;
						break;
					}
					context->ota_context->crc32_invalid_count++;
					if(context->ota_context->crc32_invalid_count >= SERIAL_OTA_CRC32_INVALID_COUNT){
						err_code = SERIAL_COM_CHKSUM_ERROR;
						context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
					}else{
						context->ota_context->packet_id = 0;
						context->ota_context->offset = 0;
						context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &context->ota_context->partition_id);
						context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;
					}
				}
			}else{
				if(context->ota_context->his_crc32 == partition_info->crc32){
					context->ota_context->crc32_invalid_count = 0;
					context->ota_context->partition_id++;
					context->ota_context->packet_id = 0;
					context->ota_context->offset = 0;
					context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &context->ota_context->partition_id);
					if(context->ota_context->buf_len == 0){
						context->ota_context->status = SERIAL_OTA_STATUS_VERIFY;
					}else{
						context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;
					}

				}else{
					SERIAL_LOG("partition id: %d, crc32 is invalid", context->ota_context->partition_id);
					context->ota_context->crc32_invalid_count++;
					if(context->ota_context->crc32_invalid_count >= SERIAL_OTA_CRC32_INVALID_COUNT){
						err_code = SERIAL_COM_CHKSUM_ERROR;
						context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
					}else{
						context->ota_context->packet_id = 0;
						context->ota_context->offset = 0;
						context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_DATA_GET, &context->ota_context->partition_id);
						context->ota_context->status = SERIAL_OTA_STATUS_TRANSFER;
					}
				}
			}

		}else{
			err_code = SERIAL_COM_CHKSUM_ERROR;
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
			qm_serial_state_set(SERIAL_STATE_HEARTBEAT);
		}

	   break;
	case CMD_OTA_VERIFY:

		if(len != sizeof(uint8_t)){
			return SERIAL_PACKET_LEN_ERR;
		}
		err_code = *data;
		if(err_code != SERIAL_COM_OK){
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
			qm_serial_state_set(SERIAL_STATE_HEARTBEAT);
		}else{
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
		}


	   break;
	default:
		break;
	}

	return SERIAL_NO_ERR;

}

int32_t qm_serial_ota_process(void)
{
	int32_t ret;
	uint8_t err_code = SERIAL_COM_OK;
	serial_state_t state;
	uint8_t type;
	serial_ota_status_t status;
	serial_context_t *context = qm_serial_context_get();
	ret = qm_serial_state_get(&state);
	if(ret != SERIAL_NO_ERR){
		return ret;
	}

	if(state != SERIAL_STATE_OTA){
		return SERIAL_PROCESS_ERR;
	}

	status = context->ota_context->status;

	switch(status){
	case SERIAL_OTA_STATUS_START:

		SERIAL_LOG("start mcu ota");

		context->ota_context->retry_count = SERIAL_OTA_RETRY_COUNT;
		context->ota_context->ota_interval = SERIAL_OTA_INTERVAL_TIMEOUT_MS;
		qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);
		type = context->ota_context->verify_type;
		qm_serial_generate_send(CMD_OTA_START, 0, &type, sizeof(type));
		context->ota_context->status = SERIAL_OTA_STATUS_WAIT_START_ACK;

		context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_START, &err_code);

		break;

	case SERIAL_OTA_STATUS_WAIT_START_ACK:

		if (!qm_utils_time_is_expired(&context->ota_context->ota_time)) {
			return SERIAL_NO_ERR;
		}
		context->ota_context->retry_count--;
		if(context->ota_context->retry_count == 0){
			err_code = SERIAL_COM_TIMEOUT;
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
		}else{
			qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);
			type = context->ota_context->verify_type;
			qm_serial_generate_send(CMD_OTA_START, 0, &type, sizeof(type));
		}

    	break;

	case SERIAL_OTA_STATUS_QUERYCRC:

		SERIAL_LOG("start query crc32");

		context->ota_context->retry_count = SERIAL_OTA_RETRY_COUNT;
		context->ota_context->ota_interval = SERIAL_OTA_INTERVAL_TIMEOUT_MS;
		qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);
		qm_serial_generate_send(CMD_OTA_QUERYCRC, 0, NULL, 0);
		context->ota_context->status = SERIAL_OTA_STATUS_WAIT_QUERYCRC_ACK;

		break;

	case SERIAL_OTA_STATUS_WAIT_QUERYCRC_ACK:
		if (!qm_utils_time_is_expired(&context->ota_context->ota_time)) {
			return SERIAL_NO_ERR;
		}

		SERIAL_LOG("query crc32 timeout");

		context->ota_context->retry_count--;
		if(context->ota_context->retry_count == 0){
			err_code = SERIAL_COM_TIMEOUT;
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
		}else{
			qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);
			qm_serial_generate_send(CMD_OTA_QUERYCRC, 0, &type, sizeof(type));
		}

		break;

	case SERIAL_OTA_STATUS_TRANSFER:

		context->ota_context->retry_count = SERIAL_OTA_RETRY_COUNT;
		context->ota_context->ota_interval = SERIAL_OTA_INTERVAL_TIMEOUT_MS;
		qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);

		if(context->ota_context->buf_len == context->ota_context->offset){
			qm_serial_generate_send(CMD_OTA_QUERYCRC, 0, NULL, 0);
			context->ota_context->status = SERIAL_OTA_STATUS_WAIT_QUERYCRC_ACK;
		}else {
			SERIAL_LOG("send ota partition id: %d, packet id :%d", context->ota_context->partition_id, context->ota_context->packet_id+1);
			serial_ota_data_send(0);
			context->ota_context->status = SERIAL_OTA_STATUS_WAIT_TRANSFER_ACK;
		}

		break;

	case SERIAL_OTA_STATUS_WAIT_TRANSFER_ACK:

		if (!qm_utils_time_is_expired(&context->ota_context->ota_time)) {
			return SERIAL_NO_ERR;
		}
		context->ota_context->retry_count--;
		if(context->ota_context->retry_count == 0){
			err_code = SERIAL_COM_TIMEOUT;
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
		}else{
			serial_ota_data_send(1);
			qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);
		}

		break;

	case SERIAL_OTA_STATUS_VERIFY:

		SERIAL_LOG("start ota verify");

		context->ota_context->retry_count = SERIAL_OTA_RETRY_COUNT;
		context->ota_context->ota_interval = SERIAL_OTA_INTERVAL_TIMEOUT_MS;
		qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);

		context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_VERIFY_GET, &err_code);

		serial_ota_verify_send();

		context->ota_context->status = SERIAL_OTA_STATUS_WAIT_VERIFY_ACK;

		break;

	case SERIAL_OTA_STATUS_WAIT_VERIFY_ACK:

		if (!qm_utils_time_is_expired(&context->ota_context->ota_time)) {
			return SERIAL_NO_ERR;
		}
		context->ota_context->retry_count--;
		if(context->ota_context->retry_count == 0){
			err_code = SERIAL_COM_TIMEOUT;
			context->callbacks.ota_event_notifycb(SERIAL_OTA_EVENT_EXIT, &err_code);
		}else{
			serial_ota_verify_send();
			qm_utils_time_countdown_ms(&context->ota_context->ota_time, context->ota_context->ota_interval);
		}

		break;

	default:
		break;
	}

	return SERIAL_NO_ERR;

}


#endif
