#ifndef __PRODTST_SPECIAL_H__
#define __PRODTST_SPECIAL_H__


typedef struct{
    char *action;
    void *(*establish)(void *param);
    int (*destroy)(void *handle);
    int (*send)(void *handle, char *buf, int size);
    int (*recv)(void *handle, char *buf, int size, int timeout);
}prodtst_special_t;


prodtst_special_t *prodtst_special_get(char *action);



#endif
