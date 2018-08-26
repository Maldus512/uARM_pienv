#ifndef __RAND_H__
#define __RAND_H__

#include "gpio.h"

#define RNG_CTRL        ((volatile unsigned int*)(IO_BASE+0x00104000))
#define RNG_STATUS      ((volatile unsigned int*)(IO_BASE+0x00104004))
#define RNG_DATA        ((volatile unsigned int*)(IO_BASE+0x00104008))
#define RNG_INT_MASK    ((volatile unsigned int*)(IO_BASE+0x00104010))


void initRand();
unsigned int rand(unsigned int min, unsigned int max);

#endif