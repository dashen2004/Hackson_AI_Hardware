#ifndef __PRODTST_CLOUD_H__
#define __PRODTST_CLOUD_H__


#include "prodtst_core.h"


enum{
    PRODTST_PACKET_TYPE_CONFIG,
    PRODTST_PACKET_TYPE_SET,
    PRODTST_PACKET_TYPE_GET,
    PRODTST_PACKET_TYPE_SPECIAL,
    PRODTST_PACKET_TYPE_RESULT,
};


int prodtst_generic_unpack_and_handle(int packet_type, int id, char *data, int len);

int prodtst_generic_pack(prodtst_data_table_t *prodtst_tbl, char **data, int *len);

int prodtst_generic_send(int event, int id, prodtst_data_table_t *prodtst_tbl);
int prodtst_test_send(prodtst_data_table_t *prodtst_tbl);
int prodtst_spcl_result_send(prodtst_data_table_t *prodtst_tbl);

#endif