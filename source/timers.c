#include "timers.h"
#include "interrupts.h"
#include "asmlib.h"
#include "libuarm.h"
#include "system.h"


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
    uint64_t tmp1  = (uint64_t)GETARMCLKFRQ();
    uint64_t value = (tmp1 * timer) / 1000000;
    writeTimerValue((uint32_t)value);
    enableCounter();
    return 0;
}


uint64_t get_ms() {
    uint32_t armTimerFrequency = 0;
    uint64_t timerCount        = readCounterCount();
    armTimerFrequency          = GETARMCLKFRQ();
    return timerCount / (armTimerFrequency / 1000);
}

uint64_t get_us() {
    uint32_t armTimerFrequency = 0;
    uint64_t timerCount        = readCounterCount();
    armTimerFrequency          = GETARMCLKFRQ();
    return (timerCount * 1000 * 1000) / armTimerFrequency;
}

void initArmTimer() {
    GIC->Core0_Timers_Interrupt_Control = 0x08;
    GIC->Core1_Timers_Interrupt_Control = 0x08;
    GIC->Core2_Timers_Interrupt_Control = 0x08;
    GIC->Core3_Timers_Interrupt_Control = 0x08;
}

void wait_msec(unsigned int n) { delay_us(n * 1000); }

void delay_us(uint32_t delay) {
    volatile unsigned long timestamp = get_us();
    volatile unsigned long end       = timestamp + delay;

    while ((unsigned long)timestamp < (unsigned long)(end)) {
        timestamp = get_us();
    }
}

void rawDelay() {
    int tim = 0;
    while (tim++ < 2000000)
        nop();
}