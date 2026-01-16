#include "qm.h"
#include "qm_ai_api.h"
#include "qm_ai_inner_rtc_api.h"

#if CONFIG_QM_AI_DRIVER_USER_GX8006
#include "gx8006_driver.h"
#endif

typedef struct qm_ai_t
{
    qm_ai_driver_type_t drvier_type;
};

/**
 * @brief 启动AI模块
 * 
 * @details 
 * 
 */
int32_t qm_ai_start(qm_ai_handle_t handle)
{
    if(handle == NULL){
        return -QM_EINVAL;
    }

    switch(handle->drvier_type)
    {   
    #if CONFIG_QM_AI_DRIVER_USER_GX8006
        case QM_AI_DRIVER_TYPE_GX8006:
            qm_ai_driver_open(&QM_AI_DRIVER_GX8006_DEFAULT_HANDLE);
        break;
    #endif

    #if CONFIG_QM_AI_DRIVER_USER_CUSTOM
        case QM_AI_DRIVER_TYPE_CUSTOM:
            qm_ai_driver_open(&QM_AI_DRIVER_CUSTOM_HANDLE);
        break;
    #endif
        default:
        break;
    }
}