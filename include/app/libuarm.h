/*
 * uARM
 *
 * Copyright (C) 2014 Marco Melletti
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef UARM_LIBUARM_H
#define UARM_LIBUARM_H

#include <stdint.h>

#define TIMER_INT_LINE 0x0001
#define UART0_INT_LINE 0x0002
#define UART1_INT_LINE 0x0003

/* prints a string *s 0-termianted on terminal 0 */
void tprint(char *s);

/* prints HALT message and terminates execution */
void HALT();

/* prints PANIC message and terminates execution */
void PANIC();

/* put the machine in idle state waiting for interrupts */
void WAIT();

/* loads processor state stored at address *addr */
void LDST(void *addr);

/* stores current processor state at address *addr */
void STST(void *addr);

unsigned int GETCOREID();

int CoreExecute(unsigned int core, void *task);

/* call kernel system call handler */
int SYSCALL(unsigned int sysNum, unsigned int arg1, unsigned int arg2, unsigned int arg3);

unsigned long get_us();
void         set_next_timer(uint64_t millis);

typedef struct _state_t {
    uint64_t general_purpose_registers[29];
    uint64_t frame_pointer;
    uint64_t link_register;
    uint64_t stack_pointer;
    uint64_t exception_link_register;
    uint64_t TTBR0;
    uint32_t status_register;
} state_t;

#endif     // UARM_LIBURAM_H