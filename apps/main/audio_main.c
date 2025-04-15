
#include "qm.h"

#include "xjiang_cloud.h"

#include "audio_main.h"
#include "audio_cloud.h"
#include "audio_light.h"
#include "audio_beacon.h"

void qm_application_start(void)
{
    audio_light_init();

    audio_beacon_init();

    xjiang_cloud_init();

    audio_cloud_init();

}
