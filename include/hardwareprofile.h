#ifndef __HARDWAREPROFILE_H__
#define __HARDWAREPROFILE_H__

#include <stdint.h>
//#define uint32_t    unsigned int

#define nop()      asm volatile ("nop") 

// Raspberry pi 3 (bcm2837 SoC) IO peripherals base
#define IO_BASE       0x3F000000

#define LED_RUN         21

#endif