#ifndef     __UART_H__
#define     __UART_H__

#include "hardwareprofile.h"

#define MU_RX_BUFFER_SIZE     1024

#define UART_BASE       (IO_BASE + 0x215000)

// TODO: mini uart register is not regular, having two 3-bit field at the beginning (shared with SPI)
//      does it make sense to have a structure or is it better to keep single defines?

#define AUX_IRQ         (*(volatile uint32_t *) (UART_BASE + 0x00))  // auxiliari interrupts
#define AUX_EN         (*(volatile uint32_t *)(UART_BASE + 0x04))    // auxiliary enables
#define MU_IO           (*(volatile uint32_t *)(UART_BASE + 0x40))    // I/O data
#define MU_IER           (*(volatile uint32_t *)(UART_BASE + 0x44))    // interrupt enable
#define MU_IIR           (*(volatile uint32_t *)(UART_BASE + 0x48))    // interrupt enable
#define MU_LCR          (*(volatile uint32_t *)(UART_BASE + 0x4c))    // line control register
#define MU_MCR          (*(volatile uint32_t *)(UART_BASE + 0x50))    // 
#define MU_LSR          (*(volatile uint32_t *)(UART_BASE + 0x54))    // line status register
#define MU_CNTL         (*(volatile uint32_t *)(UART_BASE + 0x60))    // extra control register
#define MU_BAUD         (*(volatile uint32_t *)(UART_BASE + 0x68))    // BAUDRATE register


extern volatile char RxBuffer[MU_RX_BUFFER_SIZE];
extern volatile int rx_head, rx_tail;


void initUart();
int uart_puts(const char *s);
void uart_putc(char c);
int flushRxBuffer();
void hexstring ( unsigned int d );

#endif