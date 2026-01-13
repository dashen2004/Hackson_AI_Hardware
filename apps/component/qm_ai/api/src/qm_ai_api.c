#include "qm.h"
#include "qm_ai_api.h"

#if CONFIG_QM_AI_DRIVER_USER_GX8006
#include "gx8006_driver.h"
#endif

struct qm_ai_t
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

#if CONFIG_QM_AI_DRIVER_USER_GX8006
    if()
    {   
        qm_ai_driver_param_t param = QM_AI_DRIVER_GX8006_DEFAULT_HANDLE();
        qm_ai_driver_open(&param);
    }
#endif
}