#ifndef __QUEUE_H
#define __QUEUE_H

// Include functionality relating to newlib (the standard C library).
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Make process control block
typedef enum {
    CREATED, READY, RUNNING, WAITING, TERMINATED
} status;

typedef struct {
    uint32_t cpsr;
    uint32_t pc;
    uint32_t gpr[13];
    uint32_t sp;
    uint32_t lr;
} context;

typedef struct {
    int pid;
    status state;
    context ctx;
    uint32_t tos;
    int priority;
    int slices;
} pcb_t;

typedef struct node {
    pcb_t* val;
    struct node* link;
} node;

typedef struct {
    node* head;
    node* tail;
} queue;

queue* create_queue();
void push_queue (pcb_t* val, queue* q);
pcb_t* pop_queue (queue* q);
void delete_queue (queue* q, int id);

#endif
