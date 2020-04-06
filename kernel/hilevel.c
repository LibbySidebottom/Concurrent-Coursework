/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

/* Have: interrupt vector table and initialiser [int.s], basic lolevel_handler_[rst, irq, svc]
   Need: print, process table, context (state), scheduler
   Stage 1a: P3 and P4 with preemptive multitasking (round robin scheduling)
            make timer, called via irq (for pre-organised interrupts)
            batch system
*/

// Make process table and ready queue
pcb_t* p_table[18];
int pcb_id = 0;
pcb_t* running = NULL;
queue* ready_queues[MAX_PRIORITY + 1];

uint32_t get_stack (int id) {
    return ((uint32_t) &process_stacks) - id * 0x1000;
}

pcb_t* create_pcb (uint32_t entry_point) {
    pcb_t* new_pcb = (pcb_t*) malloc(sizeof(pcb_t));
    new_pcb->state = CREATED;
    new_pcb->pid = pcb_id++;
    new_pcb->tos = get_stack(new_pcb->pid);
    new_pcb->priority = MAX_PRIORITY;
    new_pcb->slices = 1;
    new_pcb->ctx.sp = new_pcb->tos;
    new_pcb->ctx.lr = 0;
    new_pcb->ctx.pc = entry_point;
    new_pcb->ctx.cpsr = 0x50;
    for (int i = 0 ; i < 13 ; i++) {
        new_pcb->ctx.gpr[i] = 0;
    }
    return new_pcb;
}

void calc_slices (pcb_t* pcb) {
    pcb->slices = MAX_PRIORITY - pcb->priority + 1;
}

void load_pcb (pcb_t* new_pcb) {

    p_table[new_pcb->pid] = new_pcb;

    // Push pcb to ready queue
    push_queue (new_pcb, ready_queues[MAX_PRIORITY - new_pcb->priority]);
}

// Make dispatch function to run next process
void dispatch (pcb_t* next_process, context* current_context) {
    // Save current context
    if (running != NULL) {
        memcpy(&running->ctx, current_context, sizeof(context));
    }
    // Start next process
    running = next_process;
    memcpy(current_context, &running->ctx, sizeof(context));
    running->state = RUNNING;
}

void scheduler (context* current_context) {
    // Push pcb to correct ready queue
    if (running != NULL) {
        calc_slices(running);
        push_queue(running, ready_queues[MAX_PRIORITY - running->priority]);
        running->state = READY;
    }

    // Pop pcb off ready queue in order of priorities
    for (int i = 0 ; i < MAX_PRIORITY + 1 ; i++) {
        if (ready_queues[i]->head != NULL) {
            pcb_t* next_process = pop_queue(ready_queues[i]);
            dispatch(next_process, current_context);
            return;
        }
    }
}

void hilevel_handler_rst(context* current_context) {

    // Make timer which counts 1 second then sends an interrupt
    TIMER0->Timer1Load = 0x1000;
    TIMER0->Timer1Ctrl = 0xE2;
    GICD0->ISENABLER1 |= 0x10;
    GICD0->CTLR = 0x1;
    GICC0->PMR = 0xF0;
    GICC0->CTLR = 0x1;
    int_enable_irq();

    // Create ready queues
    for (int i = 0 ; i < MAX_PRIORITY + 1 ; i++) {
        ready_queues[i] = create_queue();
    }

    // Load console into process table
    load_pcb(create_pcb((uint32_t) &main_console));
    scheduler(current_context);
}

void hilevel_handler_irq(context* current_context) {

    // Handle interrupt
    uint32_t id = GICC0->IAR;
    if (id == GIC_SOURCE_TIMER0) {
        // PL011_putc(UART0, 'T', true);
        running->slices--;
        if (running->slices == 0) {
            running->priority = running->priority == 0 ? 0 : running->priority - 1;
            scheduler(current_context);
        }
        TIMER0->Timer1IntClr = 0x1;
    }
    GICC0->EOIR = id;
}

void hilevel_handler_svc(context* current_context, int id) {
        switch (id) {
            // Write
            case 0x01: {
                int n = current_context->gpr[2];
                char* x = (char*) current_context->gpr[1];
                for (int i = 0 ; i < n ; i++) {
                    PL011_putc(UART0, *x++, true);
                }
                break;
            }
            // Fork
            case 0x03: {
                pcb_t* child = create_pcb(current_context->pc);
                memcpy(&child->ctx, current_context, sizeof(context));
                child->ctx.sp = child->tos - (running->tos - running->ctx.sp);
                memcpy((uint32_t*) child->ctx.sp, (uint32_t*) running->ctx.sp, running->tos - running->ctx.sp);
                child->ctx.gpr[0] = 0;
                current_context->gpr[0] = child->pid;
                load_pcb(child);
                break;
            }
            // Exit (self termination)
            case 0x04: {
                for (int i = 0 ; i < 18 ; i++) {
                    if (p_table[i] == NULL) {
                        continue;
                    }
                    if (p_table[i]->pid == running->pid) {
                        p_table[i] = NULL;
                        running = NULL;
                        scheduler(current_context);
                        break;
                    }
                }
                break;
            }
            // Exec
            case 0x05: {
                current_context->sp = running->tos;
                current_context->pc = current_context->gpr[0];
                current_context->lr = 0;
                for (int i = 0 ; i < 13 ; i++) {
                    current_context->gpr[i] = 0;
                }
                break;
            }
            // Kill
            case 0x06: {
                uint32_t id = current_context->gpr[0];
                uint32_t sig = current_context->gpr[1];
                switch (sig) {
                    case 0x01:
                    case 0x00: {
                        for (int i = 0 ; i < 18 ; i++) {
                            if (p_table[i]->pid == id) {
                                delete_queue(ready_queues[MAX_PRIORITY - p_table[i]->priority], id);
                                p_table[i] = NULL;
                                scheduler(current_context);
                                break;
                            }
                        }
                    }
                }
                break;
            }
            // Initialise semaphore
            case 0x08: {
                uint32_t* sem = (uint32_t*) malloc(sizeof(uint32_t));
                *sem = current_context->gpr[0];
                current_context->gpr[0] = (uint32_t) sem;
                break;
            }
            // Close semaphore
            case 0x09: {
                free((uint32_t*) current_context->gpr[0]);
                break;
            }
        }
}
