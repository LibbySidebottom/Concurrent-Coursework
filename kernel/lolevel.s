/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

/* Each of the following is a low-level interrupt handler: each one is
 * tasked with handling a different interrupt type, and acts as a sort
 * of wrapper around a high-level, C-based handler.
 */

.global lolevel_handler_rst
.global lolevel_handler_irq
.global lolevel_handler_svc

lolevel_handler_rst: bl    int_init                @ initialise interrupt vector table

                     msr   cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
                     ldr   sp, =tos_irq            @ initialise IRQ mode stack
                     msr   cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
                     ldr   sp, =tos_svc            @ initialise SVC mode stack

                     sub   sp, sp, #68             @ set up context storage space
                     mov   r0, sp                  @ pass context to hilevel handler

                     bl    hilevel_handler_rst     @ invoke high-level C function
                     pop   {r0, lr}                @ pop cpsr and pc(usr) and store in r0 and lr(svc)
                     msr   spsr, r0                @ move cpsr into spsr
                     ldmia sp, {r0-r12, sp, lr}^   @ pop rest of context into user mode registers
                     add   sp, sp, #60             @ manually reset stack pointer
                     movs  pc, lr                  @ change program counter and set cpsr = spsr (changes to user mode)

lolevel_handler_irq: sub   lr, lr, #4              @ correct return address
                     sub   sp, sp, #60             @ manually set stack pointer
                     stmia sp, {r0-r12, sp, lr}^   @ push user mode context into irq mode stack
                     mrs   r0, spsr                @ move spsr into r0
                     push  {r0, lr}                @ push spsr and link register into stack

                     mov   r0, sp                  @ pass context to hilevel handler
                     bl    hilevel_handler_irq     @ invoke high-level C function

                     pop   {r0, lr}                @ pop cpsr and pc(usr) and store in r0 and lr(svc)
                     msr   spsr, r0                @ move cpsr into spsr
                     ldmia sp, {r0-r12, sp, lr}^   @ pop rest of context into user mode registers
                     add   sp, sp, #60             @ manually reset stack pointer
                     movs  pc, lr                  @ return from interrupt

lolevel_handler_svc: sub   lr, lr, #0              @ correct return address
                     sub   sp, sp, #60             @ manually set stack pointer
                     stmia sp, {r0-r12, sp, lr}^   @ push user mode context into irq mode stack
                     mrs   r0, spsr                @ move spsr into r0
                     push  {r0, lr}                @ push spsr and link register into stack

                     mov   r0, sp                  @ pass context to hilevel handler
                     ldr   r1, [lr, #-4]           @ load svc instruction to r1
                     bic   r1, #0xFF000000         @ get svc operand
                     bl    hilevel_handler_svc     @ invoke high-level C function

                     pop   {r0, lr}                @ pop cpsr and pc(usr) and store in r0 and lr(svc)
                     msr   spsr, r0                @ move cpsr into spsr
                     ldmia sp, {r0-r12, sp, lr}^   @ pop rest of context into user mode registers
                     add   sp, sp, #60             @ manually reset stack pointer
                     movs  pc, lr                  @ return from interrupt
