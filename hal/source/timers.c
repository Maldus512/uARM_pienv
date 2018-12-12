#include "timers.h"
#include "interrupts.h"
#include "asmlib.h"
#include "libuarm.h"
#include "system.h"


uint32_t armTimerFrequency = 0;

void enableCounter(void) {
    uint32_t cntv_ctl;
    cntv_ctl = 1;
    asm volatile("msr cntv_ctl_el0, %0" ::"r"(cntv_ctl));
}

void disableCounter(void) {
    uint32_t cntv_ctl;
    cntv_ctl = 0;
    asm volatile("msr cntv_ctl_el0, %0" ::"r"(cntv_ctl));
}

uint64_t readCounterCount(void) {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return (val);
}

uint32_t readCounterValue(void) {
    uint32_t val;
    asm volatile("mrs %0, cntv_tval_el0" : "=r"(val));
    return val;
}

void writeTimerValue(uint32_t val) {
    asm volatile("msr cntv_tval_el0, %0" ::"r"(val));
    return;
}

unsigned int setTimer(unsigned int timer) {
    writeTimerValue((armTimerFrequency / 10000) * timer);
    enableCounter();
    return 0;
}


uint64_t getMillisecondsSinceStart() {
    uint64_t timerCount = readCounterCount();
    armTimerFrequency   = GETARMCLKFRQ();
    return timerCount / (armTimerFrequency / 1000);
}

void initArmTimer() {
    armTimerFrequency = GETARMCLKFRQ();
    /* routing core0 counter to core0 irq */
    *(volatile uint32_t *)CORE0_TIMER_IRQCNTL = 0x08;
    // enableCounter();
}

void initTimers() {
    IRQ_CONTROLLER->Disable_Basic_IRQs = 0x1;
    ARMTIMER->LOAD                     = 0x800;     // 0xF4240;//0x400;         //
    ARMTIMER->RELOAD                   = 0x800;     // 0xF4240;//0x400;         //
    ARMTIMER->CONTROL                  = 0x003E0000;
    ARMTIMER->CONTROL |= (1 << 1);       // enable "23-bit" counter
    ARMTIMER->CONTROL |= (1 << 5);       // enable timer interrupt
    ARMTIMER->CONTROL |= (1 << 7);       // enable timer
    ARMTIMER->CONTROL |= (0x2 << 2);     // prescaler = clock/256timertimer
    ARMTIMER->IRQCLEAR                = 0;
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

void wait_msec(unsigned int n) { delay_us(n * 1000); }

void delay_us(uint32_t delay) {
    /**
     * Wait N microsec (ARM CPU only)
     */
    unsigned long f, t, r;
    // get the current counter frequency
    f = GETARMCLKFRQ();
    t = readCounterCount();
    //TODO: Fix the delay overflow thing
    t += ((f / 1000) * delay) / 1000;
    do {
        r = readCounterCount();
    } while (r < t);
}

void rawDelay() {
    int tim = 0;
    while (tim++ < 2000000)
        nop();
}