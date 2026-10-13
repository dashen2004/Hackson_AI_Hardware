#include "qm.h"
#include "vb701n_player.h"
#include "xjiang_cloud.h"

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
    vb701n_player_start();
    if (xjiang_cloud_init() != QM_EOK) {
        QM_LOGE("main", "xjiang_cloud_init failed");
    }
}
