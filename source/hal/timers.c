#include "timers.h"
#include "interrupts.h"
#include "asmlib.h"
#include "libuarm.h"
#include "system.h"


void enable_physical_counter(void) {
    uint32_t cnt_ctl = 1;
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(cnt_ctl));
}

void disable_physical_counter(void) {
    uint32_t cnt_ctl = 0;
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(cnt_ctl));
}

uint64_t read_physical_counter(void) {
    uint64_t val;
    asm volatile("mrs %0, cntpct_el0" : "=r"(val));
    return (val);
}


void write_physical_timer_value(uint32_t val) {
    asm volatile("msr cntp_tval_el0, %0" ::"r"(val));
    return;
}

unsigned int set_physical_timer(unsigned int timer) {
    uint64_t tmp1  = (uint64_t)GETARMCLKFRQ();
    uint64_t value = (tmp1 * timer) / 1000000;
    if (value > 0xFFFFFFFFUL) {
        return -1;
    }
    write_physical_timer_value((uint32_t)value);
    enable_physical_counter();
    return 0;
}

uint64_t getTOD() {
    uint32_t armTimerFrequency = 0;
    uint64_t timerCount        = read_physical_counter();
    armTimerFrequency          = GETARMCLKFRQ();
    return (timerCount * 1000 * 1000) / armTimerFrequency;
}

/*
 * Initialize the internal ARM timer interrupts. Route the physical EL1 timer
 * to IRQ and the virtual EL1 timer to FIQ
 */
void init_arm_timer_interrupt() {
    GIC->Core0_Timers_Interrupt_Control = 0x82;
    GIC->Core1_Timers_Interrupt_Control = 0x82;
    GIC->Core2_Timers_Interrupt_Control = 0x82;
    GIC->Core3_Timers_Interrupt_Control = 0x82;
}

void enable_virtual_counter(void) {
    uint32_t cnt_ctl = 1;
    asm volatile("msr cntv_ctl_el0, %0" ::"r"(cnt_ctl));
}

void disable_virtual_counter(void) {
    uint32_t cnt_ctl = 0;
    asm volatile("msr cntv_ctl_el0, %0" ::"r"(cnt_ctl));
}

void write_virtual_timer_value(uint32_t val) {
    asm volatile("msr cntv_tval_el0, %0" ::"r"(val));
    return;
}

unsigned int set_virtual_timer(unsigned int timer) {
    uint64_t tmp1  = (uint64_t)GETARMCLKFRQ();
    uint64_t value = (tmp1 * timer) / 1000000;
    if (value > 0xFFFFFFFFUL) {
        return -1;
    }
    write_virtual_timer_value((uint32_t)value);
    enable_virtual_counter();
    return 0;
}

void wait_msec(unsigned int n) { delay_us(n * 1000); }

void delay_us(uint32_t delay) {
    volatile unsigned long timestamp = getTOD();
    volatile unsigned long end       = timestamp + delay;

    while ((unsigned long)timestamp < (unsigned long)(end)) {
        timestamp = getTOD();
    }
}

void raw_delay() {
    int tim = 0;
    while (tim++ < 2000000)
        nop();
}