#include "hardwareprofile.h"
#include "mailbox.h"
#include "gpio.h"
#include "uart.h"


void initSystem() {
    initGpio();
    initUart();
}


int main(void) {
    volatile uint32_t tim;

    initSystem();


    while(1)
    {
        puts("hello world!\n");

        for(tim = 0; tim < 500000; tim++)
            nop();

        setGpio(4);
        led(1);

        for(tim = 0; tim < 500000; tim++)
            nop();

        clearGpio(4);
        led(0);
    }
}