#include "serial_protocol.h"
#include "serial_platform.h"
#include "serial_common.h"
#include "message.h"

static void network_config_callback(void)
{
    serial_usr_log("network config");
}

static void device_onlive_callback(device_ver_t *ver, uint8_t *data, uint16_t len)
{
    serial_usr_log("device onlive");
    serial_usr_log("device version:%d.%d.%d", ver->main_ver, ver->sub_ver, ver->fix_ver);
}

extern k_mm_head *g_kmm_head;

static void report_callback(frame_cmd_t cmd, uint8_t *data, uint16_t len)
{
    serial_usr_log("cmd 0x%02X", cmd);
    SERIAL_HEX_LOG("report", data, len);
}

static void serial_response_callback(frame_cmd_t cmd, platform_id_t id, uint8_t *data, uint16_t len)
{
    serial_usr_log("serial response cmd: %d, id: %u", cmd, id);

}

static int32_t serial_service_callback(void *arg)
{
    message_info_t *info = (message_info_t*)arg;

    //to do somethings

}

int serial_usr_init(void)
{
    serial_param_t serial_param;
    uint8_t *msg_readbuf = NULL;

    if (NULL == (msg_readbuf = (uint8_t *)malloc(SERIAL_MSGLEN))) {
        serial_usr_log("not enough memory");
        err_code = SERIAL_ALLOC_MEM_ERR;
        goto exit;
    }

    serial_param.pread_buf = msg_readbuf;
    serial_param.read_buf_size = SERIAL_MSGLEN;

    serial_param.callbacks.m_network_configcb = network_config_callback;
    serial_param.callbacks.m_reportcb = report_callback;
    serial_param.callbacks.m_responsecb = serial_response_callback;
    serial_param.callbacks.m_device_onlivecb = device_onlive_callback;

    qm_serial_init(&serial_param);

    qm_serial_service_register(MODULE_SERVICE_LOWPOWER, serial_service_callback);

    return 0;

}



