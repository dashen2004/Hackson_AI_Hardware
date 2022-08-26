#include "qm_config.h"

#include "qmos/qm.h"

#if CONFIG_QM_CLI_SUPPORT
#include "qm_cli.h"
#endif

void qm_kernel_init(void)
{

    qm_init();
#if CONFIG_QM_CLI_SUPPORT
    qm_cli_init();
#endif
    

}