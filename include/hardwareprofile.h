#ifndef __HARDWAREPROFILE_H__
#define __HARDWAREPROFILE_H__

#include <stdint.h>

#define nop()      asm volatile ("nop") 

// Raspberry pi 0 (bcm2835 SoC) IO peripherals base
#define IO_BASE       0x20000000

#endif