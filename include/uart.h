#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>
#include "arch.h"

#define MU_RX_BUFFER_SIZE 1024

#define UART1_BASE (IO_BASE + 0x00215000)
#define UART0_BASE (IO_BASE + 0x00201000)

#define AUX_IRQ (*(volatile uint32_t *)(UART1_BASE + 0x00))     // auxiliary interrupts
#define AUX_EN (*(volatile uint32_t *)(UART1_BASE + 0x04))      // auxiliary enables

struct UART1REG {
    volatile uint32_t IO;
    volatile uint32_t IER;
    volatile uint32_t IIR;
    volatile uint32_t LCR;
    volatile uint32_t MCR;
    volatile uint32_t LSR;
    volatile uint32_t MSR;
    volatile uint32_t SCRATCH;
    volatile uint32_t CNTL;
    volatile uint32_t STAT;
    volatile uint32_t BAUD;
};

struct UART0REG {
    volatile uint32_t DATA;     // DR
    volatile uint32_t RSRECR;
    uint32_t          _pad0[4];
    volatile uint32_t FLAG;     // FR
    uint32_t          _pad1[2];
    volatile uint32_t IBRD;           // IBRD
    volatile uint32_t FBRD;           // FBRD
    volatile uint32_t LINE_CTRL;      // LCRH
    volatile uint32_t CTRL;           // CR
    volatile uint32_t IFLS;           // IFLS
    volatile uint32_t IRQ_MASK;       // IMSC
    volatile uint32_t RAW_IRQ;        // RIS
    volatile uint32_t MASKED_IRQ;     // MIS
    volatile uint32_t IRQ_CLEAR;      // ICR
};

typedef enum {
    INFO,
    WARN,
    ERROR,
    NONE,
} LOGLEVEL;


#define UART0 ((struct UART0REG *)UART0_BASE)
#define UART1 ((struct UART1REG *)(UART1_BASE+0x40))


#ifndef VERBOSITY
    #define VERBOSITY     INFO
#endif

#define LOG(x, y)     logprint(x,y)


void init_uart1();
void uart1_puts(char *s);
void uart1_putc(char c);
char uart1_getc();

void init_uart0();
void uart0_puts(char *s);
void uart0_putc(char c);
char uart0_getc();
void uart_hex(unsigned int d);
void hexstrings(unsigned int d);
void hexstring(unsigned int d);
void startUart0Int();
void logprint(LOGLEVEL lvl, char* msg);

#endif
