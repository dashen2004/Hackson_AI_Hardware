#include "opple_serial_protocol.h"
#include "qm_log.h"

#define OPPLE_SERIAL_MSGLEN  256
uint8_t msg_readbuf[OPPLE_SERIAL_MSGLEN] = {0};

static void network_config_callback(void)
{
    QM_LOGD("test", "network config");
}

static void report_callback(opple_frame_cmd_t cmd, uint8_t *data, uint16_t len)
{
    QM_HEX_LOGD("test", "report", data, len);
}

static void serial_response_callback(opple_frame_cmd_t cmd, opple_platform_id_t id, uint8_t *data, uint16_t len)
{
    QM_LOGD("test", "reponse id: %d", id);
    QM_HEX_LOGD("test", "response", data, len);
}

static void serial_extinfo_callback(opple_frame_cmd_t cmd, uint8_t *data, uint16_t len)
{
    QM_HEX_LOGD("test", "extinfo", data, len);
}

static void serial_send_thread(void *arg)
{
    //QM_LOGD("test", "serial send started");

    while(1)
    {
        opple_serial_send_loop();
    }
}

static void serial_recv_thread(void *arg)
{
    //QM_LOGD("test", "serial recv started");

    while(1)
    {
        opple_serial_recv_loop(1000);
    }
}

void opple_serial_test(void)
{
    qm_task_t send_task = {0};
    qm_task_t recv_task = {0};

    opple_serial_param_t serial_param = {0};

    serial_param.pread_buf = msg_readbuf;
    serial_param.read_buf_size = OPPLE_SERIAL_MSGLEN;

    serial_param.callbacks.m_network_configcb = network_config_callback;
    serial_param.callbacks.m_reportcb = report_callback;
    serial_param.callbacks.m_responsecb = serial_response_callback;
    serial_param.callbacks.m_extinfocb = serial_extinfo_callback;

    opple_serial_init(&serial_param);

    qm_task_new(&send_task, "serial send", serial_send_thread, NULL, 4*1024, 20);
    qm_task_new(&recv_task, "serial recv", serial_recv_thread, NULL, 4*1024, 20);

}

