#ifndef __GPIO_H__
#define __GPIO_H__

#include "hardwareprofile.h"


struct GPIOREG {
    volatile uint32_t SEL[6];
    volatile uint32_t rsvd0;
    volatile uint32_t SET0;
    volatile uint32_t SET1;
    volatile uint32_t rsvd1;
    volatile uint32_t CLEAR0;
    volatile uint32_t CLEAR1;
    volatile uint32_t rsvd2;
    volatile uint32_t LVL0;
    volatile uint32_t LVL1;
    volatile uint32_t rsvd3;
    volatile uint32_t EVENT0;
    volatile uint32_t EVENT1;
    volatile uint32_t rsvd4;
    volatile uint32_t REDETECTEN0;
    volatile uint32_t REDETECTEN1;
    volatile uint32_t rsvd5;
    volatile uint32_t FEDETECTEN0;
    volatile uint32_t FEDETECTEN1;
    volatile uint32_t rsvd6;
    volatile uint32_t HIDETECTEN0;
    volatile uint32_t HIDETECTEN1;
    volatile uint32_t rsvd7;
    volatile uint32_t LODETECTEN0;
    volatile uint32_t LODETECTEN1;
    volatile uint32_t rsvd8;
    volatile uint32_t AREDETECTEN0;
    volatile uint32_t AREDETECTEN1;
    volatile uint32_t rsvd9;
    volatile uint32_t AFEDETECTEN0;
    volatile uint32_t AFEDETECTEN1;
    volatile uint32_t rsvd10;
    volatile uint32_t PUDEN;
    volatile uint32_t PUDCLOCK0;
    volatile uint32_t PUDCLOCK1;
};

#define GPIO_BASE       (IO_BASE + 0x200000)

#define GPIO            ((struct GPIOREG*) GPIO_BASE)


void initGpio();

void setGpio(int num);
void clearGpio(int num);

#endif