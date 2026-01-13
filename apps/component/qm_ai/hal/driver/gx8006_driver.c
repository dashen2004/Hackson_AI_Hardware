#include "qm.h"
#include "qm_ai_driver.h"

qm_ai_driver_param_t QM_AI_DRIVER_GX8006_DEFAULT_HANDLE =
{                                               
    .comm_type = QM_AI_DRIVER_COMM_TYPE_AUDIO,  
    .driver_type = QM_AI_DRIVER_TYPE_GX8006,    
    .arg = NULL,    
    open =  gx8006_driver_open,   
    close =  gx8006_driver_close,  
    write =  gx8006_driver_write,  
    read =  gx8006_driver_read,   
    set_mute =  gx8006_driver_set_mute,   
    get_mute =  gx8006_driver_get_mute,   
    set_volume = gx8006_driver_set_volume,  
    get_volume = gx8006_driver_get_volume,  
    enable_pa = gx8006_driver_enable_pa,   
};
