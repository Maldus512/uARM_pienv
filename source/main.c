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
}


int main(uint32_t r1, uint32_t r2, uint32_t atags) {
    initSystem();
    puts("hello world!\n");

    while(1) {
        delay_us(1000000);
        led(1);

        delay_us(1000000);
        led(0);
    }
}