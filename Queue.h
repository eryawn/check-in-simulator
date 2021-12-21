#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

struct Customer
{
    int id;
    int type;
    int t_arrival;
    int t_service;
    struct Customer *next;
};

struct Queue
{
    int length;
    int id;
    struct Node *head;
    struct Node *tail;
};

extern struct Customer* queue_econ;
extern struct Customer* queue_biz;

void enqueue_econ(struct Customer *cus_info);
void enquque_biz(struct Customer *cus_info);
struct Queue* newqueue(int id);
void dequeue(struct Queue *queue);
struct Node* newnode(int id, int type, int t_arrival, int t_service);
// void enqueue(struct Queue *queue, struct Node *node);

#endif