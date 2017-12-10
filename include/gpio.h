#ifndef __GPIO_H__
#define __GPIO_H__

#include "hardwareprofile.h"


#define GPIO_BASE       (IO_BASE + 0x200000)

#define GPIO_SET0       (GPIO_BASE + 0x1C)
#define GPIO_SET1       (GPIO_BASE + 0x20)

#define GPIO_CLR0       (GPIO_BASE + 0x28)
#define GPIO_CLR1       (GPIO_BASE + 0x2C)


#define GPFSEL1 (*(volatile uint32_t *)(GPIO_BASE + 0x04))
#define GPPUD   (*(volatile uint32_t *)(GPIO_BASE + 0x94))
#define GPPUDCLK0   (*(volatile uint32_t *)(GPIO_BASE + 0x98))

void initGpio();

void setGpio(int num);
void clearGpio(int num);

#endif