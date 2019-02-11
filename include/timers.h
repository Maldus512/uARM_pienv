#ifndef __TIMERS_H__
#define __TIMERS_H__

#include <stdint.h>
#include "arch.h"

#define CORE0_TIMER_IRQCNTL 0x40000040
#define CORE1_TIMER_IRQCNTL 0x40000044

void         delay_us(uint32_t delay);
void         raw_delay();
void         init_arm_timer_interrupt();
void         disable_physical_counter(void);
unsigned int set_physical_timer(unsigned int timer);
void         disable_virtual_counter(void);
unsigned int set_virtual_timer(unsigned int timer);
uint64_t     getTOD();
void         wait_msec(unsigned int t);

#endif