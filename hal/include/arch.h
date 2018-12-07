#ifndef __HARDWAREPROFILE_H__
#define __HARDWAREPROFILE_H__

#include <stdint.h>

#define nop() asm volatile("nop")

// Raspberry pi 3 (bcm2837 SoC) IO peripherals base
#define IO_BASE 0x3F000000
#define MMIO_BASE 0x3F000000

#define IL_TERMINAL         0

#define INTERRUPT_LINES     0x60000
#define DEV_REG_START       0x60050

#define LED_RUN 21

void *DEV_REG_ADDR(int line, int dev);

#endif