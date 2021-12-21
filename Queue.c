#include "Queue.h"

void enqueue_econ(struct Customer *cus_info){
    if(queue_econ == NULL){
        queue_econ = cus_info;
    }
    else{
        struct Customer *temp = queue_econ;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = cus_info;
    }
    return;
}
void enquque_biz(struct Customer *cus_info){
    if(queue_biz == NULL){
        queue_biz = cus_info;
    }
    else{
        struct Customer *temp = queue_biz;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = cus_info;
    }
    return;
}