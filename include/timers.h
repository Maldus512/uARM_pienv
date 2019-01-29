#ifndef __TIMERS_H__
#define __TIMERS_H__

#include <stdint.h>
#include "arch.h"

#define CORE0_TIMER_IRQCNTL 0x40000040
#define CORE1_TIMER_IRQCNTL 0x40000044

void         delay_us(uint32_t delay);
void         rawDelay();
void         initArmTimer();
uint64_t     readCounterCount(void);
void         disableCounter(void);
unsigned int setTimer(unsigned int timer);
uint64_t     get_us();
uint64_t     get_ms();
void         wait_msec(unsigned int t);

#endif