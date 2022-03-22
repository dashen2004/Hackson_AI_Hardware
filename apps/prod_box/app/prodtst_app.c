#include "prodtst_core.h"




void prodtst_notify_callback(int prodtst_status)
{

    switch(prodtst_status){

        case PRODTST_STATUS_SUCCESS:
            

        break;

        case PRODTST_STATUS_FAIL:


        break;

        case PRODTST_STATUS_READY:


        break;

        case PRODTST_STATUS_GENERIC_START:


        break;

        case PRODTST_STATUS_GENERIC_SUCCESS:


        break;

        case PRODTST_STATUS_FUN_START:


        break;
        
        case PRODTST_STATUS_FUN_SUCCESS:


        break;

        case PRODTST_STATUS_SPECIAL_START:


        break;

        case PRODTST_STATUS_SPECIAL_SUCCESS:


        break;

        default:
            break;
    }

}


void prodtst_app_init(void)
{
    prodtst_init(prodtst_notify_callback);
}
