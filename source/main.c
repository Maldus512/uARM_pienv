#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hardwareprofile.h"
#include "mailbox.h"
#include "gpio.h"
#include "uart.h"
#include "timers.h"
#include "interrupts.h"




void initSystem() {
    initGpio();
    initUart();
    initTimers();
    _enable_interrupts();
    uart_puts("************************************\n");
    uart_puts("*        MaldOS running...         *\n");
    uart_puts("************************************\n");
}

extern void PANIC();
extern void WAIT();

extern char _end;

int bios_main(uint32_t r1, uint32_t r2, uint32_t atags) {
    initSystem();

    char hex[10];
    memset(hex, 0, 10);

    sprintf(hex, "%x",(unsigned int) &_end); 
    uart_puts(hex);
    uart_putc('\n');

    //TODO malloc is still not working properly: returns address 8, I'm overwriting the exception vector
    char *example = (char*) malloc(sizeof(char)*32);
    sprintf(hex, "%x",(unsigned int) example); 
    uart_puts(hex);
    uart_putc('\n');
    strcpy(example, "hello world\n");
    uart_puts(example);

    //while(1) {
        //flushRxBuffer();
    //}
    PANIC();

    while(1) {
        //delay_us(1000000);
        WAIT();
        led(1);

        //delay_us(1000000);
        WAIT();
        led(0);
    }
}