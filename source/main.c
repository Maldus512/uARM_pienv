#include <string.h>
#include <stdlib.h>

#include "interrupts.h"
#include "hardwareprofile.h"
#include "gpio.h"
#include "timers.h"
#include "uart.h"



int bios_main(uint32_t r1, uint32_t r2, uint32_t atags) {
    char *test;
    initGpio();
    clearGpio(47);

    initTimers();
    initUart();

    test = malloc(sizeof(char)*16);
    strcpy(test, "beginning...");
    uart_puts(test);

    _enable_interrupts();

    while(1) {
        uart_puts("hello moto\n");
        delay_us(1000*1000);
    }
}