#include "prodtst_special.h"
#include "qm_errno.h"
#include "qm_kernel.h"

static qm_mutex_t nfc_mutex = {0};

int prodtst_nfc_process(char *data, int len)
{

    if(!qm_mutex_is_valid(&nfc_mutex)){
        qm_mutex_new(&nfc_mutex);
    }

    qm_mutex_lock(&nfc_mutex, QM_WAIT_FOREVER);










    qm_mutex_unlock(&nfc_mutex);

    return QM_EOK;
}