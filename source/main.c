#include <string.h>
#include <stdlib.h>

#include "interrupts.h"
#include "hardwareprofile.h"
#include "gpio.h"
#include "timers.h"
#include "uart.h"
#include "libuarm.h"
#include "asmlib.h"



int bios_main(uint32_t r1, uint32_t r2, uint32_t atags) {
    char *test;
    initGpio();
    clearGpio(47);

    initTimers();
    initUart();

    uart_putc('\n');
    SYSCALL(0xAB, 0,0,0);

    test = malloc(sizeof(char)*16);
    strcpy(test, "beginning...");
    uart_puts(test);


    #ifdef APP
    main();
    #endif


    while(1) {
        flushRxBuffer();
    }
    return 0;
}