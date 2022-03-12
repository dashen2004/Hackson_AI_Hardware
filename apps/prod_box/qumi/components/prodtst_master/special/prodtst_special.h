#ifndef __PRODTST_SPECIAL_H__
#define __PRODTST_SPECIAL_H__


typedef struct{

    char *action;
    int (*init)(void *param);
    int (*deinit)(void *param);
    int (*process)(char *data, int len);

}prodtst_special_t;


prodtst_special_t *prodtst_special_get(char *action);

/*** nfc process***/
extern int prodtst_nfc_init(void *param);
extern int prodtst_nfc_deinit(void *param);
extern int prodtst_nfc_process(char *data, int len);

#endif
