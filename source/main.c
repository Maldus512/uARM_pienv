#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hardwareprofile.h"
#include "mailbox.h"
#include "gpio.h"
#include "uart.h"
#include "timers.h"
#include "interrupts.h"
#include "libuarm.h"




void initSystem() {
    initGpio();
    initUart();
    initTimers();
    _enable_interrupts();
    uart_puts("************************************\n");
    uart_puts("*        MaldOS running...         *\n");
    uart_puts("************************************\n");
}


int bios_main(uint32_t r1, uint32_t r2, uint32_t atags) {
    initSystem();

    char *example = (char*) malloc(sizeof(char)*32);
    strcpy(example, "hello world\n");
    uart_puts(example);

    //while(1) {
        //flushRxBuffer();
    //}

    while(1) {
        //delay_us(1000000);
        WAIT();
        led(1);

        //delay_us(1000000);
        WAIT();
        led(0);
    }
}