#include "qm.h"
#include "bsp_uart.h"
#include "bsp_io.h"
/* #include "bsp_mpu6050.h" */
#include "voice.h"
#include "agent_app.h"
#include "ljyd_cloud.h"
#include "ljyd_provision.h"

#include <stdio.h>
#include <stdarg.h>

int qm_vsnprintf(char *str, size_t count, const char *fmt, va_list ap)
{
    return vsnprintf(str, count, fmt, ap);
}

void app_main(void)
{
    qm_main();
}

void qm_application_start(void)
{
    if (bsp_uart_init() != QM_EOK) {
        QM_LOGE("main", "uart init failed");
        return;
    }
    if (bsp_io_init() != QM_EOK) {
        QM_LOGE("main", "io init failed");
        return;
    }
    /* if (bsp_mpu6050_init() != QM_EOK) {
        QM_LOGE("main", "mpu6050 init failed");
    } */
    if (voice_start() != QM_EOK) {
        QM_LOGE("main", "audio init failed");
        return;
    }

    if (ljyd_cloud_init() != QM_EOK) {
        QM_LOGE("main", "ljyd_cloud_init failed");
        return;
    }
    if (ljyd_provision_init() != QM_EOK) {
        QM_LOGE("main", "ljyd_provision_init failed");
    }
    if (agent_app_init(ljyd_cloud_get_handle()) != QM_EOK) {
        QM_LOGE("main", "agent_app_init failed");
    }
}
