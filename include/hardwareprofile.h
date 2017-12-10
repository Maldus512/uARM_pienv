#ifndef __HARDWAREPROFILE_H__
#define __HARDWAREPROFILE_H__

#include <stdint.h>

#define REG(x)     (*(volatile uint32_t *)(x)) 
#define nop()      asm volatile ("nop") 

#define IO_BASE       0x3F000000

#endif