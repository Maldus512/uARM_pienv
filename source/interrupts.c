#include "interrupts.h"
#include "gpio.h"
#include "timers.h"

void stub_vector() {
    while(1) {
        nop();
    }
}


void  __attribute__((interrupt("IRQ")))  interrupt_vector() {
    static uint8_t led = 0;
    ARMTIMER->IRQCLEAR = 1;
    if (led) {
        setGpio(4);
    } else {
        clearGpio(4);
    }
    led = 1-led;
}