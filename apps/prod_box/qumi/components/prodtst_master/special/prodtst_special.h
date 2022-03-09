#ifndef __PRODTST_SPECIAL_H__
#define __PRODTST_SPECIAL_H__


typedef struct{

    char *action;
    int (*process)(char *data, int len);

}prodtst_special_t;


int prodtst_special_process(char *action, char *data, int len);

extern int prodtst_nfc_process(char *data, int len);

#endif
