#ifndef __HARDWAREPROFILE_H__
#define __HARDWAREPROFILE_H__

#include <stdint.h>

#define nop()      asm volatile ("nop") 

// Raspberry pi 3 (bcm2837 SoC) IO peripherals base
#define IO_BASE       0x3F000000

#endif