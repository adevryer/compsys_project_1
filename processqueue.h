#include "process.h"

#ifndef PROCESSQ_H
#define PROCESSQ_H


typedef struct node {
    struct node* next;
    Process_t* process;
} node_t;

typedef struct queue {
    node_t* head;
    int length;
} queue_t;

queue_t* create_queue();
node_t* create_node(Process_t* process);
void enqueue(queue_t* queue, Process_t* process);
Process_t* dequeue(queue_t* queue);
Process_t* requeue(queue_t* queue);
void print_queue(queue_t* queue);

#endif