#ifndef __ARCH_H__
#define __ARCH_H__

#include <stdint.h>

#define nop() asm volatile("nop")

#ifndef NULL
#define NULL ((void *) 0)
#endif

// Raspberry pi 3 (bcm2837 SoC) IO peripherals base
#define IO_BASE 0x3F000000
#define MMIO_BASE 0x3F000000

#define MAX_DEVICES         4

#define DEVICE_CLASS_PRINTER    0
#define DEVICE_CLASS_TAPE       1
#define DEVICE_CLASS_DISK       2

#define IL_LINES            4
#define IL_PRINTER          3
#define IL_DISK             2
#define IL_TAPE             1
#define IL_TIMER            0

#define DEVICE_INSTALLED    0x7F000
#define INTERRUPT_LINES     0x7F020
#define INTERRUPT_MASK      0x7F040

#define CORE0_MAILBOX0      (*(uint32_t*)0x40000080)


#endif