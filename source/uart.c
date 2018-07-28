#include "gpio.h"
#include "uart.h"
#include "interrupts.h"

volatile int rx_tail = 0;
volatile int rx_head = 0;
volatile char RxBuffer[MU_RX_BUFFER_SIZE];

void initUart (void) {
    int i;

    IRQ_CONTROLLER->Disable_IRQs_1 = 1 << 29;

    AUX_EN |= 1;		/* Enable mini-uart */

    MU_IER = 0;
    MU_CNTL = 0;
    MU_LCR = 3;		/* 8 bit.  */
    MU_MCR = 0;

    MU_IER = 0x5;
    MU_IIR = 0xC6;

    MU_BAUD = 270;	/* 115200 baud.  */
    setupGpio(14, GPIO_ALTFUNC5);
    setupGpio(15, GPIO_ALTFUNC5);
    //GPIO->SEL[1] &= ~((7 << 12) | (7 << 15));	/* GPIO14 & 15: alt5  */
    //GPIO->SEL[1] |= (2 << 12) | (2 << 15);

    /* Disable pull-up/down.  */
    GPIO->PUDEN = 0;

    for (i = 0; i < 150; i++)
        nop();

    GPIO->PUDCLOCK0 = (1 << 14) | (1 << 15);

    for (i = 0; i < 150; i++)
        nop();

    GPIO->PUDCLOCK0 = 0;

    MU_CNTL = 3;		/* Enable Tx and Rx.  */

    IRQ_CONTROLLER->Enable_IRQs_1 = 1<<29;
}



void raw_putc (char c) {
    while (!(MU_LSR & 0x20))
        nop();

    MU_IO = c;
}

void uart_putc (char c) {
    if (c == '\n') {
        raw_putc ('\r');
    }
    raw_putc (c);
}

int uart_puts (const char *s) {
    int i = 0;
    while (*s) {
        uart_putc (*s++);
        i++;
    }

    return i;
}

void tprint(char *s) {
    uart_puts(s);
}


void hexstrings ( unsigned int d )
{
    unsigned int rb;
    unsigned int rc;

    rb=32;
    while(1)
    {
        rb-=4;
        rc=(d>>rb)&0xF;
        if(rc>9) rc+=0x37; else rc+=0x30;
        raw_putc(rc);
        if(rb==0) break;
    }
    raw_putc(0x20);
}

void hexstring ( unsigned int d )
{
    hexstrings(d);
    raw_putc(0x0D);
    raw_putc(0x0A);
}

int flushRxBuffer() {
    int rx = 0;
    while (rx_tail != rx_head) {
        uart_putc(RxBuffer[rx_tail++]);
        rx_tail = rx_tail % MU_RX_BUFFER_SIZE;
        rx++;
    }
    return rx;
}