/*
 * Hardware Abstraction Layer for Raspberry Pi 3
 *
 * Copyright (C) 2018 Mattia Maldini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/******************************************************************************
 * This module contains functions for ARM timer interfacing
 ******************************************************************************/

#include "timers.h"
#include "interrupts.h"
#include "asmlib.h"
#include "libuarm.h"
#include "system.h"

/*
 * Activates the physical counter of the ARM timer
 */
void enable_physical_counter(void) {
    uint32_t cnt_ctl = 1;
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(cnt_ctl));
}

/*
 * De-activates the physical counter of the ARM timer
 */
void disable_physical_counter(void) {
    uint32_t cnt_ctl = 0;
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(cnt_ctl));
}

/*
 * Returns the physical counter of the ARM timer
 */
uint64_t read_physical_counter(void) {
    uint64_t val;
    asm volatile("mrs %0, cntpct_el0" : "=r"(val));
    return (val);
}

/*
 * Writes a new timer to expire in the future
 */
void write_physical_timer_value(uint32_t val) {
    asm volatile("msr cntp_tval_el0, %0" ::"r"(val));
    return;
}

/*
 * Creates a new timer set `timer` microseconds from now. Also enables the
 * counter if it was disabled.
 */
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

/*
 * Returns the number of microseconds elapsed since reset
 */
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

/*
 * Activates the virtual counter for the ARM timer
 */
void enable_virtual_counter(void) {
    uint32_t cnt_ctl = 1;
    asm volatile("msr cntv_ctl_el0, %0" ::"r"(cnt_ctl));
}

/*
 * De-activates the virtual counter for the ARM timer
 */
void disable_virtual_counter(void) {
    uint32_t cnt_ctl = 0;
    asm volatile("msr cntv_ctl_el0, %0" ::"r"(cnt_ctl));
}

/*
 * Writes a new value into the virtual timer
 */
void write_virtual_timer_value(uint32_t val) {
    asm volatile("msr cntv_tval_el0, %0" ::"r"(val));
    return;
}

/*
 * Creates a new timer to expire `timer` microseconds in the future
 */
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

/*
 * Delay execution for `delay` milliseconds
 */
void wait_msec(unsigned int delay) { delay_us(delay * 1000); }

/*
 * Delay execution for `delay` microseconds
 */
void delay_us(uint32_t delay) {
    volatile unsigned long timestamp = getTOD();
    volatile unsigned long end       = timestamp + delay;

    while ((unsigned long)timestamp < (unsigned long)(end)) {
        timestamp = getTOD();
    }
}

/*
 * A raw loop delay
 */
void raw_delay() {
    int tim = 0;
    while (tim++ < 2000000)
        nop();
}