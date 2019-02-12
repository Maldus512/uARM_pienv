/*
 * Hardware Abstraction Layer for Raspberry Pi 3
 *
 * Copyright (C) 2018 Mattia Maldini
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

/******************************************************************************
 * This module contains functions to use the two serial interfaces on the BCM2837
 ******************************************************************************/

#include "gpio.h"
#include "uart.h"
#include "mailbox.h"
#include "interrupts.h"
#include "utils.h"
#include "timers.h"

/**********************
 * Mini UART (UART 1) *
 **********************/

/*
 * Initializes mini UART (aka UART1)
 */
void init_uart1(void) {
    // IRQ_CONTROLLER->Disable_IRQs_1 = 1 << 29;

    AUX_EN |= 1; /* Enable mini-uart */

    UART1->IER  = 0;
    UART1->CNTL = 0;
    UART1->LCR  = 3; /* 8 bit.  */
    UART1->MCR  = 0;

    UART1->IER = 0x0;
    UART1->IIR = 0xC6;

    UART1->BAUD = 270; /* 115200 baud.  */
    setupGpio(14, GPIO_ALTFUNC5);
    setupGpio(15, GPIO_ALTFUNC5);

    /* Disable pull-up/down.  */
    setPullUpDown(14, GPIO_PUD_DISABLE);
    setPullUpDown(15, GPIO_PUD_DISABLE);

    UART1->CNTL = 3; /* Enable Tx and Rx.  */

    IRQ_CONTROLLER->Enable_IRQs_1 |= 1<<29;
}

/*
 * Receives 1 character from UART1 (blocking)
 */
char uart1_getc() {
    char r;
    /* wait until something is in the buffer */
    do {
        nop();
    } while (!(UART1->LSR & 0x01));
    /* read it and return */
    r = (char)(UART1->IO);
    /* convert carrige return to newline */
    return r == '\r' ? '\n' : r;
}

/*
 * Sends 1 character to UART1
 */
void uart1_send(char c) {
    while (!(UART1->LSR & 0x20))
        nop();

    UART1->IO = c;
}

/*
 * Wrapper around uart1_send, adds a \r character for every \n
 */
void uart1_putc(char c) {
    if (c == '\n') {
        uart1_send('\r');
    }
    uart1_send(c);
}

/*
 * Writes a null terminated string to UART1
 */
void uart1_puts(char *s) {
    int i = 0;
    while (*s) {
        uart1_putc(*s++);
        i++;
    }
}

/************
 *  UART 0  *
 ************/

/*
 * Initializes UART0
 */
void init_uart0() {
    /* initialize UART */
    UART0->CTRL = 0;

    /* set up clock for consistent divisor values */
    set_uart0_baud();

    /* map UART0 to GPIO pins */
    setupGpio(14, GPIO_ALTFUNC0);
    setupGpio(15, GPIO_ALTFUNC0);

    setPullUpDown(14, GPIO_PUD_DISABLE);
    setPullUpDown(15, GPIO_PUD_DISABLE);

    UART0->IRQ_CLEAR = 0x7FF;
    UART0->IBRD      = 2;
    UART0->FBRD      = 0xB;
    UART0->LINE_CTRL = 0x3 << 5;
    UART0->CTRL      = 0x301;
}

/*
 * Sends 1 character to UART0
 */
void uart0_putc(char c) {
    /* wait until we can send */
    do {
        nop();
    } while (UART0->FLAG & 0x20);
    /* write the character to the buffer */
    UART0->DATA = c;
}

/*
 * Receives 1 character from UART0 (blocking)
 */
char uart0_getc() {
    char r;
    /* wait until something is in the buffer */
    do {
        nop();
    } while (UART0->FLAG & 0x10);
    /* read it and return */
    r = (char)(UART0->DATA);
    /* convert carrige return to newline */
    return r == '\r' ? '\n' : r;
}

/*
 * Writes a null terminated string to UART0
 */
void uart0_puts(char *s) {
    while (*s) {
        /* convert newline to carrige return + newline */
        if (*s == '\n')
            uart0_putc('\r');
        uart0_putc(*s++);
    }
}

/*
 * Enables uart interrupts
 */
void enable_uart_interrupts() {
    // enable UART RX interrupt.
    UART0->IRQ_MASK = 1 << 4;
    // enable UART TX interrupt.
    UART0->IRQ_MASK |= 1 << 5;

    // UART interrupt routing.
    IRQ_CONTROLLER->Enable_IRQs_2 |= 1 << 25;

    // IRQ routeing to CORE0.
    GIC->GPU_Interrupts_Routing = 0x00;
}

/*
 * Prints a message with a specified loglevel and a timestamp
 */
void logprint(LOGLEVEL lvl, char *msg) {
    uint64_t timer = getTOD();
    int      len = 0, i = 1;
    char     string[256];
    char     tmp[32];

    if (lvl < VERBOSITY)
        return;

    string[0] = '[';
    itoa(timer, tmp, 10);

    while (tmp[len] != '\0')
        len++;
    for (i = 1; i < 14 - len + 1; i++)
        string[i] = '0';
    memcpy(&string[i], tmp, len);
    string[15] = ']';

    len = 0;
    while (msg[len] != '\0')
        len++;

    switch (lvl) {
        case INFO:
            memcpy(&string[16], " [INFO]  ", 9);
            break;
        case WARN:
            memcpy(&string[16], " [WARN]  ", 9);
            break;
        case ERROR:
            memcpy(&string[16], " [ERROR] ", 9);
            break;
    }
    memcpy(&string[25], msg, len + 1);
    string[strlen(string)+1] = '\0';
    string[strlen(string)] = '\n';
    uart0_puts(string);
}