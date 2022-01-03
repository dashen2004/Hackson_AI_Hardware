#include "qmos/qm.h"
#include "qm_cli.h"


void qm_kernel_init(void)
{

    qm_init();
#if CONFIG_QM_CLI
    qm_cli_init();
#endif
    

}