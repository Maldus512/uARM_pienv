#include "timers.h"
#include "interrupts.h"
#include "asmlib.h"
#include "libuarm.h"
#include "libuarmv2.h"

void initTimers() {
    IRQ_CONTROLLER->Disable_Basic_IRQs = 0x1;
    ARMTIMER->LOAD = 0x800;//0xF4240;//0x400;         // 
    ARMTIMER->RELOAD = 0x800;//0xF4240;//0x400;         // 
    ARMTIMER->CONTROL = 0x003E0000;
    ARMTIMER->CONTROL |= (1 << 1); // enable "23-bit" counter
    ARMTIMER->CONTROL |= (1 << 5); // enable timer interrupt
    ARMTIMER->CONTROL |= (1 << 7); // enable timer
    ARMTIMER->CONTROL |= (0x2 << 2); // prescaler = clock/256timertimer
    ARMTIMER->IRQCLEAR = 0;
    IRQ_CONTROLLER->Enable_Basic_IRQs = 0x1;
}

int set_timer(uint32_t delay, uint8_t timer) {
    if (timer < 0 || timer > 3) {
        return -1;
    }

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
    /**
     * Wait N microsec (ARM CPU only)
     */
    unsigned long f, t, r;
    // get the current counter frequency
    f = SYSCALL(SYS_GETARMCLKFRQ,0,0,0);
    t = SYSCALL(SYS_GETARMCOUNTER,0,0,0);
    t+=((f/1000)*delay)/1000;
    do {
        r = SYSCALL(SYS_GETARMCOUNTER,0,0,0);
    } while(r<t);
}

void rawDelay() {
    int tim = 0;
    while(tim++ < 2000000)
        nop();
}