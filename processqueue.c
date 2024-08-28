#include <stdio.h> 
#include <stdlib.h> 
#include <assert.h>
#include "process.h"
#include "processqueue.h"

queue_t* create_queue() {
    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    assert(queue);
    queue->length = 0;
    queue->head = NULL;
    return queue;
}

node_t* create_node(Process_t* process) {
    node_t* node = (node_t*) malloc(sizeof(node_t));
    assert(node);
    node->next = NULL;
    node->process = process;
    //printf("PROCESS ID: %s\n", process->PID);
    return node;
}

// add node to the end of the queue
void enqueue(queue_t* queue, Process_t* process) {
    node_t* process_node = create_node(process);
    if (queue->head == NULL) {
        queue->head = process_node;
        queue->length += 1;
        //printf("HEAD OF QUEUE IS: %s\n", process->PID);
        return;
    }
    node_t* curr = queue->head;
    while (curr->next != NULL) {
        curr = curr->next; // go to end of queue
    }
    curr->next = process_node;
    process_node->process->state = READY;
    queue->length += 1;
    return;
}

// pop the node highest in the queue and return it
Process_t* dequeue(queue_t* queue) {
    node_t* top = queue->head;
    if (top == NULL) {
        return NULL;
    }
    Process_t* process = top->process;
    queue->head = top->next;
    queue->length -= 1;
    process->state = FINISHED;
    free(top);
    return process;
}

// return the highest node's data and then move it back to the end of the queue
Process_t* requeue(queue_t* queue) {
    node_t* top = queue->head;
    if (top == NULL) {
        return NULL;
    }
    if (top->next == NULL) {
        return top->process;
    }
    queue->head = top->next;
    node_t* curr = queue->head;
    while (curr->next != NULL) {
        curr = curr->next; // go to end of queue
    }
    curr->next = top;
    top->process->state = READY;
    top->next = NULL;

    return top->process;
}

void print_queue(queue_t* queue) {
    node_t* curr = queue->head;
    int queue_pos = 0;
    while(curr != NULL) {
        printf("Queue Pos:%d, PID:%s\n\tState:%d\n\tDuration:%u\n\tAllocated:%d\n", 
        queue_pos, curr->process->PID, curr->process->state, curr->process->duration, curr->process->allocated);
        queue_pos += 1;
        curr = curr->next;
    }
}
