
#include "qm.h"

#include "audio_light.h"
#include "audio_beacon.h"

#include "xjiang_cloud.h"

#include "audio_main.h"

void qm_application_start(void)
{
    audio_light_init();

    audio_beacon_init();

    xjiang_cloud_init();

    QM_LOGD(LOG_TAG, "Version: %s!!!", CONFIG_AUDIO_LIGHT_HW_VERSION);
}
