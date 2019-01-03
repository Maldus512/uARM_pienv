#ifndef __ARCH_H__
#define __ARCH_H__

#include <stdint.h>

#define nop() asm volatile("nop")

// Raspberry pi 3 (bcm2837 SoC) IO peripherals base
#define IO_BASE 0x3F000000
#define MMIO_BASE 0x3F000000

#define MAX_DEVICES         4

#define IL_LINES            3
#define IL_TERMINAL         2
#define IL_TAPE             1
#define IL_TIMER            0

#define DEVICE_INSTALLED    0x7F000
#define INTERRUPT_LINES     0x7F020
#define INTERRUPT_MASK      0x7F040
#define DEV_REG_START       0x7F100

#define LED_RUN 21


static inline void *DEV_REG_ADDR(int line, int dev) {
    return (void *)(uint64_t)(DEV_REG_START+MAX_DEVICES*line*4*4 + 4*4*dev);
}



#endif