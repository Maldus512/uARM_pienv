#include "gpio.h"
#include "uart.h"
#include "mailbox.h"
#include "interrupts.h"

volatile int  rx_tail = 0;
volatile int  rx_head = 0;
volatile char RxBuffer[MU_RX_BUFFER_SIZE];

/**********************
 * Mini UART (UART 1) *
 **********************/

void initUart1(void) {
    int i;

    // IRQ_CONTROLLER->Disable_IRQs_1 = 1 << 29;

    AUX_EN |= 1; /* Enable mini-uart */

    MU_IER  = 0;
    MU_CNTL = 0;
    MU_LCR  = 3; /* 8 bit.  */
    MU_MCR  = 0;

    MU_IER = 0x5;
    MU_IIR = 0xC6;

    MU_BAUD = 270;                            /* 115200 baud.  */
    GPIO->SEL[1] &= ~((7 << 12) | (7 << 15)); /* GPIO14 & 15: alt5  */
    GPIO->SEL[1] |= (2 << 12) | (2 << 15);

    /* Disable pull-up/down.  */
    GPIO->PUDEN = 0;

    for (i = 0; i < 150; i++)
        nop();

    GPIO->PUDCLOCK0 = (2 << 14) | (2 << 15);

    for (i = 0; i < 150; i++)
        nop();

    GPIO->PUDCLOCK0 = 0;

    MU_CNTL = 3; /* Enable Tx and Rx.  */

    // IRQ_CONTROLLER->Enable_IRQs_1 = 1<<29;
}

char uart1_getc() {
    char r;
    /* wait until something is in the buffer */
    do {
        nop();
    } while (!(MU_LSR & 0x01));
    /* read it and return */
    r = (char)(MU_IO);
    /* convert carrige return to newline */
    return r == '\r' ? '\n' : r;
}


void uart1_send(char c) {
    while (!(MU_LSR & 0x20))
        nop();

    MU_IO = c;
}

void uart1_putc(char c) {
    if (c == '\n') {
        uart1_send('\r');
    }
    uart1_send(c);
}

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

void initUart0() {
    int i;

    /* initialize UART */
    UART0->CTRL = 0;

    /* set up clock for consistent divisor values */
    setUart0Baud();

    /* map UART0 to GPIO pins */
    GPIO->SEL[1] &= ~((7 << 12) | (7 << 15)); /* GPIO14 & 15: alt5  */
    GPIO->SEL[1] |= (4 << 12) | (4 << 15);

    /* Disable pull-up/down.  */
    GPIO->PUDEN = 0;

    for (i = 0; i < 150; i++)
        nop();

    GPIO->PUDCLOCK0 = (2 << 14) | (2 << 15);

    for (i = 0; i < 150; i++)
        nop();

    GPIO->PUDCLOCK0 = 0;

    UART0->IRQ_CLEAR = 0x7FF;
    UART0->IBRD      = 2;
    UART0->FBRD      = 0xB;
    UART0->LINE_CTRL = 0x3 << 5;
    UART0->CTRL      = 0x301;
}

void uart0_putc(char c) {
    /* wait until we can send */
    do {
        nop();
    } while (UART0->FLAG & 0x20);
    /* write the character to the buffer */
    UART0->DATA = c;
}

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

void uart0_puts(char *s) {
    while (*s) {
        /* convert newline to carrige return + newline */
        if (*s == '\n')
            uart0_putc('\r');
        uart0_putc(*s++);
    }
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int          c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart0_putc(n);
    }
}

void hexstrings(unsigned int d) {
    // unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    rb = 32;
    while (1) {
        rb -= 4;
        rc = (d >> rb) & 0xF;
        if (rc > 9)
            rc += 0x37;
        else
            rc += 0x30;
        uart0_putc(rc);
        if (rb == 0)
            break;
    }
    uart0_putc(0x20);
}

void hexstring(unsigned int d) {
    hexstrings(d);
    uart0_putc(0x0D);
    uart0_putc(0x0A);
}

void uart_dump(void *ptr) {
    unsigned long a, b, d;
    unsigned char c;
    for (a = (unsigned long)ptr; a < (unsigned long)ptr + 512; a += 16) {
        hexstrings(a);
        uart0_puts(": ");
        for (b = 0; b < 16; b++) {
            c = *((unsigned char *)(a + b));
            d = (unsigned int)c;
            d >>= 4;
            d &= 0xF;
            d += d > 9 ? 0x37 : 0x30;
            uart0_putc(d);
            d = (unsigned int)c;
            d &= 0xF;
            d += d > 9 ? 0x37 : 0x30;
            uart0_putc(d);
            uart0_putc(' ');
            if (b % 4 == 3)
                uart0_putc(' ');
        }
        for (b = 0; b < 16; b++) {
            c = *((unsigned char *)(a + b));
            uart0_putc(c < 32 || c >= 127 ? '.' : c);
        }
        uart0_putc('\r');
        uart0_putc('\n');
    }
}

void startUart0Int() {
    // enable UART RX interrupt.
    UART0->IRQ_MASK = 1 << 4;
    // enable UART TX interrupt.
    //    UART0->IRQ_MASK |= 1 << 5;

    // UART interrupt routing.
    IRQ_CONTROLLER->Enable_IRQs_2 |= 1 << 25;

    // IRQ routeing to CORE0.
    *GPU_INTERRUPTS_ROUTING = 0x00;
}