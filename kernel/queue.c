#include "queue.h"

queue* create_queue() {
    queue* new_queue = (queue*) (malloc(sizeof(queue)));
    new_queue->head = NULL;
    new_queue->tail = NULL;
    return new_queue;
}

void push_queue (pcb_t* val, queue* q) {
    node* new_node = (node*) (malloc(sizeof(node)));
    new_node->link = NULL;
    new_node->val = val;
    if (q->head == NULL) {
        q->head = new_node;
        q->tail = new_node;
    } else {
        q->tail->link = new_node;
        q->tail = new_node;
    }
}

pcb_t* pop_queue (queue* q) {
    node* temp = q->head;
    pcb_t* pcb = q->head->val;
    q->head = q->head->link;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    free(temp);
    return pcb;
}

void delete_queue (queue* q, int id) {
    node* current_node = q->head;
    node* prev_node = NULL;
    if (current_node == NULL) {
        return;
    }
    while (1) {
        if(current_node->val->pid == id) {
            if (prev_node == NULL) {
                q->head = current_node->link;
            } else {
                prev_node->link = current_node->link;
            }
            if (current_node == q->tail) {
                q->tail = prev_node;
            }
            current_node = NULL;
            free(current_node);
        }
        prev_node = current_node;
        current_node = current_node->link;
        if (current_node == NULL) {
            return;
        }
    }
}
