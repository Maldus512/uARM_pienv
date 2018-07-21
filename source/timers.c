#include "timers.h"
//#include "interrupts.h"

void initTimers() {
//    IRQ_CONTROLLER->Enable_Basic_IRQs = 0x1;
    ARMTIMER->LOAD = 0xF4240;//0x400;         // 
    ARMTIMER->CONTROL |= (0xF9 << 16);
    ARMTIMER->CONTROL |= (1 << 1); // enable "23-bit" counter
//    ARMTIMER->CONTROL |= (1 << 5); // enable timer interrupt
    ARMTIMER->CONTROL |= (1 << 7); // enable timer
    ARMTIMER->CONTROL |= (1 << 9); // enable free running counter
    ARMTIMER->CONTROL |= 0;//(0x2 << 2); // prescaler = clock/256timertimer
}

int set_timer(uint32_t delay, uint8_t timer) {
    if (timer < 0 || timer > 3) {
        return -1;
    }

    /* Reset status register */
    SYSTIMER->STATUS |= 1 << timer;
    SYSTIMER->COMPARE[timer] = SYSTIMER->COUNTER_LOW + delay;
    return timer;
}

int is_timer_reached(uint8_t timer) {
    if (timer < 0 || timer > 3) {
        return -1;
    }

    return (SYSTIMER->STATUS & (1 << timer));
}

void delay_us(uint32_t delay) {
    set_timer(delay, 0);

    while ( !is_timer_reached(0) )
        nop();
}

void delay_us_arm(uint32_t delay) {
    uint32_t begin = ARMTIMER->COUNTER;
    while(ARMTIMER->COUNTER -begin < delay);
}


void rawDelay() {
    int tim = 0;
    while(tim++ < 2000000)
    while(tim++ < 500000)
        nop();
}