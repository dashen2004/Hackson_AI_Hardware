
#include "qm.h"
#if 0
#include "bsp_board.h"

#include "audio_beacon.h"

#include "xjiang_cloud.h"
#endif

#include "audio_main.h"
#include "audio_device.h"

void qm_application_start(void)
{
#if 0
    bsp_board_init();

    audio_dev_init();

    audio_beacon_init();

    xjiang_cloud_init();
#endif
    audio_device_init();

    QM_LOGD(LOG_TAG, "Version: %s!!!", CONFIG_AUDIO_LIGHT_HW_VERSION);
}
