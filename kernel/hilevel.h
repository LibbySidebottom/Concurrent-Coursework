/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H
#define MAX_PRIORITY (1)

// Include functionality relating to newlib (the standard C library).
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Include functionality relating to the kernel.
#include "lolevel.h"
#include "int.h"
#include "queue.h"

// Include timer, interrupt controller, UART
#include "SP804.h"
#include "GIC.h"
#include "PL011.h"

// Declare tos and entry point variables
extern uint32_t process_stacks;
extern void* main_console;

#endif
