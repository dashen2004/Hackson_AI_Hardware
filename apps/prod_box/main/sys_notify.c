#include "sys_notify.h"
#include <stdint.h>
#include <stddef.h>
#include "user_debug.h"
#include "common_error.h"
#include <stdlib.h>

typedef struct _notify_list {
    void* function;
    void* arg;
    struct _notify_list* next;
    void* contex;
} _Notify_list_t;

_Notify_list_t* Notify_list[notify_MAX] = { NULL };


/*system defined notifications */
typedef void (*notify_WIFI_STATUS_CHANGED_function)          (WiFiEvent status, void* inContext);
typedef void (*notify_WIFICONFIG_START_function)              (void* inContext);
typedef void (*notify_WIFICONFIG_GET_EXTRA_DATA_function)      (int datalen, char* data, void* inContext);
typedef void (*notify_TCP_CLIENT_CONNECTED_function)         (int fd, void* inContext);
typedef void (*notify_DNS_RESOLVE_COMPLETED_function)        (uint8_t* hostname, uint32_t ip, void* inContext);
typedef void (*notify_SYS_WILL_POWER_OFF_function)           (void* inContext);
typedef void (*notify_WIFI_CONNECT_FAILED_function)          (int err, void* inContext);
typedef void (*notify_CLOUD_EVENT_CHANGED_function)          (int event, void* const inContext);
typedef void (*notify_SYSTEM_BUTTON_CLICKED_function)                    (void* const inContext);
typedef void (*notify_SYSTEM_BUTTON_LONGPRESSED_function)                    (void* const inContext);
typedef void (*notify_SYSTEM_BUTTON_DOUBLECLICKED_function)                    (void* const inContext);
typedef void (*notify_NETWORK_STATUS_CHANGED_function)          (network_status_t status, void* inContext);

/* User defined notifications */

void network_status_changed(network_status_t status)
{
    _Notify_list_t* temp = Notify_list[notify_NETWORK_STATUS_CHANGED];
    if (temp == NULL)
        return;
    else {
        do {
            ((notify_NETWORK_STATUS_CHANGED_function)(temp->function))(status, temp->arg);
            temp = temp->next;
        } while (temp != NULL);
    }

}

void system_button_clicked_notify(void)
{
    _Notify_list_t* temp = Notify_list[notify_SYSTEM_BUTTON_CLICKED_CHANGED];
    if (temp == NULL)
        return;
    else {
        do {
            ((notify_SYSTEM_BUTTON_CLICKED_function)(temp->function))(temp->arg);
            temp = temp->next;
        } while (temp != NULL);
    }

}

void system_button_longpressed_notify(void)
{
    _Notify_list_t* temp = Notify_list[notify_SYSTEM_BUTTON_LONGPRESSED_CHANGED];
    if (temp == NULL)
        return;
    else {
        do {
            ((notify_SYSTEM_BUTTON_LONGPRESSED_function)(temp->function))(temp->arg);
            temp = temp->next;
        } while (temp != NULL);
    }

}

void system_button_doubleclicked_notify(void)
{
    _Notify_list_t* temp = Notify_list[notify_SYSTEM_BUTTON_DOUBLECLICKED_CHANGED];
    if (temp == NULL)
        return;
    else {
        do {
            ((notify_SYSTEM_BUTTON_DOUBLECLICKED_function)(temp->function))(temp->arg);
            temp = temp->next;
        } while (temp != NULL);
    }

}


void cloud_event_changed(int event)
{

    _Notify_list_t* temp = Notify_list[notify_CLOUD_EVNET_CHANGED];
    if (temp == NULL)
        return;
    else {
        do {
            ((notify_CLOUD_EVENT_CHANGED_function)(temp->function))(event, temp->arg);
            temp = temp->next;
        } while (temp != NULL);
    }

}


int system_notify_register(notify_types_t notify_type, void* functionAddress, void* arg)
{
    int err = 0;
    _Notify_list_t* temp = Notify_list[notify_type];
    _Notify_list_t* notify = (_Notify_list_t*)malloc(sizeof(_Notify_list_t));
    if(notify == NULL){
        err = -1;
        goto exit;
    }

    notify->function = functionAddress;
    notify->arg = arg;
    notify->next = NULL;
    if (Notify_list[notify_type] == NULL) {
        Notify_list[notify_type] = notify;
        notify->next = NULL;
    }
    else {
        if (temp->function == functionAddress){
            goto exit;   //Nodify already exist
        }
        while (temp->next != NULL) {
            temp = temp->next;
            if (temp->function == functionAddress){
                goto exit;  //Nodify already exist
            }
        }
        temp->next = notify;
    }

    return 0;
    
exit:
    if(notify){
        free(notify);
        notify = NULL;
    }
    return err;
}

int system_notify_remove(notify_types_t notify_type, void* functionAddress)
{
    int err = 0;
    _Notify_list_t* temp = Notify_list[notify_type];
    _Notify_list_t* temp2 = temp;
    do {
        if (temp->function == functionAddress) {
            if (temp == Notify_list[notify_type]) {  //first element
                Notify_list[notify_type] = Notify_list[notify_type]->next;
                free(temp);
            }
            else {
                temp2->next = temp->next;
                free(temp);
            }
            break;
        }
        if(temp->next == NULL){
            goto exit;
        }
        temp2 = temp;
        temp = temp->next;
    } while (temp != NULL);

exit:
    return err;
}

int mico_system_notify_remove_all(notify_types_t notify_type)
{
    _Notify_list_t* temp = Notify_list[notify_type];

    while (temp) {
        Notify_list[notify_type] = Notify_list[notify_type]->next;
        free(temp);
        temp = Notify_list[notify_type];
    }

    return 0;
}

